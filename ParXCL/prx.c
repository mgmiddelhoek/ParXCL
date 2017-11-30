/*
 * ParX - prx.c
 * Model Compiler, convert functions and derivatives to intermediate code
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

#include "parx.h"
#include "mem_def.h"
#include "bt_def.h"
#include "prx_def.h"

#define ERROR(s)       { printf("%s:%d: error: %s\n", prx_filename, prx_lineno, s); \
bError |= 2;  return 0; }

#define ERRORA(f,s)    { printf("%s:%d: error: ", prx_filename, prx_lineno); \
printf(f,s);  printf("\n"); bError |= 2;  return 0; }

#define WARNING(s)     { printf("%s:%d: warning: %s\n", prx_filename, prx_lineno, s); \
bError |= 1; }

#define WARNINGA(f,s)  { printf("%s:%d: warning: ", prx_filename, prx_lineno); \
printf(f,s);  printf("\n"); bError |= 1; }

#define NODE(p,op,op1,op2)  p = (PRX_NODE *)mem_slot(Tree,sizeof(PRX_NODE)); \
p->opr = op;  p->o1 = op1;  p->c.o2 = op2;

#define WRITEM(i)      { sh = (i); if (!fwrite((void *)&sh,sizeof(sh),1,oFile)) \
{ write_error(); return 0; } }

static const struct {
    char *name;
    OPR opr;
} FunSt[] = {
    { "sin", SIN},
    { "cos", COS},
    { "tan", TAN},
    { "asin", ASIN},
    { "acos", ACOS},
    { "atan", ATAN},
    { "exp", EXP},
    { "log", LOG},
    { "log10", LG},
    { "sqrt", SQRT},
    { "abs", ABS},
    { "sign", SGN},
    { "not", NOT}
};

static const int nFunSt = sizeof (FunSt) / sizeof (FunSt[0]);
/* Number of standard functions */

static FILE *oFile; /* File pointer for code file */
static FILE *mFile; /* File pointer for model file */

static struct MEM_TREE *Tree, *DTree; /* memory trees */
static struct BT_HEAD *BtNames; /* balanced bin. tree of names */
static struct BT_HEAD *BtNumbers; /* balanced bin. tree of numbers */

static int bError; /* general error flag */
static int bDeriv; /* Deriv.s are (not) actually computed */
static int ifLevel; /* if nesting level */
static int bAssign; /* subexpression can start with assign */

static char *sModel, *sDate, *sAuthor, *sVersion, *sIdent;
static int nVar, nAux, nPar, nCon, nFlag;
static int nRes, nNum, nTmp;
static HEADER *sVar, *sAux, *sPar, *sCon, *sFlg;
static HEADER *sRes;
static double *Nums;

static PRX_NODE *NodeH[MAXEQU]; /* array of tree pointers */
static PRX_NODE **pHead; /* pointer for array NodeH */
static int nHead; /* number of expression trees */

static PRX_NODE *pNode; /* pointer to any node in a tree */
static PRX_NODE *St[128], **pSt; /* priority stack */
static char Prio[32]; /* operator priority */
static int IfStatus[MAXLEVEL + 1];

static char UsageFlag[MAXEQU]; /* bit flag: operand of corr. is */
static char TmpTyp[MAXEQU]; /* flag: corresponding temporary derivative
                             * is (not) needed to compute further */
static PRX_OPD **varDefs; /* pointer to variables list */
static PRX_OPD **auxDefs; /* pointer to auxiliaries list */
static PRX_OPD **parDefs; /* pointer to parameters list */

static PRX_NODE *N_0, *N_1, *N_2, *N_0p5, *N_1_ln10;

static int prx_simplify(PRX_NODE * p);
static int prx_genCode(PRX_NODE * pNode);
static int write_error(void);
static int bt_cmp_names(PRX_OPD * s1, PRX_OPD * s2);
static int bt_cmp_numbers(PRX_NUM * s1, PRX_NUM * s2);
static PRX_NODE *getNum(double z);
static PRX_OPD *newName(char *name, TYP typ, int ind);
static int namTraverse(char *rec);
static int namTraverse2(char *rec);
static int prx_expr(char *ein);
static int prx_genCode(PRX_NODE * pNode);
static int numTraverse(char *rec);
static int numOut(void);
static int prx_deriv(PRX_OPD * pOpd);
static int prx_deriv_expr(PRX_NODE * p, PRX_OPD * arg, PRX_OPD * fval);


/* ========================================================================== */

/* Initialization routine */
int prx_init(FILE * outFile, FILE * modFile) {
    short sh;
    int i;
    char *pc;
    
    oFile = outFile;
    mFile = modFile;
    
    Tree = mem_tree();
    DTree = mem_tree();
    BtNames = bt_define_tree(Tree, bt_cmp_names);
    BtNumbers = bt_define_tree(Tree, bt_cmp_numbers);
    
    bError = 0;
    bDeriv = 0;
    ifLevel = 0;
    bAssign = 1;
    
    sModel = sDate = sAuthor = sVersion = sIdent = NULL;
    nVar = nAux = nPar = nCon = nFlag = 0;
    nRes = nNum = nTmp = 0;
    sVar = sAux = sPar = sCon = sFlg = sRes = NULL;
    Nums = NULL;
    
    for (i = 0; i < MAXEQU; i++) {
        NodeH[i] = NULL;
        TmpTyp[i] = 0;
        UsageFlag[i] = 0;
    }
    
    pHead = NodeH;
    nHead = 0;
    pNode = NULL;
    
    for (i = 0; i < 128; i++)
        St[i] = NULL;
    pSt = NULL;
    
    Prio[INVAL] = 0;
    Prio[AND] = Prio[OR] = 1;
    Prio[NOT] = 2;
    Prio[LT] = Prio[GT] = Prio[LE] = Prio[GE] = Prio[EQ] = Prio[NE] = 3;
    Prio[ADD] = 5; /* overwritten in prx_equation */
    Prio[NEG] = Prio[SUB] = 6;
    Prio[MUL] = 7;
    Prio[DIV] = Prio[REV] = 8;
    Prio[POW] = Prio[SQR] = 9;
    
    for (i = 0; i < (MAXLEVEL + 1); i++)
        IfStatus[i] = 0;
    
    N_0 = getNum(0.);
    N_1 = getNum(1.);
    N_2 = getNum(2.);
    N_0p5 = getNum(0.5);
    N_1_ln10 = getNum(M_LOG10E);
    
    varDefs = auxDefs = parDefs = NULL;
    
    WRITEM(-1);
    WRITEM(-1); /* placeholder for nNum & nTmp */
    pc = FILEID;
    WRITEM(strlen(pc) + 1);
    if (!fwrite(pc, 1, strlen(pc) + 1, oFile)) {
        write_error();
        return 0;
    }
    WRITEM(CODE_VERSION);
    return 1;
}

/* ========================================================================== */

/* end game routine */
int prx_exit(void) {
    size_t sT, sD;
    
    sT = mem_free(Tree);
    sD = mem_free(DTree);
    printf("Total Memory: %lu\n", (unsigned long) (sT + sD));
    return (int)(sT + sD);
}

