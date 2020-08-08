/*
 * ParX - pprint.c
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

#include "dbase.h"
#include "parx.h"
#include "pprint.h"

/* print values: -inf <= v <= inf */

static void printv(fnum v) {
    if ((1.01 * v) >= INF) {
        fprintf(output_stream, "%-15s", "inf");
    } else if ((1.01 * v) <= MININF) {
        fprintf(output_stream, "%-15s", "-inf");
    } else {
        fprintf(output_stream, "%-15.6g", v);
    }
}

/* give the type of a parxsymbol */

void list_parxsymbol(parxsymbol_list sl) {
    dbnode n;

    for (; sl != parxsymbolNIL; sl = sl->next) {
        fprintf(output_stream, "%s: ", sl->name);
        if ((n = check_name(sl->name)) == dbnodeNIL) {
            fputs("?\n", output_stream);
            continue;
        }
        switch (tag_dbnode(n)) {
        case TAGModel:
            fputs("Model\n", output_stream);
            break;
        case TAGSystem:
            fputs("System\n", output_stream);
            break;
        case TAGDatatable:
            fputs("Datatable\n", output_stream);
            break;
        case TAGStimulus:
            fputs("Stimulus\n", output_stream);
            break;
        case TAGMeasurement:
            fputs("Measurement\n", output_stream);
            break;
        default:
            return;
        }
    }
}

/* list all the names of nodes of the given type */

void list_type(tags_dbnode tag) {
    dbnode n;
    inum c;

    switch (tag) {
    case TAGModel:
        fputs("Models:\n", output_stream);
        break;
    case TAGSystem:
        fputs("Systems:\n", output_stream);
        break;
    case TAGDatatable:
        fputs("Datatables:\n", output_stream);
        break;
    case TAGStimulus:
        fputs("Stimuli:\n", output_stream);
        break;
    case TAGMeasurement:
        fputs("Measurements:\n", output_stream);
        break;
    default:
        break;
    }

    for (c = 0, n = dbase; n != dbnodeNIL; n = n->next)
        if (tag_dbnode(n) == tag) {
            if (c++ >= 5) { /* wrap */
                fputc('\n', output_stream);
                c = 1;
            }
            fprintf(output_stream, "%-15.14s", name_dbnode(n));
        }
    fputc('\n', output_stream);
}

static void show_mod(dbnode n) {
    modeltemplate mt;
    pspec p;
    xspec x;
    inum c;

    fprintf(output_stream, "Model: %s\n", to_Model(n)->modname);

    mt = to_Model(n)->moddata;

    if (strlen(mt->info)) {
        fprintf(output_stream, "info: %s\n", mt->info);
    }

    fprintf(output_stream, "model type: %s\n", mt->id);

    fputs("externals:\n", output_stream);
    for (x = mt->xext, c = 0; x != xspecNIL; x = x->next) {
        if (c++ >= 5) { /* wrap */
            fputc('\n', output_stream);
            c = 1;
        }
        fprintf(output_stream, "%-15.14s", x->name);
    }

    fputs("\nparameters:\n", output_stream);
    for (p = mt->parm; p != pspecNIL; p = p->next) {
        fprintf(output_stream, "%-15.14s: ", p->name);
        fputs(" = ", output_stream);
        printv(p->dval);
        fputs(" > ", output_stream);
        printv(p->lval);
        fputs(" < ", output_stream);
        printv(p->uval);
        fputc('\n', output_stream);
    }
}

static void show_sys(dbnode n) {
    systemtemplate st;
    syspar p;

    fprintf(output_stream, "System: %s\n", to_System(n)->sysname);

    st = to_System(n)->sysdata;

    if (strlen(st->info)) {
        fprintf(output_stream, "info: %s\n", st->info);
    }

    fprintf(output_stream, "model: %s\n", st->model);

    for (p = st->parm; p != sysparNIL; p = p->next) {
        fprintf(output_stream, "%-15.14s: ", p->name);
        switch (p->val->tag) {
        case TAGPunkn:
            fputs("unkn: ", output_stream);
            fputs(" = ", output_stream);
            printv(to_Punkn(p->val)->unknval);
            fputs(" > ", output_stream);
            printv(to_Punkn(p->val)->unknlval);
            fputs(" < ", output_stream);
            printv(to_Punkn(p->val)->unknuval);
            fputc('\n', output_stream);
            break;
        case TAGPmeas:
            fputs("meas: ", output_stream);
            fputs(" = ", output_stream);
            printv(to_Pmeas(p->val)->measval);
            fputs(" # ", output_stream);
            printv(to_Pmeas(p->val)->measint);
            fputc('\n', output_stream);
            break;
        case TAGPcalc:
            fputs("calc: ", output_stream);
            fputs(" = ", output_stream);
            printv(to_Pcalc(p->val)->calcval);
            fputs(" # ", output_stream);
            printv(to_Pcalc(p->val)->calcint);
            fputc('\n', output_stream);
            break;
        case TAGPfact:
            fputs("fact: ", output_stream);
            fputs(" = ", output_stream);
            printv(to_Pfact(p->val)->factval);
            fputc('\n', output_stream);
            break;
        case TAGPconst:
            fputs("const:", output_stream);
            fputs(" = ", output_stream);
            printv(to_Pconst(p->val)->constval);
            fputc('\n', output_stream);
            break;
        case TAGPflag:
            fputs("flag: ", output_stream);
            fputs(" = ", output_stream);
            printv(to_Pflag(p->val)->flagval);
            fputc('\n', output_stream);
            break;
        }
    }
}

