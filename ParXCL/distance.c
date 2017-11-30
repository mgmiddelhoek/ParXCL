/*
 * ParX - distance.c
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
 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "parx.h"
#include "vecmat.h"
#include "golden.h"
#include "residual.h"
#include "distance.h"

/************************ global variables *****************************/

static vector c_res; /* constraint residual */

static vector d_dist; /* differential distance vector */
static vector ddt; /* d_distance tangent */
static vector ddn; /* d_distance normal */
static vector n_dist; /* next distance vector */

static vector d_aux; /* differential aux. vector */
static vector n_aux; /* next aux. vector */

static vector lambda; /* new estimate of Lagrange vector */
static vector d_lambda; /* differential Lagrange vector */

static matrix jjt; /* design matrix Jx.Jxt */
static matrix jdf; /* right hand side matrix Jx.d - f, -f, Jx.d */

static fnum machinep; /* machine precision */
static fnum f_tol; /* approx. precision of model functions */
static fnum r_tol; /* relative precision */
static fnum a_tol; /* absolute precision */
static vector aux_tol; /* absolute precision aux. var */

#define G_ITMAX	8L /* maximum number of golden sections */

static vector powmu; /* Powell penalty factor */

/***********************************************************************/

static boolean dist_step_direction(vector dist, vector aux,
                                   vector lagrange, vector lambda, vector d_lambda,
                                   vector c_res, matrix jx, matrix ja, inum trace);

static fnum dist_step_size(vector dist, vector aux, vector lambda,
                           vector c_res, matrix jx, matrix ja, fnum min_alpha, inum trace);

/***********************************************************************/

/* Allocate global data structures */

void new_distance(
                  inum nl, /* number of Lagrange multipliers */
                  inum nx, /* number of variables */
                  inum na, /* number of auxiliary variables */
                  fnum prec, /* global relative precision */
                  fnum tol, /* modes tolerance factor */
                  vector atol /* abstol of auxiliary variables */
) {
    d_dist = rnew_vector(nx);
    
    c_res = rnew_vector(nl);
    jjt = rnew_matrix(nl + na, nl + na);
    jdf = rnew_matrix(nl + na, 3L);
    
    lambda = rnew_vector(nl);
    d_lambda = rnew_vector(nl);
    powmu = rnew_vector(nl);
    
    n_dist = rnew_vector(nx);
    
    ddt = rnew_vector(nx);
    ddn = rnew_vector(nx);
    
    d_aux = rnew_vector(na);
    n_aux = rnew_vector(na);
    
    machinep = FNUM_EPS; /* machine precision */
    
    f_tol = sqrt(machinep);
    r_tol = prec;
    a_tol = sqrt(prec) * ((fabs(tol) < 1.0) ? fabs(tol) : 1.0);
    
    aux_tol = rdup_vector(atol);
}

/* Free global data structures */

void fre_distance(void) {
    rfre_vector(d_dist);
    
    rfre_vector(c_res);
    rfre_matrix(jjt);
    rfre_matrix(jdf);
    
    rfre_vector(lambda);
    rfre_vector(d_lambda);
    rfre_vector(powmu);
    
    rfre_vector(n_dist);
    
    rfre_vector(d_aux);
    rfre_vector(n_aux);
    
    rfre_vector(ddt);
    rfre_vector(ddn);
    
    rfre_vector(aux_tol);
}