/* ========================================================================== */

/* returns current error status */
int prx_error() {
    return bError;
}

/* ========================================================================== */

/* on write error */
int write_error() {
    ERROR("Error writing output file");
}

/* ========================================================================== */

/* comparison routine for the balanced binary tree of names */
int bt_cmp_names(PRX_OPD * s1, PRX_OPD * s2) {
    return strcmp(s1->name, s2->name);
}

/* ========================================================================== */

/* comparison routine for the balanced binary tree of numbers */
int bt_cmp_numbers(PRX_NUM * s1, PRX_NUM * s2) {
    return (s1->val < s2->val) ? -1 : (s1->val > s2->val) ? 1 : 0;
}

/* ========================================================================== */

/*
 * Providing a constant contained in balanced binary tree of constants. If
 * desired constant is not yet present it is generated
 */
PRX_NODE *getNum(double z) {
    PRX_NUM *pNum;
    PRX_NODE *p;
    
    pNum = (PRX_NUM *) bt_search(BtNumbers, (char *) &z);
    if (pNum)
        return pNum->node;
    pNum = (PRX_NUM *) mem_slot(Tree, sizeof (PRX_NUM));
    pNum->val = z;
    pNum->ind = nNum++;
    NODE(p, NUM, NULL, (PRX_NODE *) pNum);
    pNum->node = p;
    bt_insert(BtNumbers, (char *) pNum);
    return p;
}

/* ========================================================================== */

/*
 * Generating an object in balanced binary tree of names (var, par, aux,
 * const, flag, res, temp)
 */
PRX_OPD *newName(char *name, TYP typ, int ind) {
    PRX_OPD *pOpd;
    char *pc;
    
    pOpd = (PRX_OPD *) mem_slot(Tree, sizeof (PRX_OPD) + strlen(name) + 1);
    pc = ((char *) pOpd) + sizeof (PRX_OPD);
    strcpy(pc, name);
    pOpd->name = pc;
    pOpd->typ = typ;
    pOpd->ind = ind;
    pOpd->node = NULL;
    bt_insert(BtNames, (char *) pOpd);
    return pOpd;
}
/* ========================================================================== */

/*
 * Checking a header statement of PARX model description file and generating
 * interpreter code for boundary checks return value: 0 - error 1 - okay 2 -
 * end of header part (on equation statement)
 */
