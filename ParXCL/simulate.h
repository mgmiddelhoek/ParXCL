/*
 * ParX - simulate.h
 * Simulate a System
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

#ifndef __SIMULATE_H
#define __SIMULATE_H

#include "primtype.h"
#include "datastruct.h"
#include "numdat.h"

extern boolean simulate(numblock numb, fnum tol, inum maxiter, inum trace);

extern boolean sim_constraints(
                               vector x, /* variable vector */
                               boolean *rf, /* want residuals? */
                               vector r, /* residual vector */
                               boolean *jxf, /* want Jacobian? */
                               matrix jx, /* Jacobian matrix */
                               inum trace /* trace level */
);

#endif
