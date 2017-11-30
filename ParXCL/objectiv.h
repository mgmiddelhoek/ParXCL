/*
 * ParX - objectiv.h
 * Objective function for MODES
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

#ifndef __OBJECTIV_H
#define __OBJECTIV_H

#include "primtype.h"
#include "numdat.h"

extern boolean objective(
                         vector p, /* parameter values */
                         boolean rf, /* residual flag */
                         vector *rp, /* pointer to residual vector */
                         boolean jf, /* Jacobian flag */
                         matrix *jpp, /* pointer to Jacobian matrix */
                         boolean modify, /* allow modify point set */
                         boolean all, /* evaluate objective for all points */
                         inum *npoints, /* remaining number of data points */
                         inum *mc_r, /* total number of model residual evaluations */
                         inum *mc_jx, /* total number of model Jx evaluations */
                         inum *mc_jp, /* total number of model Jp evaluations */
                         inum trace /* trace level */
);

extern boolean new_objective(
                             numblock numb, /* numerical data block */
                             fnum prec, /* relative precision */
                             fnum tol, /* modes tolerance factor */
                             inum *maxeq, /* maximum number of equations */
                             inum *ngroup /* number of equations per point */
);

extern void fre_objective(numblock numb);

extern inum remove_data_point(
                              inum n, /* index of point to be moved */
                              inum g, /* target group */
                              inum trace /* trace level */
);


#endif
