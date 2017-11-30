/*
 * ParX - error.c
 * Error Handling
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

#include "parx.h"
#include "primtype.h"
#include "parser.h"
#include "error.h"

#include <errno.h>

inum errcode; /* global variable for storing error codes */
char error_mesg[BUFSIZ]; /* global buffer for error messages */

typedef struct {
    inum px_errno;
    tmstring std_mesg;
} ERROR;

typedef struct {
    ERROR *l;
    inum n;
} ERRORLIST;

static ERROR errors[] = {
    { UNKNOWN_ERROR, "unknown error number"},
    
    { SYNTAX_PERR, "syntax error"},
    { UNK_IDENT_PERR, "unknown identifier"},
    { UNK_FIELD_PERR, "unknown field identifier"},
    { UNK_MODEL_PERR, "unknown model code identifier"},
    { ILL_MNAME_PERR, "illegal model name"},
    { ILL_REDEC_PERR, "illegal re-declaration"},
    { ILL_SPEC_PERR, "illegal field specification"},
    { ILL_TYPE_PERR, "identifier has wrong type"},
    { ILL_ASSIGN_PERR, "illegal assignment"},
    { ILL_NEGVAL_PERR, "illegal negative value"},
    { WRONG_ARG_PERR, "wrong argument type"},
    { NO_FILE_PERR, "unable to open file"},
    { NEST_PERR, "reads nested to deeply"},
    { TMERROR_PERR, "format error"},
    
    { ILL_SETUP_SERR, "illegal setup"},
    { MIS_SETUP_SERR, "no matching model definition"},
    { NO_DATA_SERR, "no data points available"},
    { NO_KEY_SERR, "interface variable not in data"},
    { NO_VAR_SERR, "no unbound variables in data"},
    { NO_PAR_SERR, "no unknown parameters"},
    { UNKN_VAR_SERR, "interface variable has unknown value"},
    
    { UNKNOWN_CERR, "unknown error in calculation"},
    { EXEPT_MODEL_CERR, "illegal evaluation of model equations"},
    { SLOW_CONV_CERR, "rate of convergence too slow"},
    { NO_DIREC_CERR, "no search direction can be found"},
    { NO_LOWP_CERR, "no lower point can be found"},
    { OBJ_FAIL_CERR, "objective function evaluation failed"},
    { NUMEQ_CERR, "insufficient data points available"},
    { MODIFY_CERR, "unable to modify data point set"},
    
    { MEM_IERR, "memory error in model interpreter"},
    { VER_IERR, "model code file has wrong version"},
    { COD_IERR, "illegal opcode"},
    { EOF_IERR, "unexpected end of file in model code file"},
    { CON_IERR, "missing constants field in model code file"},
    { NPX_IERR, "not a legal model code file"},
    { STK_IERR, "stack overflow"},
    
    { DNG_DERR, "SECURITY ERROR, no valid license !!!"}
    
};

static ERRORLIST errorlist = {errors, sizeof (errors) / sizeof (*errors)};

static ERROR *lookup(inum no) {
    inum i;
    ERROR *e;
    
    for (i = 0, e = errorlist.l; i < errorlist.n; i++, e++)
        if (e->px_errno == no)
            break;
    if (i == errorlist.n)
        return ((ERROR *) NULL);
    
    return (e);
}

/* print error message */

void error(tmstring s) {
    ERROR *ep;
    
    if (!errcode)
        return;
    
    if (!isatty(fileno(input_stream))) /* are we in a file */
        fprintf(error_stream, "\nParX: %s (%d): ", yyfilename, yylineno);
    
    if (trace_stream != error_stream) /* is there a trace file */
        fprintf(trace_stream, "\n\nERROR  (File: %s  Line: %d) : ",
                yyfilename, yylineno);
    
    if (errcode > 0) {
        errno = (int) errcode;
        if (trace_stream != error_stream)
            fprintf(trace_stream, "SYSTEM ERROR %d\n\n", errno);
        fflush(output_stream);
        fflush(trace_stream);
        perror((char *) s);
        fflush(error_stream);
    } else {
        ep = lookup(errcode);
        if (!ep) {
            sprintf(error_mesg, "%ld", (long) errcode);
            errcode = UNKNOWN_ERROR;
            error(error_mesg);
            exit((int) errcode);
        } else
            fprintf(error_stream, "\n%s : %s\n", s, ep->std_mesg);
        if (trace_stream != error_stream)
            fprintf(trace_stream, "\n%s : %s\n\n", s, ep->std_mesg);
        fflush(error_stream);
        fflush(trace_stream);
        fflush(output_stream);
    }
    errno = 0;
    errcode = 0;
}

/* handler for memory allocation errors of TM */

void tm_noroom() {
    fprintf(error_stream, "\nParX: out of memory\n");
    
    if (trace_stream != error_stream) { /* is there a trace file */
        fprintf(trace_stream, "\n\nERROR  (File: %s  Line: %d) : ",
                yyfilename, yylineno);
        fprintf(trace_stream, "OUT OF MEMORY\n");
    }
    fflush(error_stream);
    fflush(trace_stream);
    fflush(output_stream);
    exit(1);
}
