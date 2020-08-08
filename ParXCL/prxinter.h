/*
 * ParX - prxinter.h
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

#ifndef __PRXINTER_H
#define __PRXINTER_H

#include "modlib.h"
#include "primtype.h"
#include <assert.h>

/* Input and adaptation of interpreter code (once per ParX execution) */
extern boolean prx_inCode(moddat dat, FILE *inFile);

/* Execution of interpreter code */
extern boolean prx_compute(moddat dat);

#endif
