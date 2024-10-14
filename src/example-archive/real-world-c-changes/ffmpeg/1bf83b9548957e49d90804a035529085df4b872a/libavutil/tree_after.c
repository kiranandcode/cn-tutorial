/*
 * copyright (c) 2006 Michael Niedermayer <michaelni@gmx.at>
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

#include <stdlib.h>

typedef struct AVTreeNode{
    struct AVTreeNode *child[2];
    void *elem;
    int state;
}AVTreeNode;

const int av_tree_node_size = sizeof(AVTreeNode);

void *av_tree_find(const AVTreeNode *t, void *key, int (*cmp)(void *key, const void *b), void *next[2]){
    if(t){
        unsigned int v= cmp(key, t->elem);
        if(v){
            if(next) next[v>>31]= t->elem;
            return av_tree_find(t->child[(v>>31)^1], key, cmp, next);
        }else{
            if(next){
                av_tree_find(t->child[0], key, cmp, next);
                av_tree_find(t->child[1], key, cmp, next);
            }
            return t->elem;
        }
    }
    return NULL;
}

void av_tree_enumerate(AVTreeNode *t, void *opaque, int (*cmp)(void *opaque, void *elem), int (*enu)(void *opaque, void *elem)){
    int v= cmp ? cmp(opaque, t->elem) : 0;
    if(v>=0) av_tree_enumerate(t->child[0], opaque, cmp, enu);
    if(v==0) enu(opaque, t->elem);
    if(v<=0) av_tree_enumerate(t->child[1], opaque, cmp, enu);
}
