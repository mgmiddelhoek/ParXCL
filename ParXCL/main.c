/*
 * ParX - main.c
 *
 ****************************************************************************
 
 PPPPPP       AAAA       RRRRRR      XX   XX
 PP   PP     AA  AA      RR   RR     XX   XX
 PP   PP    AA    AA     RR   RR      XX XX
 PPPPPP     AAAAAAAA     RRRRRR        XXX
 PP         AA    AA     RR  RR       XX XX
 PP         AA    AA     RR   RR     XX   XX
 PP         AA    AA     RR   RR     XX   XX
 
 
 Parameter eXtraction program for analytical device models
 
 ****************************************************************************
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
#include "actions.h"

FILE *error_stream; /* stream for error messages and diagnostics */
FILE *trace_stream; /* stream for tracing information */

char *parx_path;
char *model_path;
char *input_path;

int main(int argc, char *argv[]) {
    int c;
    int ri, ro, re, rt; /* stream redirection flags */
    int yyr;
    
    /* default streams */
    
    input_path = NULL;
    input_stream = stdin;
    output_stream = stdout;
    error_stream = stdout;
    trace_stream = stdout;
    ri = ro = re = rt = 0; /* no redirection */
    
    /* parse command line options */
    
    while (--argc > 0) {
        if (((*++argv)[0]) == '-') {
            
            switch (c = (*argv)[1]) {
                    
                case 't': /* redirect trace stream */
                    if (rt == 1) {
                        fprintf(stderr, "duplicate option -%c\n", c);
                        exit(1);
                    }
                    if (argc <= 1) {
                        fprintf(stderr, "missing argument -%c\n", c);
                        exit(1);
                    }
                    trace_stream = fopen(*++argv, "w");
                    argc--;
                    if (trace_stream != NULL) {
                        rt = 1;
                        break;
                    }
                    fprintf(stderr, "ParX: unable to open trace file %s\n", *argv);
                    exit(1);
                    
                case 'e': /* redirect error stream */
                    if (re == 1) {
                        fprintf(stderr, "duplicate option -%c\n", c);
                        exit(1);
                    }
                    if (argc <= 1) {
                        fprintf(stderr, "missing argument -%c\n", c);
                        exit(1);
                    }
                    error_stream = fopen(*++argv, "w");
                    argc--;
                    if (error_stream != NULL) {
                        re = 1;
                        break;
                    }
                    fprintf(stderr, "ParX: unable to open error file %s\n", *argv);
                    exit(1);
                    
                case 'o': /* redirect output stream */
                    if (ro == 1) {
                        fprintf(stderr, "duplicate option -%c\n", c);
                        exit(1);
                    }
                    if (argc <= 1) {
                        fprintf(stderr, "missing argument -%c\n", c);
                        exit(1);
                    }
                    output_stream = fopen(*++argv, "w");
                    argc--;
                    if (output_stream != NULL) {
                        ro = 1;
                        break;
                    }
                    fprintf(stderr, "ParX: unable to open output file %s\n", *argv);
                    exit(1);
                    
                default:
                    fprintf(stderr, "ParX: illegal option -%c\n", c);
                    exit(1);
            }
            
        } else { /* redirect input stream */
            if (ri == 1) {
                fprintf(stderr, "ParX: illegal argument %s\n", *argv);
                exit(1);
            }
            
            if (set_input_stream(*argv) == TRUE) {
                ri = 1;
                input_path = *argv;
            } else {
                fprintf(stderr, "ParX: unable to open input file %s\n", *argv);
                exit(1);
            }
        }
    }
    
    /* determine database file paths */
    
    if ((parx_path = getenv("PARX")) == (char *) NULL) {
        parx_path = PARX_PATH;
    }
    
    model_path = MODEL_PATH;
    
    start_parx(); /* initialize ParX */
    
    yyr = yyparse(); /* start parsing */
    
    end_parx(); /* final bookkeeping */
    
    exit(yyr);
    
    return (yyr); /* dummy for ANSI */
}
