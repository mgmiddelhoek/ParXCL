/*
 * ParX - mem_def.h
 * Model Compiler memory management functions
 *
 * Copyright (c) 1994 M.G.Middelhoek <martin@middelhoek.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __MEM_DEF_H
#define __MEM_DEF_H

struct MEM_LEAF {
    struct MEM_LEAF *next;
    void *mem;
};

struct MEM_TREE {
    struct MEM_LEAF *first;
    struct MEM_LEAF *last;
    long cnt;
    size_t size;
};


extern struct MEM_TREE *mem_tree(void);
extern void *mem_slot(struct MEM_TREE * tptr, size_t size);
extern size_t mem_free(struct MEM_TREE * tptr);

#endif
