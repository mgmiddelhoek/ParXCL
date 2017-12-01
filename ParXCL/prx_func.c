/*
 * ParX - prx_func.c
 * Model Compiler code generator helpers
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <ctype.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include "prx_def.h"

/* syntax check of a name */
int prx_name(char *ps) {
    int b;
    char *pe;
    
    pe = ps;
    /* 1. character = letter */
    if (!isalpha(*pe) && *pe != '_')
        return 0;
    /* 2-nd - last character */
    while (b = *(++pe), isalnum(b) || *pe == '_');
    if (pe - ps > MAXNAME)
        return (int)(ps - pe);
    return (int)(pe - ps);
}

/* syntax check of a value list */
int prx_values(char *ps, double *Vals, int *pNVals) {
    char *pe;
    int lae, nVals;
    
    pe = ps;
    nVals = 0;
    if (*(pe++) != '{')
        return 0;
    do {
        if (nVals >= 5)
            return 0;
        if (memcmp(pe, "inf", 3) == 0 || memcmp(pe, "Inf", 3) == 0) {
            Vals[nVals++] = HUGE_VAL;
            pe += 3;
            continue;
        } else if (memcmp(pe, "-inf", 4) == 0 || memcmp(pe, "-Inf", 4) == 0) {
            Vals[nVals++] = -HUGE_VAL;
            pe += 4;
            continue;
        }
        if (ci_ing_in(pe, Vals + nVals, &lae))
            return 0;
        nVals++;
        pe += lae;
    } while (*(pe++) == ',');
    *pNVals = nVals;
    if (*(pe - 1) != '}')
        return 0;
    return (int)(pe - ps);
}

/* functions for input and output of engineering units */

/* Input function */
int ci_ing_in(char *text, double *value, int *len)
/* text: value in engineering format */
/* value: output value as double */
/* len: number of characters in input */ {
    int i;
    char t;
    double fak, zahl;
    int bZ;
    
    i = 0;
    bZ = 0;
    t = text[i++];
    if (t == '+' || t == '-')
        t = text[i++];
    while (t >= '0' && t <= '9') { /* digits before dot */
        t = text[i++];
        bZ++;
    }
    if (t == '.')
        t = text[i++];
    while (t >= '0' && t <= '9') { /* digits after dot */
        t = text[i++];
        bZ++;
    }
    if (t == 'e' || t == 'E') {
        t = text[i++];
        if (t == '-' || t == '+')
            t = text[i++];
        while (t >= '0' && t <= '9')
            t = text[i++]; /* digits exponent */
    }
    if (!bZ)
        return -100;
    switch (t) {
        case 'a':
            fak = 1e-18;
            break;
        case 'A':
            fak = 1e-18;
            break;
        case 'f':
            fak = 1e-15;
            break;
        case 'F':
            fak = 1e-15;
            break;
        case 'p':
            fak = 1e-12;
            break;
        case 'P':
            fak = 1e-12;
            break;
        case 'n':
            fak = 1e-9;
            break;
        case 'N':
            fak = 1e-9;
            break;
        case 'u':
            fak = 1e-6;
            break;
        case 'U':
            fak = 1e-6;
            break;
        case 'm':
            fak = 1e-3;
            break;
        case 'k':
            fak = 1e3;
            break;
        case 'K':
            fak = 1e3;
            break;
        case 'M':
            fak = 1e6;
            break;
        case 'G':
            fak = 1e9;
            break;
        case 'T':
            fak = 1e12;
            break;
        default:
            fak = 1;
            i--;
    }
    *len = i;
    if (i > 32)
        return -100;
    i = sscanf(text, "%le", &zahl);
    if (i == 0)
        return -100; /* not a valid number */
    if (fak != 1)
        zahl *= fak;
    *value = zahl;
    return 0;
}

/* Output functions */

/*--------------------------------------------------------------------------
 Ret.code:
 0 - engineering notation created
 1 - scientific notation created
 --------------------------------------------------------------------------*/
int ci_ing_out(double value, int n, char *ing)
/* value: input value as a double */
/* n:   number of digits */
/* ing:  number in engineering notation */ {
    int ibasis, itrans;
    int iex; /* Exponent */
    char *skal; /* scale */
    int l; /* sprintf length */
    char *cptr;
    
    l = n + 6;
    sprintf(ing, "%*.*e", l, l - 7, value);
    
    /* find the exponent */
    cptr = strchr(ing, 'e');
    if (cptr == NULL)
        cptr = strchr(ing, 'E');
    if (cptr == NULL)
        return (1);
    sscanf(cptr + 1, "%d", &iex);
    if (iex < -18 || iex >= 15)
        return (1);
    *cptr = 0;
    
    /* set the decimal dot */
    cptr = strchr(ing, '.');
    if (cptr == NULL)
        return (1);
    
    ibasis = ((iex + 18) / 3) * 3 - 18;
    itrans = iex - ibasis;
    
    if (itrans == 1) {
        cptr[0] = cptr[1];
        cptr[1] = '.';
    } else if (itrans == 2) {
        cptr[0] = cptr[1];
        cptr[1] = cptr[2];
        cptr[2] = '.';
    }
    skal = "";
    switch (ibasis) {
        case -3:
            skal = "m";
            break;
        case 3:
            skal = "k";
            break;
        case -6:
            skal = "u";
            break;
        case 6:
            skal = "M";
            break;
        case -9:
            skal = "n";
            break;
        case 9:
            skal = "G";
            break;
        case -12:
            skal = "p";
            break;
        case 12:
            skal = "T";
            break;
        case -15:
            skal = "f";
            break;
        case -18:
            skal = "a";
            break;
    }
    strcat(ing, skal);
    return (0);
}