boolean distance(
                 vector dist, /* distance vector */
                 vector aux, /* auxiliary vector */
                 vector lagrange, /* Lagrange multipliers */
                 matrix jx, /* Jacobian matrix */
                 matrix ja, /* Jacobian matrix */
                 inum maxiter, /* maximum number of iterations */
                 inum trace /* trace level */
) {
    boolean cf, jf; /* residual and Jx evaluation flag */
    inum iter; /* iteration count */
    inum fullstep; /* number of full steps */
    inum partstep; /* number of line searches */
    fnum alpha; /* step size */
    boolean conv; /* convergence flag */
    fnum d_norm; /* norm of distance vector */
    fnum dd_norm; /* norm of d_dist vector */
    fnum ddn_norm; /* norm of normal component */
    fnum ddt_norm; /* norm of tangent component */
    fnum min_alpha; /* minimum step size */
    fnum d_tol; /* tolerance on distance */
    boolean sd;
    inum i;
    TMPRINTSTATE *pst;
    
    if (trace >= 1)
        fputs("Distance determination:\n", trace_stream);
    
    for (i = 0; i < VECN(lambda); i++) /* initial Lagrange estimate */
        VEC(lambda, i) = VEC(lagrange, i);
    
    fullstep = 0;
    partstep = 0;
    conv = FALSE;
    cf = jf = TRUE;
    
    /* MAIN LOOP */
    
    for (iter = 0; (conv == FALSE) && (iter <= maxiter); iter++) {
        
        if (ext_constraints(dist, aux, cf, c_res, jf, jx, ja, trace - 2)
            == FALSE) {
            
            if (trace >= 2)
                fputs("constraint equation failure\n", trace_stream);
            break;
        }
        
        sd = dist_step_direction(dist, aux, lagrange, lambda, d_lambda,
                                 c_res, jx, ja, trace - 1);
        
        if (sd == FALSE) /* no search direction */
            break;
        
        if (maxiter == 0) { /* fast one-step mode */
            
            /* update dist and Lagrangian, jx should not change */
            
            for (i = 0; i < VECN(lagrange); i++)
                VEC(lagrange, i) += VEC(d_lambda, i);
            
            for (i = 0; i < VECN(dist); i++)
                VEC(dist, i) += VEC(d_dist, i);
            
            for (i = 0; i < VECN(aux); i++)
                VEC(aux, i) += VEC(d_aux, i);
            
            conv = TRUE;
            break;
        }
        
        /* test for convergence, d_dist & d_aux must be small */
        
        for (i = 0, conv = TRUE; i < VECN(d_dist); i++) {
            d_tol = r_tol * fabs(VEC(dist, i)) + a_tol;
            conv = (fabs(VEC(d_dist, i)) < d_tol) ? conv : FALSE;
        }
        
        for (i = 0; i < VECN(d_aux); i++) {
            d_tol = r_tol * fabs(VEC(aux, i)) + fabs(VEC(aux_tol, i));
            conv = (fabs(VEC(d_aux, i)) < d_tol) ? conv : FALSE;
        }
        
        if (conv == TRUE) break; /* do not step, jx would change */
        
        for (i = 0; i < VECN(lambda); i++) /* update lambda */
            VEC(lambda, i) += VEC(d_lambda, i);
        
        d_norm = norm_vector(dist);
        dd_norm = norm_vector(d_dist);
        
        min_alpha = machinep * (d_norm / dd_norm);
        
        ddn_norm = norm_vector(ddn);
        ddt_norm = norm_vector(ddt);
        
        for (i = 0; i < VECN(powmu); i++) {
            if (iter == 0) {
                VEC(powmu, i) = VEC(lambda, i);
            } else {
                VEC(powmu, i)  = MAX(fabs(VEC(lambda, i)), 0.5 * fabs(VEC(powmu, i) + fabs(VEC(lambda, i))));
            }
        }
        
        alpha = dist_step_size(dist, aux, lambda, c_res, jx, ja, min_alpha,
                               trace - 1);
        
        if (alpha == 0.0)
            break; /* no step size */
        if (alpha == 1.0) {
            cf = FALSE;
            jf = FALSE;
            fullstep++;
        } else {
            cf = TRUE;
            jf = TRUE;
            partstep++;
        }
        
        for (i = 0; i < VECN(dist); i++) /* update distance */
            VEC(dist, i) += alpha * VEC(d_dist, i);
        
        for (i = 0; i < VECN(aux); i++) /* update aux */
            VEC(aux, i) += alpha * VEC(d_aux, i);
    }
    
    if (trace >= 1) {
        
        if (conv == TRUE)
            fputs("\ndistance found.\n", trace_stream);
        else fputs("\ndistance not found.\n", trace_stream);
        
        fputs("final distance:\n", trace_stream);
        pst = tm_setprint(trace_stream, 0, 80, 8, 0);
        print_vector(pst, dist);
        fputs("final auxiliary:\n", trace_stream);
        print_vector(pst, aux);
        fputs("final Lagrangian:\n", trace_stream);
        print_vector(pst, lagrange);
        tm_endprint(pst);
        
        if (maxiter != 0)
            fprintf(trace_stream,
                    "no. of iterations: %ld, full step: %ld, partial step: %ld\n",
                    (long) iter, (long) fullstep, (long) partstep);
        else fputs("one step mode\n", trace_stream);
    }
    
    return (conv);
}

