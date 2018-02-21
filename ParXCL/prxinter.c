/*
 * ParX - prxinter.c
 * Model Interpreter, functions and derivatives
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

#include "parx.h"
#include "error.h"
#include "prx_def.h"
#include "mem_def.h"
#include "prxinter.h"

#define BUFSIZE	1024
#define STACKSIZE 64

#define READITEM if ((nItems = (int)fread((char *)&sh,sizeof(sh),1,file)) == 0) \
{ errcode = NPX_IERR; return FALSE; }

static fnum *prx_getAddress(TYP, int, moddat);

/*************************** global variables ***************************/
static CODE *kindStart[4]; /* Start pointer for kinds of deriv.s [0]:
                            * function code [1]: variables derivatives
                            * [2]: auxiliaries derivatives [3]:
                            * parameters derivatives     */

static fnum *Stack; /* operand stack */
static fnum *Stack_end; /* pointer to end of stack */

static int nNum; /* number of numerical constants */
static int nTmp; /* number of temporaries */
static int iDvt; /* index of current deriv. variable */
static fnum *Tmp; /* pointer to temporaries */
static fnum *DTmp; /* pointer to deriv. of temporaries */
static fnum *Num; /* pointer to numerical constants */
static matrix jac; /* basic pointer for current Jacobian */

static struct MEM_TREE *Tree = NULL; /* memory tree */

inum prx_errcode;

/* Input and adaptation of interpreter code (once per PARX execution) */
boolean prx_inCode(moddat dat, FILE * inFile) {
    FILE *file;
    OPR opr; /* current operator */
    TYP typ; /* type of variable */
    int ind; /* array index */
    int kod; /* kind of derivatives (var, aux or par) */
    CODE *code; /* pointer in interpreter code */
    int nFree; /* available positions in current code buffer */
    CODE * IfPos[MAXLEVEL + 1] = { NULL };
    CODE * ElsePos[MAXLEVEL + 1] = { NULL };
    int level;
    int nItems;
    int i;
    char Buf[MAXLINE + 2];
    short sh;
    
    iDvt = 0; /* index of current deriv. variable */
    
    if (Tree)
        mem_free(Tree);
    Tree = mem_tree();
    file = inFile;
    level = 0;
    READITEM;
    nNum = sh;
    READITEM;
    nTmp = sh;
    READITEM;
    i = sh;
    if (i != strlen(FILEID) + 1) {
        errcode = NPX_IERR;
        return FALSE;
    }
    fread(Buf, i, 1, file);
    if (strcmp(Buf, FILEID) != 0) {
        errcode = NPX_IERR;
        return FALSE;
    }
    READITEM;
    i = sh;
    if (i != CODE_VERSION) {
        errcode = VER_IERR;
        return FALSE;
    }
    if (nNum < 0 || nTmp < 0) {
        errcode = CON_IERR;
        return FALSE;
    }
    if (nTmp) {
        Tmp = (fnum *) mem_slot(Tree, nTmp * sizeof (fnum));
        DTmp = (fnum *) mem_slot(Tree, nTmp * sizeof (fnum));
    }
    Num = (fnum *) mem_slot(Tree, nNum * sizeof (fnum));
    
    /* get 1st code buffer */
    code = (CODE *) mem_slot(Tree, BUFSIZE * sizeof (CODE));
    kindStart[0] = code;
    nFree = BUFSIZE - 4;
    kod = 0;
    jac = NULL;
    opr = INVAL;
    
    READITEM;
    while (nItems) {
        opr = (OPR) sh;
        if (opr >= STOP)
            break;
        if (nFree <= 0) { /* add new code buffer */
            (*code++).o = JMP;
            (*code).c = (CODE *) mem_slot(Tree, BUFSIZE * sizeof (CODE));
            code = (*code).c;
            nFree = BUFSIZE - 3;
        }
        switch (opr) {
            default:
                (*code++).o = opr;
                nFree--;
                break;
            case OPD:
                READITEM;
                typ = (TYP) sh;
                READITEM;
                ind = sh;
                (*code++).o = (typ != DRES) ? OPD : DOPD;
                if (typ == FLG)
                    (*(code - 1)).o = LDF;
                (*code++).f = prx_getAddress(typ, ind, dat);
                nFree -= 2;
                break;
            case NUM:
                READITEM;
                ind = sh;
                (*code++).o = opr;
                (*code++).f = (Num + ind);
                nFree -= 2;
                break;
            case ASS:
            case NASS:
                READITEM;
                typ = (TYP) sh;
                READITEM;
                ind = sh;
                (*code++).o = opr;
                (*code++).f = prx_getAddress(typ, ind, dat);
                nFree -= 2;
                break;
            case CLR:
                READITEM;
                typ = (TYP) sh;
                READITEM;
                ind = sh;
                (*code++).o = opr;
                (*code++).f = prx_getAddress(typ, ind, dat);
                nFree -= 2;
                break;
            case IF:
                (*code++).o = IF;
                IfPos[++level] = code++;
                ElsePos[level] = (CODE *) NULL;
                nFree -= 2;
                break;
            case ELSE:
                (*code++).o = JMP;
                ElsePos[level] = code++;
                assert(IfPos[level] != NULL);
                (*IfPos[level]).c = code;
                nFree -= 2;
                break;
            case FI:
                if (ElsePos[level])
                    (*ElsePos[level]).c = code;
                else {
                    assert(IfPos[level] != NULL);
                    (*IfPos[level]).c = code;
                }
                level--;
                break;
            case EOD: /* End Of (single) Derivative */
                (*code++).o = EOD;
                iDvt++;
                nFree--;
                break;
            case SOK: /* Start Of Kind of derivatives */
                kindStart[++kod] = code;
                iDvt = 0;
                jac = (kod == 1) ? dat->jx : (kod == 2) ? dat->ja : dat->jp;
                (*code++).o = SOK;
                nFree--;
                break;
        }
        READITEM;
    }
    
    if (opr != STOP) {
        errcode = EOF_IERR;
        return FALSE;
    }
    (*code).o = INVAL;
    
    /* constants of model function */
    if (!fread((char *) Num, sizeof (fnum), nNum, file)) {
        errcode = CON_IERR;
        return FALSE;
    }
    /* stack */
    
    Stack = (fnum *) mem_slot(Tree, (STACKSIZE) * sizeof (fnum));
    Stack_end = &(Stack[STACKSIZE - 1]);
    
    return TRUE;
}

