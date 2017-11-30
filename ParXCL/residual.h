/*
 * ParX - residual.h
 * Calculate the Residuals for the Objective function
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

#ifndef __RESIDUAL_H
#define __RESIDUAL_H

#include "primtype.h"
#include "numdat.h"

extern boolean residual(
                        xset xs, /* set of measurements */
                        vector pv, /* variable parameters */
                        boolean rf, /* residual request flag */
                        vector r, /* residual vector */
                        boolean jpf, /* Jacobian request flag */
                        matrix jp, /* Jacobian matrix */
                        boolean sf, /* scaling matrix request flag */
                        matrix s, /* scaling matrix */
                        inum *mc_r, /* number of model residual evaluations */
                        inum *mc_jx, /* number of model Jacobian_x evaluations */
                        inum *mc_jp, /* number of model Jacobian_p evaluations */
                        inum trace /* trace level */
);


extern boolean new_residual(
                            modres mr, /* model result structure */
                            moddat mi, /* model interface structure */
                            fnum prec, /* relative precision */
                            fnum tol, /* modes tolerance factor */
                            vector atol, /* abstol of aux. variables */
                            inum *nup /* number of unknown parameters */
);

extern void fre_residual(void);

extern void new_pvar(
                     pset ps, /* parameter set with upper and lower bounds */
                     vector *pval, /* scaled values */
                     vector *plow, /* scaled lower bounds */
                     vector *pup /* scaled upper bounds */
);

extern void fre_pvar(
                     vector pval, /* scaled parameter values */
                     vector plow, /* scaled lower bounds */
                     vector pup, /* scaled upper bounds */
                     pset ps /* output parameter set */
);

extern void set_p_scale(
                        vector val, /* parameter values */
                        vector lb, /* lower bounds */
                        vector ub, /* upper bounds */
                        matrix jacp /* Jacobian_p matrix */
);

extern void unscale_p(
                      vector ps, /* scaled p */
                      vector p /* un-scaled p */
);

extern boolean ext_constraints(
                               vector dxv, /* difference variable x */
                               vector auxv, /* auxiliary variable a */
                               boolean rf, /* request flag */
                               vector rv, /* model equation residual */
                               boolean jxf, /* request flag */
                               matrix jxv, /* Jacobian x */
                               matrix jav, /* Jacobian a */
                               inum trace /* trace level */
);

#endif