/* calculate step direction by sequential linear constrained method */

boolean dist_step_direction(
                            vector dist, /* current distance vector */
                            vector aux, /* current auxiliary vector */
                            vector lagrange, /* current Lagrange vector estimate */
                            vector lambda, /* previous Lagrange vector predictor */
                            vector d_lambda, /* next Lagrange vector predictor */
                            vector c_res, /* constraint residual */
                            matrix jx, /* Jacobian matrix */
                            matrix ja, /* Jacobian matrix */
                            inum trace
                            ) {
    inum nequ, nvar, naux;
    inum i, r, c;
    fnum f;
    TMPRINTSTATE *pst;
    
    if (trace >= 1)
        fputs("step direction:\n", trace_stream);
    
    nequ = MATM(jx); /* number of equations */
    nvar = MATN(jx); /* number of variables */
    naux = VECN(aux); /* number of auxiliaries */
    
    /* compute design matrix: Jx . Jxt */
    
    for (r = 0; r < nequ; r++)
        for (c = r; c < nequ; c++) {
            for (f = 0.0, i = 0; i < nvar; i++)
                f += MAT(jx, r, i) * MAT(jx, c, i);
            MAT(jjt, r, c) = f;
            MAT(jjt, c, r) = f;
        }
    
    /* add Ja part */
    
    for (r = 0; r < naux; r++)
        for (c = 0; c < nequ; c++) {
            MAT(jjt, (nequ + r), c) = MAT(ja, c, r);
            MAT(jjt, c, (nequ + r)) = MAT(ja, c, r);
        }
    
    /* zero rest */
    
    for (r = 0; r < naux; r++)
        for (c = 0; c < naux; c++) {
            MAT(jjt, (nequ + r), (nequ + c)) = 0.0;
        }
    
    /* compute right hand side: Jx . dist - c_res; - c_res; Jx . dist */
    
    for (r = 0; r < nequ; r++) {
        for (f = 0.0, i = 0; i < nvar; i++)
            f += MAT(jx, r, i) * VEC(dist, i);
        MAT(jdf, r, 0) = f - VEC(c_res, r);
        MAT(jdf, r, 1) = -VEC(c_res, r);
        MAT(jdf, r, 2) = f;
    }
    
    /* zero rest */
    
    for (r = 0; r < naux; r++) {
        MAT(jdf, (nequ + r), 0) = 0.0;
        MAT(jdf, (nequ + r), 1) = 0.0;
        MAT(jdf, (nequ + r), 2) = 0.0;
    }
    
    /* solve (Jx . Jxt) jdf = jdf */
    
    if (solvesym_m(jjt, jdf, jdf) == FALSE) {
        
        if (trace >= 1)
            fputs("Jacobian decomposition failed: indefinite model constraints\n",
                  trace_stream);
        return (FALSE);
    }
    
    /* current Lagrangian: l_c = (Jx.Jxt)'.(Jx.dist) */
    
    for (i = 0; i < nequ; i++) {
        VEC(lagrange, i) = MAT(jdf, i, 2);
    }
    
    /* calculate Lagrange update: d_l = (Jx.Jxt)'.(Jx.dist - c_res) - l */
    
    for (i = 0; i < nequ; i++) {
        VEC(d_lambda, i) = MAT(jdf, i, 0) - VEC(lambda, i);
    }
    
    /* calculate distance update: d_d = Jxt.(Jx.Jxt)'(Jx.dist - c_res) - dist */
    
    for (c = 0; c < nvar; c++) {
        VEC(d_dist, c) = 0.0;
        VEC(ddn, c) = 0.0;
        for (i = 0; i < nequ; i++) {
            VEC(d_dist, c) += MAT(jx, i, c) * MAT(jdf, i, 0);
            VEC(ddn, c) += MAT(jx, i, c) * MAT(jdf, i, 1);
        }
        VEC(d_dist, c) -= VEC(dist, c);
        VEC(ddt, c) = VEC(d_dist, c) - VEC(ddn, c);
    }
    
    /* get aux. vector update */
    
    for (c = 0; c < naux; c++) {
        VEC(d_aux, c) = MAT(jdf, (nequ + c), 0);
    }
    
    if (trace >= 1) {
        pst = tm_setprint(trace_stream, 0, 80, 8, 0);
        fputs("distance  :\n", trace_stream);
        print_vector(pst, dist);
        fputs("d_distance:\n", trace_stream);
        print_vector(pst, d_dist);
        fputs("d_normal  :\n", trace_stream);
        print_vector(pst, ddn);
        fputs("d_tangent :\n", trace_stream);
        print_vector(pst, ddt);
        fputs("d_aux :\n", trace_stream);
        print_vector(pst, d_aux);
        fputs("lagrange  :\n", trace_stream);
        print_vector(pst, lagrange);
        fputs("lambda    :\n", trace_stream);
        print_vector(pst, lambda);
        fputs("d_lambda  :\n", trace_stream);
        print_vector(pst, d_lambda);
        fputs("Jacobian x:\n", trace_stream);
        print_matrix(pst, jx);
        fputs("Jacobian a:\n", trace_stream);
        print_matrix(pst, ja);
        tm_endprint(pst);
    }
    
    return (TRUE);
}


