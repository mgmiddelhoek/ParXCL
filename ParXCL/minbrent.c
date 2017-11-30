/*
 * ParX - minbrent.c
 * Line Minimization of a function using Brent's method of Parabolic Interpolation
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
#include "minbrent.h"

#define CGOLD 0.3819660

boolean brent(
              fnum ax, /* left point */
              fnum bx, /* mid point */
              fnum cx, /* right point */
              fnum(*obj)(fnum x), /* objective function */
              fnum r_tol, /* relative tolerance */
              fnum a_tol, /* absolute tolerance */
              inum *itmax, /* maximum number of iterations */
              fnum *xmin, /* minimum point */
              fnum *fmin /* minimum value */
) {
    inum iter;
    fnum a, b, d, etemp, fu, fv, fw, fx, p, q, r;
    fnum tol1, tol2, u, v, w, x, xm;
    fnum e = 0.0;
    
    a = ((ax < cx) ? ax : cx);
    b = ((ax > cx) ? ax : cx);
    d = 0.0;
    x = w = v = bx;
    fw = fv = fx = *fmin;
    for (iter = 1; iter <= (*itmax); iter++) {
        xm = 0.5 * (a + b);
        tol2 = 2.0 * (tol1 = r_tol * fabs(x) + a_tol);
        if (fabs(x - xm) <= (tol2 - 0.5 * (b - a))) {
            *xmin = x;
            *fmin = fx;
            *itmax = iter;
            return (TRUE);
        }
        if (fabs(e) > tol1) {
            r = (x - w) * (fx - fv);
            q = (x - v) * (fx - fw);
            p = (x - v) * q - (x - w) * r;
            q = 2.0 * (q - r);
            if (q > 0.0) p = -p;
            q = fabs(q);
            etemp = e;
            e = d;
            if ((fabs(p) >= fabs(0.5 * q * etemp)) ||
                (p <= q * (a - x)) || (p >= q * (b - x)))
                d = CGOLD * (e = (x >= xm ? a - x : b - x));
            else {
                d = p / q;
                u = x + d;
                if ((u - a < tol2) || (b - u < tol2))
                    d = (xm - 1) > 0.0 ? fabs(tol1) : -fabs(tol1);
            }
        } else {
            d = CGOLD * (e = (x >= xm ? a - x : b - x));
        }
        u = (fabs(d) >= tol1 ? x + d :
             x + (d > 0.0 ? fabs(tol1) : -fabs(tol1)));
        fu = (*obj)(u);
        if (fu <= fx) {
            if (u >= x) a = x;
            else b = x;
            v = w;
            w = x;
            x = u;
            fv = fw;
            fw = fx;
            fx = fu;
        } else {
            if (u < x) a = u;
            else b = u;
            if ((fu <= fw) || (w == x)) {
                v = w;
                w = u;
                fv = fw;
                fw = fu;
            } else if ((fu <= fv) || (v == x) || (v == w)) {
                v = u;
                fv = fu;
            }
        }
    }
    
    *xmin = x;
    *fmin = fx;
    *itmax = iter;
    
    return (FALSE); /* to many iterations */
}
