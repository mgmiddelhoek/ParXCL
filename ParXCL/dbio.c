/*
 * ParX - dbio.c
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

#include "parx.h"
#include "error.h"
#include "dbio.h"
#include "jsonio.h"

static char buf[1024]; /* filename buffer */

/* find out if the file path is absolute */

boolean absolute_path(tmstring fname) {
    char *drive;
    
    drive = strchr(fname, ':');
    if (drive != NULL) { /* there is a drive letter */
        drive++;
        if (*drive == '/' || *drive == '\\') {
            return (TRUE);
        }
    } else {
        if (*fname == '/' || *fname == '\\') {
            return (TRUE);
        }
    }
    return (FALSE);
}

/* determine the path part of the file name including the last path separator */

void set_path(tmstring fname) {
    char *sep, *lsep;
    
    buf[0] = '\0';
    
    if (absolute_path(fname) == TRUE) {
        strcat(buf, fname);
    } else {
        if (input_path != NULL) {
            strcat(buf, input_path);
            sep = buf;
            lsep = NULL;
            while ((sep = strpbrk(sep, "/\\:")) != NULL) {
                lsep = ++sep;
            }
            if (lsep != NULL) {
                *lsep = '\0';
            } else {
                buf[0] = '\0';
            }
        }
        strcat(buf, fname);
    }
    sep = buf;
    lsep = NULL;
    while ((sep = strpbrk(sep, "/\\:")) != NULL) {
        lsep = ++sep;
    }
    if (lsep != NULL) {
        *lsep = '\0';
    } else {
        buf[0] = '\0';
    }
}

/* find the first character of the base name */

tmstring find_basename(tmstring fname) {
    char *sep, *lsep;
    
    sep = fname;
    lsep = NULL;
    while ((sep = strpbrk(sep, "/\\:")) != NULL) {
        lsep = ++sep;
    }
    if (lsep == NULL) {
        return (fname);
    } else {
        return (lsep);
    }
}

/* find the file name extention */
tmstring find_extention() {
    char *sep, *lsep;
    
    sep = buf;
    lsep = NULL;
    while ((sep = strpbrk(sep, ".")) != NULL) {
        lsep = sep++;
    }
    return (lsep);
}

/* remove the file name extention */

void cut_extention() {
    char *sep;
    
    sep = find_extention();
    if (sep != NULL) {
        *sep = '\0';
    }
}

/* find model file by name */

modeltemplate get_model(tmstring fname) {
    FILE *fp;
    modeltemplate mt;
    int cmp;
    int extern prx_compile(tmstring);
    
    set_path(fname);
    strcat(buf, find_basename(fname));
    cut_extention();
    strcat(buf, MODEL_EXT);
    
    if ((fp = fopen(buf, "r")) == NULL) { /* check if model definition file is present */
        
        if (absolute_path(fname) == FALSE) { /* relative path */
            strcpy(buf, parx_path);
            if (strlen(buf) != 0 && strncmp(buf + strlen(buf) - 1, PATH_SEP, 1) != 0) {
                strcat(buf, PATH_SEP);
            }
            strcat(buf, model_path);
            if (strlen(buf) != 0 && strncmp(buf + strlen(buf) - 1, PATH_SEP, 1) != 0) {
                strcat(buf, PATH_SEP);
            }
            strcat(buf, fname);
            cut_extention();
            strcat(buf, MODEL_EXT);
            
            fp = fopen(buf, "r"); /* check if model definition file is present */
            
        }
    }
    if (fp == NULL) {
        errcode = UNK_MODEL_PERR;
        error(fname);
        return (modeltemplateNIL);
    }
    fclose(fp);
    
    cmp = prx_compile(buf);
    
    if (cmp) {
        errcode = UNK_MODEL_PERR;
        error(fname);
        return (modeltemplateNIL);
    }
    
    cut_extention();
    strcat(buf, MODEL_INTERFACE_EXT);
    
    if ((fp = fopen(buf, "r")) == NULL) {
        errcode = UNK_MODEL_PERR;
        error(fname);
        return (modeltemplateNIL);
    }
    if (error_stream != trace_stream) {
        fprintf(error_stream, "\nloading model: %s\n", fname);
    }
    
    tm_lineno = 1;
    if (fscan_modeltemplate(fp, &mt)) {
        errcode = TMERROR_PERR;
        sprintf(error_mesg, "%s {%d}: %s", buf, tm_lineno, tm_errmsg);
        error(error_mesg);
        rfre_modeltemplate(mt);
        mt = modeltemplateNIL;
    }
    
    fclose(fp);
    
    return (mt);
}

