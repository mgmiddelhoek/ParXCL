/*
 * ParX - modes.c
 * MODES Parameter Estimator
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
#include "actions.h"
#include "error.h"
#include "vecmat.h"
#include "residual.h"
#include "minbrent.h"
#include "modify.h"
#include "objectiv.h"
#include "modes.h"

/************************ defined constants *****************************/

#define MAX_IT		20L	/* factor for total number of iterations */
#define EQ_SLACK	1.50	/* demand more equations than parameters */
#define LINE_IT		5L	/* number of local line optimizations    */
#define REL_FAC		0.20	/* initial underrelaxation factor        */
#define CUTBOUND	0.10	/* limit for cutting step to bound       */

/************************ global variables ******************************/

static vector p; /* current variable vector */
static vector dp; /* change vector */
static vector p0, p1; /* buffers for current parameter set */

static boolean rf; /* residual vector evaluation flag */
static vector res; /* residual vector */
static boolean jf; /* Jacobian matrix evaluation flag */
static matrix jacp; /* Jacobian matrix */
static vector grad; /* gradient vector */

static inum meval_f; /* number of model equation evaluations */
static inum meval_jx; /* number of model Jx evaluations */
static inum meval_jp; /* number of model Jp evaluations */

static inum funceval; /* number of objective function evaluations */
static inum mineval; /* objective function evaluations for local search */
static clock_t cpu_time; /* used CPU time */

static inum ls_trace; /* trace level for line search */

/***************************************************************************/

static boolean step_direction(vector res, matrix jacp, vector dp, fnum *dc,
                              vector s_val, matrix s_vec, fnum stol, vector qtr,
                              inum *rank, inum trace);

static void check_bounds(vector p, vector dp, vector plow, vector pup,
                         vector step, inum trace);

static fnum step_size(fnum res_norm, fnum releps, fnum abseps,
                      vector bound_alpha, inum trace);

static void conf_lim(vector p, vector p_p, vector p_r, vector res, vector s_val,
                     matrix s_vec, inum rank);

/***************************************************************************/

