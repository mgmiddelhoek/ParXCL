/*
 * ParX - numdat.c
 * Numerical data interface block
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
#include "error.h"
#include "dbase.h"
#include "dbio.h"
#include "prxinter.h"
#include "numdat.h"

/* create and fill a numblock */

numblock make_numblock(modeltemplate mt, systemtemplate st,
                       datatemplate dt, inum trace) {
    numblock numb;
    boolean modex; /* is the model external */
    parxmodel proc;
    codefile mcode;
    inum nres, naux;
    modreq mrq;
    modres mrs;
    xspec xs;
    colhead_list header;
    pset_list parmset;
    cset_list consset;
    fset_list flagset;
    aset_list auxsset;
    xgroup_list xextgrp;
    stateflag_list xstat, pstat;
    inum_list xtrans;
    fnum_list xval;
    fnum_list pdval, plval, puval;
    fnum_list cval;
    fnum_list aval;
    fnum_list fval;
    statevector xstatv, pstatv;
    inum setid;
    boolean rc, rci;
    
    /* model interface fields */
    moddat modi; /* model interface structure */
    vector m_x; /* variable vector */
    vector m_a; /* auxiliary vector */
    vector m_p; /* parameter vector */
    vector m_c; /* constant vector */
    vector m_f; /* flag vector */
    vector m_r; /* residual vector */
    matrix m_jp; /* Jacobian matrix dr/dp */
    matrix m_jx; /* Jacobian matrix dr/dx */
    matrix m_ja; /* Jacobian matrix dr/da */
    boolvector xf, pf; /* evaluation flags */
    
    assert(mt != modeltemplateNIL);
    assert(st != systemtemplateNIL);
    assert(dt != datatemplateNIL);
    
    /* get model interface procedure from modlib */
    
    modex = FALSE;
    mcode = codefileNIL;
    
    proc = find_model_proc(mt->id);
    
    if (proc == parxmodelNIL) { /* not an internal model */
        
        /* check for code file */
        
        mcode = get_modelcode(st->model);
        
        if (mcode == codefileNIL) { /* not an interpreted model either */
            errcode = UNK_MODEL_PERR;
            error(st->model);
            return (numblockNIL);
        }
        
        modex = TRUE;
    }
    
    /* print info */
    
    fprintf(trace_stream, "\nModel name    : %s\n", st->model);
    fprintf(trace_stream, "Model type    : %s\n", mt->id);
    fprintf(trace_stream, "Model version : %s\n", mt->version);
    fprintf(trace_stream, "Model author  : %s\n", mt->author);
    fprintf(trace_stream, "Model date    : %s\n", mt->date);
    fprintf(trace_stream, "Model info.   : %s\n\n", mt->info);
    if (strlen(dt->info) != 0)
        fprintf(trace_stream, "Data info.    : %s\n\n", dt->info);
    if (strlen(st->info) != 0)
        fprintf(trace_stream, "System info.  : %s\n\n", st->info);
    
    
    /* check if data available */
    
    if (dt->data == datarowNIL) {
        errcode = NO_DATA_SERR;
        error(dt->info);
        return (numblockNIL);
    }
    
    for (xs = mt->xext; xs != xspecNIL; xs = xs->next) {
        for (header = dt->header; header != colheadNIL; header = header->next) {
            if (strcmp(xs->name, header->name) == 0)
                break;
        }
        if (header == colheadNIL) {
            errcode = NO_KEY_SERR;
            error(xs->name);
            return (numblockNIL);
        }
    }
    
    /* get state, transpose and value lists */
    
    xstat = new_stateflag_list(); /* xext state vector */
    xtrans = new_inum_list(); /* xext data vector transformation */
    xval = new_fnum_list(); /* variable values */
    get_xst(mt, dt, xval, xstat, xtrans, trace);
    xstatv = new_statevector((inum) xstat->sz, (statearray) (xstat->arr));
    
    pstat = new_stateflag_list(); /* parm state vector */
    pdval = new_fnum_list(); /* parameter values */
    plval = new_fnum_list(); /* parameter lower bounds */
    puval = new_fnum_list(); /* parameter upper bounds */
    get_pst(mt, st, pstat, pdval, plval, puval, trace);
    pstatv = new_statevector((inum) pstat->sz, (statearray) (pstat->arr));
    
    cval = new_fnum_list(); /* constant values */
    get_cst(mt, st, cval, trace);
    
    fval = new_fnum_list(); /* flag values */
    get_fst(mt, st, fval, trace);
    
    aval = new_fnum_list(); /* aux values */
    naux = get_amt(mt, aval, trace);
    
    nres = get_rmt(mt);
    
    mrq = modreqNIL;
    
    if (modex == FALSE) { /* Compiled model */
        
        /* construct a model request */
        
        mrq = new_modreq((inum) xstat->sz, (inum) pstat->sz, xstatv, pstatv);
        
        /* setup a default model result */
        
        mrs = new_modres(0, 0, 0, 0, 0, 0, statevectorNIL, statevectorNIL,
                         procedureNIL, procedureNIL, procedureNIL, procedureNIL, procedureNIL);
        
        /* call model interface procedure */
        
        rc = (* proc)(mrq, mrs);
        
        if (rc == FALSE) {
            rfre_modreq(mrq);
        }
        
    } else { /* Interpreted model */
        
        mrs = new_modres(nres, (inum) xstat->sz, naux,
                         (inum) pstat->sz, (inum) cval->sz, (inum) fval->sz,
                         statevectorNIL, statevectorNIL, procedureNIL,
                         procedureNIL, procedureNIL, procedureNIL, procedureNIL);
        
        mrs->xstat = rdup_statevector(xstatv);
        mrs->pstat = rdup_statevector(pstatv);
        mrs->model = (procedure) prx_compute;
        
        rc = TRUE;
    }
    
    if (rc == FALSE) {
        errcode = ILL_SETUP_SERR;
        error(mt->id);
        numb = numblockNIL;
        
    } else {
        
        /* construct parameter set, constant set and flag set */
        
        setid = 0; /* default set id */
        
        parmset = make_parmset(mrs, setid, pstatv, pdval, plval, puval);
        consset = make_consset(setid, cval);
        flagset = make_flagset(setid, fval);
        auxsset = make_auxsset(setid, aval);
        
        /* construct externals set */
        
        xextgrp = make_xextgrp(mrs, parmset, xstatv, xtrans, xval, dt->data);
        
        /* construct model interface structure */
        
        m_x = rnew_vector(mrs->nx);
        m_a = rnew_vector(mrs->na);
        m_p = rnew_vector(mrs->np);
        m_c = rnew_vector(mrs->nc);
        m_f = rnew_vector(mrs->nf);
        m_r = rnew_vector(mrs->nr);
        xf = rnew_boolvector(mrs->nx);
        m_jx = rnew_matrix(mrs->nr, mrs->nx);
        m_ja = rnew_matrix(mrs->nr, mrs->na);
        pf = rnew_boolvector(mrs->np);
        m_jp = rnew_matrix(mrs->nr, mrs->np);
        
        modi = new_moddat(m_x, m_a, m_p, m_c, m_f, FALSE, m_r,
                          FALSE, xf, m_jx, m_ja, FALSE, pf, m_jp);
        
        /* construct numerical data block */
        
        numb = new_numblock(mrs, modi, parmset, consset, flagset, auxsset,
                            xextgrp);
        
        rci = TRUE;
        
        if (modex == TRUE) {
            rci = prx_inCode(modi, mcode);
            if (rci == FALSE) error(mt->id);
            fclose(mcode);
        }
        
        if ((parmset == pset_listNIL) || (xextgrp == xgroup_listNIL) || (rci == FALSE)) {
            /* something went wrong with setup */
            rfre_numblock(numb);
            numb = numblockNIL;
        }
        
        /* shared data */
        fre_statevector(xstatv);
        fre_statevector(pstatv);
        if (modex == FALSE && mrq != modreqNIL)
            fre_modreq(mrq);
    }
    
    /* free all unused data */
    
    fre_stateflag_list(xstat);
    fre_inum_list(xtrans);
    
    fre_stateflag_list(pstat);
    fre_fnum_list(pdval);
    fre_fnum_list(plval);
    fre_fnum_list(puval);
    
    fre_fnum_list(cval);
    fre_fnum_list(aval);
    fre_fnum_list(xval);
    fre_fnum_list(fval);
    
    return (numb);
}

