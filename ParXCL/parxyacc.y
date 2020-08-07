/*
 * ParX - parxyacc.y
 * Grammar Analyzer for Commandline Interpreter
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
    
#pragma clang diagnostic ignored "-Wunreachable-code"
#pragma clang diagnostic ignored "-Wdocumentation"
    
#include "parx.h"
#include "error.h"
#include "datastruct.h"
#include "actions.h"
#include "pprint.h"
#include "parser.h"

#define YYDEBUG 0
#define register

static inum trace_sim = 1;			/* trace level of simulation, 0 is non */
static inum trace_ext = 1;			/* trace level of extraction, 0 is non */

#define DEFSENS	1.0
#define	DEFPREC	1.0e-6
#define DEFTOL	0.0

static fnum sens = DEFSENS;			/* default sensitivity threshold */
static fnum prec = DEFPREC;			/* default precision */
static fnum tolerance = DEFTOL;     /* default tolerance */

static inum maxiter = 0L;			/* maximum # of iterations, 0 is default */
static opttype optfl = MODES;       /* request optimization type */

static dbnode src_dbnode;			/* dbase source node */
static dbnode dst_dbnode;			/* dbase destination node */
static tags_dbnode dst_tag;			/* type of destination node */
static tmstring name_set;			/* name of field to be set */

/* pointers to modifiable parts of nodes */

static union {
	syspar sys;
	stimtemplate stim;
	meastemplate meas;
} set;

%}

%union {
	inum i;
	fnum f;
	boolean b;
	tmstring s;
	scaleflag sf;
	stateflag tf;
	parsenode pn;
	parsenode_list pnl;
}

%start input

%token		'=' '<'  '>' '#' '{' '}' ',' ';' '@'

%token	<pn>	_SYM
%token  <pn>	_STRING
%token	<f>     _REALVALUE
%token	<i>     _INTVALUE
%token	<sf>	_SCALE
%token	<tf>	_UMCF

%token		_INF _MININF

%token		_DSYS _DDATA _DSTIM _DMEAS _DMOD

%token		_INPUT _OUTPUT
%token		_SHOW _SUB _SIM _EXT _PLOT _PRINT
%token		_EXIT _CLEAR

%token		_TRACE
%token		_ITER
%token		_TOL _PREC _SENS
%token		_CRIT _MODES _BESTFIT _CHISQ _STRICT _CONSIST

%type	<f>     value
%type	<pn>	parxsymbol
%type   <pnl>	parxsymbollist
%type   <pn>	string
%type	<pn>	sysparxsymbol
%type	<pnl>	sysparxsymbollist

%%

input       :	/* empty */
			{	yymainprompt();
				yyprompt();					}
			|	input	statement
			{	stat_end();					}
			|	input 	error	';'
			{	yyerrok;
				stat_end();					}

                /* store and restore dbase state */

			|	input	'@'	_OUTPUT	string	';'
			{	output_dbase(STR($4));
				stat_end();
                                            }
			|	input	'@'	_INPUT	string	';'
			{	input_dbase(STR($4));
				stat_end();					}

string      :	/* empty */
			{	$$ = parsenodeNIL;          }
			|	_STRING
			;

statement   :	/* empty */	';'
			|	declaration	';'
			|	inputoutput
			|	assignment	';'
			|	action
			;

declaration :	_DSYS	sysparxsymbollist
			{	dec_dbnode(SYMLIST($2), TAGSystem);         }
			|	_DDATA	parxsymbollist
			{	dec_dbnode(SYMLIST($2), TAGDatatable);		}
			|	_DSTIM	parxsymbollist
			{	dec_dbnode(SYMLIST($2), TAGStimulus);		}
			|	_DMEAS	parxsymbollist
			{	dec_dbnode(SYMLIST($2), TAGMeasurement);	}
			;