boolean modes(
              inum neq, /* maximum number of equations */
              inum ng, /* number of equations per point */
              vector pval, /* initial and final scaled parameter values */
              vector plow, /* lower bounds on the parameter values */
              vector pup, /* upper bounds on the parameter values */
              opttype opt, /* type of optimization needed */
              fnum tol, /* modes tolerance factor */
              fnum prec, /* relative precision of the residuals */
              fnum sens, /* sensitivity threshold */
              inum maxiter, /* maximum number of iterations */
              inum trace /* trace level */
) {
    fnum machinep; /* machine precision */
    fnum rtol; /* approx. precision of derived functions */
    fnum stol; /* tolerance for SVD rank determination */
    fnum p_norm; /* norm of parameter vector */
    fnum dp_norm; /* norm of delta parm. vector */
    vector s_val; /* singular values */
    matrix s_vec; /* singular vectors */
    inum rank; /* rank of the Jacobian */
    fnum condn; /* condition number */
    fnum consist; /* consistency number */
    inum npoints; /* number of active data points */
    inum opoints; /* original number of active data points */
    fnum eq_slack; /* demand more equations than parameters */
    fnum res_norm; /* norm of the last residual vector */
    fnum sumsq; /* sum of squares */
    fnum alpha; /* step size */
    fnum min_alpha; /* minimal step size */
    vector bound_alpha; /* step sizes to bounds */
    fnum dc; /* predicted reduction in objective function */
    fnum bound_dc; /* convergence bound */
    boolean conv; /* do we have convergence? */
    boolean prox; /* is match close enough? */
    fnum maxcon; /* current value of maximum consistency */
    boolean fail; /* did the objective evaluation fail */
    inum iter; /* total iteration count */
    inum loc_iter; /* local iteration count */
    boolean modify; /* allow modification of point set */
    boolean moddir; /* is this a modified search direction? */
    inum fullstep; /* number of full Gauss-Newton steps */
    inum partstep; /* number of line minimizations */
    matrix wrkm; /* work space for point reduction */
    vector wrkv; /* work space for point reduction */
    boolean b;
    inum i;
    TMPRINTSTATE *pst;
    
    /* initialize */
    
    meval_f = meval_jx = meval_jp = 0;
    funceval = 0;
    mineval = 0;
    cpu_time = clock();
    
    p = rdup_vector(pval);
    dp = rnew_vector(VECN(p));
    p0 = rnew_vector(VECN(p));
    p1 = rnew_vector(VECN(p));
    
    s_val = rnew_vector(VECN(p));
    s_vec = rnew_matrix(VECN(p), VECN(p));
    
    wrkm = rnew_matrix(neq, MAX(ng, VECN(p)));
    wrkv = rnew_vector(neq);
    grad = rnew_vector(VECN(p));
    
    bound_alpha = rnew_vector(VECN(p));
    
    machinep = FNUM_EPS; /* machine precision */
    rtol = sqrt(machinep);
    prec = MAX(prec, rtol);
    stol = MAX(((double) VECN(p)) * machinep, prec * sens);
    
    rank = VECN(pval);
    
    if (maxiter == 0L)
        maxiter = MAX_IT * roundi(sqrt((fnum) VECN(p)));
    
    eq_slack = (opt == BESTFIT) ? 1.0 : EQ_SLACK;
    
    opoints = npoints = neq / ng; /* total number of data points */
    
    fullstep = 0;
    partstep = 0;
    
    rf = jf = TRUE; /* no data yet */
    
    conv = FALSE;
    prox = FALSE;
    fail = FALSE;
    maxcon = INF; /* no real value yet */
    
    /* start the search for the optimum */
    
    if (error_stream != trace_stream)
        fprintf(error_stream, "\nextracting: (%ld) ", (long) npoints);
    
    if (trace >= 0) {
        fprintf(trace_stream, "\nMODES Parameter Extraction:\n\n");
        fputs("Criterion  : ", trace_stream);
        pst = tm_setprint(trace_stream, 0, 80, 8, 0);
        print_opttype(pst, opt);
        fputc('\n', trace_stream);
        fprintf(trace_stream, "Tolerance  : %e\n", tol);
        fprintf(trace_stream, "Sensitivity: %e\n", sens);
        fprintf(trace_stream, "Precision  : %e\n", prec);
        fprintf(trace_stream, "Max. iter. : %ld\n", (long) maxiter);
        fprintf(trace_stream, "Number of data points: %ld\n\n", (long) npoints);
        fflush(trace_stream);
    }
    
    /* MAIN LOOP */
    
    for (iter = 1, loc_iter = 1, modify = TRUE;
         (conv == FALSE) || (prox == FALSE);
         iter++, loc_iter++, modify = FALSE) {
        
        if (trace >= 2) {
            fprintf(trace_stream, "\nIteration: %ld : %ld  #data points %ld\n",
                    (long) iter, (long) loc_iter, (long) npoints);
            fputs("parameter values:\n", trace_stream);
            pst = tm_setprint(trace_stream, 0, 80, 8, 0);
            unscale_p(p, p1);
            print_vector(pst, p1);
            tm_endprint(pst);
        }
        
        /* evaluate the objective function */
        
        funceval++;
        
        b = objective(p, rf, &res, jf, &jacp, modify, FALSE, &npoints,
                      &meval_f, &meval_jx, &meval_jp, trace - 4);
        
        if (b == FALSE) {
            fail = TRUE;
            errcode = OBJ_FAIL_CERR;
            error("ext");
            if (trace >= 1)
                fputs("Objective function failed\n", trace_stream);
            break;
        }
        
        if (error_stream != trace_stream) {
            if ((npoints != opoints) && (modify == TRUE))
                fprintf(error_stream, "(%ld) ", (long) npoints);
        }
        
        res_norm = norm_vector(res);
        sumsq = res_norm * res_norm;
        
        if (trace >= 2) {
            fprintf(trace_stream, "Objective function: %.*e\n",
                    FNUM_DIG, sumsq);
        }
        
        /* check status */
        
        if (MATM(jacp) < (eq_slack * MATN(jacp))) { /* under-determined */
            fail = TRUE;
            errcode = NUMEQ_CERR;
            error("ext");
            if (trace >= 1)
                fprintf(trace_stream,
                        "Insufficient data points remaining, "
                        "#pnt: %ld  #eq: %ld  #par: %ld\n",
                        (long) npoints, (long) MATM(jacp), (long) MATN(jacp));
            break;
        }
        
        /* determine the optimal step direction */
        /* left hand singular vectors are returned in jacp */
        
        b = step_direction(res, jacp, dp, &dc, s_val, s_vec, stol,
                           p0, &rank, trace - 2);
        
        moddir = FALSE; /* this is the original step direction */
        
        if (b == FALSE) { /* no step direction found */
            fail = TRUE;
            errcode = NO_DIREC_CERR;
            error("ext");
            if (trace >= 1)
                fputs("No step direction found\n", trace_stream);
            break;
        }
        
        if (loc_iter >= maxiter) { /* max iteration count reached */
            errcode = SLOW_CONV_CERR;
            error("ext");
            break;
        }
        
        /* test for convergence, full step dc must be small */
        
        bound_dc = (prec * sumsq) + (10.0 * prec * prec * npoints);
        
        conv = (dc < bound_dc) ? TRUE : FALSE;
        
        /* test for proximity of data to model curve */
        
        if (conv == TRUE) { /* can't improve objective by dp */
            
            loc_iter = 1; /* reset iteration counter */
            
            prox = proximity(res, s_val, ng, rank, opt, tol, &maxcon, trace - 1);
            
            if (prox == FALSE) { /* not good enough */
                
                /* modify number of data points and find new step direction */
                /* also adjust current value of residual norm for line search */
                
                b = modify_point_set(res, ng, s_val, s_vec, jacp, rank,
                                     dp, &dc, &res_norm, &npoints, p0, wrkv, wrkm, trace - 1);
                
                if (b == FALSE) { /* can't modify point set */
                    errcode = MODIFY_CERR;
                    error("ext");
                    break;
                }
                
                moddir = TRUE; /* search direction has been modified */
                
                sumsq = res_norm * res_norm;
                
                bound_dc = (prec * sumsq) + (prec * prec * npoints);
                
                conv = (dc < bound_dc) ? TRUE : FALSE;
            }
        }
        
        if ((conv == TRUE) && (prox == TRUE)) { /* optimum reached */
            break; /* step out of iteration loop */
        }
        
        if (conv == FALSE) { /* determine the optimal step size */
            
            p_norm = norm_vector(p);
            dp_norm = norm_vector(dp);
            
            if (dp_norm > 0.0)
                min_alpha = machinep * (p_norm / dp_norm);
            else
                min_alpha = machinep * p_norm;
            
            check_bounds(p, dp, plow, pup, bound_alpha, trace - 2);
            
            alpha = step_size(res_norm, rtol, min_alpha, bound_alpha,
                              trace - 2);
            
        } else { /* accept small step from modify as is */
            
            alpha = 1.0;
            rf = TRUE;
            jf = TRUE;
        }
        
        if ((alpha == 0.0) && (moddir == FALSE)) { /* no step size found */
            errcode = NO_LOWP_CERR;
            error("ext");
            if (trace >= 1)
                fputs("No step size found\n", trace_stream);
            break;
        } else if (alpha == 1.0) {
            fullstep++;
        } else {
            partstep++;
        }
        
        condn = fabs(VEC(s_val, 0) / VEC(s_val, rank - 1));
        
        for (consist = 1.0, i = 0; i < rank; i++)
            consist *= res_norm / VEC(s_val, i);
        consist = pow(consist, 1.0 / ((double) rank));
        
        for (i = 0; i < VECN(p); i++) /* go for it */
            VEC(p, i) += alpha * VEC(dp, i);
        
        /* rescale parameters, and jacp if needed by next iteration */
        
        set_p_scale(p, plow, pup, (jf == FALSE) ? jacp : matrixNIL);
        
    }
    
    /* END GAME */
    
    for (i = 0; i < VECN(pval); i++) /* copy solution */
        VEC(pval, i) = VEC(p, i);
    
    if (trace >= 0) { /* output diagnostics */
        
        if ((conv == FALSE) || (prox == FALSE)) {
            fprintf(trace_stream, "\nExtraction FAILED\n");
        } else if (rank < VECN(pval)) {
            fprintf(trace_stream, "\nExtraction DOUBTFUL\n");
        } else {
            fprintf(trace_stream, "\nExtraction SUCCEEDED\n");
        }
    }
    
    (void) proximity(res, s_val, ng, rank, opt, tol, &maxcon, trace);
    
    conf_lim(pval, plow, pup, res, s_val, s_vec, rank);
    
    if ((trace >= 1) && (fail == FALSE)) { /* report results */
        
        fputs("\n\nExtraction Result Report\n", trace_stream);
        fputs("------------------------\n\n", trace_stream);
        
        if (conv == FALSE)
            fputs("WARNING: No convergence was reached\n", trace_stream);
        else if (prox == FALSE)
            fputs("WARNING: No proximity was reached\n", trace_stream);
        
        fputs("\nFinal parameter values:\n", trace_stream);
        pst = tm_setprint(trace_stream, 0, 80, 8, 0);
        unscale_p(pval, p1);
        print_vector(pst, p1);
        tm_endprint(pst);
        
        fputs("\nConfidence limits:\n", trace_stream);
        pst = tm_setprint(trace_stream, 0, 80, 8, 0);
        unscale_p(plow, p1);
        print_vector(pst, p1);
        tm_endprint(pst);
        
        fputs("\nRedundancy factors:\n", trace_stream);
        pst = tm_setprint(trace_stream, 0, 80, 8, 0);
        print_vector(pst, pup);
        tm_endprint(pst);
        fputc('\n', trace_stream);
        
        if (rank < VECN(pval)) {
            fputs("WARNING: Not full rank, remove redundant parameters\n",
                  trace_stream);
        }
    }
    
    if ((trace >= 2) && (fail == FALSE)) { /* report SVD */
        
        fputs("\n\nSingular Value Decomposition Report\n", trace_stream);
        fputs("-----------------------------------\n\n", trace_stream);
        
        fputs("\nSingular values =\n", trace_stream);
        pst = tm_setprint(trace_stream, 0, 80, 8, 0);
        print_vector(pst, s_val);
        fputs("\nSingular vectors =\n", trace_stream);
        print_matrix(pst, s_vec);
        tm_endprint(pst);
        
        condn = fabs(VEC(s_val, 0) / VEC(s_val, rank - 1));
        
        fprintf(trace_stream,
                "\nEstimated rank = %ld, Condition number = %.*e\n",
                (long) rank, FNUM_DIG, condn);
    }
    
    if (error_stream != trace_stream)
        fprintf(error_stream, " (%ld) ", (long) npoints);
    
    /* calculate the final distances */
    
    (void) objective(p, TRUE, &res, FALSE, &jacp, FALSE, TRUE, &npoints,
                     &meval_f, &meval_jx, &meval_jp, trace - 4);
    
    funceval++;
    
    if (error_stream != trace_stream) fputc('>', error_stream);
    if (error_stream != trace_stream) fputc('\n', error_stream);
    
    if (trace >= 1) { /* report effort */
        
        fputs("\n\nEffort Report\n", trace_stream);
        fputs("-------------\n\n", trace_stream);
        
        fprintf(trace_stream,
                "\nIterations: %ld, full step: %ld, partial step: %ld\n",
                (long) iter, (long) fullstep, (long) partstep);
        fprintf(trace_stream,
                "Objective function evaluations: %ld, in line searches: %ld\n",
                (long) funceval, (long) mineval);
        fprintf(trace_stream,
                "Model equation evaluations: f %ld, Jx %ld, Jp %ld\n",
                (long) meval_f, (long) meval_jx, (long) meval_jp);
        if (cpu_time > -1)
            fprintf(trace_stream,
                    "CPU time: %.0f sec\n\n",
                    (double) (clock() - cpu_time) / CLOCKS_PER_SEC);
        
        fflush(trace_stream);
    }
    
    /* clean up workspace */
    
    rfre_vector(p);
    rfre_vector(dp);
    rfre_vector(p0);
    rfre_vector(p1);
    rfre_matrix(wrkm);
    rfre_vector(wrkv);
    rfre_vector(grad);
    rfre_vector(s_val);
    rfre_matrix(s_vec);
    rfre_vector(bound_alpha);
    
    
    return (prox);
}

