#ifndef VM_PAGE_H
#define VM_PAGE_H

#include <stdint.h>

struct swdevt {
	int	sw_flags;
	int	sw_nblks;
	int     sw_used;
	/* dev_t	sw_dev; */
	/* struct vnode *sw_vp; */
	void	*sw_id;
	/* __daddr_t sw_first; */
	/* __daddr_t sw_end; */
	/* struct blist *sw_blist; */
	/* TAILQ_ENTRY(swdevt)	sw_list; */
	/* sw_strategy_t		*sw_strategy; */
	/* sw_close_t		*sw_close; */
};

struct pctrie_node {
	uint64_t	pn_owner;			/* Owner of record. */
	/* pn_popmap_t	pn_popmap;			/\* Valid children. *\/ */
	uint8_t		pn_clev;			/* Level * WIDTH. */
	/* smr_pctnode_t	pn_child[PCTRIE_COUNT];		/\* Child nodes. *\/ */
};

struct pctrie {
	struct pctrie_node	*pt_root;
};


typedef	uint64_t vm_pindex_t;
typedef int daddr_t;

struct vm_object {
	unsigned int flags;			/* see below */
};

typedef struct vm_object *vm_object_t;

typedef	uint64_t	vm_size_t;

struct vm_page {
  int field;
};
typedef struct vm_page *vm_page_t;

#define PCTRIE_LIMIT	100

struct pctrie_iter {
	struct pctrie *ptree;
	struct pctrie_node *path[PCTRIE_LIMIT];
	uint64_t index;
	uint64_t limit;
	int top;
};

extern void vm_page_iter_init(struct pctrie_iter *, vm_object_t);
extern vm_page_t vm_page_iter_lookup(struct pctrie_iter *, vm_pindex_t);
extern vm_page_t vm_page_next(vm_page_t m);
extern vm_page_t vm_page_lookup(vm_object_t, vm_pindex_t);




#endif
