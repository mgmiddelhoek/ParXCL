/*
 * ParX - error.h
 * Error Codes
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

#ifndef __ERROR_H
#define __ERROR_H

#include "primtype.h"

extern char error_mesg[];
extern inum errcode;
extern void error(tmstring s);

extern inum prx_errcode; /* error code of interpreter */


/* user defined error codes are negative to avoid conflicts with sys errors */

#define UNKNOWN_ERROR	-1

/* parser error codes */

#define PERR			UNKNOWN_ERROR

#define SYNTAX_PERR		(PERR-1)
#define UNK_IDENT_PERR	(PERR-2)
#define UNK_FIELD_PERR	(PERR-3)
#define UNK_MODEL_PERR	(PERR-4)
#define ILL_MNAME_PERR	(PERR-5)
#define ILL_REDEC_PERR	(PERR-6)
#define ILL_SPEC_PERR	(PERR-7)
#define ILL_TYPE_PERR	(PERR-8)
#define ILL_ASSIGN_PERR	(PERR-9)
#define ILL_NEGVAL_PERR	(PERR-10)
#define WRONG_ARG_PERR	(PERR-11)
#define NO_FILE_PERR	(PERR-12)
#define NEST_PERR		(PERR-13)
#define TMERROR_PERR	(PERR-14)

/* setup error codes */

#define SERR 			(PERR-14)

#define ILL_SETUP_SERR	(SERR-1)
#define MIS_SETUP_SERR	(SERR-2)
#define NO_DATA_SERR	(SERR-3)
#define NO_KEY_SERR		(SERR-4)
#define NO_VAR_SERR		(SERR-5)
#define NO_PAR_SERR		(SERR-6)
#define UNKN_VAR_SERR	(SERR-7)

/* calculation error codes */

#define CERR			(SERR-7)

#define UNKNOWN_CERR	(CERR-1)
#define EXEPT_MODEL_CERR (CERR-2)
#define SLOW_CONV_CERR	(CERR-3)
#define NO_DIREC_CERR	(CERR-4)
#define NO_LOWP_CERR	(CERR-5)
#define OBJ_FAIL_CERR	(CERR-6)
#define NUMEQ_CERR		(CERR-7)
#define MODIFY_CERR		(CERR-8)

/* model interpreter error codes */

#define IERR			(CERR-8)

#define MEM_IERR		(IERR-1)
#define VER_IERR		(IERR-2)
#define COD_IERR		(IERR-3)
#define EOF_IERR		(IERR-4)
#define CON_IERR		(IERR-5)
#define NPX_IERR		(IERR-6)
#define STK_IERR		(IERR-7)

#define DERR			(IERR-7)

/* security error codes */

#define DNG_DERR		(DERR-1)


#endif