int prx_header(char *Buf) {
    TYP typ;
    int lae;
    int nVals;
    char *pe, *pa;
    int *pCount; /* pointer to counter to be increased */
    char Name[MAXNAME + 1];
    char *name = Name;
    double Vals[5]; /* Array for default, bounds, scales */
    double minLim, maxLim;
    PRX_NODE *pNode;
    PRX_OPD *pOpd;
    char *sep;
    char sep1[] = "  ";
    char sep2[] = ",\n  ";
    HEADER *sHeader, *pHFree, **pHeader;
    short sh;
    
    lae = prx_name(Buf);
    if (lae <= 2 || Buf[lae] != ':') {
        ERROR("Invalid keyword");
    }
    pe = Buf + lae + 1;
    if (memcmp(Buf, "parameters", lae) == 0) {
        pHeader = &sPar;
        typ = PAR;
        pCount = &nPar;
    } else if (memcmp(Buf, "variables", lae) == 0) {
        pHeader = &sVar;
        typ = VAR;
        pCount = &nVar;
    } else if (memcmp(Buf, "constants", lae) == 0) {
        pHeader = &sCon;
        typ = CON;
        pCount = &nCon;
    } else if (memcmp(Buf, "flags", lae) == 0) {
        pHeader = &sFlg;
        typ = FLG;
        pCount = &nFlag;
    } else if (memcmp(Buf, "residuals", lae) == 0) {
        pHeader = &sRes;
        typ = RES;
        pCount = &nRes;
    } else if (memcmp(Buf, "auxiliary", lae) == 0) {
        pHeader = &sAux;
        typ = AUX;
        pCount = &nAux;
    } else if (memcmp(Buf, "auxiliaries", lae) == 0) {
        pHeader = &sAux;
        typ = AUX;
        pCount = &nAux;
    } else if (memcmp(Buf, "model", lae) == 0) {
        sModel = (char *) mem_slot(DTree, (strlen(pe) + 1) * sizeof (char));
        strcpy(sModel, pe);
        return 1;
    } else if (memcmp(Buf, "date", lae) == 0) {
        sDate = (char *) mem_slot(DTree, (strlen(pe) + 1) * sizeof (char));
        strcpy(sDate, pe);
        return 1;
    } else if (memcmp(Buf, "author", lae) == 0) {
        sAuthor = (char *) mem_slot(DTree, (strlen(pe) + 1) * sizeof (char));
        strcpy(sAuthor, pe);
        return 1;
    } else if (memcmp(Buf, "version", lae) == 0) {
        sVersion = (char *) mem_slot(DTree, (strlen(pe) + 1) * sizeof (char));
        strcpy(sVersion, pe);
        return 1;
    } else if (memcmp(Buf, "ident", lae) == 0) {
        sIdent = (char *) mem_slot(DTree, (strlen(pe) + 1) * sizeof (char));
        strcpy(sIdent, pe);
        return 1;
    } else if (memcmp(Buf, "equations", lae) == 0) {
        fputs("|| Generated by ParX Code Generator\n", mFile);
        fputs("(\n", mFile);
        if (sModel)
            fprintf(mFile, "|| model\n\"%s\",\n", sModel);
        else
            fputs("|| no model\n\"no model\",\n", mFile);
        if (sVersion)
            fprintf(mFile, "|| version\n\"%s\",\n", sVersion);
        else
            fputs("|| no version\n\"no version\"\n", mFile);
        if (sAuthor)
            fprintf(mFile, "|| author\n\"%s\",\n", sAuthor);
        else
            fputs("|| no author\n\"no author\",\n", mFile);
        if (sDate)
            fprintf(mFile, "|| date\n\"%s\",\n", sDate);
        else
            fputs("|| no date\n\"no date\",\n", mFile);
        if (sIdent)
            fprintf(mFile, "|| ident\n\"%s\",\n\n", sIdent);
        else
            fputs("|| no ident\n\"no ident\",\n\n", mFile);
        if (sRes) {
            sHeader = sRes->next;
            sRes->next = NULL;
            fprintf(mFile, "|| residuals\n[\n");
            for (; sHeader; sHeader = sHeader->next) {
                fprintf(mFile, "%s", sHeader->text);
                if (sHeader->next)
                    fprintf(mFile, ",\n");
            }
            fprintf(mFile, "\n],\n");
        } else
            fputs("|| no residuals\n[],\n", mFile);
        if (sVar) {
            sHeader = sVar->next;
            sVar->next = NULL;
            fprintf(mFile, "|| variables\n[\n");
            for (; sHeader; sHeader = sHeader->next) {
                fprintf(mFile, "%s", sHeader->text);
                if (sHeader->next)
                    fprintf(mFile, ",\n");
            }
            fprintf(mFile, "\n],\n");
        } else
            fputs("|| no variables\n[],\n", mFile);
        if (sAux) {
            sHeader = sAux->next;
            sAux->next = NULL;
            fprintf(mFile, "|| auxiliaries\n[\n");
            for (; sHeader; sHeader = sHeader->next) {
                fprintf(mFile, "%s", sHeader->text);
                if (sHeader->next)
                    fprintf(mFile, ",\n");
            }
            fprintf(mFile, "\n],\n");
        } else
            fputs("|| no auxiliaries\n[],\n", mFile);
        if (sPar) {
            sHeader = sPar->next;
            sPar->next = NULL;
            fprintf(mFile, "|| parameters\n[\n");
            for (; sHeader; sHeader = sHeader->next) {
                fprintf(mFile, "%s", sHeader->text);
                if (sHeader->next)
                    fprintf(mFile, ",\n");
            }
            fprintf(mFile, "\n],\n");
        } else
            fputs("|| no parameters\n[],\n", mFile);
        if (sCon) {
            sHeader = sCon->next;
            sCon->next = NULL;
            fprintf(mFile, "|| constants\n[\n");
            for (; sHeader; sHeader = sHeader->next) {
                fprintf(mFile, "%s", sHeader->text);
                if (sHeader->next)
                    fprintf(mFile, ",\n");
            }
            fprintf(mFile, "\n],\n");
        } else
            fputs("|| no constants\n[],\n", mFile);
        if (sFlg) {
            sHeader = sFlg->next;
            sFlg->next = NULL;
            fprintf(mFile, "|| flags\n[\n");
            for (; sHeader; sHeader = sHeader->next) {
                fprintf(mFile, "%s", sHeader->text);
                if (sHeader->next)
                    fprintf(mFile, ",\n");
            }
            fprintf(mFile, "\n]\n)");
        } else
            fputs("|| no flags\n[]\n)\n", mFile);
        
        return 2;
    } else {
        ERROR("Invalid keyword");
    }
    /* if (*pCount > 0)  ERROR("multiple keyword\n"); */
    pHFree = (HEADER *) mem_slot(DTree, sizeof (HEADER));
    ;
    pHFree->text = (char *) mem_slot(DTree, (MAXLINE + 4) * sizeof (char));
    if (*pHeader != NULL) {
        pHFree->next = (*pHeader)->next;
        (*pHeader)->next = pHFree;
    } else
        pHFree->next = pHFree;
    *pHeader = pHFree;
    pa = pHFree->text;
    sep = sep1;
    do {
        lae = prx_name(pe);
        if (lae == 0)
            ERROR("syntax in name list");
        if (lae < 0) {
            lae = MAXNAME;
            memcpy(name, pe, lae);
            name[lae] = 0;
            ERRORA("name too long : %s", name);
        }
        memcpy(name, pe, lae);
        name[lae] = 0;
        
        pOpd = (PRX_OPD *) bt_search(BtNames, (char *) &name);
        if (pOpd)
            ERRORA("%s has already been defined", name);
        sprintf(pa, "%s(\"%s\"", sep, name);
        pa += strlen(pa);
        pOpd = newName(name, typ, (*pCount)++);
        pe += lae;
        Vals[0] = 0;
        Vals[1] = -HUGE_VAL;
        Vals[2] = HUGE_VAL;
        Vals[3] = -HUGE_VAL;
        Vals[4] = HUGE_VAL;
        if (*pe == '=') {
            lae = prx_values(++pe, Vals, &nVals);
            if (lae <= 0)
                ERRORA("Value list of %s", name);
            if (typ == PAR || typ == VAR || typ == AUX) {
                if (typ != PAR && nVals > 3)
                    WARNINGA("Maximum quantity of values exceeded for %s", name);
                minLim = (typ == PAR) ? Vals[3] : Vals[1];
                maxLim = (typ == PAR) ? Vals[4] : Vals[2];
                if (minLim > -HUGE_VAL) {
                    WRITEM(OPD);
                    WRITEM(pOpd->typ);
                    WRITEM(pOpd->ind);
                    pNode = getNum(minLim);
                    WRITEM(NUM);
                    WRITEM(pNode->c.nptr->ind);
                    WRITEM(CHKL);
                }
                if (maxLim < HUGE_VAL) {
                    WRITEM(OPD);
                    WRITEM(pOpd->typ);
                    WRITEM(pOpd->ind);
                    pNode = getNum(maxLim);
                    WRITEM(NUM);
                    WRITEM(pNode->c.nptr->ind);
                    WRITEM(CHKG);
                }
            } else if (nVals > 1)
                WARNINGA("Maximum quantity of values exceeded for %s", name);
            pe += lae;
        }
        if (typ == PAR || typ == VAR || typ == AUX || typ == CON) {
            sprintf(pa, ",%g", Vals[0]);
            pa += strlen(pa);
        }
        if (typ == PAR) {
            sprintf(pa, ",%g", Vals[1]);
            pa += strlen(pa);
            sprintf(pa, ",%g", Vals[2]);
            pa += strlen(pa);
        }
        if (typ == FLG) {
            sprintf(pa, ",%g", trunc(Vals[0]));
            pa += strlen(pa);
        }
        *(pa++) = ')';
        *(pa) = 0;
        sep = sep2;
    } while (*(pe++) == ',');
    if (*(--pe) != 0) {
        ERROR("syntax in name list");
    }
    
    return 1;
}

/* ========================================================================== */

/* Action routine used in tree traverse of the following procedure */
int namTraverse(char *rec) {
    PRX_OPD *pOpd;
    
    pOpd = (PRX_OPD *) rec;
    if (pOpd->typ == PAR)
        parDefs[pOpd->ind] = pOpd;
    else if (pOpd->typ == VAR)
        varDefs[pOpd->ind] = pOpd;
    else if (pOpd->typ == AUX)
        auxDefs[pOpd->ind] = pOpd;
    return 0;
}

/* ========================================================================== */

/* Generation of lists of derivative variables */
int prx_derivLists(void) {
    varDefs = (PRX_OPD **) mem_slot(Tree, nVar * sizeof (PRX_OPD *));
    auxDefs = (PRX_OPD **) mem_slot(Tree, nAux * sizeof (PRX_OPD *));
    parDefs = (PRX_OPD **) mem_slot(Tree, nPar * sizeof (PRX_OPD *));
    bt_traverse(BtNames, namTraverse);
    return 1;
}

/* ========================================================================== */

/* Action routine used in tree traverse of the following procedure */
int namTraverse2(char *rec) {
    PRX_OPD *pOpd;
    TYP typ;
    
    pOpd = (PRX_OPD *) rec;
    typ = pOpd->typ;
    if (typ == PAR || typ == VAR || typ == TMP || typ == AUX) {
        if (!(UsageFlag[pOpd->ind] & (1 << typ)))
            WARNINGA("%s is never used", pOpd->name);
    } else if (typ == RES) {
        if (!(UsageFlag[pOpd->ind] & (1 << typ)))
            WARNINGA("%s is never assigned", pOpd->name);
    }
    return 0;
}

/* ========================================================================== */

