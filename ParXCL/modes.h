/*
 * ParX - modes.h
 * MODES Parameter Estimator
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

#ifndef __MODES_H
#define __MODES_H

#include "primtype.h"

extern boolean modes(
                     inum neq, /* maximum number of equations */
                     inum ng, /* number of equations per point */
                     vector pval, /* initial and final scaled parameter values */
                     vector plow, /* lower bounds on the parameter values */
                     vector pup, /* upper bounds on the parameter values */
                     opttype opt, /* type of optimization needed */
                     fnum tol, /* modes tolerance factor */
                     fnum prec, /* relative precision */
                     fnum sens, /* sensitivity threshold */
                     inum maxiter, /* maximum number of iterations */
                     inum trace /* trace level */
);

#endif
