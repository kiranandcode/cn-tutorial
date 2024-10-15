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
#include "../../include/sys/zfs_refcount_after.h"
/* #include "../../include/sys/avl.h" */
/* #include "../../include/sys/list.h" */

#define	TREE_ISIGN(a)	(((a) > 0) - ((a) < 0))
#define	TREE_CMP(a, b)	(((a) > (b)) - ((a) < (b)))
#define	TREE_PCMP(a, b)	\
	(((uintptr_t)(a) > (uintptr_t)(b)) - ((uintptr_t)(a) < (uintptr_t)(b)))

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

static int zfs_refcount_compare(const void *x1, const void *x2)
{
	const reference_t *r1 = (const reference_t *)x1;
	const reference_t *r2 = (const reference_t *)x2;

	int cmp1 = TREE_CMP(r1->ref_holder, r2->ref_holder);
	int cmp2 = TREE_CMP(r1->ref_number, r2->ref_number);
	int cmp = cmp1 ? cmp1 : cmp2;
	return ((cmp || r1->ref_search) ? cmp : TREE_PCMP(r1, r2));
}

void zfs_refcount_create(zfs_refcount_t *rc)
{
	/* mutex_init(&rc->rc_mtx, NULL, MUTEX_DEFAULT, NULL); */
	avl_create(&rc->rc_tree, zfs_refcount_compare, sizeof (reference_t),
	    offsetof(reference_t, ref_link/* .a */));
	list_create(&rc->rc_removed, sizeof (reference_t),
	    offsetof(reference_t, ref_link/* .l */));
	rc->rc_count = 0;
	rc->rc_removed_count = 0;
	rc->rc_tracked = reference_tracking_enable;
}


extern void kmem_cache_free(void *);


void
zfs_refcount_destroy_many(zfs_refcount_t *rc, uint64_t number)
{
	reference_t *ref;
	void *cookie = NULL;

	/* ASSERT3U(rc->rc_count, ==, number); */
	while ((ref = avl_destroy_nodes(&rc->rc_tree, &cookie)) != NULL)
		kmem_cache_free(ref);
	avl_destroy(&rc->rc_tree);

	while ((ref = list_remove_head(&rc->rc_removed)))
		kmem_cache_free(ref);
	list_destroy(&rc->rc_removed);
	/* mutex_destroy(&rc->rc_mtx); */
}

void zfs_refcount_transfer(zfs_refcount_t *dst, zfs_refcount_t *src)
{
	avl_tree_t tree;
	list_t removed;
	reference_t *ref;
	void *cookie = NULL;
	uint64_t count;
	uint64_t removed_count;

	avl_create(&tree, zfs_refcount_compare, sizeof (reference_t),
	    offsetof(reference_t, ref_link/* .a */));
	list_create(&removed, sizeof (reference_t),
	    offsetof(reference_t, ref_link/* .l */));

	/* mutex_enter(&src->rc_mtx); */
	count = src->rc_count;
	removed_count = src->rc_removed_count;
	src->rc_count = 0;
	src->rc_removed_count = 0;
	avl_swap(&tree, &src->rc_tree);
	list_move_tail(&removed, &src->rc_removed);
	/* mutex_exit(&src->rc_mtx); */

	/* mutex_enter(&dst->rc_mtx); */
	dst->rc_count += count;
	dst->rc_removed_count += removed_count;
	if (avl_is_empty(&dst->rc_tree))
		avl_swap(&dst->rc_tree, &tree);
	else while ((ref = avl_destroy_nodes(&tree, &cookie)) != NULL)
		avl_add(&dst->rc_tree, ref);
	list_move_tail(&dst->rc_removed, &removed);
	/* mutex_exit(&dst->rc_mtx); */

	avl_destroy(&tree);
	list_destroy(&removed);
}


