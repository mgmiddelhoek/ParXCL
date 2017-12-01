/*
 * ParX - dbase.h
 * Database definition
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

#ifndef __DBASE_H
#define __DBASE_H

/* include used datastructure specifications */

#include "primtype.h"
#include "datastruct.h"
#include "datatpl.h"

/* macro's for unpacking parser stack nodes */

/* pn -> parxsymbol_list */
#define SYMLIST(N)	((N) ? ((to_Symnode(N))->sym) : parxsymbolNIL)
/* pn -> tmstring */
#define SYM(N)		((N) ? ((to_Symnode(N))->sym->name) : tmstringNIL)
/* pn -> tmstring */
#define STR(N)		((N) ? ((to_Strnode(N))->str) : tmstringNIL)

/* global variables */

extern dbnode_list dbase;

#define DEF_DBASE_FILE	"parx.st"

/* functions defined in dbase.ct */

extern parsenode dec_sym(tmstring s);
extern parsenode dec_str(tmstring s);
extern parsenode concat_parxsymbol(parsenode pa, parsenode pb);
extern void stat_end(void);
extern void init_dbase(void);
extern void output_dbase(tmstring fname);
extern void input_dbase(tmstring fname);
extern tags_dbnode tag_dbnode(dbnode n);
extern tmstring name_dbnode(dbnode n);
extern void del_dbnode(dbnode n);
extern void copy_dbnode(dbnode dn, dbnode sn);
extern dbnode check_name(tmstring name);
extern dbnode find_dbnode(tmstring name, tags_dbnode tag);
extern void dec_dbnode(parxsymbol_list sl, tags_dbnode tag);
extern systemtemplate mod_sys(dbnode mnode);
extern dbnode find_model_dbnode(tmstring mname);
extern void dec_sys(tmstring sname, tmstring mname);
extern void dec_data(tmstring name);
extern void dec_stim(tmstring name);
extern void dec_meas(tmstring name);
extern void input_dbnode(tmstring s, tmstring f);
extern void output_dbnode(tmstring s, tmstring f);
extern syspar find_syspar(syspar_list l, tmstring name);
extern stimtemplate find_stim(stimtemplate_list l, tmstring name);
extern meastemplate find_meas(meastemplate_list l, tmstring name);
extern syspar sys_set(dbnode n, tmstring name);
extern stimtemplate stim_set(dbnode n, tmstring name);
extern meastemplate meas_set(dbnode n, tmstring name);
extern void init_sys_set(dbnode n, syspar p);
extern void init_stim_set(dbnode n, stimtemplate st);
extern void init_meas_set(dbnode n, meastemplate mt);
extern void cast_syspar(syspar p, stateflag f);

#endif
