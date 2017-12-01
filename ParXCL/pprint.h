/*
 * ParX - pprint.h
 * Pretty Print the contents of database nodes
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

#ifndef __PPRINT_H
#define __PPRINT_H

#include "datastruct.h"
#include "dbase.h"

extern void list_type(tags_dbnode tag);
extern void list_parxsymbol(parxsymbol_list sl);
extern void show_parxsymbol(parxsymbol_list sl);

#endif
