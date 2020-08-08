/*
 * ParX - parser.h
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __PARSER_H
#define __PARSER_H

#include "primtype.h"

extern tmstring yyfilename; /* current input stream */
extern int yylineno;        /* current line counter */
extern char *yytext;

extern int yyparse(void);
extern int yylex(void);

extern boolean set_input_stream(tmstring fname);

extern void yymainprompt(void);
extern void yysubprompt(void);
extern void yyprompt(void);
extern int yywrap(void);

extern void yyerror(char *s);

extern int isatty(int);

#endif