void get_xst(modeltemplate mt, datatemplate dt, fnum_list xval,
             stateflag_list xstat, inum_list xtrans, inum trace) {
    xspec xs;
    colhead_list header;
    inum index;
    inum idxu = 1;
    inum idxm = 1;
    inum i = 1;
    
    if (trace >= 1)
        fprintf(trace_stream, "\nVariable Mapping:\n\n");
    
    for (xs = mt->xext; xs != xspecNIL; xs = xs->next) {
        
        for (header = dt->header, index = 0; header != colheadNIL; header = header->next, index++) {
            if (strcmp(xs->name, header->name) == 0)
                break;
        }
        
        assert(header != colheadNIL);
        
        xval = append_fnum_list(xval, xs->dval);
        switch (header->type) {
            case UNKN:
                append_stateflag_list(xstat, UNKN);
                append_inum_list(xtrans, index);
                if (trace >= 1) {
                    fprintf(trace_stream, "%-3ld: %3ld   -  (U): %-8s ", (long) i++, (long) idxu++, xs->name);
                    fprintf(trace_stream, "= %.8e \n", xs->dval);
                }
                break;
            case MEAS:
                append_stateflag_list(xstat, MEAS);
                append_inum_list(xtrans, index);
                if (trace >= 1) {
                    fprintf(trace_stream, "%-3ld:   - %3ld  (M): %s\n", (long) i++, (long) idxm++, xs->name);
                }
                break;
            case CALC:
                append_stateflag_list(xstat, CALC);
                append_inum_list(xtrans, index);
                if (trace >= 1) {
                    fprintf(trace_stream, "%-3ld:   - %3ld  (C): %s\n", (long) i++, (long) idxm++, xs->name);
                }
                break;
            case SWEEP:
                append_stateflag_list(xstat, SWEEP);
                append_inum_list(xtrans, index);
                if (trace >= 1) {
                    fprintf(trace_stream, "%-3ld:   - %3ld (Sw): %s\n", (long) i++, (long) idxm++, xs->name);
                }
                break;
            case FACT:
                append_stateflag_list(xstat, FACT);
                append_inum_list(xtrans, index);
                if (trace >= 1) {
                    fprintf(trace_stream, "%-3ld:   -   -  (F): %s\n", (long) i++, xs->name);
                }
                break;
            case STIM:
                append_stateflag_list(xstat, STIM);
                append_inum_list(xtrans, index);
                if (trace >= 1) {
                    fprintf(trace_stream, "%-3ld:   -   - (St): %s\n", (long) i++, xs->name);
                }
                break;
            default: break;
        }
    }
}