inputoutput :	parxsymbol	_INPUT	_STRING	';'
			{	input_dbnode(SYM($1), STR($3));             }
			|	parxsymbol 	_OUTPUT	_STRING	';'
			{	output_dbnode(SYM($1), STR($3));            }
			;

assignment  :	parxsymbol	'='
			{	/* select destination node */
				dst_dbnode = find_dbnode(SYM($1), TAGName);
                if (dst_dbnode == dbnodeNIL) { YYERROR; }
				dst_tag = tag_dbnode(dst_dbnode);
			}
                source
			;

source      :	parxsymbol
			{	/* select source node and copy */
				src_dbnode = find_dbnode(SYM($1), dst_tag);
                if (src_dbnode == dbnodeNIL) { YYERROR; }
				copy_dbnode(dst_dbnode, src_dbnode);
			}
			|	'@'
			{	del_dbnode(dst_dbnode);                     }
			|	'{'	'}'
			|	'{'	setlist '}'
			;

setlist     :	set
			|	setlist	','	set
			;

set			:	parxsymbol
			{	/* select setting with key 'parxsymbol' in list */
				switch (dst_tag) {
				case TAGSystem:
					set.sys = sys_set(dst_dbnode, SYM($1));
                    if (set.sys == sysparNIL) { YYERROR; }
					break;
				case TAGMeasurement:
					set.meas = meas_set(dst_dbnode, SYM($1));
                    if (set.meas == meastemplateNIL) { YYERROR; }
					break;
				case TAGStimulus:
					set.stim = stim_set(dst_dbnode, SYM($1));
                    if (set.stim == stimtemplateNIL) { YYERROR; }
					break;
				default:
					errcode = ILL_ASSIGN_PERR;
					error(name_dbnode(dst_dbnode));
					YYERROR;
				}
				name_set = SYM($1);
			}
                speclist
			;

speclist    :	/* empty */
			|	speclist	spec
			;

