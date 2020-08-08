/*
 * ParX - modlib.c
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "modlib.h"
#include "parx.h"

/* find a named entry in the model library */

parxmodel find_model_proc(tmstring name) {
    inum i;

    for (i = 0; i < modlib_size; i++) {
        if (strcmp(modlib[i].name, name) == 0) {
            return (modlib[i].p);
        }
    }
    return (parxmodelNIL);
}

/* reverse a xset list */

xset_list rev_xset_list(xset_list l) {
    xset_list last, next;

    last = xsetNIL;

    while (l != xsetNIL) {
        next = l->next;
        l->next = last;
        last = l;
        l = next;
    }
    return (last);
}
