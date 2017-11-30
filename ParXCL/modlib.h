/*
 * ParX - modlib.h
 *
 * Copyright (c) 1990 M.G.Middelhoek <martin@middelhoek.com>
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

#ifndef __MODLIB_H
#define __MODLIB_H

#include "primtype.h"
#include "datastruct.h"

/* model library index table */

typedef boolean(*parxmodel)(modreq req, modres res);
#define parxmodelNIL (parxmodel)0

typedef struct {
    tmstring name;
    parxmodel p;
} ModelLibrary[];

extern ModelLibrary modlib; /* default model library */
extern inum modlib_size; /* number of entries - 1 */

extern parxmodel find_model_proc(tmstring name);
extern xset_list rev_xset_list(xset_list l);

#endif