/* find model code file by name */

codefile get_modelcode(tmstring fname) {
    FILE *fp;
    int extern prx_compile(tmstring);
    
    set_path(fname);
    strcat(buf, find_basename(fname));
    cut_extention();
    strcat(buf, MODEL_CODE_EXT);
    
    if ((fp = fopen(buf, "rb")) == NULL) { /* check if .mdl file is present */
        
        if (absolute_path(fname) == FALSE) { /* relative path */
            strcpy(buf, parx_path);
            if (strlen(buf) != 0 && strncmp(buf + strlen(buf) - 1, PATH_SEP, 1) != 0) {
                strcat(buf, PATH_SEP);
            }
            strcat(buf, model_path);
            if (strlen(buf) != 0 && strncmp(buf + strlen(buf) - 1, PATH_SEP, 1) != 0) {
                strcat(buf, PATH_SEP);
            }
            strcat(buf, fname);
            cut_extention();
            strcat(buf, MODEL_CODE_EXT);
            
            fp = fopen(buf, "rb"); /* check if model definition file is present */
            
        }
    }
    if (fp == NULL) {
        errcode = UNK_MODEL_PERR;
        error(fname);
        return (codefileNIL);
    }
    
    return (fp);
}

systemtemplate get_system(tmstring fname) {
    FILE *fp;
    systemtemplate st;
    
    set_path(fname);
    strcat(buf, find_basename(fname));
    cut_extention();
    strcat(buf, SYSTEM_EXT);
    
    if ((fp = fopen(buf, "r")) == NULL) {
        errcode = NO_FILE_PERR;
        error(buf);
        return (systemtemplateNIL);
    }
    
    if (error_stream != trace_stream) {
        fprintf(error_stream, "\nloading system: %s\n", fname);
    }
    
    tm_lineno = 1;
    if (fscan_systemtemplate(fp, &st)) {
        errcode = TMERROR_PERR;
        sprintf(error_mesg, "%s {%d}: %s", buf, tm_lineno, tm_errmsg);
        error(error_mesg);
        rfre_systemtemplate(st);
        st = systemtemplateNIL;
    }
    
    fclose(fp);
    
    return (st);
}

datatemplate get_datatable(tmstring fname) {
    FILE *fp;
    datatemplate dt = datatemplateNIL;
    char *dot;
    boolean pxd = FALSE;
    boolean csv = FALSE;
    boolean json = FALSE;
    extern inum readcsv(FILE *, datatemplate);
    
    set_path(fname);
    strcat(buf, find_basename(fname));
    dot = find_extention();
    
    if (dot == NULL) { /* no extension */
        strcat(buf, DATATABLE_EXT);
        pxd = TRUE;
    } else {
        if (strcmp(dot, DATATABLE_EXT) == 0) {
            pxd = TRUE;
        } else if (strcmp(dot, CSV_EXT) == 0) {
            csv = TRUE;
        } else if (strcmp(dot, JSON_EXT) == 0) {
            json = TRUE;
        } else {
            errcode = NO_FILE_PERR;
            error(fname);
            return (datatemplateNIL);
        }
    }
    
    if ((fp = fopen(buf, "r")) == NULL) {
        errcode = NO_FILE_PERR;
        error(buf);
        return (datatemplateNIL);
    }
    
    if (error_stream != trace_stream) {
        fprintf(error_stream, "\nloading database: %s\n", fname);
    }
    
    if (pxd) { /* read .pxd file */
        tm_lineno = 1;
        if (fscan_datatemplate(fp, &dt)) {
            errcode = TMERROR_PERR;
            sprintf(error_mesg, "%s {%d}: %s", buf, tm_lineno, tm_errmsg);
            error(error_mesg);
            rfre_datatemplate(dt);
            dt = datatemplateNIL;
        }
    } else if (csv) { /* read .csv file */
        dt = new_datatemplate(new_tmstring(buf), colheadNIL, datarowNIL);
        if (readcsv(fp, dt)) {
            errcode = TMERROR_PERR;
            sprintf(error_mesg, "%s {%d}: %s", buf, tm_lineno, tm_errmsg);
            error(error_mesg);
            rfre_datatemplate(dt);
            dt = datatemplateNIL;
        }
    } else if (json) { /* read .json file */
        if (read_data_json(fp, &dt)) {
            errcode = TMERROR_PERR;
            sprintf(error_mesg, "%s", tm_errmsg);
            error(error_mesg);
            rfre_datatemplate(dt);
            dt = datatemplateNIL;
        }
    }
    
    fclose(fp);
    
    return (dt);
}

