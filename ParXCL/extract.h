/*
 * ParX - extract.h
 * Extract the Parameter set from Measurements
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

#ifndef __EXTRACT_H
#define __EXTRACT_H

#include "numdat.h"
#include "primtype.h"

extern boolean extract(numblock numb, /* numerical data block */
                       fnum prec,     /* relative precision */
                       fnum tol,      /* modes tolerance factor */
                       opttype opt,   /* type of optimization needed */
                       fnum sens,     /* sensitivity threshold */
                       inum maxiter,  /* maximum number of iterations */
                       inum trace     /* trace flag */
);

#endif
