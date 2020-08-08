/*
 * ParX - golden.h
 * Minimization of a function using Golden Section Search
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

#ifndef __GOLDEN_H
#define __GOLDEN_H

#include "primtype.h"

extern boolean golden(fnum ax,           /* left point */
                      fnum bx,           /* mid point */
                      fnum cx,           /* right point */
                      fnum (*f)(fnum x), /* objective function */
                      fnum rtol,         /* tolerance */
                      inum *itmax,       /* maximum number of iterations */
                      fnum *xmin,        /* minimum point */
                      fnum *fmin         /* minimum value */
);

#endif