/* calculate the modified Gauss-Newton step direction */

boolean step_direction(
                       vector res, /* current residual vector */
                       matrix jacp, /* current Jacobian matrix */
                       vector dp, /* parameter step direction */
                       fnum *dc, /* predicted reduction in objective function */
                       vector s_val, /* singular values */
                       matrix s_vec, /* Pt, right hand singular vectors */
                       /* Q, left hand singular vectors are returned in jacp */
                       /* Qt.res, is returned in workp */
                       fnum stol, /* tolerance for SVD */
                       vector qtr, /* workspace with same dimension as p */
                       inum *rank, /* estimated rank of the Jacobian */
                       inum trace /* trace level */
) {
    fnum condn; /* condition number */
    inum i, pi;
    TMPRINTSTATE *pst;
    
    /* SVD of Jp = Q.D.Pt, Q is returned in jacp */
    
    *rank = svd(jacp, jacp, s_val, s_vec, stol);
    
    if (*rank <= 0) { /* no direction can be found */
        fputs("Step direction failure, system has zero rank\n",
              trace_stream);
        return (FALSE);
    }
    
    mul_matt_vec(jacp, res, qtr); /* calculate Qt.r */
    
    /* calculate: dp = - P.1/Dr.(Qt.r)    */
    /* zero (1/D)i if rank of Jp not full */
    /* P is in s_vec, Qt.r is in qtr      */
    
    for (pi = 0; pi < VECN(dp); pi++) {
        VEC(dp, pi) = 0.0;
        for (i = 0; i < *rank; i++)
            VEC(dp, pi) -= (MAT(s_vec, i, pi) / VEC(s_val, i)) * VEC(qtr, i);
    }
    
    /* calculate: dc = rt.Q.Ir.Qt.r */
    
    for (i = 0, *dc = 0.0; i < *rank; i++)
        *dc += VEC(qtr, i) * VEC(qtr, i);
    
    if (trace >= 1) {
        
        pst = tm_setprint(trace_stream, 0, 80, 8, 0);
        fputs("Step direction: (relative)\n", trace_stream);
        print_vector(pst, dp);
        tm_endprint(pst);
        fprintf(trace_stream, "Predicted objective reduction: %.*e\n",
                FNUM_DIG, *dc);
    }
    
    if (trace >= 2) {
        
        condn = fabs(VEC(s_val, 0) / VEC(s_val, *rank - 1));
        
        pst = tm_setprint(trace_stream, 0, 80, 8, 0);
        fputs("singular values =\n", trace_stream);
        print_vector(pst, s_val);
        fputs("singular vectors =\n", trace_stream);
        print_matrix(pst, s_vec);
        tm_endprint(pst);
        fprintf(trace_stream,
                "Estimated rank = %ld, Condition number = %.*e\n",
                (long) *rank, FNUM_DIG, condn);
    }
    fflush(trace_stream);
    
    return (TRUE);
}

