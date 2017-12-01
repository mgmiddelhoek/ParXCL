/*
 * ParX - jsonio.h
 * JSON I/O for database nodes
 *
 * Copyright (c) 2012 M.G.Middelhoek <martin@middelhoek.com>
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

#ifndef __JSONIO_H
#define __JSONIO_H

#include "cJSON.h"

extern void write_data_json(FILE *fp, datatemplate dt);
extern inum read_data_json(FILE *fp, datatemplate *dt);

extern void write_sys_json(FILE *fp, systemtemplate st);

#endif
