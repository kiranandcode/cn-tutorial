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
 * Copyright (c) 2012, 2021 by Delphix. All rights reserved.
 */

/* #include <sys/zfs_context.h> */
/* #include <sys/zfs_refcount.h> */
#include "../../include/sys/zfs_refcount_before.h"

#include <stdint.h>
#include <stdbool.h>


/*
 * Reference count tracking is disabled by default.  It's memory requirements
 * are reasonable, however as implemented it consumes a significant amount of
 * cpu time.  Until its performance is improved it should be manually enabled.
 */
int reference_tracking_enable = false;
static uint64_t reference_history = 3; /* tunable */

/* static kmem_cache_t *reference_cache; */
/* static kmem_cache_t *reference_history_cache; */

void
zfs_refcount_create(zfs_refcount_t *rc)
{
	/* mutex_init(&rc->rc_mtx, NULL, MUTEX_DEFAULT, NULL); */
	list_create(&rc->rc_list, sizeof (reference_t),
	    offsetof(reference_t, ref_link));
	list_create(&rc->rc_removed, sizeof (reference_t),
	    offsetof(reference_t, ref_link));
	rc->rc_count = 0;
	rc->rc_removed_count = 0;
	rc->rc_tracked = reference_tracking_enable;
}

extern void kmem_cache_free(void *);

void
zfs_refcount_destroy_many(zfs_refcount_t *rc, uint64_t number)
{
	reference_t *ref;

	/* ASSERT3U(rc->rc_count, ==, number); */
	while ((ref = list_remove_head(&rc->rc_list)))
		kmem_cache_free(ref);
	list_destroy(&rc->rc_list);

	while ((ref = list_remove_head(&rc->rc_removed))) {
		kmem_cache_free(ref->ref_removed);
		kmem_cache_free(ref);
	}
	list_destroy(&rc->rc_removed);
	/* mutex_destroy(&rc->rc_mtx); */
}


void
zfs_refcount_transfer(zfs_refcount_t *dst, zfs_refcount_t *src)
{
	int64_t count, removed_count;
	list_t list, removed;

	list_create(&list, sizeof (reference_t),
	    offsetof(reference_t, ref_link));
	list_create(&removed, sizeof (reference_t),
	    offsetof(reference_t, ref_link));

	/* mutex_enter(&src->rc_mtx); */
	count = src->rc_count;
	removed_count = src->rc_removed_count;
	src->rc_count = 0;
	src->rc_removed_count = 0;
	list_move_tail(&list, &src->rc_list);
	list_move_tail(&removed, &src->rc_removed);
	/* mutex_exit(&src->rc_mtx); */

	/* mutex_enter(&dst->rc_mtx); */
	dst->rc_count += count;
	dst->rc_removed_count += removed_count;
	list_move_tail(&dst->rc_list, &list);
	list_move_tail(&dst->rc_removed, &removed);
	/* mutex_exit(&dst->rc_mtx); */

	list_destroy(&list);
	list_destroy(&removed);
}