/* Is everything ok before generating derivatives? */
int prx_check(void) {
    if (ifLevel > 0)
        ERROR("if condition not closed by fi");
    if (nHead > MAXEQU)
        ERRORA("maximum number of statements (%d) exceeded", MAXEQU);
    bt_traverse(BtNames, namTraverse2);
    if (nRes <= 0)
        ERROR("no residuals");
    *pHead = NULL;
    return 1;
}

/* ========================================================================== */

/* Parsing a command line */
int prx_equation(char *ein) {
    int lae;
    PRX_NODE *pNodeV;
    
    if (*ein == 0)
        return 1;
    if (++nHead > MAXEQU)
        pHead = NodeH;
    
    pSt = St;
    Prio[ADD] = 5;
    /* if ... then ... else */
    if (memcmp(ein, "if(", 3) == 0) {
        bAssign = 0;
        lae = prx_expr(ein + 2);
        if (lae <= 0)
            return 0;
        pNodeV = pNode;
        NODE(pNode, IF, pNodeV, NULL);
        *(pHead++) = pNode;
        prx_genCode(pNode);
        if (ifLevel >= MAXLEVEL)
            ERROR("Maximum 'if' hierarchy depth exceeded");
        ifLevel++;
        IfStatus[ifLevel] = 0;
        return 1;
    } else if (strcmp(ein, "else") == 0) {
        if (ifLevel <= 0)
            ERROR("No preceding 'if' statement");
        if (IfStatus[ifLevel] > 0)
            ERROR("Multiple 'else'");
        NODE(pNode, ELSE, NULL, NULL);
        *(pHead++) = pNode;
        prx_genCode(pNode);
        IfStatus[ifLevel] = 1;
        return 1;
    } else if (strcmp(ein, "fi") == 0) {
        if (ifLevel <= 0)
            ERROR("No active 'if' statement");
        NODE(pNode, FI, NULL, NULL);
        *(pHead++) = pNode;
        prx_genCode(pNode);
        ifLevel--;
        return 1;
    }
    /* Assignment */
    bAssign = 1;
    lae = prx_expr(ein);
    if (lae <= 0 || ein[lae] != 0)
        return 0;
    prx_simplify(pNode);
    prx_simplify(pNode);
    *(pHead++) = pNode;
    prx_genCode(pNode);
    return 1;
}

/* ========================================================================== */

/*
 * Checking a substring Arguments: - ein :  Pointer to substring to be
 * analyzed
 */

int prx_expr(char *ein) {
    char *pe;
    char Name[MAXNAME + 1], *name;
    int lae, l;
    int k;
    double z;
    OPR opr;
    OPR Op[16]; /* Operator buffer for Priority control */
    int iOp;
    PRX_OPD *pOpd;
    
    name = Name;
    pe = ein;
    iOp = 0;
    
    if (!bAssign)
        goto operand;
    
    if (memcmp(pe, "error(", 6) == 0) {
        bAssign = 0;
        pe += 6;
        lae = prx_expr(pe);
        if (lae <= 0)
            return 0;
        pSt--;
        NODE(pNode, RET, *pSt, NULL);
        *(pSt++) = pNode;
        pe += lae;
        if (*pe == ')')
            pe++;
        return (int)(pe - ein);
    }
    lae = prx_name(pe);
    if (lae == 0)
        ERROR("syntax error in variable name");
    if (lae < 0)
        ERROR("variable name too long");
    if (pe[lae] != '=' || pe[lae + 1] == '=') {
        ERROR("assignment expected");
    }
    /* bAssign = 0;  goto operand; } */
    /* assignment */
    bAssign = 0;
    memcpy(name, pe, lae);
    name[lae] = 0;
    l = lae + 1;
    pe += l;
    lae = prx_expr(pe);
    if (lae <= 0)
        return 0;
    pOpd = (PRX_OPD *) bt_search(BtNames, (char *) &name);
    if (pOpd) {
        if (pOpd->typ == CON || pOpd->typ == FLG)
            ERROR("invalid assignment")
            if (pOpd->typ != RES && pOpd->typ != TMP)
                WARNINGA("assignment to '%s' may be an error", name);
    } else {
        pOpd = newName(name, TMP, nTmp++);
    }
    if (pOpd->typ == RES)
        UsageFlag[pOpd->ind] |= 1 << RES;
    if (!pOpd->node) {
        NODE(pNode, OPD, NULL, (PRX_NODE *) pOpd);
        pOpd->node = pNode;
    }
    pSt--;
    NODE(pNode, ASS, *pSt, (PRX_NODE *) pOpd);
    *(pSt++) = pNode;
    pe += lae;
    return (int)(pe - ein);
operation:
    switch (*pe) {
        case '&':
            opr = AND;
            break;
        case '|':
            opr = OR;
            break;
        case '<':
            if (*(pe + 1) == '=') {
                opr = LE;
                pe++;
            } else if (*(pe + 1) == '>') {
                opr = NE;
                pe++;
            } else
                opr = LT;
            break;
        case '>':
            if (*(pe + 1) == '=') {
                opr = GE;
                pe++;
            } else
                opr = GT;
            break;
        case '!':
            if (*(pe + 1) == '=') {
                opr = NE;
                pe++;
            } else
                opr = INVAL;
            break;
        case '=':
            if (*(pe + 1) == '=') {
                opr = EQ;
                pe++;
            } else
                opr = INVAL;
            break;
        case '+':
            opr = ADD;
            break;
        case '-':
            opr = SUB;
            break;
        case '*':
            opr = MUL;
            break;
        case '/':
            opr = DIV;
            break;
        case '^':
            opr = POW;
            break;
        default:
            opr = INVAL;
    }
    while (iOp > 0) {
        if (Prio[opr] > Prio[Op[iOp - 1]])
            break;
        pSt--;
        iOp--;
        if (Op[iOp] == NEG || Op[iOp] == NOT) { /* 1 operand  */
            NODE(pNode, Op[iOp], *pSt, NULL);
        } else { /* 2 operands */
            pSt--;
            NODE(pNode, Op[iOp], *pSt, *(pSt + 1));
        }
        *(pSt++) = pNode;
    }
    if (*pe == ',' || *pe == ')' || *pe == ';' || *pe == 0)
        return (int)(pe - ein);
    if (opr == INVAL)
        ERROR("syntax");
    Op[iOp++] = opr;
    pe++;
    goto operand;
    /* arithmetic operand */
operand:
    if (!bAssign) { /* digest prefix operators */
        if (*pe == '+') {
            pe++;
            goto operand;
        }
        if (*pe == '-') {
            Op[iOp++] = NEG;
            pe++;
            goto operand;
        }
        if (*pe == '!') {
            Op[iOp++] = NOT;
            pe++;
            goto operand;
        }
    }
    if (ci_ing_in(pe, &z, &lae))
        goto name;
    /* ___ operand is number */
    pNode = getNum(z);
    *(pSt++) = pNode;
    pe += lae;
    goto operation;
name:
    lae = prx_name(pe);
    if (lae <= 0)
        goto parenth;
    memcpy(name, pe, lae);
    name[lae] = 0;
    if (pe[lae] == ')' - 1)
        goto function;
    /* ___ operand is variable */
    pOpd = (PRX_OPD *) bt_search(BtNames, (char *) &name);
    if (!pOpd)
        ERRORA("undefined item %s", name);
    if (pOpd->typ != RES)
        UsageFlag[pOpd->ind] |= (9) << pOpd->typ;
    else if (!(UsageFlag[pOpd->ind] & ((1) << RES)))
        ERRORA("%s is used but not assigned before", name);
    pNode = pOpd->node;
    if (!pNode) {
        NODE(pNode, OPD, NULL, (PRX_NODE *) pOpd);
        pOpd->node = pNode;
    }
    *(pSt++) = pNode;
    pe += lae;
    goto operation;
parenth:
    if (*pe != '(')
        ERRORA("syntax error, expected ( but got '%s'", pe);
    /* ___ expression in parentheses */
    pe++;
    lae = prx_expr(pe);
    if (lae <= 0)
        return 0;
    pe += lae;
    if (*(pe++) != ('(' + 1))
        ERROR("closing parenthesis missing");
    goto operation;
    /* ___ function call */
function:
    pe += lae;
    for (k = 0; k < nFunSt; k++)
        if (!strcmp(FunSt[k].name, name))
            break;
    if (k >= nFunSt)
        ERRORA("function %s undefined", name);
    lae = prx_expr(++pe);
    if (lae <= 0 || pe[lae] != ')')
        ERRORA("argument error in function '%s'", name);
    pSt--;
    NODE(pNode, FunSt[k].opr, *pSt, NULL);
    *(pSt++) = pNode;
    pe += lae + 1;
    goto operation;
}

