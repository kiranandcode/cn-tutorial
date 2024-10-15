/*-
 * SPDX-License-Identifier: BSD-4-Clause
 *
 * Copyright (c) 1998 Matthew Dillon,
 * Copyright (c) 1994 John S. Dyson
 * Copyright (c) 1990 University of Utah.
 * Copyright (c) 1982, 1986, 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *				New Swap System
 *				Matthew Dillon
 *
 * Radix Bitmap 'blists'.
 *
 *	- The new swapper uses the new radix bitmap code.  This should scale
 *	  to arbitrarily small or arbitrarily large swap spaces and an almost
 *	  arbitrary degree of fragmentation.
 *
 * Features:
 *
 *	- on the fly reallocation of swap during putpages.  The new system
 *	  does not try to keep previously allocated swap blocks for dirty
 *	  pages.
 *
 *	- on the fly deallocation of swap
 *
 *	- No more garbage collection required.  Unnecessarily allocated swap
 *	  blocks only exist for dirty vm_page_t's now and these are already
 *	  cycled (in a high-load system) by the pager.  We also do on-the-fly
 *	  removal of invalidated swap blocks when a page is destroyed
 *	  or renamed.
 *
 * from: Utah $Hdr: swap_pager.c 1.4 91/04/30$
 */

/* #include <sys/cdefs.h> */
/* #include "opt_vm.h" */

/* #include <sys/param.h> */
/* #include <sys/bio.h> */
/* #include <sys/blist.h> */
/* #include <sys/buf.h> */
/* #include <sys/conf.h> */
/* #include <sys/disk.h> */
/* #include <sys/disklabel.h> */
/* #include <sys/eventhandler.h> */
/* #include <sys/fcntl.h> */
/* #include <sys/limits.h> */
/* #include <sys/lock.h> */
/* #include <sys/kernel.h> */
/* #include <sys/mount.h> */
/* #include <sys/namei.h> */
/* #include <sys/malloc.h> */
/* #include <sys/pctrie.h> */
/* #include <sys/priv.h> */
/* #include <sys/proc.h> */
/* #include <sys/racct.h> */
/* #include <sys/resource.h> */
/* #include <sys/resourcevar.h> */
/* #include <sys/rwlock.h> */
/* #include <sys/sbuf.h> */
/* #include <sys/sysctl.h> */
/* #include <sys/sysproto.h> */
/* #include <sys/systm.h> */
/* #include <sys/sx.h> */
/* #include <sys/unistd.h> */
/* #include <sys/user.h> */
/* #include <sys/vmmeter.h> */
/* #include <sys/vnode.h> */

/* #include <security/mac/mac_framework.h> */

/* #include <vm/vm.h> */
/* #include <vm/pmap.h> */
/* #include <vm/vm_map.h> */
/* #include <vm/vm_kern.h> */
/* #include <vm/vm_object.h> */
/* #include <vm/vm_page.h> */
/* #include <vm/vm_pager.h> */
/* #include <vm/vm_pageout.h> */
/* #include <vm/vm_param.h> */
/* #include <vm/swap_pager.h> */
/* #include <vm/vm_extern.h> */
/* #include <vm/uma.h> */

/* #include <geom/geom.h> */

/*
 * MAX_PAGEOUT_CLUSTER must be a power of 2 between 1 and 64.
 * The 64-page limit is due to the radix code (kern/subr_blist.c).
 */
/* #ifndef MAX_PAGEOUT_CLUSTER */
/* #define	MAX_PAGEOUT_CLUSTER	32 */
/* #endif */

/* #if !defined(SWB_NPAGES) */
/* #define SWB_NPAGES	MAX_PAGEOUT_CLUSTER */
/* #endif */

#define	SWAP_META_PAGES		100
#include <stdbool.h>
#include "vm_page.h"
#include "queue.h"
#define	OBJ_DEAD	0x00000008	/* dead objects (during rundown) */

#define	VPO_KMEM_EXEC	0x01		/* kmem mapping allows execution */
#define	VPO_SWAPSLEEP	0x02		/* waiting for swap to finish */
#define	VPO_UNMANAGED	0x04		/* no PV management for page */
#define	VPO_SWAPINPROG	0x08		/* swap I/O in progress on page */

#define NULL 0

/*
 * A swblk structure maps each page index within a
 * SWAP_META_PAGES-aligned and sized range to the address of an
 * on-disk swap block (or SWAPBLK_NONE). The collection of these
 * mappings for an entire vm object is implemented as a pc-trie.
 */
struct swblk {
	vm_pindex_t	p;
	daddr_t		d[SWAP_META_PAGES];
};

/*
 * A page_range structure records the start address and length of a sequence of
 * mapped page addresses.
 */
struct page_range {
	daddr_t start;
	daddr_t num;
};

static TAILQ_HEAD(, swdevt) swtailq = TAILQ_HEAD_INITIALIZER(swtailq);
/* static struct swdevt *swdevhd;	/\* Allocate from here next *\/ */
static int nswapdev;		/* Number of swap devices */
int swap_pager_avail;
/* static struct sx swdev_syscall_lock;	/\* serialize swap(on|off) *\/ */

