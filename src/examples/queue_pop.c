#include "queue_headers.h"
#include "queue_pop_lemma.h"

int IntQueue_pop (struct int_queue *q)
/*@ requires take before = IntQueuePtr(q);
             before != Seq_Nil{};
    ensures take after = IntQueuePtr(q);
            after == tl(before);
            return == hd(before);
@*/
{
  struct int_queueCell* h = q->front;
  struct int_queueCell* t = q->back;
  /*@ split_case is_null(h); @*/
  if (h == t) {
    int x = h->first;
    freeIntQueueCell(h);
    q->front = 0;
    q->back = 0;
    /*@ unfold snoc(Seq_Nil{}, x); @*/
    return x;
  } else {
    int x = h->first;
    struct int_queueCell* n = h->next;
    q->front = n;
    freeIntQueueCell(h);
    /*@ apply tl_snoc(n, (*q).back, before, x); @*/
    return x;
  }
}

