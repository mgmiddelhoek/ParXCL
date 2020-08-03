/*
 * ParX - parxlex.l
 * Lexical Analyzer for Commandline Interpreter
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

%{
    
#pragma clang diagnostic ignored "-Wunused-function"
#pragma clang diagnostic ignored "-Wdocumentation"

#include "parx.h"
#include "error.h"
#include "datastruct.h"
#include "parser.h"
#include "dbase.h"

#if defined(LINUX) || defined(MSDOS) || defined(WINDOWS) || defined(WIN32) || defined(WIN64)
#include "parxyacc.h"
#elif defined(OSX)
#include "y.tab.h"
#endif


#define INRETURN(T)         {yysubprompt(); return(T);}        /* inside statement */
#define ENDRETURN(T)        {yymainprompt(); return(T);}       /* outside statement */

#undef YY_READ_BUF_SIZE
#define YY_READ_BUF_SIZE        512

#define MAX_READ_DEPTH          3L

/* context stacks for read's */

YY_BUFFER_STATE     read_stack[MAX_READ_DEPTH];
tmstring            fname_stack[MAX_READ_DEPTH];
inum                lineno_stack[MAX_READ_DEPTH];
FILE                *newinput;
inum                read_stack_ptr = 0L;

/* NOTREACHED */
%}

%S       SCOMMENT SSPEC SFILE SREAD

D        [0-9]
E        [Ee][-+]?{D}+
SNAME    [a-zA-Z./\\][a-zA-Z0-9_./\\:\-]*
FNAME    [^ \t\n;]+

%%

        /* C style comments */

"/""*"                  {   BEGIN SCOMMENT;                                 }
<SCOMMENT>[^\n\*]*      {                                                   }
<SCOMMENT>"*"           {                                                   }
<SCOMMENT>\n            {   yylineno++;                                     }
<SCOMMENT>"*""/"        {   BEGIN(INITIAL);                                 }

        /* TM style comments */

"||"[^\n]*              {                                                   }

        /* specification block */

"{"                     {   BEGIN SSPEC;
                            INRETURN('{');                                  }

<SSPEC>"<"              {   INRETURN('<');                                  }
<SSPEC>">"              {   INRETURN('>');                                  }

<SSPEC>"+"?inf          {   INRETURN(_INF);                                 }
<SSPEC>"+"?infinite     {   INRETURN(_INF);                                 }
<SSPEC>"-"inf           {   INRETURN(_MININF);                              }
<SSPEC>"-"infinite      {   INRETURN(_MININF);                              }

<SSPEC>lin              {   yylval.sf = SLIN;
                            INRETURN(_SCALE);                               }
<SSPEC>ln               {   yylval.sf = SLN;
                            INRETURN(_SCALE);                               }
<SSPEC>log              {   yylval.sf = SLOG;
                            INRETURN(_SCALE);                               }
<SSPEC>alin             {   yylval.sf = ALIN;
                            INRETURN(_SCALE);                               }
<SSPEC>aln              {   yylval.sf = ALN;
                            INRETURN(_SCALE);                               }
<SSPEC>alog             {   yylval.sf = ALOG;
                            INRETURN(_SCALE);                               }

<SSPEC>unkn             {   yylval.tf = UNKN;
                            INRETURN(_UMCF);                                }
<SSPEC>meas             {   yylval.tf = MEAS;
                            INRETURN(_UMCF);                                }
<SSPEC>calc             {   yylval.tf = CALC;
                            INRETURN(_UMCF);                                }
<SSPEC>fact             {   yylval.tf = FACT;
                            INRETURN(_UMCF);                                }

<SSPEC>group            {   yylval.pn = dec_sym("0");
                            INRETURN(_SYM);                                 }
<SSPEC>grp              {   yylval.pn = dec_sym("0");
                            INRETURN(_SYM);                                 }
<SSPEC>curve{D}+        {   yylval.pn = dec_sym(yytext+5);
                            INRETURN(_SYM);                                 }
<SSPEC>crv{D}+          {   yylval.pn = dec_sym(yytext+3);
                            INRETURN(_SYM);                                 }

<SSPEC>{SNAME}          {   yylval.pn = dec_sym(yytext);
                            INRETURN(_SYM);                                 }

"}"                     {   BEGIN(INITIAL);
                            INRETURN('}');                                  }

        /* include files */