static void show_data(dbnode n) {
    datatemplate dt;
    colhead_list h;
    datarow_list dr;
    inum nump, nums, numu, numf, numo;

    fprintf(output_stream, "Data: %s\n", to_Datatable(n)->dataname);

    dt = to_Datatable(n)->datdata;

    if (strlen(dt->info)) {
        fprintf(output_stream, "info: %s\n", dt->info);
    }

    /* count number of points */
    nump = numf = numu = nums = numo = 0L;
    for (dr = dt->data; dr != datarowNIL; dr = dr->next) {
        nump++;
        switch (dr->grpid) {
        case FGROUP:
            numf++;
            break;
        case UGROUP:
            numu++;
            break;
        case SGROUP:
            nums++;
            break;
        default:
            numo++;
            break;
        }
    }

    for (h = dt->header; h != colheadNIL; h = h->next) {
        fprintf(output_stream, "%-15.14s: ", h->name);
        switch (h->type) {
        case UNKN:
            fputs("unkn\n", output_stream);
            break;
        case MEAS:
            fputs("meas\n", output_stream);
            break;
        case CALC:
            fputs("calc\n", output_stream);
            break;
        case FACT:
            fputs("fact\n", output_stream);
            break;
        case STIM:
            fputs("stim\n", output_stream);
            break;
        case SWEEP:
            fputs("sweep\n", output_stream);
            break;
        default:
            putc('\n', output_stream);
            break;
        }
    }
}

static void show_stim(dbnode n) {
    stimtemplate st;

    fprintf(output_stream, "Stimulus: %s\n", to_Stimulus(n)->stimname);

    st = to_Stimulus(n)->stimdata;

    for (; st != stimtemplateNIL; st = st->next) {
        fprintf(output_stream, "%-15.14s: ", st->name);
        fputs(" > ", output_stream);
        printv(st->lval);
        fputs(" < ", output_stream);
        printv(st->uval);
        fprintf(output_stream, " # %ld  ", (long)(st->nint));
        switch (st->scale) {
        case SLIN:
            fputs("lin ", output_stream);
            break;
        case SLOG:
            fputs("log ", output_stream);
            break;
        case SLN:
            fputs("ln  ", output_stream);
            break;
        case ALIN:
            fputs("alin", output_stream);
            break;
        case ALOG:
            fputs("alog", output_stream);
            break;
        case ALN:
            fputs("aln ", output_stream);
            break;
        default:
            break;
        }
        fputc('\n', output_stream);
    }
}

static void show_meas(dbnode n) {
    meastemplate mt;

    fprintf(output_stream, "Measurement: %s\n", to_Measurement(n)->measname);

    mt = to_Measurement(n)->measdata;

    for (; mt != meastemplateNIL; mt = mt->next) {
        if (mt->mtype < 0) {
            fprintf(output_stream, "%-15.14s: ", mt->name);
        }
        if (mt->mtype == 0) {
            fprintf(output_stream, "group          :");
        }
        if (mt->mtype > 0) {
            fprintf(output_stream, "curve%-10ld:", (long)(mt->mtype));
        }
        fputs(" > ", output_stream);
        printv(mt->lval);
        fputs(" < ", output_stream);
        printv(mt->uval);
        if (mt->mtype > 0) {
            fprintf(output_stream, " # %ld", (long)(mt->subs));
        }
        fputc('\n', output_stream);
    }
}

void show_parxsymbol(parxsymbol_list sl) {
    dbnode n;

    for (; sl != parxsymbolNIL; sl = sl->next) {
        if ((n = find_dbnode(sl->name, TAGName)) == dbnodeNIL) {
            continue;
        }
        switch (tag_dbnode(n)) {
        case TAGModel:
            show_mod(n);
            break;
        case TAGSystem:
            show_sys(n);
            break;
        case TAGDatatable:
            show_data(n);
            break;
        case TAGStimulus:
            show_stim(n);
            break;
        case TAGMeasurement:
            show_meas(n);
            break;
        default:
            return;
        }
    }
}