/* determine step size to parameter bounds */

void check_bounds(
                  vector p, /* scaled parameter vector */
                  vector dp, /* scaled parameter step */
                  vector plow, /* scaled lower bounds on parameters */
                  vector pup, /* scaled upper bounds on parameters */
                  vector step, /* step sizes to bounds */
                  inum trace /* trace level */
) {
    fnum pcur, pdif, pnew, pl, pu, s;
    inum i;
    
    for (i = 0; i < VECN(p); i++) {
        
        pcur = VEC(p, i);
        pdif = VEC(dp, i);
        pnew = pcur + pdif;
        pl = VEC(plow, i);
        pu = VEC(pup, i);
        
        if ((pnew > pu) && (pdif != 0.0)) {
            
            s = fabs((pu - pcur) / pdif);
            
            if (s < 1.0) {
                
                VEC(step, i) = s;
                
                if (trace >= 1)
                    fprintf(trace_stream,
                            "Hit upper bound par %ld, limit step = %.*e\n",
                            (long) (i + 1), FNUM_DIG, s);
                
            } else VEC(step, i) = 1.0;
            
        } else if ((pnew < pl) && (pdif != 0.0)) {
            
            s = fabs((pcur - pl) / pdif);
            
            if (s < 1.0) {
                
                VEC(step, i) = s;
                
                if (trace >= 1)
                    fprintf(trace_stream,
                            "Hit lower bound par %ld, limit step = %.*e\n",
                            (long) (i + 1), FNUM_DIG, s);
                
            } else VEC(step, i) = 1.0;
            
        } else VEC(step, i) = 1.0;
    }
}