read                    {   BEGIN SREAD;                                    }

<SREAD>[ \t]*           {   /* eat white space */                           }

<SREAD>{FNAME}          {   /* got the filename */

                            if (read_stack_ptr >= MAX_READ_DEPTH) {
                                errcode = NEST_PERR; /* too deep */
                                error("");
                            } else {
                                newinput = fopen(yytext, "r");
                                if (newinput == (FILE *)NULL) {
                                    errcode = NO_FILE_PERR; /* open failed */
                                    error(yytext);
                                } else { /* switch to file */
                                    fname_stack[read_stack_ptr] = yyfilename;
                                    lineno_stack[read_stack_ptr] = yylineno;
                                    read_stack[read_stack_ptr++] = YY_CURRENT_BUFFER;
                                    input_stream = newinput;
                                    yyfilename = new_tmstring(yytext);
                                    yylineno = 1L;
                                    yy_switch_to_buffer(yy_create_buffer(input_stream, YY_BUF_SIZE));
                                }
                            }
                            BEGIN(INITIAL);                                 }


        /* file escape */

"<"                     {   BEGIN SFILE;
                            INRETURN(_INPUT);                               }
">"                     {   BEGIN SFILE;
                            INRETURN(_OUTPUT);                              }
<SFILE>{FNAME}          {   yylval.pn = dec_str(yytext);
                            INRETURN(_STRING);                              }

        /* top level keywords */

mod                     {   INRETURN(_DMOD);                                }
sys                     {   INRETURN(_DSYS);                                }
data                    {   INRETURN(_DDATA);                               }
stim                    {   INRETURN(_DSTIM);                               }
meas                    {   INRETURN(_DMEAS);                               }

plot                    {   INRETURN(_PLOT);                                }
print                   {   INRETURN(_PRINT);                               }

show                    {   INRETURN(_SHOW);                                }

sub                     {   INRETURN(_SUB);                                 }

sim                     {   INRETURN(_SIM);                                 }
ext                     {   INRETURN(_EXT);                                 }

tol                     {   INRETURN(_TOL);                                 }
prec                    {   INRETURN(_PREC);                                }
sens                    {   INRETURN(_SENS);                                }

trace                   {   INRETURN(_TRACE);                               }

iter                    {   INRETURN(_ITER);                                }

crit                    {   INRETURN(_CRIT);                                }
modes                   {   INRETURN(_MODES);                               }
bestfit                 {   INRETURN(_BESTFIT);                             }
chisq                   {   INRETURN(_CHISQ);                               }
strict                  {   INRETURN(_STRICT);                              }
consist                 {   INRETURN(_CONSIST);                             }

clear                   {   INRETURN(_CLEAR);                               }

exit                    |
quit                    {   INRETURN(_EXIT);                                }

        /* values */

-?{D}+                  {   yylval.i = atoi(yytext);
                            INRETURN(_INTVALUE);                            }
-?{D}+"."{D}*({E})?     |
-?{D}*"."{D}+({E})?     |
-?{D}+{E}               {   yylval.f = atof(yytext);
                            INRETURN(_REALVALUE);                           }

        /* parxsymbol names */

{SNAME}                 {   yylval.pn = dec_sym(yytext);
                            INRETURN(_SYM);                                 }
        /* strings */

\"[^\"\n]*\"            {   yytext[yyleng-1] = '\0';
                            yylval.pn = dec_str(yytext+1);
                            INRETURN(_STRING);                              }

        /* white space */

[\t ]+                  {                                                   }

        /* end of statement */

";"                     {   BEGIN(INITIAL);
                            ENDRETURN(';');                                 }

\n                      {   yylineno++;
                            yyprompt();                                     }
\r                      {   /* for windows */                               }

<<EOF>>                 {   if (--read_stack_ptr < 0L) {
                                yyterminate();
                            } else {
                                fclose(input_stream);
                                fre_tmstring(yyfilename);
                                yy_delete_buffer(YY_CURRENT_BUFFER);
                                yyfilename = fname_stack[read_stack_ptr];
                                yylineno = lineno_stack[read_stack_ptr];
                                yy_switch_to_buffer(read_stack[read_stack_ptr]);
                            }
                                                                            }

.                       {   INRETURN(yytext[0]);                            }

%%
