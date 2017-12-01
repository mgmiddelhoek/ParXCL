/*
 * ParX - extract.c
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

#include "parx.h"
#include "error.h"
#include "modes.h"
#include "objectiv.h"
#include "residual.h"
#include "extract.h"

boolean extract(
                numblock numb, /* numerical data block */
                fnum prec, /* relative precision */
                fnum tol, /* modes tolerance factor */
                opttype opt, /* type of optimization needed */
                fnum sens, /* sensitivity threshold */
                inum maxiter, /* maximum number of iterations */
                inum trace /* trace flag */
) {
    inum neq; /* maximum number of equations */
    inum ng; /* number of equations per point */
    vector pval; /* variable parameter set values */
    vector plow; /* lower bounds */
    vector pup; /* upper bounds */
    boolean b;
    
    /* setup a new objective function */
    
    if (new_objective(numb, prec, tol, &neq, &ng) == FALSE)
        return (FALSE);
    
    /* get initial values and precision of variable parameters */
    
    new_pvar(numb->p, &pval, &plow, &pup);
    
    /* minimize the objective function using the ModeS method */
    
    b = modes(neq, ng, pval, plow, pup, opt, tol, prec, sens, maxiter, trace);
    
    fre_pvar(pval, plow, pup, numb->p); /* get the parameter values */
    
    fre_objective(numb); /* trash the objective function */
    
    return (b);
}