/* evaluate the objective function in the step direction */

boolean eval_obj_line(
                      fnum alpha, /* step size */
                      boolean ff, /* objective value flag */
                      fnum *fval, /* objective value */
                      boolean df, /* direction flag */
                      fnum *slope /* gradient direction relative to step direction */
) {
    boolean rf, jf, lf;
    inum npoints;
    inum i;
    TMPRINTSTATE *pst;
    
    for (i = 0; i < VECN(p0); i++)
        VEC(p0, i) = VEC(p, i) + alpha * VEC(dp, i);
    
    if (ls_trace >= 1) {
        fprintf(trace_stream, "\nTrying step size : %.*e\n", FNUM_DIG, alpha);
        fputs("parameter values:\n", trace_stream);
        pst = tm_setprint(trace_stream, 0, 80, 8, 0);
        unscale_p(p0, p1);
        print_vector(pst, p1);
        tm_endprint(pst);
    }
    
    rf = ff;
    if (df == TRUE) {
        rf = TRUE;
        jf = TRUE;
    } else jf = FALSE;
    
    funceval++;
    mineval++;
    
    lf = objective(p0, rf, &res, jf, &jacp, FALSE, FALSE, &npoints,
                   &meval_f, &meval_jx, &meval_jp, ls_trace - 1);
    
    if (lf == FALSE) {
        if (ls_trace >= 1)
            fputs("Objective function failed in line search\n", trace_stream);
        
        return (FALSE);
    }
    
    if (ff == TRUE)
        *fval = norm_vector(res);
    
    if (df == TRUE) {
        mul_matt_vec(jacp, res, grad);
        *slope = inp_vector(grad, dp);
    }
    
    if (ls_trace >= 1) {
        if (ff == TRUE)
            fprintf(trace_stream, "resulting objective: %.*e\n",
                    FNUM_DIG, *fval);
        if (df == TRUE)
            fprintf(trace_stream, "resulting slope: %.*e\n",
                    FNUM_DIG, *slope);
    }
    
    return (TRUE);
}

