/*
 * ParX - parxmods.c
 * Model Library Index Table for compiled models
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

/************************************************************************
        PARX MODEL LIBRARY INDEX TABLE
 ************************************************************************/

#include "parxmods.h"

/****************** DO NOT TOUCH ABOVE THIS LINE ************************/

/************************************************************************
        Model Library Identification String
 ************************************************************************/

char *modlib_version = "no compiled models";

/************************************************************************
        External Model Function Declarations
 ************************************************************************/

/*
extern boolean mod_linear(modreq req, modres res);
 */

/************************************************************************
        Global Model Library Configuration Table, the entries are:
        1) full modelname (string),	
        2) pointer to function returning the model specification 
 ************************************************************************/

ModelLibrary modlib = {{"default", NULL}};
/* 
 {{"BiM_Linear", mod_linear}}
 */

/****************** DO NOT TOUCH BELOW THIS LINE ************************/

/* size of the model table */
inum modlib_size = 0; /* sizeof (modlib) / sizeof (*modlib); */

/************************* END OF FILE *********************************/
