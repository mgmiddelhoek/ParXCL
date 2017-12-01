/*
 * ParX - actions.c
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

#include "parx.h"
#include "error.h"
#include "parser.h"
#include "numdat.h"
#include "subset.h"
#include "simulate.h"
#include "stim2dat.h"
#include "extract.h"
#include "actions.h"

void show_status(void) {
    fprintf(output_stream, "Parx   directory: %s\n", parx_path);
    fprintf(output_stream, "Model  directory: %s\n\n", model_path);
}

void plot(parxsymbol_list sl, tmstring fname) {
    fputs("Plot command not implemented\n", error_stream);
}

void print(parxsymbol_list sl, tmstring fname) {
    fputs("Print command not implemented\n", error_stream);
}

void call_subset(tmstring smeas, tmstring sdata_s, tmstring sdata_d) {
    dbnode node;
    meastemplate meast;
    datatemplate datat_s;
    datatemplate datat_d;
    
    node = find_dbnode(smeas, TAGMeasurement);
    if (node == dbnodeNIL) return;
    meast = to_Measurement(node)->measdata;
    
    node = find_dbnode(sdata_s, TAGDatatable);
    if (node == dbnodeNIL) return;
    datat_s = to_Datatable(node)->datdata;
    
    node = find_dbnode(sdata_d, TAGDatatable);
    if (node == dbnodeNIL) return;
    
    datat_d = subset_data(meast, datat_s);
    
    if (datat_d == datatemplateNIL) return;
    else {
        rfre_datatemplate(to_Datatable(node)->datdata);
        to_Datatable(node)->datdata = datat_d;
    }
    
    return;
}

void call_simulate(tmstring sstim, tmstring ssys, tmstring sdata,
                   fnum prec, inum maxiter, inum trace) {
    dbnode node;
    stimtemplate stimt;
    systemtemplate syst;
    datatemplate datat;
    modeltemplate modt;
    numblock numb;
    
    node = find_dbnode(sstim, TAGStimulus);
    if (node == dbnodeNIL) return;
    stimt = to_Stimulus(node)->stimdata;
    
    node = find_dbnode(ssys, TAGSystem);
    if (node == dbnodeNIL) return;
    syst = to_System(node)->sysdata;
    
    node = find_dbnode(syst->model, TAGModel);
    if (node == dbnodeNIL) return;
    modt = to_Model(node)->moddata;
    
    node = find_dbnode(sdata, TAGDatatable);
    if (node == dbnodeNIL) return;
    
    datat = stim2dat(stimt, modt);
    
    if (datat == datatemplateNIL) return;
    else {
        rfre_datatemplate(to_Datatable(node)->datdata);
        to_Datatable(node)->datdata = datat;
    }
    
    numb = make_numblock(modt, syst, datat, trace);
    if (numb == numblockNIL) return;
    
#ifdef STAT
    write_numblock(numb, "simin.nb"); /* DEBUGGING CODE */
#endif
    
    if (simulate(numb, prec, maxiter, trace) == FALSE) {
        fputs("Simulation failed\n", error_stream);
    } else {
        fputs("Simulation done\n", error_stream);
        read_numblock_x(numb->mod, numb->x, modt, datat);
    }
    
#ifdef STAT
    write_numblock(numb, "simout.nb"); /* DEBUGGING CODE */
#endif
    
    rfre_numblock(numb);
}

void call_extract(tmstring ssys, tmstring sdata, fnum prec, fnum tol,
                  opttype opt, fnum sens, inum maxiter, inum trace) {
    dbnode node;
    systemtemplate syst;
    datatemplate datat;
    modeltemplate modt;
    numblock numb;
    
    node = find_dbnode(ssys, TAGSystem);
    if (node == dbnodeNIL) return;
    syst = to_System(node)->sysdata;
    
    node = find_dbnode(syst->model, TAGModel);
    if (node == dbnodeNIL) return;
    modt = to_Model(node)->moddata;
    
    node = find_dbnode(sdata, TAGDatatable);
    if (node == dbnodeNIL) return;
    datat = to_Datatable(node)->datdata;
    
    numb = make_numblock(modt, syst, datat, trace);
    if (numb == numblockNIL) return;
    
#ifdef STAT
    write_numblock(numb, "extin.nb"); /* DEBUGGING CODE */
#endif
    
    if (extract(numb, prec, tol, opt, sens, maxiter, trace) == FALSE) {
        fputs("Extraction failed\n", error_stream);
    } else {
        fputs("Extraction done\n", error_stream);
    }
    
    read_numblock_p(numb->mod, numb->p, modt, syst);
    read_numblock_x(numb->mod, numb->x, modt, datat);
    
#ifdef STAT
    write_numblock(numb, "extout.nb"); /* DEBUGGING CODE */
#endif
    
    rfre_numblock(numb);
}

/* start up ParX */

void start_parx(void) {
    time_t timer;
    char *timestr;
    
    /* print banner string */
    
    fputc('\n', error_stream);
    fputc('\n', error_stream);
    fputc('\n', error_stream);
    fputs(parx_banner, error_stream);
    fputs(VERSION, error_stream);
    fputs(COPYRIGHT, error_stream);
    fputs("\n\n\n", error_stream);
    fflush(error_stream);
    
    if (trace_stream != error_stream) {
        fputc('\n', trace_stream);
        fputs(parx_banner, trace_stream);
        fputs(VERSION, trace_stream);
        fputs(COPYRIGHT, trace_stream);
        fputs("\n\n", trace_stream);
        timer = time(NULL);
        if (timer != -1) {
            fputc('\n', trace_stream);
            timestr = ctime(&timer);
            fputs(timestr, trace_stream);
            fputc('\n', trace_stream);
        }
        fflush(trace_stream);
    }
    
    init_dbase(); /* initialize database */
}

/* final bookkeeping operations before ending ParX */

void end_parx(void) {
    
#ifdef STAT
    
    FILE *f;
    
    /* write TM allocation statistics to file */
    
    if ((f = fopen("tm.sts", "w")) == NULL) {
        fprintf(error_stream, "PARX: Unable to write TM statistics file\n");
        return;
    }
    
    init_dbase();
    stat_end();
    
    stat_dbase(f);
    stat_modeltemplate(f);
    stat_systemtemplate(f);
    stat_datatemplate(f);
    stat_numblock(f);
    stat_modlib(f);
    stat_primtype(f);
    stat_atoms(f);
    stat_tmstring(f);
    report_lognew(f);
    fclose(f);
    
#endif
    
}
