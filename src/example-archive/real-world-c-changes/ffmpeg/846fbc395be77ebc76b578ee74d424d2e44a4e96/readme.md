# commit 846fbc: avcodec/vvc: simplify priority logical to improve performance for 4K/8K
[link](https://github.com/ffmpeg/ffmpeg/commit/846fbc395be77ebc76b578ee74d424d2e44a4e96)

```diff
+typedef struct Queue {
+    FFTask *head;
+    FFTask *tail;
+} Queue;
+
 struct FFExecutor {
     FFTaskCallbacks cb;
     int thread_count;
@@ -60,29 +65,39 @@ struct FFExecutor {
     AVCond cond;
     int die;
 
-    FFTask *tasks;
+    Queue *q;
 };
 
-static FFTask* remove_task(FFTask **prev, FFTask *t)
+static FFTask* remove_task(Queue *q)
 {
-    *prev  = t->next;
-    t->next = NULL;
+    FFTask *t = q->head;
+    if (t) {
+        q->head = t->next;
+        t->next = NULL;
+        if (!q->head)
+            q->tail = NULL;
+    }
     return t;
 }
```