/* Local search, find optimal step size in one direction */

static vector g_dist; /* global distance vector pointer */
static vector g_aux; /* global aux vector pointer */
static vector g_lambda; /* global Lagrange vector pointer */
static inum g_trace; /* global trace level */

/* Powell function: p(x) = 1/2 |dist|^2 + mu * |c(x)| */

static fnum powell(vector dist, vector lambda, vector c_res) {
    fnum p, f;
    inum i;
    
    for (p = 0.0, i = 0; i < VECN(dist); i++) {
        f = VEC(dist, i);
        p += f * f;
    }
    
    p *= 0.5;
    
    for (i = 0; i < VECN(powmu); i++) {
        p += VEC(powmu, i) * fabs(VEC(c_res, i));
    }
    
    return (p);
}

/* function for line search, evaluate model constraints along step direction */

static fnum constr_linef(fnum alpha) {
    fnum pow;
    boolean b;
    inum i;
    
    for (i = 0; i < VECN(g_dist); i++) /* partial step */
        VEC(n_dist, i) = VEC(g_dist, i) + alpha * VEC(d_dist, i);
    
    for (i = 0; i < VECN(g_aux); i++) /* partial step */
        VEC(n_aux, i) = VEC(g_aux, i) + alpha * VEC(d_aux, i);
    
    b = ext_constraints(n_dist, n_aux, TRUE, c_res,
                        FALSE, matrixNIL, matrixNIL, g_trace - 1);
    
    if (b == TRUE)
        pow = powell(n_dist, g_lambda, c_res);
    else pow = INF;
    
    if (g_trace >= 1) {
        if (b == TRUE)
            fprintf(trace_stream,
                    "directional search\nstep: %.*e, Powell residual: %.*e\n",
                    FNUM_DIG, alpha, FNUM_DIG, pow);
        else
            fprintf(trace_stream,
                    "directional search\nstep: %.*e, Powell residual: inf\n",
                    FNUM_DIG, alpha);
    }
    
    return (pow);
}

