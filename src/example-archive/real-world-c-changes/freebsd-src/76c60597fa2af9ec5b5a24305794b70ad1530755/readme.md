# commit 76c605: swap_pager: use vm_page_iterators for lookup
[link](https://github.com/freebsd/freebsd-src/commit/76c60597fa2af9ec5b5a24305794b70ad1530755)

```diff
 			swp_pager_update_freerange(&range, sb->d[i]);
 			if (freed != NULL) {
-				m = (m != NULL && m->pindex == sb->p + i - 1) ?
-				    vm_page_next(m) :
-				    vm_page_lookup(object, sb->p + i);
+				m = vm_page_iter_lookup(&pages, sb->p + i);
 				if (m == NULL || vm_page_none_valid(m))
```