void get_pst(modeltemplate mt, systemtemplate st, stateflag_list pstat,
             fnum_list pdval, fnum_list plval, fnum_list puval, inum trace) {
    pspec ps;
    syspar parm;
    parmval pv;
    inum idx = 1;
    inum i = 1;
    
    if (trace >= 1)
        fprintf(trace_stream, "\nParameter Mapping:\n\n");
    
    for (ps = mt->parm; ps != pspecNIL; ps = ps->next) {
        parm = find_syspar(st->parm, ps->name);
        if (parm == sysparNIL) { /* verry illegal setup */
            errcode = MIS_SETUP_SERR;
            error(ps->name);
            exit(1);
        }
        pv = parm->val;
        switch (pv->tag) {
            case TAGPunkn:
                append_stateflag_list(pstat, UNKN);
                pdval = append_fnum_list(pdval, to_Punkn(pv)->unknval);
                plval = append_fnum_list(plval, to_Punkn(pv)->unknlval);
                puval = append_fnum_list(puval, to_Punkn(pv)->unknuval);
                if (trace >= 1) {
                    fprintf(trace_stream, "%-3ld: %3ld (U): %-8s ",
                            (long) i++, (long) idx++, ps->name);
                    fprintf(trace_stream, "= %.8e ", to_Punkn(pv)->unknval);
                    fprintf(trace_stream, "> %.8e ", to_Punkn(pv)->unknlval);
                    fprintf(trace_stream, "< %.8e\n", to_Punkn(pv)->unknuval);
                }
                break;
            case TAGPmeas:
                append_stateflag_list(pstat, MEAS);
                pdval = append_fnum_list(pdval, to_Pmeas(pv)->measval);
                plval = append_fnum_list(plval, to_Pmeas(pv)->measint);
                puval = append_fnum_list(puval, to_Pmeas(pv)->measint);
                if (trace >= 1) {
                    fprintf(trace_stream, "%-3ld:   - (M): %-8s ", (long) i++, ps->name);
                    fprintf(trace_stream, "= %.8e ", to_Pmeas(pv)->measval);
                    fprintf(trace_stream, "# %.8e\n", to_Pmeas(pv)->measint);
                }
                break;
            case TAGPcalc:
                append_stateflag_list(pstat, CALC);
                pdval = append_fnum_list(pdval, to_Pcalc(pv)->calcval);
                plval = append_fnum_list(plval, to_Pcalc(pv)->calcint);
                puval = append_fnum_list(puval, to_Pcalc(pv)->calcint);
                if (trace >= 1) {
                    fprintf(trace_stream, "%-3ld:   - (C): %-8s ", (long) i++, ps->name);
                    fprintf(trace_stream, "= %.8e ", to_Pcalc(pv)->calcval);
                    fprintf(trace_stream, "# %.8e\n", to_Pcalc(pv)->calcint);
                }
                break;
            case TAGPfact:
                append_stateflag_list(pstat, FACT);
                pdval = append_fnum_list(pdval, to_Pfact(pv)->factval);
                plval = append_fnum_list(plval, 0.0);
                puval = append_fnum_list(puval, 0.0);
                if (trace >= 1) {
                    fprintf(trace_stream, "%-3ld:   - (F): %-8s ", (long) i++, ps->name);
                    fprintf(trace_stream, "= %.8e\n", to_Pfact(pv)->factval);
                }
                break;
            default: break;
        }
    }
}