/* find optimum step size along step direction */

fnum dist_step_size(
                    vector dist, /* current distance */
                    vector aux, /* current aux. vector */
                    vector lambda, /* current Lagrange multiplier */
                    vector c_res, /* constraint residuals */
                    matrix jx, /* Jacobian matrix */
                    matrix ja, /* Jacobian matrix */
                    fnum min_alpha, /* minimum step size */
                    inum trace /* trace level */
) {
    inum itmax;
    fnum xl, xm, xr;
    fnum fl, fm, fr;
    fnum xmin, fmin;
    boolean found;
    boolean b;
    inum i;
    
    if (trace >= 1)
        fputs("\nstep size:\n", trace_stream);
    
    fl = powell(dist, lambda, c_res);
    
    for (i = 0; i < VECN(dist); i++) /* try full step first */
        VEC(n_dist, i) = VEC(dist, i) + VEC(d_dist, i);
    
    for (i = 0; i < VECN(aux); i++)
        VEC(n_aux, i) = VEC(aux, i) + VEC(d_aux, i);
    
    b = ext_constraints(n_dist, n_aux, TRUE, c_res, TRUE, jx, ja, trace - 1);
    
    if (b == TRUE) {
        
        fr = powell(n_dist, lambda, c_res);
        
        if ((fr - fl) < (r_tol * fl + f_tol)) { /* no significant increase */
            if (trace >= 1)
                fprintf(trace_stream, "full step accepted: %.*e <= %.*e\n\n",
                        FNUM_DIG, fr, FNUM_DIG, fl);
            return (1.0);
        }
    } else {
        
        if (trace >= 1)
            fputs("full step failed\n", trace_stream);
        
        fr = INF;
    }
    
    /* partial step, line search for a minimum of the powell function */
    
    if (trace >= 1) {
        fprintf(trace_stream, "starting directional search:\n");
        if (b == TRUE)
            fprintf(trace_stream, "Powell residuals: %.*e, %.*e\n",
                    FNUM_DIG, fl, FNUM_DIG, fr);
        else
            fprintf(trace_stream, "Powell residuals: %.*e, inf\n",
                    FNUM_DIG, fl);
    }
    
    g_lambda = lambda;
    g_dist = dist;
    g_aux = aux;
    g_trace = trace - 1;
    
    xl = 0.0; /* left point */
    xr = 1.0; /* right point */
    
    /* try to bracket the minimum more closely */
    
    for (;;) {
        xm = 1.0e-1 * xr;
        if (xm < min_alpha) {
            if (trace >= 1)
                fprintf(trace_stream, "step size too small: %e\n", xm);
            return (0.0);
        }
        fm = constr_linef(xm);
        if (fm > fl) {
            xr = xm;
        } else break;
    }
    
    xmin = xm;
    fmin = fm;
    
    itmax = G_ITMAX; /* only gain a few digits precision */
    
    /* golden section search */
    
    found = golden(xl, xm, xr, constr_linef, r_tol, &itmax, &xmin, &fmin);
    
    if (found == FALSE) { /* it usually is */
        
        if (fmin > fm) {
            xmin = xm; /* better than nothing */
            fmin = fm;
        }
        
        if (trace >= 1)
            fprintf(trace_stream, "local opt. count reached : %ld\n", (long) itmax);
    }
    
    if (trace >= 1)
        fprintf(trace_stream,
                "end directional search\nstep: %.*e, Powell residual: %.*e\n",
                FNUM_DIG, xmin, FNUM_DIG, fmin);
    
    return (xmin);
}
