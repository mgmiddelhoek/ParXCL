/*
 * ParX - parser.c
 * Parser support functions
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
#include "error.h"
#include "dbase.h"
#include "parser.h"

static char mainprompt[] = "-> "; /* statement prompt */
static char subprompt[] = "+> "; /* sub-statement prompt */
static char *prompt; /* current prompt */

tmstring yyfilename = ""; /* current input stream */
/* int yylineno = 1L; */ /* current line counter */

/* set the default input stream for batch mode */

boolean set_input_stream(tmstring fname) {
    FILE *istream;
    
    if (fname == tmstringNIL)
        return (FALSE);
    
    istream = fopen(fname, "r");
    
    if (istream) {
        input_stream = istream;
        yylineno = 1L;
        yyfilename = new_tmstring(fname);
        return (TRUE);
    } else return (FALSE);
}

/* YACC and LEX Support functions */

void yymainprompt(void) {
    prompt = mainprompt;
}

void yysubprompt(void) {
    prompt = subprompt;
}

void yyprompt(void) {
    if (isatty(fileno(input_stream))) {
        fprintf(output_stream, "%s", prompt);
        fflush(output_stream);
    }
}

void yyerror(char *s) {
    if (!errcode) {
        errcode = SYNTAX_PERR;
        error("");
    }
}

int yywrap(void) {
    return (1);
}
