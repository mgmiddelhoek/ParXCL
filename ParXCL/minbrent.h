/*
 * ParX - minbrent.c
 * Line Minimization of a function using Brent's method of Parabolic
 * Interpolation
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

#ifndef __MINBRENT_H
#define __MINBRENT_H

#include "primtype.h"

extern boolean brent(fnum ax,             /* left point */
                     fnum bx,             /* mid point */
                     fnum cx,             /* right point */
                     fnum (*obj)(fnum x), /* objective function */
                     fnum r_tol,          /* relative tolerance */
                     fnum a_tol,          /* absolute tolerance */
                     inum *itmax,         /* maximum number of iterations */
                     fnum *xmin,          /* mimimum point */
                     fnum *fmin           /* minimum value */
);

#endif
