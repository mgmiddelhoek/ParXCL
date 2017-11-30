/*
 * ParX - newton.c
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

#include "parx.h"
#include "minbrent.h"
#include "vecmat.h"
#include "simulate.h"
#include "newton.h"

/************************ global variables ******************************/

static inum dim = 0; /* number of variables */

static vector x; /* variable vector */
static vector dx; /* change vector */
static vector xn; /* next x vector */
static vector f; /* residual vector */
static vector fn; /* next residual vector */
static vector b; /* -f */
static vector f1, f2; /* f(x-dx), f(x+dx) */
static matrix jac; /* Jacobian matrix, df/dx + df/da */

static inum funceval; /* total number of function evaluations */
static inum jaceval; /* total number of Jacobian evaluations */
static inum mineval; /* function evaluations for local search */

static inum trace; /* trace level */

#define IT_FAC	500L /* factor for maxiter */

/***************************************************************************/

/* numerical calc of derivatives */
static boolean calcjac(vector reltol, vector abstol, matrix jac);

/* local minimisation function */
static fnum optstep(fnum releps, fnum abseps, fnum fcurr, fnum ffull, inum itmax);

static fnum fvec_norm(fnum a); /* calculate |f| at x + a*dx */

/***************************************************************************/

void new_newton(inum nx) {
    dim = nx; /* set new dimensions */
    
    /* allocate global arrays */
    
    xn = rnew_vector(dim);
    dx = rnew_vector(dim);
    f = rnew_vector(dim);
    fn = rnew_vector(dim);
    f1 = rnew_vector(dim);
    f2 = rnew_vector(dim);
    b = rnew_vector(dim);
    jac = rnew_matrix(dim, dim);
    
    new_vecmat(dim, dim); /* setup vecmat library */
}

void fre_newton(void) {
    /* free all allocated memory */
    
    rfre_vector(xn);
    rfre_vector(dx);
    rfre_vector(f);
    rfre_vector(fn);
    rfre_vector(f1);
    rfre_vector(f2);
    rfre_vector(b);
    rfre_matrix(jac);
    
    fre_vecmat();
}

/* modified Newton-Raphson optimizer */

