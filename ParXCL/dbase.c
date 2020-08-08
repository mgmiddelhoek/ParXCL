/*
 * ParX - dbase.c
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

#include "dbase.h"
#include "dbio.h"
#include "error.h"
#include "parx.h"

/* since in the linked list library you can't distinguish between
 * an empty database and no database at all, there is the flag 'gotdbase'.
 */
static boolean gotdbase = FALSE;
dbnode_list dbase; /* main parser database */

static boolean gotparselist = FALSE;
static parsenode_list parselist = parsenodeNIL; /* list of parsed tokens */

/* parse list handling */

/* declare a new parser node, put it in the parselist, and return its pointer */

parsenode dec_sym(tmstring s) {
    parsenode pn;
    parxsymbol_list sl;

    if (!gotparselist) {
        parselist = new_parsenode_list();
        gotparselist = TRUE;
    }
    sl = new_parxsymbol_list();
    sl = append_parxsymbol_list(sl, new_parxsymbol(new_tmstring(s)));
    pn = new_Symnode(sl);
    parselist = insert_parsenode_list(parselist, 0, pn);
    return (pn);
}

parsenode dec_str(tmstring s) {
    parsenode pn;

    if (!gotparselist) {
        parselist = new_parsenode_list();
        gotparselist = TRUE;
    }
    pn = new_Strnode(new_tmstring(s));
    parselist = insert_parsenode_list(parselist, 0, pn);
    return (pn);
}

/* move a parxsymbol list from one parsenode to another by concatenation */

parsenode concat_parxsymbol(parsenode pa, parsenode pb) {
    parxsymbol_list sl;

    sl = concat_parxsymbol_list(to_Symnode(pa)->sym, to_Symnode(pb)->sym);
    to_Symnode(pa)->sym = sl;
    to_Symnode(pb)->sym = new_parxsymbol_list();
    return (pa);
}

/* end of input statement detected, clear the parselist */

void stat_end(void) {
    if (gotparselist) {
        rfre_parsenode_list(parselist);
    }
    gotparselist = FALSE;
}

/* dblist handling */

/* clear dbase */

void init_dbase(void) {
    if (gotdbase) {
        rfre_dbnode_list(dbase);
    }
    gotdbase = TRUE;
    dbase = new_dbnode_list();
}

/* store dbase in file */

void output_dbase(tmstring fname) {
    FILE *f;
    TMPRINTSTATE *st;

    if (fname == tmstringNIL) {
        fname = DEF_DBASE_FILE;
    }

    f = fopen(fname, "w");
    if (!f) {
        errcode = NO_FILE_PERR;
        error(fname);
        return;
    }
    st = tm_setprint(f, 1, 80, 8, 0);
    print_dbnode_list(st, dbase);

    fclose(f);
    tm_endprint(st);
    return;
}

/* restore dbase from file */

void input_dbase(tmstring fname) {
    FILE *f;
    dbnode_list newdbase;

    if (fname == tmstringNIL) {
        fname = DEF_DBASE_FILE;
    }

    f = fopen(fname, "r");
    if (!f) {
        errcode = NO_FILE_PERR;
        error(fname);
        return;
    }

    if (fscan_dbnode_list(f, &newdbase)) {
        errcode = TMERROR_PERR;
        error(tm_errmsg);
        rfre_dbnode_list(newdbase);
    } else {
        init_dbase();
        dbase = newdbase;
        gotdbase = TRUE;
    }

    fclose(f);
}

/* return the tag of a dbnode */

tags_dbnode tag_dbnode(dbnode n) { return (n->tag); }

/* return the name of a dbnode */

tmstring name_dbnode(dbnode n) {
    switch (tag_dbnode(n)) {
    case TAGModel:
        return (to_Model(n)->modname);
    case TAGSystem:
        return (to_System(n)->sysname);
    case TAGDatatable:
        return (to_Datatable(n)->dataname);
    case TAGStimulus:
        return (to_Stimulus(n)->stimname);
    case TAGMeasurement:
        return (to_Measurement(n)->measname);
    default:
        return (tmstringNIL);
    }
    return (tmstringNIL);
}

/* delete a database node */

void del_dbnode(dbnode n) {
    dbnode *p;

    if (tag_dbnode(n) == TAGModel) {
        return;
    }

    /* get pointer to pointer to node */
    for (p = &dbase; (*p != dbnodeNIL) && (*p != n); p = &((*p)->next)) {
    }

    assert(*p != dbnodeNIL); /* node must be in list! */

    *p = n->next; /* skip node in list */

    rfre_dbnode(n);
}

