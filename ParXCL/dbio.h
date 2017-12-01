/*
 * ParX - dbio.h
 * I/O for Database Nodes
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

#ifndef __DBIO_H
#define __DBIO_H

#include "primtype.h"
#include "dbase.h"

extern modeltemplate get_model(tmstring name);
extern codefile get_modelcode(tmstring name);
extern systemtemplate get_system(tmstring fname);
extern datatemplate get_datatable(tmstring fname);
extern void put_system(systemtemplate st, tmstring fname);
extern void put_datatable(datatemplate dt, tmstring fname);

#endif
