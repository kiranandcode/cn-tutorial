# commit 1bf83b95: commit 1bf83b95: 
[link](https://github.com/ffmpeg/ffmpeg/commit/1bf83b9548957e49d90804a035529085df4b872a)

```diff
-void av_tree_enumerate(AVTreeNode *t, void *opaque, int (*f)(void *opaque, void *elem)){
-    int v= f(opaque, t->elem);
-    if(v>=0) av_tree_enumerate(t->child[0], opaque, f);
-    if(v<=0) av_tree_enumerate(t->child[1], opaque, f);
+void av_tree_enumerate(AVTreeNode *t, void *opaque, int (*cmp)(void *opaque, void *elem), int (*enu)(void *opaque, void *elem)){
+    int v= cmp ? cmp(opaque, t->elem) : 0;
+    if(v>=0) av_tree_enumerate(t->child[0], opaque, cmp, enu);
+    if(v==0) enu(opaque, t->elem);
+    if(v<=0) av_tree_enumerate(t->child[1], opaque, cmp, enu);
```