spec        :	'@'
			{	switch (dst_tag) {
				case TAGSystem:
					init_sys_set(dst_dbnode, set.sys);
					break;
				case TAGStimulus:
					init_stim_set(dst_dbnode, set.stim);
					break;
				case TAGMeasurement:
					init_meas_set(dst_dbnode, set.meas);
					break;
				default:
					errcode = ILL_SPEC_PERR;
					error(name_set);
					YYERROR;
				}
			}
			|	'='	value
			{	switch (dst_tag) {
				case TAGSystem:
					switch (set.sys->val->tag) {
					case TAGPunkn:
						to_Punkn(set.sys->val)->unknval = $2;
						break;
					case TAGPmeas:
						to_Pmeas(set.sys->val)->measval = $2;
						break;
					case TAGPcalc:
						to_Pcalc(set.sys->val)->calcval = $2;
						break;
					case TAGPfact:
						to_Pfact(set.sys->val)->factval = $2;
						break;
					case TAGPconst:
						to_Pconst(set.sys->val)->constval = $2;
						break;
					case TAGPflag:
						to_Pflag(set.sys->val)->flagval = (($2 != 0) ? 1.0 : 0.0);
						break;
					}
					break;
				case TAGStimulus:
					set.stim->lval = $2;
					set.stim->uval = $2;
					set.stim->nint = 0;
					break;
				case TAGMeasurement:
					set.meas->lval = $2;
					set.meas->uval = $2;
					break;
				default:
					errcode = ILL_SPEC_PERR;
					error(name_set);
					YYERROR;
				}
			}
			|	'<'	value
			{	switch (dst_tag) {
				case TAGSystem:
					switch (set.sys->val->tag) {
					case TAGPunkn:
						to_Punkn(set.sys->val)->unknuval = $2;
						break;
					default:
						errcode = ILL_SPEC_PERR;
						error(name_set);
						YYERROR;
					}
					break;
				case TAGStimulus:
					set.stim->uval = $2;
					break;
				case TAGMeasurement:
					set.meas->uval = $2;
					break;
				default:
					errcode = ILL_SPEC_PERR;
					error(name_set);
					YYERROR;
				}
			}
			|	'>'	value
			{	switch (dst_tag) {
				case TAGSystem:
					switch (set.sys->val->tag) {
					case TAGPunkn:
						to_Punkn(set.sys->val)->unknlval = $2;
						break;
					default:
						errcode = ILL_SPEC_PERR;
						error(name_set);
						YYERROR;
					}
					break;
				case TAGStimulus:
					set.stim->lval = $2;
					break;
				case TAGMeasurement:
					set.meas->lval = $2;
					break;
				default:
					errcode = ILL_SPEC_PERR;
					error(name_set);
					YYERROR;
				}
			}
			|	'#'	_INTVALUE
			{	if ($2 < 0) {
					errcode = ILL_NEGVAL_PERR;
					error(name_set);
					YYERROR;
				}
				switch (dst_tag) {
				case TAGSystem:
					switch (set.sys->val->tag) {
					case TAGPmeas:
						to_Pmeas(set.sys->val)->measint = (fnum)$2;
						break;
					case TAGPcalc:
						to_Pcalc(set.sys->val)->calcint = (fnum)$2;
					default:
						errcode = ILL_SPEC_PERR;
						error(name_set);
						YYERROR;
					}
				break;
				case TAGStimulus:
					set.stim->nint = $2;
					break;
				case TAGMeasurement:
					set.meas->subs = $2;
					break;
				default:
					errcode = ILL_SPEC_PERR;
					error(name_set);
					YYERROR;
				}
			}
			|	'#'	_REALVALUE

			{	if ($2 < 0) {
					errcode = ILL_NEGVAL_PERR;
					error(name_set);
					YYERROR;
				}
				switch (dst_tag) {
				case TAGSystem:
					switch (set.sys->val->tag) {
					case TAGPmeas:
						to_Pmeas(set.sys->val)->measint = $2;
						break;
					case TAGPcalc:
						to_Pcalc(set.sys->val)->calcint = $2;
						break;
					default:
						errcode = ILL_SPEC_PERR;
						error(name_set);
						YYERROR;
					}
					break;
				default:
					errcode = ILL_SPEC_PERR;
					error(name_set);
					YYERROR;
				}
			}
			|	_SCALE
			{	switch (dst_tag) {
				case TAGStimulus:
					set.stim->scale = $1;
					break;
				default:
					errcode = ILL_SPEC_PERR;
					error(name_set);
					YYERROR;
				}
			}
			|	_UMCF
			{	switch(dst_tag) {
				case TAGSystem:
					cast_syspar(set.sys, $1);
					break;
				default:
					errcode = ILL_SPEC_PERR;
					error(name_set);
					YYERROR;
				}
			}
			;

value       :	_REALVALUE
			|	_INTVALUE
			{	$$ = (fnum)$1;              }
			|	_INF
			{	$$ = INF;                   }
			|	_MININF
			{	$$ = MININF;                }
			;

