# commit dda30f129: Use more compact LIST instead of TAILQ for session hash.
[link](https://github.com/freebsd/freebsd-src/commit/dda30f129e1cb57b09399b6eb56f101c612e8202)

```diff
 	mtx_lock(&privp->sesshash[hash].mtx);
-	TAILQ_FOREACH(sp, &privp->sesshash[hash].head, sessions) {
+	LIST_FOREACH(sp, &privp->sesshash[hash].head, sessions) {
 		if (sp->Session_ID == session &&
 		    bcmp(sp->pkt_hdr.eh.ether_dhost,
```