/* ========================================================================== */

/* Generation of arithmetic code from code tree */
int prx_genCode(PRX_NODE * pNode) {
    OPR opr;
    TYP typ;
    short sh;
    double z;
    PRX_NODE *pN;
    
    if (!pNode)
        return 0;
    opr = pNode->opr;
    switch (opr) {
        case AND:
        case OR:
        case LT:
        case GT:
        case LE:
        case GE:
        case EQ:
        case NE:
        case MUL:
        case DIV:
        case POW:
            if (!prx_genCode(pNode->o1))
                return 0;
            if (!prx_genCode(pNode->c.o2))
                return 0;
            WRITEM(opr);
            break;
        case SUB:
            if (!prx_genCode(pNode->o1))
                return 0;
            if (pNode->c.o2 == N_1) {
                WRITEM(DEC);
            } else {
                if (!prx_genCode(pNode->c.o2))
                    return 0;
                WRITEM(opr);
            }
            break;
        case ADD:
            if (pNode->c.o2 == N_1) {
                if (!prx_genCode(pNode->o1))
                    return 0;
                WRITEM(INC);
            } else if (pNode->o1 == N_1) {
                if (!prx_genCode(pNode->c.o2))
                    return 0;
                WRITEM(INC);
            } else {
                if (!prx_genCode(pNode->o1))
                    return 0;
                if (!prx_genCode(pNode->c.o2))
                    return 0;
                WRITEM(opr);
            }
            break;
        case NEG:
            if (pNode->o1->opr == NUM) {
                z = -pNode->o1->c.nptr->val;
                if (z == HUGE_VAL)
                    ERROR("subtraction overflow");
                pN = getNum(z);
                WRITEM(NUM);
                WRITEM(pN->c.nptr->ind);
            } else {
                if (!prx_genCode(pNode->o1))
                    return 0;
                WRITEM(opr);
            }
            break;
        case REV:
            if (pNode->o1->opr == NUM) {
                z = 1 / pNode->o1->c.nptr->val;
                if (fabs(z) == HUGE_VAL)
                    ERROR("division overflow");
                pN = getNum(z);
                WRITEM(NUM);
                WRITEM(pN->c.nptr->ind);
            } else {
                if (!prx_genCode(pNode->o1))
                    return 0;
                WRITEM(opr);
            }
            break;
        case EQU:
            if (!prx_genCode(pNode->o1))
                return 0;
            break;
        case OPD:
            WRITEM(opr);
            WRITEM(pNode->c.optr->typ);
            WRITEM(pNode->c.optr->ind);
            break;
        case DOPD:
            WRITEM(OPD);
            typ = pNode->c.optr->typ;
            if (typ == RES)
                typ = DRES;
            else if (typ == TMP)
                typ = DTMP;
            WRITEM(typ);
            WRITEM(pNode->c.optr->ind);
            break;
        case NUM:
            WRITEM(NUM);
            WRITEM(pNode->c.nptr->ind);
            break;
        case ASS:
            if (pNode->o1 == N_0)
                opr = CLR;
            else {
                pN = pNode->o1;
                if (pN->opr == NEG)
                    if (pN->o1->opr != NUM) {
                        pN = pN->o1;
                        opr = NASS;
                    }
                if (!prx_genCode(pN))
                    return 0;
            }
            typ = pNode->c.optr->typ;
            if (bDeriv) {
                if (typ == RES)
                    typ = DRES;
                else if (typ == TMP) {
                    if (opr == CLR) {
                        if (TmpTyp[pNode->c.optr->ind] == 0)
                            break;
                    }
                    typ = DTMP;
                }
            }
            WRITEM(opr);
            WRITEM(typ);
            WRITEM(pNode->c.optr->ind);
            break;
        case SQR:
        case SGN:
        case IF:
        case SIN:
        case COS:
        case TAN:
        case ASIN:
        case ACOS:
        case ATAN:
        case EXP:
        case LOG:
        case LG:
        case SQRT:
        case ABS:
        case NOT:
        case RET:
            if (!prx_genCode(pNode->o1))
                return 0;
            WRITEM(opr);
            break;
        case ELSE:
        case FI:
            WRITEM(opr);
            break;
        default: break;
    }
    return 1;
}

/* ========================================================================== */

/* Output of all constants */
int numTraverse(char *rec) {
    PRX_NUM *pNum;
    
    pNum = (PRX_NUM *) rec;
    Nums[pNum->ind] = pNum->val;
    return 0;
}

/* ========================================================================== */
int numOut(void) {
    short sh;
    
    Nums = (double *) mem_slot(Tree, nNum * 8);
    bt_traverse(BtNumbers, numTraverse);
    if (!fwrite((char *) Nums, sizeof (double), nNum, oFile)) {
        write_error();
        return 0;
    }
    rewind(oFile);
    WRITEM(nNum);
    WRITEM(nTmp);
    return 1;
}

/* ========================================================================== */
int prx_deriv(PRX_OPD *);
int prx_deriv_expr(PRX_NODE *, PRX_OPD *, PRX_OPD *);
/* ========================================================================== */

/*
 * Derivative generation frame return value: 0 - error 1 - success
 */
