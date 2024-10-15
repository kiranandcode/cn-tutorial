# commit 2b86f9d: convert the hash from LIST to SLIST
[link](https://github.com/freebsd/freebsd-src/commit/2b86f9d6d013a9ad8b1f8d03286018e785d5b3f6)

```diff
	for (ncpp = nchashtbl; n_nchash > 0; n_nchash--, ncpp++) {
		count = 0;
-		CK_LIST_FOREACH(ncp, ncpp, nc_hash) {
+		CK_SLIST_FOREACH(ncp, ncpp, nc_hash) {
			count++;
		}
		if (count)
```