/* evaluate objective function for line search, used by Brent minimization */

fnum obj_line(fnum alpha) {
    boolean lf;
    fnum norm;
    
    lf = eval_obj_line(alpha, TRUE, &norm, FALSE, (fnum *) NULL);
    
    if (lf == FALSE)
        return (INF); /* maximum value */
    
    return (norm);
}

/* Local search, find the optimal step size in the Gauss-Newton direction */

fnum step_size(
               fnum res_norm, /* current minimum for step = 0 */
               fnum releps, /* relative precision */
               fnum abseps, /* smallest step */
               vector bound_alpha, /* step to bounds */
               inum trace /* trace level */
) {
    
    inum itmax;
    fnum xl, xm, xr;
    fnum fl, fm, fr;
    fnum xmin, fmin;
    fnum slope;
    fnum cutb;
    boolean found;
    inum i, mini;
    fnum mins;
    
    if (trace >= 2) {
        fputs("Starting directional search:\n", trace_stream);
        fprintf(trace_stream, "current objective: %.*e\n",
                FNUM_DIG, res_norm);
    }
    
    ls_trace = trace - 1;
    
    xl = 0.0; /* left point */
    fl = res_norm; /* current value */
    
    /* try not to overstep bounds on parameters */
    
    xr = 0.0; /* right point */
    
    cutb = MAX(abseps, CUTBOUND);
    
    do { /* get minimum step */
        
        mins = 1.0;
        mini = -1;
        
        for (i = 0; i < VECN(bound_alpha); i++)
            if (VEC(bound_alpha, i) < mins) {
                mins = VEC(bound_alpha, i);
                mini = i;
            }
        
        if (mini != -1)
            VEC(bound_alpha, mini) = 1.0;
        
        if (eval_obj_line(mins, TRUE, &fr, TRUE, &slope) == FALSE) {
            fr = INF;
            slope = INF;
        }
        mineval--;
        
        if ((mins >= 1.0) || (slope >= 0.0) || (mins > cutb)) {
            
            xr = mins;
            
            if ((trace >= 1) && (xr != 1.0))
                fprintf(trace_stream,
                        "Step cut by bound on par %ld, step = %.*e\n",
                        (long) (mini + 1), FNUM_DIG, xr);
            
        }
        
    } while (xr == 0.0);
    
    /* if step results in lower point or still descending */
    
    if ((fr < fl) || (slope <= 0.0)) {
        
        if (trace >= 1) {
            fprintf(trace_stream, "Taking %s step %s\n",
                    (xr >= 1.0) ? "full" : "partial",
                    (fr > fl) ? ", overstepping discontinuity" : "");
        }
        
        rf = jf = FALSE; /* no need to re-evaluate */
        
        return (xr); /* take full step */
    }
    
    /* try to bracked the optimum more closely */
    
    for (;;) {
        
        xm = REL_FAC * xr;
        
        if (xm < abseps) {
            if (trace >= 1)
                fprintf(trace_stream, "Step size too small\n");
            
            rf = jf = FALSE;
            
            return (0.0);
        }
        
        if (eval_obj_line(xm, TRUE, &fm, TRUE, &slope) == FALSE) {
            fm = INF;
            slope = INF;
        }
        
        if (fm > fl) {
            
            if (slope <= 0.0) { /* discontinuity or multi-modal */
                
                if (trace >= 1)
                    fprintf(trace_stream,
                            "Overstepping discontinuity, step size : %.*e\n",
                            FNUM_DIG, xm);
                
                rf = jf = FALSE;
                
                return (xm);
                
            } else {
                xr = xm;
                fr = fm;
            }
            
        } else break;
    }
    
    xmin = xm;
    fmin = fm;
    
    /* start local line search to polish up the optimum */
    
    itmax = LINE_IT; /* don't be too precise */
    
    found = brent(xl, xm, xr, obj_line, releps, abseps, &itmax, &xmin, &fmin);
    
    if (found == FALSE) {
        
        if (fmin > fm) {
            xmin = xm; /* better then nothing */
        }
    }
    
    if (trace >= 1)
        fprintf(trace_stream,
                "Directional search, step size : %.*e\n",
                FNUM_DIG, xmin);
    
    rf = jf = TRUE; /* re-evaluate, may not be current */
    
    return (xmin);
}