int prx_deriv_all(void) {
    int i;
    short sh;
    
    printf("Creating derivatives\n");
    bDeriv = 1;
    WRITEM(SOK);
    for (i = 0; i <= nVar - 1; i++) {
        printf("%s ", (varDefs[i])->name);
        if (!prx_deriv(varDefs[i])) {
            printf("\n");
            return 0;
        }
        WRITEM(EOD);
    }
    printf("\n");
    WRITEM(SOK);
    for (i = 0; i <= nAux - 1; i++) {
        printf("%s ", (auxDefs[i])->name);
        if (!prx_deriv(auxDefs[i])) {
            printf("\n");
            return 0;
        }
        WRITEM(EOD);
    }
    printf("\n");
    WRITEM(SOK);
    for (i = 0; i <= nPar - 1; i++) {
        printf("%s ", (parDefs[i])->name);
        if (!prx_deriv(parDefs[i])) {
            printf("\n");
            return 0;
        }
        WRITEM(EOD);
    }
    printf("\n");
    WRITEM(STOP);
    if (!numOut())
        return 0;
    
    return 1;
}

/* ========================================================================== */

/*
 * Derivative for a specific variable, parameter or auxiliary return value: 0
 * - error 1 - success
 */
int prx_deriv(PRX_OPD * pOpd) {
    TYP typ;
    int i;
    int level; /* if-level */
    PRX_NODE *pElse;
    PRX_NODE * IfNode[MAXLEVEL]; /* pos. of last if-node */
    PRX_NODE * ElseNode[MAXLEVEL]; /* pos. of last else-node */
    
    for (i = 0; i < nTmp; i++)
        TmpTyp[i] = 0;
    /*DTree = mem_tree();*/
    /*
     * 1st pass - simplify expressions and determine the temporaries
     * which need to be computed
     */
    for (pHead = NodeH; *pHead; pHead++) {
        pNode = *pHead;
        if (pNode->opr == IF || pNode->opr == FI || pNode->opr == ELSE) {
            pNode->abl = NULL;
            continue;
        }
        if (pNode->opr != ASS)
            continue;
        if (!prx_deriv_expr(pNode, pOpd, NULL))
            return 0;
        prx_simplify(pNode->abl);
        prx_simplify(pNode->abl);
        if (pNode->c.optr->typ == TMP) {
            if (pNode->abl->o1 != N_0)
                TmpTyp[pNode->c.optr->ind] = 1;
        }
    }
    /*
     * 2nd pass - determining the if-nodes that are necessary for the
     * current derivative variable
     */
    level = 0;
    for (pHead = NodeH; *pHead; pHead++) {
        pNode = *pHead;
        switch (pNode->opr) {
            case IF:
                IfNode[++level] = pNode;
                ElseNode[level] = NULL;
                break;
            case FI:
                if (IfNode[level--]->abl) {
                    pNode->abl = pNode;
                    if (level > 0) {
                        pElse = ElseNode[level];
                        if (pElse)
                            pElse->abl = pElse;
                        IfNode[level]->abl = IfNode[level];
                    }
                }
                break;
            case ELSE:
                ElseNode[level] = pNode;
                break;
            case ASS:
                if (level <= 0)
                    break;
                typ = pNode->c.optr->typ;
                if (typ == TMP)
                    if (!TmpTyp[pNode->c.optr->ind])
                        break;
                pElse = ElseNode[level];
                if (pElse)
                    pElse->abl = pElse;
                IfNode[level]->abl = IfNode[level];
                break;
            default: break;
        }
    }
    /* 3rd pass - output to file */
    for (pHead = NodeH; *pHead; pHead++) {
        pNode = *pHead;
        switch (pNode->opr) {
            case ASS:
                if (!prx_genCode(pNode->abl))
                    return 0;
                break;
            case IF:
            case ELSE:
            case FI: /* case RET: */
                if (pNode->abl)
                    if (!prx_genCode(pNode->abl))
                        return 0;
                break;
            default: break;
        }
    }
    /*mem_free(DTree);*/
    return 1;
}

/* ========================================================================== */
#define NODED(p,op,op1,op2)  \
p = (PRX_NODE *)mem_slot(DTree,sizeof(PRX_NODE));  \
p->opr = op;  p->o1 = op1;  p->c.o2 = op2

/*
 * Derivative for a subexpression return value: 0 - error 1 - success
 */