inum newton_raphson(
                    vector xl, /* variable vector */
                    vector reltol, /* relative tolerances */
                    vector abstol, /* absolute tolerance */
                    inum maxiter, /* maximum number of iterations */
                    inum tr /* trace level */
) {
    fnum machinep; /* machine precision */
    fnum xtol, ftol; /* tolerance on x and f */
    boolean xconv; /* has x converged ? */
    boolean fconv; /* has |f| converged ? */
    fnum fnorm, fnnorm; /* f norms */
    boolean rf; /* function return flag */
    boolean ffi, ffo; /* function eval. flags */
    boolean jfi, jfo; /* Jacobian eval. flags */
    fnum releps, abseps; /* tolerance for local minimization */
    fnum alpha; /* relative step size from local min. */
    inum fullstep; /* number of full Newton steps taken */
    inum partstep; /* number of partial (local search) steps */
    inum maxmin; /* maximum number of local search steps */
    inum iter; /* number of iterations */
    inum done; /* are we finished ? */
    inum i;
    TMPRINTSTATE *pst;
    
    trace = tr;
    
    if (trace >= 1)
        fprintf(trace_stream, "Newton-Raphson:\n");
    
    /* setup global variables */
    
    x = xl;
    funceval = 0;
    jaceval = 0;
    
    /* tolerance for fnorm, approx. precision of transcendentals */
    
    machinep = FNUM_EPS; /* machine precision */
    ftol = 100.0 * sqrt((fnum) dim * machinep);
    
    /* settings for local minimization */
    
    abseps = 100.0 * machinep; /* != 0.0 */
    releps = sqrt(machinep); /* the best we can do */
    maxmin = 20; /* this should gain about 5 digits */
    mineval = 0;
    
    /* start the search for the root */
    
    maxiter = (maxiter == 0L) ? IT_FAC * dim : maxiter;
    iter = fullstep = partstep = 0;
    
    ffi = TRUE; /* start by calculating f vector */
    jfi = TRUE; /* start by calculating Jacobian matrix */
    fnorm = 1.0;
    for (;;) {
        
        if (iter++ > maxiter) {
            done = -2;
            break;
        }
        
        /* calculate the residual vector and/or the Jacobian matrix */
        
        ffo = ffi;
        jfo = jfi;
        rf = sim_constraints(x, &ffo, f, &jfo, jac, trace - 1);
        
        if ((rf == FALSE) || (ffo != ffi)) {
            
            if (trace >= 1)
                fprintf(trace_stream, "evaluation error in model\n");
            
            done = -1;
            break;
            
        } else funceval++;
        
        if (jfo != jfi) { /* function is unable to supply Jacobian */
            if (calcjac(reltol, abstol, jac) == FALSE) {
                done = -1;
                break;
            }
        } else jaceval++;
        
        if (ffi == TRUE)
            fnorm = norm_vector(f);
        
        /* calculate dx from jac * dx = - f */
        
        for (i = 0; i < dim; i++)
            VEC(b, i) = -VEC(f, i);
        
        if (crout(jac, dx, b) == FALSE) {
            done = -3;
            break;
        }
        
        /* test x for convergence, full step dx must be small */
        
        for (i = 0, xconv = TRUE; i < dim; i++) {
            xtol = VEC(reltol, i) * fabs(VEC(x, i)) + fabs(VEC(abstol, i));
            xconv = (fabs(VEC(dx, i)) < xtol) ? xconv : FALSE;
        }
        
        /* test f for convergence */
        
        fconv = (fnorm < ftol) ? TRUE : FALSE;
        
        if ((xconv == TRUE) && (fconv == TRUE)) {
            done = 0; /* true solution found */
            break;
        }
        
        /* try to make a full step in the direction of dx */
        
        fnnorm = fvec_norm(1.0);
        
        /* if norm is not reduced try smaller step */
        
        if (fnnorm >= fnorm) {
            
            partstep++;
            
            alpha = optstep(releps, abseps, fnorm, fnnorm, maxmin);
            
            if (alpha == 0.0) {
                done = (xconv == TRUE) ? 1 : -1;
                break;
            }
            for (i = 0; i < dim; i++)
                VEC(xn, i) = VEC(x, i) + alpha * VEC(dx, i);
            
            ffi = TRUE;
            
        } else {
            
            fullstep++;
            
            ffi = FALSE; /* no need to calculate f */
            
            for (i = 0; i < dim; i++)
                VEC(f, i) = VEC(fn, i);
            
            fnorm = fnnorm;
        }
        
        /* better point has been located, so step to it */
        
        for (i = 0; i < dim; i++)
            VEC(x, i) = VEC(xn, i);
        
        if (trace >= 2) {
            fputs("step to x:\n", trace_stream);
            pst = tm_setprint(trace_stream, 1, 80, 8, 0);
            print_vector(pst, x);
            tm_endprint(pst);
        }
        
    } /* DONE */
    
    /* copy data to output parameters if solved */
    
    if (done >= 0)
        for (i = 0; i < dim; i++) {
            VEC(abstol, i) = fabs(VEC(dx, i));
            VEC(reltol, i) = fnorm;
        }
    
    if (trace >= 1) {
        fprintf(trace_stream, "iteration: %ld, full step: %ld, partial step: %ld\n",
                (long) iter, (long) fullstep, (long) partstep);
        fprintf(trace_stream,
                "total f eval: %ld, j eval: %ld, ls f eval: %ld\n",
                (long) funceval, (long) jaceval, (long) mineval);
    }
    
    /* return 0 when valid, < 0 when failed, > 0 when maybe valid */
    
    return (done);
}

/* local search, find optimal step size in Newton direction, minimize norm f */