void get_fst(modeltemplate mt, systemtemplate st, fnum_list fval, inum trace) {
    fspec fs;
    syspar parm;
    parmval pv;
    inum i = 1;
    
    if ((trace >= 1) && (mt->flags != fspecNIL))
        fprintf(trace_stream, "\nFlags Mapping:\n\n");
    
    for (fs = mt->flags; fs != fspecNIL; fs = fs->next) {
        parm = find_syspar(st->parm, fs->name);
        if (parm == sysparNIL) { /* verry illegal setup */
            errcode = MIS_SETUP_SERR;
            error(fs->name);
            exit(1);
        }
        pv = parm->val;
        switch (pv->tag) {
            case TAGPflag:
                fval = append_fnum_list(fval, to_Pflag(pv)->flagval);
                if (trace >= 1) {
                    fprintf(trace_stream, "%-3ld: %-8s ", (long) i++, fs->name);
                    fprintf(trace_stream, "= %.8e\n", to_Pflag(pv)->flagval);
                }
                break;
            default: break;
        }
    }
}

void get_cst(modeltemplate mt, systemtemplate st, fnum_list cval, inum trace) {
    cspec cs;
    syspar parm;
    parmval pv;
    inum i = 1;
    
    if ((trace >= 1) && (mt->cons != cspecNIL))
        fprintf(trace_stream, "\nConstant Mapping:\n\n");
    
    for (cs = mt->cons; cs != cspecNIL; cs = cs->next) {
        parm = find_syspar(st->parm, cs->name);
        if (parm == sysparNIL) { /* very illegal setup */
            errcode = MIS_SETUP_SERR;
            error(cs->name);
            exit(1);
        }
        pv = parm->val;
        switch (pv->tag) {
            case TAGPconst:
                cval = append_fnum_list(cval, to_Pconst(pv)->constval);
                if (trace >= 1) {
                    fprintf(trace_stream, "%-3ld: %-8s ", (long) i++, cs->name);
                    fprintf(trace_stream, "= %.8e\n", to_Pconst(pv)->constval);
                }
                break;
            default: break;
        }
    }
}

