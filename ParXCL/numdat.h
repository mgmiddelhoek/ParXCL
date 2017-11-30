/*
 * ParX - numdat.h
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

#ifndef __NUMDAT_H
#define __NUMDAT_H

/* include used datastructure specifications */

#include "datastruct.h"
#include "datatpl.h"
#include "modlib.h"

extern numblock make_numblock(modeltemplate mt, systemtemplate st,
                              datatemplate dt, inum trace);
extern void get_xst(modeltemplate mt, datatemplate dt, fnum_list xval,
                    stateflag_list xstat, inum_list xtrans, inum trace);
extern void get_pst(modeltemplate mt, systemtemplate st, stateflag_list pstat,
                    fnum_list pdval, fnum_list plval, fnum_list puval, inum trace);
extern void get_fst(modeltemplate mt, systemtemplate st, fnum_list fval, inum trace);
extern void get_cst(modeltemplate mt, systemtemplate st, fnum_list cval, inum trace);
extern inum get_rmt(modeltemplate mt);
extern inum get_amt(modeltemplate mt, fnum_list aval, inum trace);
extern pset_list make_parmset(modres mrs, inum setid, statevector pstat,
                              fnum_list pdval, fnum_list plval, fnum_list puval);
extern cset_list make_consset(inum setid, fnum_list cval);
extern fset_list make_flagset(inum setid, fnum_list fval);
extern aset_list make_auxsset(inum setid, fnum_list aval);
extern xgroup_list make_xextgrp(modres mrs, pset parmset, statevector xstat,
                                inum_list xtrans, fnum_list xval, datarow_list data);
extern void read_numblock_p(modres mrs, pset pi, modeltemplate mt, systemtemplate st);
extern inum set_xst(modeltemplate mt, datatemplate dt, stateflag_list xstat, inum_list xtrans);
extern void read_numblock_x(modres mrs, xgroup xg, modeltemplate mt, datatemplate dt);
extern void write_numblock(numblock numb, tmstring s);

#endif