fnum optstep(fnum releps, fnum abseps, fnum fcurr, fnum ffull,
             inum itmax) {
    fnum xl, xc, xr, fl, fc, fr, xmin, fmin;
    fnum fvec_norm(fnum a);
    boolean found;
    
    xl = 0.0; /* left point */
    xr = 1.0; /* right point */
    xc = 1.0; /* center point */
    
    fl = fcurr; /* current minimum */
    fr = ffull; /* full newton step */
    fc = fr;
    
    xmin = xl;
    
    /* start local line optimization */
    
    found = FALSE; /* no solution yet */
    
    if (trace >= 3)
        fprintf(trace_stream, "starting directional search:\n");
    
    /* first absorb overshoot in large steps */
    
    while ((xr >= abseps) && (fc > fl)) {
        xr = xc;
        fr = fc;
        xc *= 1.0e-1;
        fc = fvec_norm(xc);
    }
    
    if ((fr == INF) && (fc < fl)) { /* unable to bracket minimum */
        xmin = xc; /* this is at least an improvement */
        found = TRUE;
    }
    
    if ((found == FALSE) && (xr >= abseps)) {
        
        /* minimum is bracketed */
        
        fmin = fc;
        xmin = xc;
        
        found = brent(xl, xc, xr, fvec_norm, releps, abseps,
                      &itmax, &xmin, &fmin);
        
        mineval += itmax;
        
        if ((found == FALSE) && (trace >= 3))
            fprintf(trace_stream, "local opt. count reached : %ld\n", (long) itmax);
        
        if (fmin > fl)
            xmin = xc; /* better then nothing */
        
        found = TRUE;
    }
    
    if (found == FALSE) xmin = 0.0;
    
    if (trace >= 3)
        fprintf(trace_stream,
                "end directional search, step size : %14.6e\n", xmin);
    
    return (xmin);
}

/* cost function for line search, used by Brent */

fnum fvec_norm(fnum a) {
    inum i;
    boolean ff, jf, rf;
    fnum norm;
    
    if (trace >= 4)
        fprintf(trace_stream, "trying step size : %14.6e\n", a);
    
    for (i = 0; i < dim; i++)
        VEC(xn, i) = VEC(x, i) + a * VEC(dx, i);
    
    ff = TRUE; /* calculate residual */
    jf = FALSE; /* don't calculate Jacobian */
    funceval++;
    
    rf = sim_constraints(xn, &ff, fn, &jf, jac, trace - 4);
    
    if ((rf == FALSE) || (ff == FALSE)) return (INF);
    
    norm = norm_vector(fn);
    
    if (trace >= 4)
        fprintf(trace_stream, "resulting |f| : %14.6e\n", norm);
    
    return (norm);
}

/* fill Jacobian matrix */

boolean calcjac(vector reltol, vector abstol, matrix jac) {
    fnum delta;
    boolean rf, ff, jf;
    inum c, r;
    
    if (trace >= 5)
        fprintf(trace_stream, "calc Jac:\n");
    
    for (r = 0; r < dim; r++) /* copy x to xn */
        VEC(xn, r) = VEC(x, r);
    
    for (c = 0; c < dim; c++) {
        
        /* step size is 0.1 x the tolerance */
        
        delta = 1.0e-1 * (VEC(reltol, c) * fabs(VEC(x, c)) + VEC(abstol, c));
        
        /* right function value */
        
        VEC(xn, c) = VEC(x, c) + delta;
        
        ff = TRUE;
        funceval++;
        jf = FALSE;
        
        rf = sim_constraints(xn, &ff, f1, &jf, jac, trace - 5);
        
        if ((rf == FALSE) || (ff == FALSE))
            return (FALSE);
        
        /* left function value */
        
        VEC(xn, c) = VEC(x, c) - delta;
        
        ff = TRUE;
        funceval++;
        jf = FALSE;
        
        rf = sim_constraints(xn, &ff, f2, &jf, jac, trace - 5);
        
        if ((rf == FALSE) || (ff == FALSE)) return (FALSE);
        
        /* central differences */
        
        for (r = 0; r < dim; r++)
            MAT(jac, r, c) = (VEC(f1, r) - VEC(f2, r)) / (2.0 * delta);
        
        VEC(xn, c) = VEC(x, c); /* reset xn */
    }
    
    return (TRUE);
}
