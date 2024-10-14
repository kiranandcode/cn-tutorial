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


struct FFExecutor {
    /* FFTaskCallbacks cb; */
    int thread_count;
    bool recursive;

    /* ThreadInfo *threads; */
    uint8_t *local_contexts;

    /* AVMutex lock; */
    /* AVCond cond; */
    int die;

    FFTask *tasks;
};

static FFTask* remove_task(FFTask **prev, FFTask *t)
{
    *prev  = t->next;
    t->next = NULL;
    return t;
}

static void add_task(FFTask **prev, FFTask *t)
{
    t->next = *prev;
    *prev   = t;
}

static int run_one_task(FFExecutor *e, void *lc)
{
    /* FFTaskCallbacks *cb = &e->cb; */
    FFTask **prev = &e->tasks;

    if (*prev) {
        FFTask *t = remove_task(prev, *prev);
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
    /* FFTaskCallbacks *cb = &e->cb; */
    FFTask **prev;

    /* if (e->thread_count) */
    /*     ff_mutex_lock(&e->lock); */
    if (t) {
        for (prev = &e->tasks; *prev /* && cb->priority_higher(*prev, t) */; prev = &(*prev)->next)
            /* nothing */;
        add_task(prev, t);
    }
    /*if (e->thread_count) {
        ff_cond_signal(&e->cond);
        ff_mutex_unlock(&e->lock);
    }*/

    if (!e->thread_count) {
        if (e->recursive)
            return;
        e->recursive = true;
        // We are running in a single-threaded environment, so we must handle all tasks ourselves
        while (run_one_task(e, e->local_contexts))
            /* nothing */;
        e->recursive = false;
    }
}
