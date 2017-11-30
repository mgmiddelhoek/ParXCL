/*
 * ParX - prx_def.h
 * Model Compiler code generator definitions
 *
 * Copyright (c) 1994 M.G.Middelhoek <martin@middelhoek.com>
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

#ifndef  __PRX_DEF_H
#define __PRX_DEF_H

#include "primtype.h"

/*
 * header file for PARX model code generator
 */

/* File identifier */
#define FILEID  "PARX interpreter code"
/* Version */
#define CODE_VERSION   3
/* maximum line length in model description file (without newline) */
#define MAXLINE   132
/* maximum nesting level of conditional statements */
#define MAXLEVEL  10
/* maximum name length */
#define MAXNAME   16
/* maximum number of statements (assignments, if, else, fi) */
#define MAXEQU    4096

typedef enum {
    VAR, AUX, PAR, CON, FLG, RES, TMP,
    DRES, DTMP
} TYP;

typedef enum {
    INVAL, AND, OR, NOT, LT, GT, LE, GE, EQ, NE,
    NEG, ADD, SUB, MUL, DIV, POW, REV, SQR, INC, DEC, EQU,
    SIN, COS, TAN, ASIN, ACOS, ATAN,
    EXP, LOG, LG, SQRT, ABS, SGN, RET, CHKL, CHKG,
    OPD, NUM, DOPD, LDF, ASS, NASS, CLR,
    JMP, IF, ELSE, FI, EOD, SOK, STOP
} OPR;

struct PRX_NODE_S {
    OPR opr;
    struct PRX_NODE_S *o1;
    
    union {
        struct PRX_NODE_S *o2;
        struct PRX_OPD_S *optr;
        struct PRX_NUM_S *nptr;
    } c;
    
    struct PRX_NODE_S *abl;
};
typedef struct PRX_NODE_S PRX_NODE;

struct PRX_NUM_S {
    double val;
    PRX_NODE *node;
    int ind;
};
typedef struct PRX_NUM_S PRX_NUM;

struct PRX_OPD_S {
    char *name;
    PRX_NODE *node;
    int ind;
    TYP typ;
};
typedef struct PRX_OPD_S PRX_OPD;

struct HEADER_S {
    struct HEADER_S *next;
    char *text;
};
typedef struct HEADER_S HEADER;

union PRX_CODE_U {
    OPR o;
    fnum *f;
    union PRX_CODE_U *c;
};
typedef union PRX_CODE_U CODE;

extern int ci_ing_in(char *, double *, int *);
extern int ci_ing_out(double, int, char *);

extern int prx_init(FILE *, FILE *);
extern int prx_parse(FILE *inFile);
extern int prx_header(char *);
extern int prx_equation(char *);
extern int prx_derivLists();
extern int prx_check();
extern int prx_deriv_all();
extern int prx_error();
extern int prx_exit(void);
extern int prx_name(char *);
extern int prx_values(char *, double *, int *);

extern char prx_filename[];
extern int prx_lineno;

#endif
