/*
 * ParX - actions.h
 * Parser Actions
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

#ifndef __ACTIONS_H
#define __ACTIONS_H

#include "primtype.h"
#include "datastruct.h"

extern void start_parx(void);
extern void end_parx(void);

extern void show_status(void);

extern void call_subset(tmstring smeas, tmstring sdata_s, tmstring sdata_d);

extern void call_simulate(tmstring sstim, tmstring ssys, tmstring sdata,
                          fnum prec, inum maxiter, inum trace);

extern void call_extract(tmstring ssys, tmstring sdata, fnum prec, fnum tol,
                         opttype opt, fnum sens, inum maxiter, inum trace);

extern void print(parxsymbol_list sl, tmstring fname);
extern void plot(parxsymbol_list sl, tmstring fname);

#endif
