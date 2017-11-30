/*
 * ParX - newton.h
 * Locate the solution of a set of nonlinear equations by modified Newton-Raphson
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

#ifndef __NEWTON_H
#define __NEWTON_H

#include "primtype.h"
#include "datastruct.h"

extern void new_newton(inum nx);
extern void fre_newton(void);

extern inum newton_raphson(
                           vector xl, /* variable vector */
                           vector reltol, /* relative tolerances */
                           vector abstol, /* absolute tolerance */
                           inum maxiter, /* maximum number of iterations */
                           inum tr /* trace level */
);

#endif