int prx_deriv_expr(PRX_NODE * p, PRX_OPD * arg, PRX_OPD * fval) {
    PRX_NODE *p1, *p2, *p1a, *p2a, *pD, *pDv, *pDvv;
    OPR opr;
    PRX_OPD *optr;
    
    pD = NULL;
    p2 = NULL;
    opr = p->opr;
    p1 = p->o1;
    if (p1) {
        optr = (opr == ASS) ? p->c.optr : NULL;
        if (!prx_deriv_expr(p1, arg, optr))
            return 0;
        if (opr != ASS) {
            p2 = p->c.o2;
            if (p2)
                if (!prx_deriv_expr(p2, arg, NULL))
                    return 0;
        }
    }
    switch (opr) {
        case AND:
        case OR:
        case NOT:
        case LT:
        case GT:
        case LE:
        case GE:
        case EQ:
        case NE:
            p->abl = N_0;
            break;
        case NEG:
            p1a = p1->abl;
            if (p1a == N_0)
                p->abl = p1a;
            else {
                NODED(pD, NEG, p1a, NULL);
                p->abl = pD;
            }
            break;
        case ADD:
            p1a = p1->abl;
            p2a = p2->abl;
            if (p1a == N_0)
                p->abl = p2a;
            else if (p2a == N_0)
                p->abl = p1a;
            else if (p1a->opr == OPD && p2a->opr == OPD);
            else {
                NODED(pD, ADD, p1a, p2a);
                p->abl = pD;
            }
            break;
        case SUB:
            p1a = p1->abl;
            p2a = p2->abl;
            if (p1a == N_0) {
                if (p2a == N_0)
                    p->abl = N_0;
                else {
                    NODED(pD, NEG, p2a, NULL);
                    p->abl = pD;
                }
            } else if (p2a == N_0)
                p->abl = p1a;
            else {
                NODED(pD, SUB, p1a, p2a);
                p->abl = pD;
            }
            break;
        case MUL:
            p1a = p1->abl;
            p2a = p2->abl;
            if (p1a == N_0) {
                if (p2a == N_0)
                    p->abl = N_0;
                else {
                    if (p2a == N_1)
                        p->abl = p1;
                    else {
                        NODED(pD, MUL, p2a, p1);
                        p->abl = pD;
                    }
                }
            } else if (p2a == N_0) {
                if (p1a == N_1)
                    p->abl = p2;
                else {
                    NODED(pD, MUL, p1a, p2);
                    p->abl = pD;
                }
            } else if (p1 == p2) {
                NODED(pD, MUL, N_2, p1);
                if (p1a != N_1) {
                    pDv = pD;
                    NODED(pD, MUL, p1a, pDv);
                }
                p->abl = pD;
            } else {
                if (p1a == N_1)
                    pDvv = p2;
                else {
                    NODED(pDvv, MUL, p1a, p2);
                }
                if (p2a == N_1)
                    pDv = p1;
                else {
                    NODED(pDv, MUL, p2a, p1);
                }
                NODED(pD, ADD, pDvv, pDv);
                p->abl = pD;
            }
            break;
        case DIV:
            p1a = p1->abl;
            p2a = p2->abl;
            if (p2a == N_0) {
                if (p1a == N_0)
                    p->abl = N_0;
                else {
                    NODED(pD, DIV, p1a, p2);
                    p->abl = pD;
                }
            } else if (fval) {
                if (p2a == N_1)
                    pDvv = fval->node;
                else {
                    NODED(pDvv, MUL, p2a, fval->node);
                }
                if (p1a == N_0) {
                    NODED(pDv, NEG, pDvv, NULL);
                } else {
                    NODED(pDv, SUB, p1a, pDvv);
                }
                NODED(pD, DIV, pDv, p2);
                p->abl = pD;
            } else {
                if (p1a == N_0) {
                    if (p1 == N_1) {
                        NODED(pDv, NEG, p2a, NULL);
                    } else if (p2a == N_1) {
                        NODED(pDv, NEG, p1, NULL);
                    } else {
                        NODED(pDvv, MUL, p2a, p1);
                        NODED(pDv, NEG, pDvv, NULL);
                    }
                } else {
                    if (p1a == N_1)
                        pDvv = p2;
                    else {
                        NODED(pDvv, MUL, p1a, p2);
                    }
                    if (p2a == N_1)
                        pDv = p1;
                    else {
                        NODED(pDv, MUL, p2a, p1);
                    }
                    NODED(pD, SUB, pDvv, pDv);
                    pDv = pD;
                }
                NODED(pDvv, SQR, p2, NULL);
                NODED(pD, DIV, pDv, pDvv);
                p->abl = pD;
            }
            break;
        case POW:
            p1a = p1->abl;
            p2a = p2->abl;
            if (p1a == N_0) {
                if (p2a == N_0)
                    p->abl = N_0;
                else {
                    NODED(pDvv, LOG, p1, NULL);
                    if (p2a == N_1)
                        pDv = pDvv;
                    else {
                        NODED(pDv, MUL, p2a, pDvv);
                    }
                    NODED(pD, MUL, pDv, p);
                    p->abl = pD;
                }
            } else {
                if (p2a == N_0) {
                    NODED(pDvv, SUB, p2, N_1);
                    NODED(pDv, POW, p1, pDvv);
                    NODED(pD, MUL, p2, pDv);
                    if (p1a != N_1) {
                        pDv = pD;
                        NODED(pD, MUL, p1a, pDv);
                    }
                } else {
                    if (p1a == N_1) {
                        NODED(pDv, DIV, p2, p1);
                    } else {
                        NODED(pDvv, DIV, p1a, p1);
                        NODED(pDv, MUL, p2, pDvv);
                    }
                    NODED(pD, LOG, p1, NULL);
                    if (p2a == N_1)
                        pDvv = pD;
                    else {
                        NODED(pDvv, MUL, p2a, pD);
                    }
                    pD = pDv;
                    NODED(pDv, ADD, pDvv, pD);
                    if (fval) {
                        NODED(pD, MUL, pDv, fval->node);
                    } else {
                        NODED(pD, MUL, pDv, p);
                    }
                }
                p->abl = pD;
            }
            break;
        case REV:
            p1a = p1->abl;
            if (p1a == N_0)
                p->abl = N_0;
            else {
                NODED(pDvv, SQR, p1, NULL);
                if (p1a == N_1) {
                    NODED(pDv, REV, pDvv, NULL);
                } else {
                    NODED(pDv, DIV, p1a, pDvv);
                }
                NODED(pD, NEG, pDv, NULL);
                p->abl = pD;
            }
            break;
        case EQU:
            p->abl = p1->abl;
            break;
        case OPD:
            if (p->c.optr->typ == TMP) {
                if (TmpTyp[p->c.optr->ind]) {
                    NODED(pD, DOPD, NULL, (PRX_NODE *) p->c.optr);
                    p->abl = pD;
                } else
                    p->abl = N_0;
            } else if (p->c.optr->typ == RES) {
                NODED(pD, DOPD, NULL, (PRX_NODE *) p->c.optr);
                p->abl = pD;
            } else
                p->abl = (p->c.optr == arg) ? N_1 : N_0;
            break;
        case NUM:
            p->abl = N_0;
            break;
        case ASS:
            NODED(pD, ASS, p1->abl, NULL);
            pD->c.optr = p->c.optr;
            p->abl = pD;
            break;
        case SGN:
            p->abl = N_0;
            break;
        case SIN:
        case COS:
        case TAN:
        case ASIN:
        case ACOS:
        case ATAN:
        case EXP:
        case LOG:
        case LG:
        case SQRT:
        case ABS:
        case SQR:
            p1a = p1->abl;
            if (p1a == N_0) {
                p->abl = N_0;
                break;
            }
            switch (opr) {
                case SIN:
                    NODED(pD, COS, p1, NULL);
                    break;
                case COS:
                    NODED(pD, SIN, p1, NULL);
                    pDv = pD;
                    NODED(pD, NEG, pDv, NULL);
                    break;
                case TAN:
                    pDv = fval ? fval->node : p;
                    NODED(pD, SQR, pDv, NULL);
                    pDv = pD;
                    NODED(pD, ADD, pDv, N_1);
                    break;
                case ASIN:
                case ACOS:
                    NODED(pD, SQR, p1, NULL);
                    pDv = pD;
                    NODED(pD, SUB, N_1, pDv);
                    pDv = pD;
                    NODED(pD, SQRT, pDv, NULL);
                    pDv = pD;
                    NODED(pD, REV, pDv, NULL);
                    if (opr == ASIN)
                        break;
                    pDv = pD;
                    NODED(pD, NEG, pDv, NULL);
                    break;
                case ATAN:
                    NODED(pD, SQR, p1, NULL);
                    pDv = pD;
                    NODED(pD, ADD, pDv, N_1);
                    pDv = pD;
                    NODED(pD, REV, pDv, NULL);
                    break;
                case EXP:
                    if (fval) {
                        NODED(pD, EQU, fval->node, NULL);
                    } else {
                        NODED(pD, EXP, p1, NULL);
                    }
                    break;
                case LOG:
                    NODED(pD, REV, p1, NULL);
                    break;
                case LG:
                    NODED(pD, DIV, N_1_ln10, p1);
                    break;
                case SQRT:
                    if (fval) {
                        NODED(pD, DIV, N_0p5, fval->node);
                    } else {
                        NODED(pD, DIV, N_0p5, p);
                    }
                    break;
                case ABS:
                    NODED(pD, SGN, p1, NULL);
                    break;
                case SQR:
                    NODED(pD, MUL, N_2, p1);
                    break;
                default: break;
            }
            if (p1a != N_1) {
                pDv = pD;
                NODED(pD, MUL, p1a, pDv);
            }
            p->abl = pD;
            break;
        default: break;
    }
    return 1;
}

/* ========================================================================== */

