/*
 * ParX - prxcompile.c
 * Model Compiler
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
#include "primtype.h"
#include "prx_def.h"

#define FILENN 1024
char prx_filename[FILENN]; /* name model file */
int prx_lineno;            /* line number model file */

int prx_compile(tmstring mdlFileName) {
    FILE *inFile;
    FILE *codeFile;
    FILE *modFile;
    char fileName[FILENN];
    char *pe;
    int bError;

    /* opening 1 input file and 2 output files */

    printf("ParX - Model Code generator\n");

    if (!mdlFileName) {
        printf("No model file specified\n");
        return (1);
    }
    strncpy(fileName, mdlFileName, FILENN);
    if (fileName[FILENN - 5] != '\0') { /* leave room for extension */
        printf("Filename too long\n");
        return (1);
    }
    pe = strrchr(fileName, '.');
    if (!pe || strcmp(pe, MODEL_EXT)) { /* no extension, add one */
        strcat(fileName, MODEL_EXT);
    }

    strcpy(prx_filename, fileName); /* copy input filename to global */

    inFile = fopen(fileName, "r");
    if (!inFile) {
        printf("Error opening model file '%s'\n", fileName);
        return (1);
    } else {
        printf("   Reading '%s'\n", fileName);
    }

    pe = strrchr(fileName, '.');
    strcpy(pe, MODEL_CODE_EXT);
    codeFile = fopen(fileName, "w");
    if (!codeFile) {
        printf("Error opening code file for writing '%s'\n", fileName);
        fclose(inFile);
        return (1);
    } else {
        printf("   Writing code file '%s'\n", fileName);
    }

    strcpy(pe, MODEL_INTERFACE_EXT);
    modFile = fopen(fileName, "w");
    if (!modFile) {
        printf("Error opening definition file for writing '%s'\n", fileName);
        fclose(inFile);
        fclose(codeFile);
        return (1);
    } else {
        printf("   Writing definition file '%s'\n\n", fileName);
    }

    /* initialization of variables and fields */
    if (!prx_init(codeFile, modFile)) {
        goto error;
    }

    /* parse the model file */
    if (!prx_parse(inFile)) {
        goto error;
    }

    bError = prx_error();
    if (bError & 2) {
        goto error;
    }

    if (!prx_check()) {
        goto error;
    }

    if (!prx_deriv_all()) {
        goto error;
    }

    bError = prx_error();
    if (bError & 2) {
        goto error;
    }
    if (bError & 1) {
        printf("Warning(s) during creation of model code\n");
    }

    prx_exit();

    /* closing files */
    fclose(inFile);
    fclose(codeFile);
    fclose(modFile);
    printf("Creation of model code successfully completed\n");
    return 0;

    /* abnormal end of program execution */
error:
    prx_exit();
    fclose(inFile);
    fclose(codeFile);
    fclose(modFile);
    printf("Error(s) during creation of model code\n");
    return 1;
}

/* parse the model file: 0 = error, 1 = okay */
int prx_parse(FILE *inFile) {
    char Buf[MAXLINE + 4];
    char Cmd[4096];
    char *pe, *pa;
    int bComment;     /* comment switch */
    int bLineComment; /* line comment switch */
    int bString;      /* string switch */
    int bCont;        /* continuation line switch */

    /* parsing header part of PARX model description file */
    bComment = bLineComment = bString = bCont = prx_lineno = 0;
    pe = Buf;
    *pe = '\n';
    pa = Cmd;
    while (1) {
        if (*pe == '\n') {
            bLineComment = 0;
            if (!bComment && !bCont) {
                if (pa != Cmd) {
                    *pa = 0;
                    if (prx_header(Cmd) == 2) {
                        break;
                    }
                    pa = Cmd;
                }
            }
            if (bCont) {
                pa--;
            }
            if (!fgets(Buf, MAXLINE, inFile)) {
                break;
            }
            prx_lineno++;
            pe = Buf;
            bString = 0;
            bCont = 0;
            continue;
        }
        if (bComment) {
            if (*pe == '*' && *(pe + 1) == '/') {
                bComment = 0;
                pe++;
            }
            pe++;
            continue;
        }
        if (bLineComment) {
            pe++;
            continue;
        }
        if (*pe == '"') {
            bString = !bString;
            pe++;
            bCont = 0;
            continue;
        }
        if (bString) {
            *(pa++) = *(pe++);
            continue;
        }
        if (*pe == ' ' || *pe == '\t' || *pe == '\r') {
            pe++;
            continue;
        }
        bCont = 0;
        if (*pe == '/' && *(pe + 1) == '*') {
            bComment = 1;
            pe += 2;
            continue;
        }
        if (*pe == '|' && *(pe + 1) == '|') {
            bLineComment = 1;
            pe += 2;
            continue;
        }
        if (*pe == '\\') {
            *(pa++) = '\\';
            pe++;
            bCont = 1;
            continue;
        }
        if (*pe == ';') {
            if (pa != Cmd) {
                *pa = 0;
                if (prx_header(Cmd) == 2)
                    break;
                pa = Cmd;
            }
            pe++;
            continue;
        }
        *(pa++) = *(pe++);
    }

    if (prx_error() & 2) {
        return 0;
    }
    if (!prx_derivLists()) {
        return 0;
    }

    /* parsing equation part of PARX model description file */
    bComment = bLineComment = bCont = 0;
    pe = Buf;
    *pe = '\n';
    pa = Cmd;
    while (1) {
        if (*pe == '\n') {
            bLineComment = 0;
            if (!bComment && !bCont) {
                *pa = 0;
                if (!prx_equation(Cmd)) { /* break at first error */
                    return 0;
                }
                pa = Cmd;
            }
            if (bCont) {
                pa--;
            }
            if (!fgets(Buf, MAXLINE, inFile)) {
                break;
            }
            prx_lineno++;
            pe = Buf;
            bCont = 0;
            continue;
        }
        if (bComment) {
            if (*pe == '*' && *(pe + 1) == '/') {
                bComment = 0;
                pe++;
            }
            pe++;
            continue;
        }
        if (bLineComment) {
            pe++;
            continue;
        }
        if (*pe == ' ' || *pe == '\t' || *pe == '\r') {
            pe++;
            continue;
        }
        bCont = 0;
        if (*pe == '/' && *(pe + 1) == '*') {
            bComment = 1;
            pe += 2;
            continue;
        }
        if (*pe == '|' && *(pe + 1) == '|') {
            bLineComment = 1;
            pe += 2;
            continue;
        }
        if (*pe == '\\') {
            *(pa++) = '\\';
            pe++;
            bCont = 1;
            continue;
        }
        if (*pe == ';') {
            *pa = 0;
            if (!prx_equation(Cmd)) {
                return 0;
            }
            pa = Cmd;
            pe++;
            continue;
        }
        *(pa++) = *(pe++);
    }
    return 1;
}