void copy_dbnode(dbnode dn, dbnode sn) {
    if (tag_dbnode(dn) != tag_dbnode(sn)) {
        errcode = ILL_ASSIGN_PERR;
        error(name_dbnode(dn));
        return;
    }

    switch (tag_dbnode(dn)) {
    case TAGSystem:
        rfre_systemtemplate(to_System(dn)->sysdata);
        to_System(dn)->sysdata = rdup_systemtemplate(to_System(sn)->sysdata);
        break;
    case TAGDatatable:
        rfre_datatemplate(to_Datatable(dn)->datdata);
        to_Datatable(dn)->datdata =
            rdup_datatemplate(to_Datatable(sn)->datdata);
        break;
    case TAGStimulus:
        rfre_stimtemplate_list(to_Stimulus(dn)->stimdata);
        to_Stimulus(dn)->stimdata =
            rdup_stimtemplate_list(to_Stimulus(sn)->stimdata);
        break;
    case TAGMeasurement:
        rfre_meastemplate_list(to_Measurement(dn)->measdata);
        to_Measurement(dn)->measdata =
            rdup_meastemplate_list(to_Measurement(sn)->measdata);
        break;
    default:
        errcode = ILL_ASSIGN_PERR;
        error(name_dbnode(dn));
        return;
    }
}

/* check for name in dbase */

dbnode check_name(tmstring name) {
    dbnode n;

    for (n = dbase; n != dbnodeNIL; n = n->next) {
        if (strcmp(name, name_dbnode(n)) == 0) {
            return (n);
        }
    }
    return (dbnodeNIL);
}

/* find a named node in the database */

dbnode find_dbnode(tmstring name, tags_dbnode tag) {
    dbnode n;

    for (n = dbase; n != dbnodeNIL; n = n->next) {
        if (strcmp(name, name_dbnode(n)) == 0) {
            if ((tag == TAGName) || (tag == tag_dbnode(n))) {
                return (n);
            } else {
                errcode = ILL_TYPE_PERR;
                error(name);
                return (dbnodeNIL);
            }
        }
    }

    errcode = UNK_IDENT_PERR;
    error(name);
    return (dbnodeNIL);
}

/* add a node to the database */

void dec_dbnode(parxsymbol_list sl, tags_dbnode tag) {
    for (; sl != parxsymbolNIL; sl = sl->next) {

        /* no redeclaration allowed */
        if (check_name(sl->name) != dbnodeNIL) {
            errcode = ILL_REDEC_PERR;
            error(sl->name);
            if (tag == TAGSystem) {
                sl = sl->next; /* skip model name */
            }
            continue;
        }

        switch (tag) {
        case TAGSystem:
            dec_sys(sl->name, sl->next->name);
            sl = sl->next; /* skip model name */
            break;
        case TAGDatatable:
            dec_data(sl->name);
            break;
        case TAGStimulus:
            dec_stim(sl->name);
            break;
        case TAGMeasurement:
            dec_meas(sl->name);
            break;
        default:
            break;
        }
    }
}

/* copy data from model node to system template */

systemtemplate mod_sys(dbnode mnode) {
    syspar_list pl;
    syspar sp;
    pspec ps;
    cspec cs;
    fspec fs;
    systemtemplate st;
    modeltemplate mt;

    mt = to_Model(mnode)->moddata;

    /* copy default parameter list from model to system */

    pl = new_syspar_list();

    for (ps = mt->parm; ps != pspecNIL; ps = ps->next) {
        sp = new_syspar(new_tmstring(ps->name),
                        new_Punkn(ps->dval, ps->lval, ps->uval));
        pl = append_syspar_list(pl, sp);
    }

    /* copy default constant list from model to system */

    for (cs = mt->cons; cs != cspecNIL; cs = cs->next)
        pl = append_syspar_list(
            pl, new_syspar(new_tmstring(cs->name), new_Pconst(cs->dval)));

    /* copy default flag list from model to system */

    for (fs = mt->flags; fs != fspecNIL; fs = fs->next)
        pl = append_syspar_list(
            pl, new_syspar(new_tmstring(fs->name), new_Pflag(fs->dval)));

    st = new_systemtemplate(new_tmstring(""),
                            new_tmstring(to_Model(mnode)->modname), pl);

    return (st);
}

dbnode find_model_dbnode(tmstring mname) {
    dbnode mnode;
    modeltemplate mt;

    mnode = check_name(mname);
    if (mnode != dbnodeNIL) {
        if (tag_dbnode(mnode) == TAGModel) {
            return (mnode);
        } else {
            errcode = ILL_MNAME_PERR;
            error(mname);
            return (dbnodeNIL);
        }
    }

    /* model not in dbase, try to load it */

    if ((mt = get_model(mname)) == modeltemplateNIL) {
        return (dbnodeNIL);
    }

    mnode = new_Model(new_tmstring(mname), mt);
    dbase = append_dbnode_list(dbase, mnode);

    return (mnode);
}