inum get_rmt(modeltemplate mt) {
    rspec rs;
    inum nres;
    
    for (rs = mt->ress, nres = 0; rs != rspecNIL;
         rs = rs->next, nres++);
    
    return (nres);
}

inum get_amt(modeltemplate mt, fnum_list aval, inum trace) {
    aspec as;
    inum naux;
    inum i = 1;
    
    if ((trace >= 1) && (mt->auxs != aspecNIL))
        fprintf(trace_stream, "\nAux. Mapping:\n\n");
    
    for (as = mt->auxs, naux = 0; as != aspecNIL;
         as = as->next, naux++) {
        aval = append_fnum_list(aval, as->dval);
        if (trace >= 1) {
            fprintf(trace_stream, "%-3ld: %-8s ", (long) i++, as->name);
            fprintf(trace_stream, "= %.8e\n", as->dval);
        }
    }
    
    return (naux);
}

pset_list make_parmset(modres mrs, inum setid, statevector pstat,
                       fnum_list pdval, fnum_list plval, fnum_list puval) {
    pset pe, pi;
    pset_list pl;
    procedure Tp;
    vector val, lb, ub;
    boolean b;
    
    Tp = mrs->Tp; /* get transpose procedure */
    
    /* create package */
    
    val = new_vector((inum) pdval->sz, (fnumarray) pdval->arr);
    lb = new_vector((inum) plval->sz, (fnumarray) plval->arr);
    ub = new_vector((inum) puval->sz, (fnumarray) puval->arr);
    pe = new_pset(setid, val, lb, ub);
    
    if (Tp == procedureNIL) { /* no transformation needed */
        
        /* By rdup_ the vectors are copied instead of shared. */
        pi = rdup_pset(pe);
        
        fre_vector(val); /* free package */
        fre_vector(lb);
        fre_vector(ub);
        fre_pset(pe);
        
        pl = new_pset_list();
        return append_pset_list(pl, pi);
    }
    /* transpose p val vector */
    
    pi = new_pset(setid, rnew_vector(mrs->np), rnew_vector(mrs->np),
                  rnew_vector(mrs->np));
    
    b = (* Tp)(pstat, pe, mrs->pstat, pi);
    
    fre_vector(val); /* free package */
    fre_vector(lb);
    fre_vector(ub);
    fre_pset(pe);
    
    if (b == TRUE) {
        pl = new_pset_list();
        return append_pset_list(pl, pi);
    }
    errcode = ILL_SETUP_SERR;
    error("parameter transform");
    return (pset_listNIL);
}

cset_list make_consset(inum setid, fnum_list cval) {
    cset ce, ci;
    cset_list cl;
    vector val;
    
    /* create package */
    val = new_vector((inum) cval->sz, (fnumarray) cval->arr);
    
    ce = new_cset(setid, val);
    cl = new_cset_list();
    
    /* By rdup_ the vectors are copied instead of shared. */
    ci = rdup_cset(ce);
    fre_vector(val); /* free package */
    fre_cset(ce);
    
    return append_cset_list(cl, ci);
}

fset_list make_flagset(inum setid, fnum_list fval) {
    fset fe, fi;
    fset_list fl;
    vector val;
    
    /* create package */
    val = new_vector((inum) fval->sz, (fnumarray) fval->arr);
    
    fe = new_fset(setid, val);
    fl = new_fset_list();
    
    /* By rdup_ the vectors are copied instead of shared. */
    fi = rdup_fset(fe);
    fre_vector(val); /* free package */
    fre_fset(fe);
    
    return append_fset_list(fl, fi);
}

