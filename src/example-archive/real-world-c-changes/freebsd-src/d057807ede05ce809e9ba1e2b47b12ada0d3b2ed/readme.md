# commit d05780: Switch refcount tracking from lists to AVL-trees.
[link](https://github.com/freebsd/freebsd-src/commit/d057807ede05ce809e9ba1e2b47b12ada0d3b2ed)

```diff
typedef struct reference {
-	list_node_t ref_link;
+	union {
+		avl_node_t a;
+		list_node_t l;
+	} ref_link;
 	const void *ref_holder;
 	uint64_t ref_number;
-	uint8_t *ref_removed;
+	boolean_t ref_search;
 } reference_t;
```

