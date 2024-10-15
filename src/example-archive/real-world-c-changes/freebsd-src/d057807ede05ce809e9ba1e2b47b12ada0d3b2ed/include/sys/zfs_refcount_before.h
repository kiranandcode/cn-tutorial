/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or https://opensource.org/licenses/CDDL-1.0.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*
 * Copyright (c) 2005, 2010, Oracle and/or its affiliates. All rights reserved.
 * Copyright (c) 2012, 2015 by Delphix. All rights reserved.
 */

#ifndef	_SYS_ZFS_REFCOUNT_H
#define	_SYS_ZFS_REFCOUNT_H

/* #include <sys/inttypes.h> */
/* #include <sys/list.h> */
/* #include <sys/zfs_context.h> */
#include "avl.h"
#include "list.h"
#include <stdint.h>


/*
 * If the reference is held only by the calling function and not any
 * particular object, use FTAG (which is a string) for the holder_tag.
 * Otherwise, use the object that holds the reference.
 */
#define	FTAG ((char *)(uintptr_t)__func__)


typedef struct reference {
	list_node_t ref_link;
	const void *ref_holder;
	uint64_t ref_number;
	uint8_t *ref_removed;
} reference_t;

typedef struct refcount {
	/* kmutex_t rc_mtx; */
	bool rc_tracked;
	list_t rc_list;
	list_t rc_removed;
	uint64_t rc_count;
	uint64_t rc_removed_count;
} zfs_refcount_t;

/*
 * Note: zfs_refcount_t must be initialized with
 * refcount_create[_untracked]()
 */

void zfs_refcount_create(zfs_refcount_t *);
void zfs_refcount_create_untracked(zfs_refcount_t *);
void zfs_refcount_create_tracked(zfs_refcount_t *);
void zfs_refcount_destroy(zfs_refcount_t *);
void zfs_refcount_destroy_many(zfs_refcount_t *, uint64_t);
int zfs_refcount_is_zero(zfs_refcount_t *);
int64_t zfs_refcount_count(zfs_refcount_t *);
int64_t zfs_refcount_add(zfs_refcount_t *, const void *);
int64_t zfs_refcount_remove(zfs_refcount_t *, const void *);
/*
 * Note that (add|remove)_many adds/removes one reference with "number" N,
 * _not_ N references with "number" 1, which is what (add|remove)_few does,
 * or what vanilla zfs_refcount_(add|remove) called N times would do.
 *
 * Attempting to remove a reference with number N when none exists is a
 * panic on debug kernels with reference_tracking enabled.
 */
void zfs_refcount_add_few(zfs_refcount_t *, uint64_t, const void *);
void zfs_refcount_remove_few(zfs_refcount_t *, uint64_t, const void *);
int64_t zfs_refcount_add_many(zfs_refcount_t *, uint64_t, const void *);
int64_t zfs_refcount_remove_many(zfs_refcount_t *, uint64_t, const void *);
void zfs_refcount_transfer(zfs_refcount_t *, zfs_refcount_t *);
void zfs_refcount_transfer_ownership(zfs_refcount_t *, const void *,
    const void *);
void zfs_refcount_transfer_ownership_many(zfs_refcount_t *, uint64_t,
    const void *, const void *);
bool zfs_refcount_held(zfs_refcount_t *, const void *);
bool zfs_refcount_not_held(zfs_refcount_t *, const void *);

void zfs_refcount_init(void);
void zfs_refcount_fini(void);


#endif /* _SYS_REFCOUNT_H */