aset_list make_auxsset(inum setid, fnum_list aval) {
    aset ae, ai;
    aset_list al;
    vector val;
    
    /* create package */
    val = new_vector((inum) aval->sz, (fnumarray) aval->arr);
    
    ae = new_aset(setid, val);
    al = new_aset_list();
    
    /* By rdup_ the vectors are copied instead of shared. */
    ai = rdup_aset(ae);
    fre_vector(val); /* free package */
    fre_aset(ae);
    
    return append_aset_list(al, ai);
}

xgroup_list make_xextgrp(modres mrs, pset parmset, statevector xstat,
                         inum_list xtrans, fnum_list xval, datarow_list data) {
    xgroup xg;
    xset_list xsl;
    xset xse, xsi;
    vector val, err, abserr, delta;
    procedure Tx;
    inum nset;
    inum rowid;
    inum i;
    boolean b;
    
    Tx = mrs->Tx; /* get transpose procedure */
    
    val = rnew_vector(mrs->nx); /* allocate package */
    err = rnew_vector(mrs->nx);
    abserr = rnew_vector(mrs->nx);
    delta = rnew_vector(mrs->nx);
    xse = new_xset(0, val, err, abserr, delta, 0.0);
    
    xsl = new_xset_list();
    
    for (nset = 0, rowid = 1; data != datarowNIL; data = data->next, rowid++) {
        
        data->rowid = rowid; /* renumber for safety */
        xse->id = data->rowid;
        
        /* reorder externals */
        
        for (i = 0; i < LSTS(xtrans); i++) {
            VEC(xse->val, i) = LST(data->row, LST(xtrans, i));
            VEC(xse->err, i) = LST(data->err, LST(xtrans, i));
            VEC(xse->abserr, i) = LST(xval, i);
            VEC(xse->delta, i) = 0.0;
        }
        
        /* transpose externals */
        
        if (Tx == procedureNIL) { /* no transformation */
            
            xsi = rdup_xset(xse);
            
        } else {
            xsi = new_xset(xse->id, rnew_vector(mrs->nx), rnew_vector(mrs->nx),
                           rnew_vector(mrs->nx), rnew_vector(mrs->nx), 0.0);
            
            b = (* Tx)(parmset->val, xstat, xse, mrs->xstat, xsi);
            
            if (b == FALSE) {
                errcode = ILL_SETUP_SERR;
                error("interface transform");
                rfre_xset(xsi);
                rfre_xset(xse);
                rfre_xset_list(xsl);
                return (xgroup_listNIL);
            }
        }
        
        /* generate the receiving list in reverse order for speed */
        xsl = insert_xset_list(xsl, 0, xsi);
        nset++;
    }
    
    /* reverse xset list to facilitate later searching */
    xsl = rev_xset_list(xsl);
    
    rfre_xset(xse);
    
    xg = new_xgroup(1, nset, xsl); /* default group id */
    
    return append_xgroup_list(new_xgroup_list(), xg);
}

/* Get the parameter data from a numblock parameter set     */
/* To be used after parameter extraction                    */

/* All unknown input parameters are set to calculated mode  */