void dec_sys(tmstring sname, tmstring mname) {
    dbnode mnode, snode;

    if ((mnode = find_model_dbnode(mname)) == dbnodeNIL) {
        return;
    }

    snode = new_System(new_tmstring(sname), mod_sys(mnode));

    dbase = append_dbnode_list(dbase, snode);
}

void dec_data(tmstring name) {
    datatemplate dt;
    dbnode dtab;

    dt = new_datatemplate(new_tmstring(""), new_colhead_list(),
                          new_datarow_list());
    dtab = new_Datatable(new_tmstring(name), dt);
    dbase = append_dbnode_list(dbase, dtab);
}

void dec_stim(tmstring name) {
    dbase = append_dbnode_list(
        dbase, new_Stimulus(new_tmstring(name), new_stimtemplate_list()));
}

void dec_meas(tmstring name) {
    dbase = append_dbnode_list(
        dbase, new_Measurement(new_tmstring(name), new_meastemplate_list()));
}

void input_dbnode(tmstring s, tmstring f) {
    dbnode n;
    systemtemplate st;
    datatemplate dt;

    if ((n = find_dbnode(s, TAGName)) == dbnodeNIL) {
        return;
    }

    switch (tag_dbnode(n)) {
    case TAGSystem:
        if ((st = get_system(f)) == systemtemplateNIL)
            return;
        if (strcmp(st->model, to_System(n)->sysdata->model) == 0) {
            rfre_systemtemplate(to_System(n)->sysdata);
            to_System(n)->sysdata = st;
        } else {
            rfre_systemtemplate(st);
            errcode = ILL_TYPE_PERR;
            error(s);
        }
        break;
    case TAGDatatable:
        if ((dt = get_datatable(f)) == datatemplateNIL) {
            return;
        }
        rfre_datatemplate(to_Datatable(n)->datdata);
        to_Datatable(n)->datdata = dt;
        break;
    default:
        errcode = WRONG_ARG_PERR;
        error(s);
        break;
    }
}

void output_dbnode(tmstring s, tmstring f) {
    dbnode n;
    systemtemplate st;
    datatemplate dt;

    if ((n = find_dbnode(s, TAGName)) == dbnodeNIL) {
        return;
    }

    switch (tag_dbnode(n)) {
    case TAGSystem:
        st = to_System(n)->sysdata;
        if (st == systemtemplateNIL) {
            return;
        }
        put_system(st, f);
        break;
    case TAGDatatable:
        dt = to_Datatable(n)->datdata;
        if (dt == datatemplateNIL) {
            return;
        }
        put_datatable(dt, f);
        break;
    default:
        errcode = WRONG_ARG_PERR;
        error(s);
        break;
    }
}

/* find named field functions */

syspar find_syspar(syspar_list l, tmstring name) {
    for (; l != sysparNIL; l = l->next) {
        if (strcmp(name, l->name) == 0) {
            return (l);
        }
    }
    return (l);
}

stimtemplate find_stim(stimtemplate_list l, tmstring name) {
    for (; l != stimtemplateNIL; l = l->next) {
        if (strcmp(name, l->name) == 0) {
            return (l);
        }
    }
    return (l);
}

meastemplate find_meas(meastemplate_list l, tmstring name) {
    for (; l != meastemplateNIL; l = l->next) {
        if (strcmp(name, l->name) == 0) {
            return (l);
        }
    }
    return (l);
}

/* field access functions */

syspar sys_set(dbnode n, tmstring name) {
    systemtemplate st;
    syspar_list l;

    st = to_System(n)->sysdata;

    l = (st == systemtemplateNIL) ? sysparNIL : st->parm;

    l = find_syspar(l, name);

    if (l == sysparNIL) {
        errcode = UNK_FIELD_PERR;
        error(name);
    }
    return (l);
}

stimtemplate stim_set(dbnode n, tmstring name) {
    stimtemplate_list l;

    l = to_Stimulus(n)->stimdata;

    l = find_stim(l, name);

    if (l != stimtemplateNIL) {
        return (l);
    }

    /* not in list so make a new one */

    l = new_stimtemplate(new_tmstring(name), 0.0, 0.0, 0, SLIN);

    to_Stimulus(n)->stimdata =
        append_stimtemplate_list(to_Stimulus(n)->stimdata, l);

    return (l);
}