/* Simplification of generated derivatives */
int prx_simplify(PRX_NODE * p) {
    PRX_NODE *p1, *p2, *pD;
    OPR opr;
    double z;
    
    opr = p->opr;
    p1 = p->o1;
    p2 = NULL;
    if (p1) {
        prx_simplify(p1);
        if (p1->opr == EQU)
            p->o1 = p1 = p1->o1;
        if (opr != ASS) {
            p2 = p->c.o2;
            if (p2) {
                prx_simplify(p2);
                if (p2->opr == EQU)
                    p->c.o2 = p2 = p2->o1;
                if (opr == MUL && p2->opr == NUM) {
                    p->o1 = p2;
                    p->c.o2 = p1;
                    p1 = p->o1;
                    p2 = p->c.o2;
                }
            }
        }
    }
    switch (opr) {
        case NEG:
            if (p1->opr == NEG) {
                p->opr = EQU;
                p->o1 = p1->o1;
                p->c.o2 = NULL;
            } else if (p1->opr == SUB) {
                p->opr = SUB;
                p->o1 = p1->c.o2;
                p->c.o2 = p1->o1;
            } else if (p1->opr == MUL) {
                if (p1->o1->opr == NEG) {
                    p->opr = MUL;
                    p->o1 = p1->o1->o1;
                    p->c.o2 = p1->c.o2;
                } else if (p1->c.o2->opr == NEG) {
                    p->opr = MUL;
                    p->o1 = p1->o1;
                    p->c.o2 = p1->c.o2->o1;
                }
            }
            break;
        case ADD:
            if (p2->opr == NEG) {
                p->opr = SUB;
                p->c.o2 = p2->o1;
            } else if (p1->opr == NEG) {
                p->opr = SUB;
                p->o1 = p2;
                p->c.o2 = p1->o1;
            } else if (p1->opr == NUM && p2->opr == NUM) {
                p->opr = EQU;
                p->c.o2 = NULL;
                z = p1->c.nptr->val + p2->c.nptr->val;
                if (fabs(z) == HUGE_VAL)
                    ERROR("addition overflow");
                p->o1 = getNum(z);
            }
            break;
        case SUB:
            if (p2->opr == NEG) {
                p->opr = ADD;
                p->c.o2 = p2->o1;
            }
            if (p1 == p2) {
                p->opr = EQU;
                p->o1 = N_0;
                p->c.o2 = NULL;
            } else if (p1->opr == NUM && p2->opr == NUM) {
                p->opr = EQU;
                p->c.o2 = NULL;
                z = p1->c.nptr->val - p2->c.nptr->val;
                if (fabs(z) == HUGE_VAL)
                    ERROR("subtraction overflow");
                p->o1 = getNum(z);
            } else if (p2->opr == MUL) {
                if (p2->o1->opr == NEG) {
                    p->opr = ADD;
                    p2->o1 = p2->o1->o1;
                } else if (p2->c.o2->opr == NEG) {
                    p->opr = ADD;
                    p2->c.o2 = p2->c.o2->o1;
                }
            }
            break;
        case MUL:
            if (p1 == N_1) {
                p->opr = EQU;
                p->o1 = p2;
                p->c.o2 = NULL;
            } else if (p2 == N_1) {
                p->opr = EQU;
                p->c.o2 = NULL;
            } else if (p1 == N_0 || p2 == N_0) {
                p->opr = EQU;
                p->o1 = N_0;
                p->c.o2 = NULL;
            } else if (p1->opr == NEG && p2->opr == NEG) {
                p->o1 = p1->o1;
                p->c.o2 = p2->o1;
            } else if (p2->opr == NEG) {
                if (p2->o1 == N_1) {
                    p->opr = NEG;
                    p->c.o2 = NULL;
                }
            } else if (p1->opr == NEG) {
                if (p1->o1 == N_1) {
                    p->opr = NEG;
                    p->o1 = p2;
                    p->c.o2 = NULL;
                } else if (bDeriv && p1->o1->opr != NUM) {
                    NODED(pD, MUL, p1->o1, p2);
                    p->opr = NEG;
                    p->o1 = pD;
                    p->c.o2 = NULL;
                }
            } else if (p2->opr == REV) {
                p->opr = DIV;
                p->c.o2 = p2->o1;
            } else if (p1->opr == REV) {
                p->opr = DIV;
                p->o1 = p2;
                p->c.o2 = p1->o1;
            } else if (p1->opr == NUM && p2->opr == NUM) {
                p->opr = EQU;
                p->c.o2 = NULL;
                z = p1->c.nptr->val * p2->c.nptr->val;
                if (fabs(z) == HUGE_VAL)
                    ERROR("multiplication overflow");
                p->o1 = getNum(z);
            }
            break;
        case DIV:
            if (p2 == N_1) {
                p->opr = EQU;
                p->c.o2 = NULL;
            } else if (p1 == N_0) {
                p->opr = EQU;
                p->o1 = N_0;
                p->c.o2 = NULL;
            } else if (p1 == N_1) {
                p->opr = REV;
                p->o1 = p2;
                p->c.o2 = NULL;
            } else if (p2->opr == REV) {
                p->opr = MUL;
                p->c.o2 = p2->o1;
            } else if (p1->opr == NEG && p2->opr == NEG) {
                p->o1 = p1->o1;
                p->c.o2 = p2->o1;
            } else if (p1->opr == NEG) {
                if (bDeriv && p1->o1->opr != NUM) {
                    NODED(pD, DIV, p1->o1, p2);
                    p->opr = NEG;
                    p->o1 = pD;
                    p->c.o2 = NULL;
                }
            } else if (p1->opr == NUM && p2->opr == NUM) {
                p->opr = EQU;
                p->c.o2 = NULL;
                z = p1->c.nptr->val / p2->c.nptr->val;
                if (fabs(z) == HUGE_VAL)
                    ERROR("division overflow");
                p->o1 = getNum(z);
            } else if (p1->opr == OPD && p2->opr == OPD) {
                if (p1->c.optr == p2->c.optr) {
                    p->opr = EQU;
                    p->o1 = N_1;
                    p->c.o2 = NULL;
                }
            }
            break;
        case REV:
            if (p1->opr == REV) {
                p->opr = EQU;
                p->o1 = p1->o1;
                p->c.o2 = NULL;
            } else if (p1->opr == DIV) {
                p->opr = DIV;
                p->o1 = p1->c.o2;
                p->c.o2 = p1->o1;
            } else if (p1->opr == NUM) {
                p->opr = EQU;
                p->c.o2 = NULL;
                p->o1 = getNum(1 / p1->c.nptr->val);
            }
            break;
        case POW:
            if (p2 == N_1) {
                p->opr = EQU;
                p->c.o2 = NULL;
            } else if (p1 == N_0) {
                p->opr = EQU;
                p->o1 = N_0;
                p->c.o2 = NULL;
            } else if (p2 == N_0 || p1 == N_1) {
                p->opr = EQU;
                p->o1 = N_1;
                p->c.o2 = NULL;
            } else if (p2 == N_2) {
                p->opr = SQR;
                p->c.o2 = NULL;
            } else if (p2 == N_0p5) {
                p->opr = SQRT;
                p->c.o2 = NULL;
            }
            break;
        case EXP:
            if (p1->opr == LOG) {
                p->opr = EQU;
                p->o1 = p1->o1;
                p->c.o2 = NULL;
            }
            break;
        case LOG:
            if (p1->opr == EXP) {
                p->opr = EQU;
                p->o1 = p1->o1;
                p->c.o2 = NULL;
            }
            break;
        default: break;
    }
    return 1;
}