void read_numblock_p(modres mrs, pset pi, modeltemplate mt, systemtemplate st) {
    stateflag_list pstat;
    statevector pstatv;
    fnum_list pdval, plval, puval;
    vector val, lb, ub;
    pset pe;
    procedure Tpi;
    pspec psc;
    syspar parm;
    parmval pv;
    inum index;
    boolean b;
    
    /* get the values of the parameters.		*/
    /* the model transpose procedure may wish to use	*/
    /* current parameter values.			*/
    
    pstat = new_stateflag_list();
    pdval = new_fnum_list();
    plval = new_fnum_list();
    puval = new_fnum_list();
    
    val = vectorNIL;
    lb = vectorNIL;
    ub = vectorNIL;
    
    get_pst(mt, st, pstat, pdval, plval, puval, 0);
    
    pstatv = new_statevector((inum) pstat->sz, (statearray) (pstat->arr));
    
    /* set unknown mode to calculated mode */
    
    psc = mt->parm;
    
    for (index = 0; psc != pspecNIL; psc = psc->next, index++) {
        parm = find_syspar(st->parm, psc->name);
        if (parm == sysparNIL) { /* verry illegal setup */
            errcode = MIS_SETUP_SERR;
            error(psc->name);
            exit(1);
        }
        pv = parm->val;
        if (pv->tag == TAGPunkn) {
            fre_parmval(pv);
            parm->val = new_Pcalc(0.0, 0.0);
            VEC(mrs->pstat, index) = CALC;
        }
    }
    
    Tpi = mrs->Tpi; /* get transpose procedure */
    
    if (Tpi == procedureNIL) { /* no transformation needed */
        pe = rdup_pset(pi);
    } else {
        
        /* transpose p value vector, first pack lists to vectors */
        
        val = new_vector((inum) pdval->sz, (fnumarray) pdval->arr);
        lb = new_vector((inum) plval->sz, (fnumarray) plval->arr);
        ub = new_vector((inum) puval->sz, (fnumarray) puval->arr);
        pe = new_pset(0, val, lb, ub);
        
        b = (* Tpi)(mrs->pstat, pi, pstatv, pe);
        
        if (b == FALSE) {
            errcode = ILL_SETUP_SERR;
            error("inverse parameter transform");
            exit(1);
        }
    }
    
    /* copy values back */
    
    psc = mt->parm;
    
    for (index = 0; psc != pspecNIL; psc = psc->next, index++) {
        parm = find_syspar(st->parm, psc->name);
        pv = parm->val;
        switch (pv->tag) {
            case TAGPcalc:
                to_Pcalc(pv)->calcval = VEC(pe->val, index);
                to_Pcalc(pv)->calcint = fabs(VEC(pe->lb, index));
                break;
            case TAGPmeas:
                to_Pmeas(pv)->measval = VEC(pe->val, index);
                to_Pmeas(pv)->measint = fabs(VEC(pe->lb, index));
                break;
            case TAGPfact:
                to_Pfact(pv)->factval = VEC(pe->val, index);
                break;
            default: break;
        }
    }
    
    fre_stateflag_list(pstat);
    fre_statevector(pstatv);
    fre_fnum_list(pdval);
    fre_fnum_list(plval);
    fre_fnum_list(puval);
    
    if (Tpi != procedureNIL) {
        fre_vector(val);
        fre_vector(lb);
        fre_vector(ub);
        fre_pset(pe);
    } else rfre_pset(pe);
}

/* set unknown data headers to calculated                        */
/* set measured and calculated data headers to extracted         */
/* transpose tables                                              */
/* return the number of extra data row entries that must added   */

#define RESNORM	"residual"	/* header of residual data entry */

inum set_xst(modeltemplate mt, datatemplate dt, stateflag_list xstat, inum_list xtrans) {
    xspec xs;
    colhead_list header; /* header list */
    inum index; /* index for new row entries */
    
    for (xs = mt->xext; xs != xspecNIL; xs = xs->next) {
        for (header = dt->header, index = 0; header != colheadNIL; header = header->next, index++) {
            if (strcmp(xs->name, header->name) == 0)
                break;
        }
        if (header == colheadNIL) {
            continue;
            /*
             errcode = MIS_SETUP_SERR;
             error(xs->name);
             exit(1);
             */
        }
        switch (header->type) {
            case UNKN:
                /* change unknown header to calculated */
                header->type = CALC;
                xstat = append_stateflag_list(xstat, CALC);
                xtrans = append_inum_list(xtrans, index);
                break;
            case CALC:
            case MEAS:
                /* change measured header to calculated */
                header->type = CALC;
                xstat = append_stateflag_list(xstat, CALC);
                xtrans = append_inum_list(xtrans, index);
                break;
            case FACT:
                xstat = append_stateflag_list(xstat, FACT);
                xtrans = append_inum_list(xtrans, index);
                break;
            case SWEEP:
                xstat = append_stateflag_list(xstat, SWEEP);
                xtrans = append_inum_list(xtrans, index);
                break;
            case STIM:
                xstat = append_stateflag_list(xstat, STIM);
                xtrans = append_inum_list(xtrans, index);
                break;
            default: break;
        }
    }
    
    /* add residual header to database */
    
    for (header = dt->header, index = 0; header != colheadNIL; header = header->next, index++) {
        if (strcmp(RESNORM, header->name) == 0)
            break;
    }
    if (header == colheadNIL) { /* add header */
        dt->header = append_colhead_list(dt->header, new_colhead(new_tmstring(RESNORM), FACT));
        return (1);
    } else {
        header->type = FACT;
        return (0);
    }
}

