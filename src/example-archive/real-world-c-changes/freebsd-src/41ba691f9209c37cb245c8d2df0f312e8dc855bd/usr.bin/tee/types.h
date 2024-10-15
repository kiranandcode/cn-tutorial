#ifndef TYPES_H
#define TYPES_H

#include "queue.h"

typedef void FILE;

struct entry {
	FILE *fd;
	const char *name;
	STAILQ_ENTRY(entry) entries;
};

typedef struct _list {
	struct _list *next;
	int fd;
	const char *name;
} LIST;


#endif
