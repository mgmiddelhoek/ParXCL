/*
 * ParX - datatpl.c
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

#include "parx.h"
#include "datatpl.h"

/* find a named variable in de header list */

colhead find_header(colhead_list l, tmstring name) {
    
    for (; l != colheadNIL; l = l->next) {
        if (strcmp(name, l->name) == 0) {
            return (l);
        }
    }
    return (colheadNIL);
}

/* find a datarow by its rowid, searches are in partial order */

datarow find_datarow(datarow_list l, datarow_list last, inum id) {
    datarow r;
    
    /* start by searching the remainder of the list */
    for (r = last; r != datarowNIL; r = r->next) {
        if (id == r->rowid) {
            return (r);
        }
    }
    /* search top part */
    for (r = l; r != last; r = r->next) {
        if (id == r->rowid) {
            return (r);
        }
    }
    return (datarowNIL);
}