meastemplate meas_set(dbnode n, tmstring name) {
    meastemplate_list l;

    l = to_Measurement(n)->measdata;

    l = find_meas(l, name);

    if (l != meastemplateNIL) {
        return (l);
    }

    /* not in list so make a new one */

    if (name[0] == '0') { /* group spec. */
        l = new_meastemplate(new_tmstring(name), 0L, -1.0, 1.0, 1);
    } else if (isdigit(name[0])) { /* curve nr. */
        l = new_meastemplate(new_tmstring(name), atoi(name), -INF, INF, 1);
    } else { /* external quant. */
        l = new_meastemplate(new_tmstring(name), -1L, -INF, INF, 1);
    }

    to_Measurement(n)->measdata =
        append_meastemplate_list(to_Measurement(n)->measdata, l);

    return (l);
}

void init_sys_set(dbnode n, syspar p) {
    systemtemplate st;
    dbnode mnode;
    modeltemplate mt;
    pspec_list mp;
    cspec_list mc;
    fspec_list mf;

    st = to_System(n)->sysdata;

    mnode = check_name(st->model);

    mt = to_Model(mnode)->moddata;

    mp = mt->parm;

    for (; mp != pspecNIL; mp = mp->next) {
        if (strcmp(p->name, mp->name) == 0) {
            break;
        }
    }

    if (mp != pspecNIL) {
        cast_syspar(p, UNKN);
        to_Punkn(p->val)->unknval = mp->dval;
        to_Punkn(p->val)->unknlval = mp->lval;
        to_Punkn(p->val)->unknuval = mp->uval;
        return;
    }

    mc = mt->cons;

    for (; mc != cspecNIL; mc = mc->next) {
        if (strcmp(p->name, mc->name) == 0) {
            break;
        }
    }

    if (mc != cspecNIL) {
        to_Pconst(p->val)->constval = mc->dval;
        return;
    }

    mf = mt->flags;

    for (; mf != fspecNIL; mf = mf->next) {
        if (strcmp(p->name, mf->name) == 0) {
            break;
        }
    }

    if (mf != fspecNIL) {
        to_Pflag(p->val)->flagval = mf->dval;
        return;
    }
}

void init_stim_set(dbnode n, stimtemplate st) {
    stimtemplate *p;

    /* get pointer to pointer to node */

    p = &(to_Stimulus(n)->stimdata);
    for (; (*p != stimtemplateNIL) && (*p != st); p = &((*p)->next)) {
    }

    assert(*p != stimtemplateNIL); /* node must be in list! */

    *p = st->next; /* skip node in list */

    rfre_stimtemplate(st);
}

void init_meas_set(dbnode n, meastemplate mt) {
    meastemplate *p;

    /* get pointer to pointer to node */

    p = &(to_Measurement(n)->measdata);
    for (; (*p != meastemplateNIL) && (*p != mt); p = &((*p)->next)) {
    }

    assert(*p != meastemplateNIL); /* node must be in list! */

    *p = mt->next; /* skip node in list */

    rfre_meastemplate(mt);
}

void cast_syspar(syspar p, stateflag f) {
    parmval np;
    fnum val, lval, uval, intv;

    /* write to tmp variables */

    switch (p->val->tag) {
    case TAGPunkn:
        val = to_Punkn(p->val)->unknval;
        lval = to_Punkn(p->val)->unknlval;
        uval = to_Punkn(p->val)->unknuval;
        intv = INF;
        break;
    case TAGPmeas:
        val = to_Pmeas(p->val)->measval;
        lval = MININF;
        uval = INF;
        intv = to_Pmeas(p->val)->measint;
        break;
    case TAGPcalc:
        val = to_Pcalc(p->val)->calcval;
        lval = MININF;
        uval = INF;
        intv = to_Pcalc(p->val)->calcint;
        break;
    case TAGPfact:
        val = to_Pfact(p->val)->factval;
        lval = MININF;
        uval = INF;
        intv = 0.0;
        break;
    default:
        val = to_Punkn(p->val)->unknval;
        lval = to_Punkn(p->val)->unknlval;
        uval = to_Punkn(p->val)->unknuval;
        intv = INF;
        break;
    }

    switch (f) {
    case UNKN:
        if (p->val->tag == TAGPunkn) {
            return;
        }
        np = new_Punkn(val, lval, uval);
        break;
    case MEAS:
        if (p->val->tag == TAGPmeas) {
            return;
        }
        np = new_Pmeas(val, intv);
        break;
    case CALC:
        if (p->val->tag == TAGPcalc) {
            return;
        }
        np = new_Pcalc(val, intv);
        break;
    case FACT:
        if (p->val->tag == TAGPfact) {
            return;
        }
        np = new_Pfact(val);
        break;
    default:
        return;
    }

    fre_parmval(p->val);
    p->val = np;
}
