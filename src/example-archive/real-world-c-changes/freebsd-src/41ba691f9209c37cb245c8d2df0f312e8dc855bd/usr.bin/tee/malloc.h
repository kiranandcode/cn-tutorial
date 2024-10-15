#ifndef MALLOC_H
#define MALLOC_H

#include "types.h"

extern struct entry *malloc__entry();
/*@ spec malloc__entry();
    requires true;
    ensures take R = Block<struct entry>(return);
            !ptr_eq(return,NULL);
@*/ 


extern LIST *malloc__list();
/*@ spec malloc__list();
    requires true;
    ensures take R = Block<struct _list>(return);
            !ptr_eq(return,NULL);
@*/ 



#endif