action      : 	_CRIT	'='	_MODES ';'
			{	optfl = MODES;				}
			|	_CRIT	'='	_BESTFIT ';'
			{	optfl = BESTFIT;			}
			|	_CRIT	'='	_CHISQ ';'
			{	optfl = CHISQ;				}
			|	_CRIT	'='	_STRICT ';'
			{	optfl = STRICT;				}
			|	_CRIT	'='	_CONSIST ';'
			{	optfl = CONSIST;			}
			|	_CRIT	'='	'@'	';'
			{	optfl = MODES;				}
			|	_CRIT	';'
			{	fprintf(output_stream, "crit = %s\n",
				optfl == MODES ? "modes" : optfl == BESTFIT ? "bestfit" :
				optfl == CHISQ ? "chisq" : optfl == STRICT ? "strict" :
				optfl == CONSIST ? "consist" : "unknown");	}

			|	_TRACE	_SIM	'='	_INTVALUE	';'
			{	trace_sim = $4 >= 0L ? $4 : 1L;     }
			|	_TRACE	_EXT	'='	_INTVALUE	';'
			{	trace_ext = $4 >= 0L ? $4 : 1L;     }
			|	_TRACE	';'
			{	fprintf(output_stream, "trace sim = %ld\n", (long) trace_sim);
				fprintf(output_stream, "trace ext = %ld\n", (long) trace_ext);
			}

			|	_ITER	'='	_INTVALUE	';'
			{	maxiter = $3 >= 0L ? $3 : 0L;       }
			|	_ITER	'='	'@'	';'
			{	maxiter = 0L;						}
			|	_ITER	';'
			{	fprintf(output_stream, "iter = %ld\n", (long) maxiter); }

			|	_TOL	'='	value	';'
			{	if (($3 >= 0.0) || ($3 <= 1.0))
					tolerance = $3;					}
			|	_TOL	'='	'@'	';'
			{	tolerance = DEFTOL;					}
			|	_TOL	';'
			{	fprintf(output_stream, "tol = %e\n", tolerance); }

			|	_PREC	'='	value	';'
			{	if (($3 >= 0.0) || ($3 <= 1.0))
					prec = $3;                      }
			|	_PREC	'='	'@'	';'
			{	prec = DEFPREC;						}
			|	_PREC	';'
			{	fprintf(output_stream, "prec = %e\n", prec); }

			|	_SENS	'='	value	';'
			{	if ($3 >= 0.0)
					sens = $3;                      }
			|	_SENS	'='	'@'	';'
			{	sens = DEFSENS;						}
			|	_SENS	';'
			{	fprintf(output_stream, "sens = %e\n", sens); }

			|	_DMOD	';'
			{	list_type(TAGModel);				}
			|	_DSYS	';'
			{	list_type(TAGSystem);				}
			|	_DDATA	';'
			{	list_type(TAGDatatable);			}
			|	_DMEAS	';'
			{	list_type(TAGMeasurement);			}
			|	_DSTIM	';'
			{	list_type(TAGStimulus);				}

			|	parxsymbollist	';'
			{	list_parxsymbol(SYMLIST($1));		}
			|	_SHOW	parxsymbollist	';'
			{	show_parxsymbol(SYMLIST($2));		}
			|	_SHOW	'@'	';'
			{	show_status();                      }

			|	_SUB	parxsymbol	parxsymbol	parxsymbol	';'
			{	call_subset(SYM($2), SYM($3), SYM($4));     }

			|	_EXT 	parxsymbol	parxsymbol	';'
			{	call_extract(SYM($2), SYM($3), prec, tolerance, optfl, sens,
					maxiter, trace_ext);					}

			|	_SIM parxsymbol	parxsymbol parxsymbol	';'
			{	call_simulate(SYM($2), SYM($3), SYM($4), prec, maxiter,
				trace_sim);							}

			|	_PLOT	parxsymbollist	';'
			{	plot(SYMLIST($2), tmstringNIL);		}
			|	_PRINT	parxsymbollist	';'
			{	print(SYMLIST($2), tmstringNIL);	}

			|	_CLEAR	';'
			{	init_dbase();                       }
			|	_EXIT	';'
			{	return(0);                          }
			;

parxsymbollist      :	parxsymbol
			{	$$ = $1;				}
			|	parxsymbollist	parxsymbol
			{	$$ = concat_parxsymbol($1, $2);		}
			;

sysparxsymbollist	:	sysparxsymbol
			{	$$ = $1;				}
			|	sysparxsymbollist	sysparxsymbol
			{	$$ = concat_parxsymbol($1, $2);		}
			;

sysparxsymbol       :	parxsymbol	'='	parxsymbol
			{	$$ = concat_parxsymbol($1, $3);		}
			;

parxsymbol  :	_SYM
			;
%%
