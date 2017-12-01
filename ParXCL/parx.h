/*
 * ParX - parx.h
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

#ifndef __PARX_H
#define __PARX_H

/* system include files */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <limits.h>
#include <float.h>
#include <math.h>
#include <fenv.h>
#include <assert.h>

/* file paths and extensions */

extern char *parx_path; /* ParX home directory */
extern char *model_path; /* Model subdirectory */
extern char *input_path; /* Input subdirectory */

#define VERSION     "Version: 6.5\n"
#define COPYRIGHT   "Copyright (c) 1989-2017 M.G. Middelhoek.\n" \
                    "This program comes with ABSOLUTELY NO WARRANTY.\n" \
                    "This is free software, and you are welcome to redistribute it under certain conditions.\n" \
                    "You should have received a copy of the GNU General Public License along with this program.\n" \
                    "If not, see <https://www.gnu.org/licenses/gpl-3.0>"


#if defined(MSDOS) || defined(WINDOWS) || defined(WIN32) || defined(WIN64)
#define PATH_SEP	"\\"
#else
#define PATH_SEP    "/"
#endif

#define PARX_PATH	""
#define MODEL_PATH	"model"

#define EXT_CHAR                "."
#define MODEL_EXT               ".parx"
#define MODEL_INTERFACE_EXT     ".pxi"
#define MODEL_CODE_EXT          ".pxc"
#define SYSTEM_EXT              ".pxs"
#define DATATABLE_EXT           ".pxd"
#define CSV_EXT                 ".csv"
#define JSON_EXT                ".json"

/* default streams */

extern FILE *yyin, *yyout; /* we are using Lex and Yacc */

#define input_stream	yyin	
#define output_stream	yyout

extern FILE *error_stream; /* stream for error messages */
extern FILE *trace_stream; /* stream for tracing information */

extern char *parx_banner; /* startup text */

#endif