fnum *prx_getAddress(TYP typ, int ind, moddat dat) {
    fnum *adr;
    
    switch (typ) {
        case VAR:
            adr = VECP(dat->x, ind);
            break;
        case AUX:
            adr = VECP(dat->a, ind);
            break;
        case PAR:
            adr = VECP(dat->p, ind);
            break;
        case CON:
            adr = VECP(dat->c, ind);
            break;
        case FLG:
            adr = VECP(dat->f, ind);
            break;
        case RES:
            adr = VECP(dat->r, ind);
            break;
        case TMP:
            adr = Tmp + ind;
            break;
        case DRES:
            assert(jac != NULL);
            adr = MATP(jac, ind, iDvt);
            break;
        case DTMP:
            adr = DTmp + ind;
            break;
        default:
            adr = 0;
            break;
    }
    return adr;
}

#define STCHECK(PST) {if (PST >= Stack_end) {errcode = STK_IERR; error("Model interpreter"); return FALSE;}}

/* Execution of interpreter code */
boolean prx_compute(moddat dat) {
    int kod; /* kind of derivatives */
    CODE *code; /* interpreter code pointer */
    fnum *pSt; /* operand stack pointer */
    
    prx_errcode = 0;
    
    code = kindStart[0];
    pSt = Stack;
    kod = 0;
    for (;;) {
        switch ((*code++).o) {
            default:
                errcode = COD_IERR;
                error("Model interpreter");
                return FALSE;
            case INVAL:
                return TRUE; /* finished */
            case AND:
                pSt--;
                *pSt = (*pSt != 0 && *(pSt + 1) != 0) ? 1 : 0;
                break;
            case OR:
                pSt--;
                *pSt = (*pSt != 0 || *(pSt + 1) != 0) ? 1 : 0;
                break;
            case NOT:
                *pSt = (*pSt == 0) ? 1 : 0;
                break;
            case LT:
                pSt--;
                *pSt = (*pSt < *(pSt + 1)) ? 1 : 0;
                break;
            case GT:
                pSt--;
                *pSt = (*pSt > *(pSt + 1)) ? 1 : 0;
                break;
            case LE:
                pSt--;
                *pSt = (*pSt <= *(pSt + 1)) ? 1 : 0;
                break;
            case GE:
                pSt--;
                *pSt = (*pSt >= *(pSt + 1)) ? 1 : 0;
                break;
            case EQ:
                pSt--;
                *pSt = (*pSt == *(pSt + 1)) ? 1 : 0;
                break;
            case NE:
                pSt--;
                *pSt = (*pSt != *(pSt + 1)) ? 1 : 0;
                break;
            case ADD:
                pSt--;
                *pSt = *pSt + *(pSt + 1);
                break;
            case SUB:
                pSt--;
                *pSt = *pSt - *(pSt + 1);
                break;
            case MUL:
                pSt--;
                *pSt = *pSt * *(pSt + 1);
                break;
            case DIV:
                pSt--;
                *pSt = *pSt / *(pSt + 1);
                break;
            case POW:
                pSt--;
                *pSt = pow(*pSt, *(pSt + 1));
                break;
            case SGN:
                *pSt = (*pSt >= 0) ? 1 : -1;
                break;
            case SIN:
                *pSt = sin(*pSt);
                break;
            case COS:
                *pSt = cos(*pSt);
                break;
            case TAN:
                *pSt = tan(*pSt);
                break;
            case ASIN:
                *pSt = asin(*pSt);
                break;
            case ACOS:
                *pSt = acos(*pSt);
                break;
            case ATAN:
                *pSt = atan(*pSt);
                break;
            case EXP:
                *pSt = exp(*pSt);
                break;
            case LOG:
                *pSt = log(*pSt);
                break;
            case LG:
                *pSt = log10(*pSt);
                break;
            case SQRT:
                *pSt = sqrt(*pSt);
                break;
            case SQR:
                *pSt = *pSt * *pSt;
                break;
            case NEG:
                *pSt = -*pSt;
                break;
            case REV:
                *pSt = 1 / *pSt;
                break;
            case INC:
                *pSt += 1;
                break;
            case DEC:
                *pSt -= 1;
                break;
            case ABS:
                if (*pSt < 0)
                    *pSt = -*pSt;
                break;
            case RET:
                prx_errcode = *pSt;
                return (*pSt != 0) ? FALSE : TRUE;
                break;
            case CHKL:
                pSt--;
                if (*pSt < *(pSt + 1))
                    return FALSE;
                pSt--;
                break;
            case CHKG:
                pSt--;
                if (*pSt > *(pSt + 1))
                    return FALSE;
                pSt--;
                break;
            case DOPD:
            case OPD:
                *(++pSt) = *((*code++).f);
                STCHECK(pSt);
                break;
            case NUM:
                *(++pSt) = *((*code++).f);
                STCHECK(pSt);
                break;
            case LDF:
                *(++pSt) = trunc(*((*code++).f)) ? 1 : 0;
                STCHECK(pSt);
                break;
            case ASS:
                *((*code++).f) = *(pSt--);
                break;
            case NASS:
                *((*code++).f) = -(*(pSt--));
                break;
            case CLR:
                *((*code++).f) = 0.0;
                break;
            case IF:
                if (*(pSt--) == 0)
                    code = (*code).c;
                else
                    code++;
                break;
            case EOD:
                iDvt++;
                if ((kod == 1) && (iDvt < VECN(dat->xf)) && (VEC(dat->xf, iDvt) == FALSE))
                    for (code++; (*code).o != EOD; code++);
                if ((kod == 3) && (iDvt < VECN(dat->pf)) && (VEC(dat->pf, iDvt) == FALSE))
                    for (code++; (*code).o != EOD; code++);
                break;
            case SOK:
                kod++;
                iDvt = 0;
                if (kod == 1) {
                    if (!dat->jxf) {
                        code = kindStart[3];
                        kod++;
                    } else if (VEC(dat->xf, iDvt) == FALSE)
                        for (code++; (*code).o != EOD; code++);
                } else if (kod == 3) {
                    if (!dat->jpf)
                        return TRUE;
                    if (VEC(dat->pf, iDvt) == FALSE)
                        for (code++; (*code).o != EOD; code++);
                }
                break;
            case JMP:
                code = (*code).c;
                break;
        }
    }
}