/* calculate approximate confidence limits on the parameters */

void conf_lim(
              vector p, /* scaled parameter values */
              vector p_p, /* parameter precision */
              vector p_r, /* redundancy indicators */
              vector res, /* residual vector */
              vector s_val, /* singular values */
              matrix s_vec, /* singular vectors */
              inum rank /* rank of the Jacobian */
) {
    fnum rssq; /* root of sum of squares */
    fnum c, ci, cm;
    inum i, w, im;
    
    rssq = norm_vector(res);
    
    for (i = 0; i < VECN(p); i++) {
        c = 0.0;
        for (w = 0; w < rank; w++) {
            ci = MAT(s_vec, w, i) / VEC(s_val, w);
            c += ci * ci;
        }
        c = sqrt(c);
        VEC(p_p, i) = fabs(rssq * c);
        VEC(p_r, i) = 0.0; /* set non_redundant */
    }
    
    /* determine and mark redundant parameters */
    
    for (i = 0; i < VECN(p); i++) {
        cm = 0.0;
        im = 0;
        for (w = 0; w < VECN(s_val); w++) {
            ci = fabs(MAT(s_vec, w, i));
            if (ci > cm) {
                cm = ci;
                im = w;
            }
        }
        if (im >= rank) { /* parameter is dominated by singular direction */
            VEC(p_p, i) = fabs(VEC(p, i));
            VEC(p_r, i) = cm; /* set redundant */
        }
    }
}
