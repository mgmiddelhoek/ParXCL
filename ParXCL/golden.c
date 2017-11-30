/*
 * ParX - golden.c
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
 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "parx.h"
#include "golden.h"

#define R 0.61803399
#define C (1.0-R)

boolean golden(
               fnum ax, /* left point */
               fnum bx, /* mid point */
               fnum cx, /* right point */
               fnum(*f)(fnum x), /* objective function */
               fnum rtol, /* tolerance */
               inum *itmax, /* maximum number of iterations */
               fnum *xmin, /* minimum point */
               fnum *fmin /* minimum value */
) {
    fnum x0, x1, x2, x3;
    fnum f1, f2;
    inum iter;
    
    x0 = ax;
    x3 = cx;
    
    if (fabs(cx - bx) > fabs(bx - ax)) {
        x1 = bx;
        f1 = *fmin;
        x2 = bx + C * (cx - bx);
        f2 = (*f)(x2);
    } else {
        x2 = bx;
        f2 = *fmin;
        x1 = bx - C * (bx - ax);
        f1 = (*f)(x1);
    }
    
    iter = 0;
    
    while ((fabs(x3 - x0) > rtol * (fabs(x1) + fabs(x2))) &&
           (iter <= (*itmax))) {
        
        if (f2 < f1) {
            x0 = x1;
            x1 = x2;
            x2 = R * x1 + C * x3;
            f1 = f2;
            f2 = (*f)(x2);
        } else {
            x3 = x2;
            x2 = x1;
            x1 = R * x2 + C * x0;
            f2 = f1;
            f1 = (*f)(x1);
        }
        iter++;
    }
    
    if (f1 < f2) {
        *xmin = x1;
        *fmin = f1;
    } else {
        *xmin = x2;
        *fmin = f2;
    }
    
    if (iter > (*itmax))
        return (FALSE); /* to many iterations */
    
    *itmax = iter;
    
    return (TRUE);
}