static unsigned long swap_reserved;
static unsigned long swap_total;
/* static int sysctl_page_shift(SYSCTL_HANDLER_ARGS); */


int vm_overcommit = 0;

static int swap_pager_full = 2;	/* swap space exhaustion (task killing) */
static int swap_pager_almost_full = 1; /* swap space exhaustion (w/hysteresis)*/
static int nsw_wcount_async;	/* limit async write buffers */
static int nsw_wcount_async_max;/* assigned maximum			*/
int nsw_cluster_max; 		/* maximum VOP I/O allowed		*/


/*
 * "named" and "unnamed" anon region objects.  Try to reduce the overhead
 * of searching a named list by hashing it just a little.
 */

#define NOBJLISTS		8



/*
 * pagerops for OBJT_SWAP - "swap pager".  Some ops are also global procedure
 * calls hooked from other parts of the VM system and do not appear here.
 * (see vm/swap_pager.h).
 */

/*
 *	swap_pager_swapoff_object:
 *
 *	Page in all of the pages that have been paged out for an object
 *	to a swap device.
 */
static void
swap_pager_swapoff_object(struct swdevt *sp, vm_object_t object)
{
	struct page_range range;
	struct swblk *sb;
	vm_page_t m;
	vm_pindex_t pi;
	daddr_t blk;
	int i, rahead, rv;
	bool sb_empty;

	/* VM_OBJECT_ASSERT_WLOCKED(object); */
	/* KASSERT((object->flags & OBJ_SWAP) != 0, */
	/*     ("%s: Object not swappable", __func__)); */

	pi = 0;
	i = 0;
	/* swp_pager_init_freerange(&range); */
	for (;;) {
		if (i == 0 && (object->flags & OBJ_DEAD) != 0) {
			/*
			 * Make sure that pending writes finish before
			 * returning.
			 */
			/* vm_object_pip_wait(object, "swpoff"); */
			/* swp_pager_meta_free_all(object); */
			break;
		}

		/* if (i == SWAP_META_PAGES) { */
		/* 	pi = sb->p + SWAP_META_PAGES; */
		/* 	if (sb_empty) { */
		/* 		swblk_lookup_remove(object, sb); */
		/* 		uma_zfree(swblk_zone, sb); */
		/* 	} */
		/* 	i = 0; */
		/* } */

		/* if (i == 0) { */
		/* 	sb = swblk_start(object, pi); */
		/* 	if (sb == NULL) */
		/* 		break; */
		/* 	sb_empty = true; */
		/* 	m = NULL; */
		/* } */

		/* Skip an invalid block. */
		blk = sb->d[i];
		/* if (blk == SWAPBLK_NONE || !swp_pager_isondev(blk, sp)) { */
		/* 	if (blk != SWAPBLK_NONE) */
		/* 		sb_empty = false; */
		/* 	m = NULL; */
		/* 	i++; */
		/* 	continue; */
		/* } */

		/*
		 * Look for a page corresponding to this block, If the found
		 * page has pending operations, sleep and restart the scan.
		 */
		m = m != NULL ? vm_page_next(m) :
		    vm_page_lookup(object, sb->p + i);
		/* if (m != NULL && (m->oflags & VPO_SWAPINPROG) != 0) { */
		/* 	m->oflags |= VPO_SWAPSLEEP; */
		/* 	VM_OBJECT_SLEEP(object, &object->handle, PSWP, "swpoff", */
		/* 	    0); */
		/* 	i = 0;	/\* Restart scan after object lock dropped. *\/ */
		/* 	continue; */
		/* } */

		/*
		 * If the found page is valid, mark it dirty and free the swap
		 * block.
		 */
		/* if (m != NULL && vm_page_all_valid(m)) { */
		/* 	swp_pager_force_dirty(&range, m, &sb->d[i]); */
		/* 	i++; */
		/* 	continue; */
		/* } */

		/* Is there a page we can acquire or allocate? */
		if (m == NULL) {
			/* m = vm_page_alloc(object, sb->p + i, */
			/*     VM_ALLOC_NORMAL | VM_ALLOC_WAITFAIL); */
		} /* else if (!vm_page_busy_acquire(m, VM_ALLOC_WAITFAIL)) */
			/* m = NULL; */

		/* If no page available, repeat this iteration. */
		if (m == NULL)
			continue;

		/* Get the page from swap, mark it dirty, restart the scan. */
		/* vm_object_pip_add(object, 1); */
		rahead = SWAP_META_PAGES;
		/* rv = swap_pager_getpages_locked(object, &m, 1, NULL, &rahead); */
		/* if (rv != VM_PAGER_OK) */
		/* 	panic("%s: read from swap failed: %d", __func__, rv); */
		/* VM_OBJECT_WLOCK(object); */
		/* vm_object_pip_wakeupn(object, 1); */
		/* KASSERT(vm_page_all_valid(m), */
		/*     ("%s: Page %p not all valid", __func__, m)); */
		/* vm_page_xunbusy(m); */
		i = 0;	/* Restart scan after object lock dropped. */
	}
	/* swp_pager_freeswapspace(&range); */
}