void put_system(systemtemplate st, tmstring fname) {
    FILE *fp;
    TMPRINTSTATE *pst;
    char *dot;
    boolean pxs = FALSE;
    boolean json = FALSE;
    
    set_path(fname);
    strcat(buf, find_basename(fname));
    dot = find_extention();
    
    if (dot == NULL) { /* no extension */
        strcat(buf, SYSTEM_EXT);
        pxs = TRUE;
    } else {
        if (strcmp(dot, SYSTEM_EXT) == 0) {
            pxs = TRUE;
        } else if (strcmp(dot, JSON_EXT) == 0) {
            json = TRUE;
        } else {
            errcode = NO_FILE_PERR;
            error(fname);
            return;
        }
    }
    
    if ((fp = fopen(buf, "w")) == NULL) {
        errcode = NO_FILE_PERR;
        error(buf);
        return;
    }
    
    if (error_stream != trace_stream) {
        fprintf(error_stream, "\nstoring system: %s\n", fname);
    }
    
    if (pxs) { /* write .pxs file */
        pst = tm_setprint(fp, 1, 80, 8, 0);
        print_systemtemplate(pst, st);
        tm_endprint(pst);
    }
    if (json) { /* write .json file */
        write_sys_json(fp, st);
    }
    
    fclose(fp);
}

void put_datatable(datatemplate dt, tmstring fname) {
    FILE *fp;
    TMPRINTSTATE *pst;
    char *dot;
    boolean pxd = FALSE;
    boolean csv = FALSE;
    boolean json = FALSE;
    
    set_path(fname);
    strcat(buf, find_basename(fname));
    dot = find_extention();
    
    if (dot == NULL) { /* no extension */
        strcat(buf, DATATABLE_EXT);
        pxd = TRUE;
    } else {
        if (strcmp(dot, DATATABLE_EXT) == 0) {
            pxd = TRUE;
        } else if (strcmp(dot, CSV_EXT) == 0) {
            csv = TRUE;
        } else if (strcmp(dot, JSON_EXT) == 0) {
            json = TRUE;
        } else {
            errcode = NO_FILE_PERR;
            error(fname);
            return;
        }
    }
    
    if ((fp = fopen(buf, "w")) == NULL) {
        errcode = NO_FILE_PERR;
        error(buf);
        return;
    }
    
    if (error_stream != trace_stream) {
        fprintf(error_stream, "\nstoring database: %s\n", fname);
    }
    
    if (pxd == TRUE) { /* write .pxd file */
        pst = tm_setprint(fp, 1, 80, 8, 0);
        print_datatemplate(pst, dt);
        tm_endprint(pst);
    }
    if (json == TRUE) { /* write .json file */
        write_data_json(fp, dt);
    }
    
    if (csv == TRUE) { /* do nothing, not implemented yet */
    }
    
    fclose(fp);
}
