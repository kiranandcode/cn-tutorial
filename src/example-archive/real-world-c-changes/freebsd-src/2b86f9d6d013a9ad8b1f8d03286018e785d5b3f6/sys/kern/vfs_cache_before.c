/*-
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 1989, 1993, 1995
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Poul-Henning Kamp of the FreeBSD Project.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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
 *	@(#)vfs_cache.c	8.5 (Berkeley) 3/22/95
 */

/* #include <sys/cdefs.h> */
/* __FBSDID("$FreeBSD$"); */

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

/* #include "opt_ddb.h" */
/* #include "opt_ktrace.h" */

/* #include <sys/param.h> */
/* #include <sys/systm.h> */
/* #include <sys/capsicum.h> */
/* #include <sys/counter.h> */
/* #include <sys/filedesc.h> */
/* #include <sys/fnv_hash.h> */
/* #include <sys/kernel.h> */
/* #include <sys/ktr.h> */
/* #include <sys/lock.h> */
/* #include <sys/malloc.h> */
/* #include <sys/fcntl.h> */
/* #include <sys/jail.h> */
/* #include <sys/mount.h> */
/* #include <sys/namei.h> */
/* #include <sys/proc.h> */
/* #include <sys/rwlock.h> */
/* #include <sys/seqc.h> */
/* #include <sys/sdt.h> */
/* #include <sys/smr.h> */
/* #include <sys/smp.h> */
/* #include <sys/syscallsubr.h> */
/* #include <sys/sysctl.h> */
/* #include <sys/sysproto.h> */
/* #include <sys/vnode.h> */
/* #include <ck_queue.h> */
/* #ifdef KTRACE */
/* #include <sys/ktrace.h> */
/* #endif */

#include "ck_queue.h"
#include "queue.h"

/* #include <sys/capsicum.h> */

/* #include <security/audit/audit.h> */
/* #include <security/mac/mac_framework.h> */

/* #ifdef DDB */
/* #include <ddb/ddb.h> */
/* #endif */

/* #include <vm/uma.h> */


struct namecache {
	LIST_ENTRY(namecache) nc_src;	/* source vnode list */
	TAILQ_ENTRY(namecache) nc_dst;	/* destination vnode list */
	CK_LIST_ENTRY(namecache) nc_hash;/* hash chain */
	/* struct	vnode *nc_dvp;		/\* vnode of parent of name *\/ */
	/* union { */
	/* 	struct	vnode *nunsigned vp;	/\* vnode the name refers to *\/ */
	/* 	struct	negstate nunsigned neg;/\* negative entry state *\/ */
	/* } n_un; */
	unsigned char	nc_flag;		/* flag bits */
	unsigned char	nc_nlen;		/* length of name */
	/* char	nc_name[0];		/\* segment name + nul *\/ */
};


static CK_LIST_HEAD(nchashhead, namecache) *nchashtbl;/* Hash Table */
static unsigned long nchash;			/* size of hash table */


struct debug_hashstat_result { size_t n_nchash; int *cntbuf; };

struct debug_hashstat_result
sysctl_debug_hashstat_rawnchash()
{
	struct nchashhead *ncpp;
	struct namecache *ncp;
	int i, error, n_nchash, *cntbuf;

/* retry: */
	n_nchash = nchash + 1;	/* nchash is max index, not count */
	/* if (req->oldptr == NULL) */
	/* 	return SYSCTL_OUT(req, 0, n_nchash * sizeof(int)); */
	cntbuf = malloc(n_nchash * sizeof(int));
	/* cache_lock_all_buckets(); */
	/* if (n_nchash != nchash + 1) { */
	/* 	cache_unlock_all_buckets(); */
	/* 	free(cntbuf, M_TEMP); */
	/* 	goto retry; */
	/* } */
	/* Scan hash tables counting entries */
	for (ncpp = nchashtbl, i = 0; i < n_nchash; ncpp++, i++)
		CK_LIST_FOREACH(ncp, ncpp, nc_hash)
			cntbuf[i]++;
	/* cache_unlock_all_buckets(); */
	/* for (error = 0, i = 0; i < n_nchash; i++) */
	/* 	if ((error = SYSCTL_OUT(req, &cntbuf[i], sizeof(int))) != 0) */
	/* 		break; */
	free(cntbuf);
	return ((struct debug_hashstat_result){.n_nchash=n_nchash, .cntbuf=cntbuf});
}

struct debug_hashstat_stats {
  int used; int maxlength; int pct;
};

struct debug_hashstat_stats
sysctl_debug_hashstat_nchash()
{
	struct nchashhead *ncpp;
	struct namecache *ncp;
	int n_nchash;
	int count, maxlength, used, pct;

	/* cache_lock_all_buckets(); */
	n_nchash = nchash + 1;	/* nchash is max index, not count */
	used = 0;
	maxlength = 0;

	/* Scan hash tables for applicable entries */
	for (ncpp = nchashtbl; n_nchash > 0; n_nchash--, ncpp++) {
		count = 0;
		CK_LIST_FOREACH(ncp, ncpp, nc_hash) {
			count++;
		}
		if (count)
			used++;
		if (maxlength < count)
			maxlength = count;
	}
	n_nchash = nchash + 1;
	/* cache_unlock_all_buckets(); */
	pct = (used * 100) / (n_nchash / 100);

        return ((struct debug_hashstat_stats){ .used = used, .maxlength=maxlength, .pct = pct});
}
