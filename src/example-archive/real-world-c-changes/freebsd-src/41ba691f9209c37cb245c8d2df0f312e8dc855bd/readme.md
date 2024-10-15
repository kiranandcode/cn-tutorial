# commit 41ba6: use queue(9) in place of a custom linked list
[link](https://github.com/freebsd/freebsd-src/commit/41ba691f9209c37cb245c8d2df0f312e8dc855bd)

```diff
	while ((rval = read(STDIN_FILENO, buf, BSIZE)) > 0)
-		for (p = head; p; p = p->next) {
+		STAILQ_FOREACH(p, &head, entries) {
			n = rval;
			bp = buf;
```
