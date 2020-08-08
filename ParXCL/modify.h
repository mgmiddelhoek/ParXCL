/*
 * ParX - modify.h
 * Modify the number of data points and test for proximity
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

#ifndef __MODIFY_H
#define __MODIFY_H

#include "primtype.h"

extern boolean proximity(vector res,   /* residuals */
                         vector s_val, /* singular values */
                         inum ng,      /* number of equations per point */
                         inum rank,    /* dimension of the parameter space */
                         opttype opt,  /* optimization type */
                         fnum tol,     /* required tolerance */
                         fnum *pmc, /* previous value of maximum consistancy */
                         inum trace /* trace level */
);

extern boolean
modify_point_set(vector res,     /* residual vector */
                 inum ng,        /* number of equations per point */
                 vector sv,      /* S, singular values */
                 matrix pt,      /* Pt, right hand singular vectors */
                 matrix q,       /* Q, left hand singular vectors */
                 inum rank,      /* rank of the Jacobian matrix */
                 vector dp,      /* corrected step direction */
                 fnum *dc,       /* correction on objective function */
                 fnum *res_norm, /* corrected norm of the residual vector */
                 inum *npoints,  /* number of data points */
                 vector wrkp,    /* workspace, same dimension as dp */
                 vector wrkv,    /* workspace, same dimension as res */
                 matrix wrkm,    /* workspace, same dimensions as q */
                 inum trace      /* trace level */
);

#endif