void read_numblock_x(modres mrs, xgroup_list xg, modeltemplate mt, datatemplate dt) {
    colhead_list header;
    inum_list xtrans;
    stateflag_list xstat;
    statevector xstatv;
    procedure Txi;
    vector val, err, abserr, delta;
    xset xe, xi;
    datarow_list dr;
    inum gid; /* group id */
    inum anum;
    inum i;
    inum idx;
    inum residx;
    boolean b;
    
    /* set the transpose tables for x */
    
    xstat = new_stateflag_list();
    xtrans = new_inum_list();
    
    anum = set_xst(mt, dt, xstat, xtrans);
    
    xstatv = new_statevector((inum) xstat->sz, (statearray) (xstat->arr));
    
    Txi = mrs->Txi; /* get transform procedure */
    
    if (Txi != procedureNIL) {
        val = rnew_vector(mrs->nx); /* allocate package */
        err = rnew_vector(mrs->nx);
        abserr = rnew_vector(mrs->nx);
        delta = rnew_vector(mrs->nx);
        
        xe = new_xset(0, val, err, abserr, delta, 0.0);
    } else {
        xe = xsetNIL;
    }
    
    /* find residual index */
    
    for (header = dt->header, residx = 0; header != colheadNIL; header = header->next, residx++) {
        if (strcmp(RESNORM, header->name) == 0)
            break;
    }
    assert(header != colheadNIL);
    
    for (; xg != xgroupNIL; xg = xg->next) {
        
        gid = xg->id;
        dr = datarowNIL;
        
        for (xi = xg->g; xi != xsetNIL; xi = xi->next) {
            
            dr = find_datarow(dt->data, dr, xi->id);
            assert(dr != datarowNIL); /* would be very strange */
            
            dr->grpid = gid;
            
            /* add the extra entries to the data rows */
            
            for (i = 0; i < anum; i++) {
                dr->row = append_fnum_list(dr->row, 0.0);
                dr->err = append_fnum_list(dr->err, 0.0);
            }
            
            if (Txi == procedureNIL) {
                xe = xi;
            } else {
                
                /* transpose externals */
                
                b = (* Txi)(mrs->xstat, xi, xstatv, xe);
                
                if (b == FALSE) {
                    errcode = ILL_SETUP_SERR;
                    error("inverse interface transform");
                    exit(1);
                }
            }
            
            /* put xe back into data row */
            
            for (i = 0; i < LSTS(xtrans); i++) {
                
                idx = LST(xtrans, i);
                LST(dr->row, idx) = VEC(xe->val, i);
                LST(dr->err, idx) = VEC(xe->delta, i); /* calculated accuracy */
            }
            
            /* put residual in data row */
            
            LST(dr->row, residx) = xe->res;
            
        }
    }
    
    fre_stateflag_list(xstat);
    fre_statevector(xstatv);
    fre_inum_list(xtrans);
    
    if (Txi != procedureNIL)
        rfre_xset(xe); /* clean up package */
}

void write_numblock(numblock numb, tmstring s) /* DEBUGGING CODE */ {
    FILE *fp;
    TMPRINTSTATE *st;
    
    if ((fp = fopen(s, "w")) == NULL) {
        fprintf(stderr, "\nUnable to write NUMBLOCK to file %s\n", s);
        exit(1);
    }
    st = tm_setprint(fp, 1, 80, 8, 0);
    print_numblock(st, numb);
    fclose(fp);
    tm_endprint(st);
}
