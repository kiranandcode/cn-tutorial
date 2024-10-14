/*
 * Copyright (C) 2024 Nuo Mi
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/* #include "config.h" */

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

/* #include "libavutil/mem.h" */
/* #include "libavutil/thread.h" */

#include "executor.h"

typedef struct Queue {
    FFTask *head;
    FFTask *tail;
} Queue;

struct FFExecutor {
    /* FFTaskCallbacks cb; */
    int thread_count;
    bool recursive;

    /* ThreadInfo *threads; */
    uint8_t *local_contexts;

    /* AVMutex lock; */
    /* AVCond cond; */
    int die;

    Queue *q;
};

static FFTask* remove_task(Queue *q)
{
    FFTask *t = q->head;
    if (t) {
        q->head = t->next;
        t->next = NULL;
        if (!q->head)
            q->tail = NULL;
    }
    return t;
}

static void add_task(Queue *q, FFTask *t)
{
    t->next = NULL;
    if (!q->head)
        q->tail = q->head = t;
    else
        q->tail = q->tail->next = t;
}

static int run_one_task(FFExecutor *e, void *lc)
{
    /* FFTaskCallbacks *cb = &e->cb; */
    FFTask *t = NULL;

    for (int i = 0; !t; i++)
        t = remove_task(e->q + i);

    if (t) {
        /* if (e->thread_count > 0) */
        /*     ff_mutex_unlock(&e->lock); */
        /* cb->run(t, lc, cb->user_data); */
        /* if (e->thread_count > 0) */
        /*     ff_mutex_lock(&e->lock); */
        return 1;
    }
    return 0;
}


void ff_executor_execute(FFExecutor *e, FFTask *t)
{
    /* if (e->thread_count) */
    /*     ff_mutex_lock(&e->lock); */
    if (t)
        add_task(e->q + t->priority /* % e->cb.priorities */, t);
    /* if (e->thread_count) { */
    /*     ff_cond_signal(&e->cond); */
    /*     ff_mutex_unlock(&e->lock); */
    /* } */

    if (!e->thread_count /* || !HAVE_THREADS */) {
        if (e->recursive)
            return;
        e->recursive = true;
        // We are running in a single-threaded environment, so we must handle all tasks ourselves
        while (run_one_task(e, e->local_contexts))
            /* nothing */;
        e->recursive = false;
    }
}
