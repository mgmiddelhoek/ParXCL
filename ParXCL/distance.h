/*
 * ParX - distance.h
 * Calculate the Distance between a data point and the model-subspace
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

#ifndef __DISTANCE_H
#define __DISTANCE_H

#include "primtype.h"

extern void new_distance(inum nl,      /* number of Lagrange multipliers */
                         inum nx,      /* number of variables */
                         inum na,      /* number of aux. variables */
                         fnum prec,    /* global relative precision */
                         fnum tol,     /* modes tolerance factor */
                         vector auxtol /* abstol of aux. variables */
);

extern void fre_distance(void);

extern boolean distance(vector dist,     /* distance vector */
                        vector aux,      /* auxilary vector */
                        vector lagrange, /* Lagrange multipliers */
                        matrix jx,       /* Jacobian matrix */
                        matrix ja,       /* Jacobian matrix */
                        inum maxiter,    /* maximum number of iterations */
                        inum trace       /* trace level */
);

#endif
