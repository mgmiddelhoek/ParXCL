/*
 * ParX - simulate.c
 * Simulate a System
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

#include "actions.h"
#include "error.h"
#include "newton.h"
#include "parx.h"
#include "simulate.h"
#include "vecmat.h"

/*********************** global data ***********************************/

static procedure model_code;   /* model procedure code */
static moddat model_interface; /* model data interface block */
static inum_list xtrans;       /* transpose index list */

static inum nxv; /* number of unknowns */
static inum nav; /* number of aux unknowns */

/***********************************************************************/

static void init_x(xset xs, fnum tol, vector xv, vector relerr, vector abserr);
static void finish_x(xset xs, vector xv, vector relerr, vector abserr);
static void jx2jxv(matrix jx, matrix jxv);
static void xv2xs(vector xv, vector xs);
static void xs2xv(vector xs, vector xv);

/***********************************************************************/

boolean simulate(numblock numb, fnum tol, inum maxiter, inum trace) {
    modres mod;            /* model data */
    boolvector xf;         /* variant x elements */
    vector xvec;           /* unknown vector */
    vector xv;             /* unknown x vector */
    vector av;             /* unknown a vector */
    vector abserr, relerr; /* absolute and relative error vectors */

    xgroup xg, xg_invalid; /* x groups */
    xset xs, xs_tmp;       /* current x set */
    xset_list xsv, xsi;    /* lists of valid and invalid x sets */
    inum npoints;          /* number of valid points in group */
    inum nxgv, nxgi;       /* number of valid and invalid x sets */
    inum nr;               /* Newton-Raphson return code */
    inum i;
    TMPRINTSTATE *pst;

    mod = numb->mod;

    /* are there unknown parameters */

    for (i = 0; i < mod->np; i++)
        if (VEC(mod->pstat, i) == UNKN) {
            fprintf(trace_stream, "\nWarning: unknown parameters in "
                                  "simulation, using default values\n\n");
            break;
        }

    /* check simulation data and setup transformation tables */

    xtrans = new_inum_list();
    xf = numb->modi->xf;

    for (i = 0; i < mod->nx; i++) {
        if (VEC(mod->xstat, i) == UNKN) {
            append_inum_list(xtrans, i);
            VEC(xf, i) = TRUE;
        } else {
            VEC(xf, i) = FALSE;
        }
    }

    nxv = LSTS(xtrans);
    nav = mod->na;

    if ((nxv + nav) != mod->nr) { /* more or less unknowns then relations */
        errcode = ILL_SETUP_SERR;
        error("sim: illegal number of unknowns");
        fre_inum_list(xtrans);
        return (FALSE);
    }

    /* create data structures for model interfacing */

    model_code = mod->model;

    model_interface = numb->modi;

    copy_vector(numb->p->val, model_interface->p);
    copy_vector(numb->c->val, model_interface->c);
    copy_vector(numb->f->val, model_interface->f);
    model_interface->rf = FALSE;
    model_interface->jxf = FALSE;
    model_interface->jpf = FALSE;

    xvec = rnew_vector(nxv + nav);
    xv = new_sub_vector(xvec);
    sub_vector(xvec, 0, nxv, xv);
    av = new_sub_vector(xvec);
    sub_vector(xvec, nxv, nav, av);

    abserr = rnew_vector(nxv + nav);
    relerr = rnew_vector(nxv + nav);

    /* solve equations for all x groups and sets */

    new_newton(nxv + nav);

    xsi = new_xset_list();
    nxgi = 0;

    /* MAIN LOOP */

    for (xg = numb->x; xg != xgroupNIL; xg = xg->next) {

        npoints = xg->n;

        if (error_stream != trace_stream) {
            fprintf(error_stream, "\nsimulating: (%ld) ", (long)npoints);
        }

        if (trace >= 1) {
            fprintf(trace_stream, "\nSimulate: group %ld, points %ld\n",
                    (long)(xg->id), (long)(xg->n));
        }

        xs = xg->g;
        xsv = new_xset_list();
        nxgv = 0;

        while (xs != xsetNIL) {

            if (trace >= 1) {
                fprintf(trace_stream, "\nsim: point %ld\n", (long)(xs->id));
            }
            if (trace >= 3) {
                pst = tm_setprint(trace_stream, 1, 80, 8, 0);
                fputs("[val] =\n", trace_stream);
                print_vector(pst, xs->val);
                fputs("[err] =\n", trace_stream);
                print_vector(pst, xs->err);
                fputs("[abserr] =\n", trace_stream);
                print_vector(pst, xs->abserr);
                tm_endprint(pst);
            }

            copy_vector(xs->val, model_interface->x);

            /* calculate initial values and accuracies */

            zero_vector(av); /* no initial value */

            zero_vector(relerr);
            zero_vector(abserr);

            for (i = 0; i < nav; i++) {
                VEC(abserr, nxv + i) = fabs(VEC(numb->a->val, i));
                VEC(relerr, nxv + i) = tol;
            }

            init_x(xs, tol, xv, relerr, abserr);

            if (trace >= 3) {
                pst = tm_setprint(trace_stream, 1, 80, 8, 0);
                fputs("[relerr] =\n", trace_stream);
                print_vector(pst, relerr);
                fputs("[abserr] =\n", trace_stream);
                print_vector(pst, abserr);
                tm_endprint(pst);
            }

            nr = newton_raphson(xvec, relerr, abserr, maxiter, trace - 2);

            /* transfer solution back to x set if valid */

            if (nr >= 0) {
                finish_x(xs, xv, relerr, abserr);
            }

            if (trace >= 2) {
                pst = tm_setprint(trace_stream, 1, 80, 8, 0);
                fprintf(trace_stream, "result:\n");
                if (trace >= 3) {
                    fputs("[var] =\n", trace_stream);
                    print_vector(pst, xv);
                    fputs("[aux] =\n", trace_stream);
                    print_vector(pst, av);
                }
                fputs("[val] =\n", trace_stream);
                print_vector(pst, xs->val);
                fputs("[delta] =\n", trace_stream);
                print_vector(pst, xs->delta);
                tm_endprint(pst);
            }
            if (trace >= 1) {
                switch (nr) {
                case 0: /* all is well */
                    break;

                case 1: /* maybe a valid solution */
                    fprintf(trace_stream,
                            "Warning: No better solution can be found\n");
                    break;

                case -1: /* no valid solution */
                    fprintf(trace_stream,
                            "Error: Illegal evaluation of model equations\n");
                    break;
                case -2:
                    fprintf(trace_stream,
                            "Error: Rate of convergence too slow\n");
                    break;
                case -3:
                    fprintf(trace_stream,
                            "Error: No search direction can be found\n");
                    break;
                case -4:
                    fprintf(trace_stream,
                            "Error: No better solution can be found\n");
                    break;
                default:
                    fprintf(trace_stream,
                            "Error: Unknown error in simulation\n");
                    break;
                }
            }

            xs_tmp = xs;
            xs = xs->next;          /* goto next, before set is moved */
            xs_tmp->next = xsetNIL; /* change list to element */

            if (nr == 0) { /* move set to valid group */
                nxgv++;
                xsv = append_xset_list(xsv, xs_tmp);
            } else { /* move set to invalid group */
                nxgi++;
                npoints--;
                xsi = append_xset_list(xsi, xs_tmp);
            }
        }
        fre_xset_list(xg->g);
        xg->g = rev_xset_list(xsv);
        xg->n = nxgv;

        if (error_stream != trace_stream) {
            fprintf(error_stream, " (%ld) ", (long)npoints);
        }
    }

    fre_newton();

    /* create a new x group for invalid x sets */

    xg_invalid = new_xgroup(-1, nxgi, rev_xset_list(xsi)); /* invalid id */

    numb->x = append_xgroup_list(numb->x, xg_invalid);

    /* clean up data structures */

    fre_inum_list(xtrans);
    rfre_vector(xvec);
    fre_sub_vector(xv);
    fre_sub_vector(av);
    rfre_vector(abserr);
    rfre_vector(relerr);

    if (trace >= 1) {
        fflush(trace_stream);
    }

    if (error_stream != trace_stream)
        fputc('>', error_stream);
    if (error_stream != trace_stream)
        fputc('\n', error_stream);

    return (TRUE);
}

/* calculate the value and absolute and relative tolerances for unknown x */

void init_x(xset xs, fnum tol, vector xv, vector relerr, vector abserr) {
    inum i;

    xs2xv(xs->val, xv);
    xs2xv(xs->err, relerr);
    xs2xv(xs->abserr, abserr);

    for (i = 0; i < nxv; i++) {

        if (fabs(VEC(abserr, i)) < (tol * FNUM_EPS)) {
            VEC(abserr, i) = tol * FNUM_EPS;
        }

        VEC(relerr, i) = fabs(tol);
    }
}

void finish_x(xset xs, vector xv, vector relerr, vector abserr) {

    xs->res = VEC(relerr, 0);

    xv2xs(xv, xs->val);
    xv2xs(relerr, xs->err);
    xv2xs(abserr, xs->delta);
}

void xs2xv(vector xs, vector xv) {
    inum i;

    for (i = 0; i < LSTS(xtrans); i++) {
        VEC(xv, i) = VEC(xs, LST(xtrans, i));
    }
}

void xv2xs(vector xv, vector xs) {
    inum i;

    for (i = 0; i < LSTS(xtrans); i++) {
        VEC(xs, LST(xtrans, i)) = VEC(xv, i);
    }
}

/* Transpose the columns of the Jacobian matrix */

void jx2jxv(matrix jx, matrix jxv) {
    inum c, r;

    for (c = 0; c < LSTS(xtrans); c++) {
        for (r = 0; r < MATM(jxv); r++) {
            MAT(jxv, r, c) = MAT(jx, r, LST(xtrans, c));
        }
    }
}

/* Model constraints to be zeroed */

boolean sim_constraints(vector x,     /* variable vector */
                        boolean *rf,  /* want residuals? */
                        vector r,     /* residual vector */
                        boolean *jxf, /* want Jacobian? */
                        matrix jx,    /* Jacobian matrix */
                        inum trace    /* trace level */
) {
    inum i, j;
    boolean rc;
    TMPRINTSTATE *pst;
    fenv_t env;

    if (trace > 0) {
        fputs("| model call:\n", trace_stream);
        fprintf(trace_stream, "in: rf = %s, jxf = %s\n",
                *rf == TRUE ? "1" : "0", *jxf == TRUE ? "1" : "0");
        pst = tm_setprint(trace_stream, 1, 80, 8, 0);
        fputs("[x] =\n", trace_stream);
        print_vector(pst, x);
        tm_endprint(pst);
    }

    xv2xs(x, model_interface->x);

    for (i = 0; i < nav; i++) {
        VEC(model_interface->a, i) = VEC(x, nxv + i);
    }

    model_interface->rf = *rf;
    model_interface->jxf = *jxf;

    feholdexcept(&env);

    rc = (*model_code)(model_interface); /* call model */

    if (rc == FALSE) {
        if (trace > 0)
            fputs("Model: illegal return\n\n", trace_stream);
        fesetenv(&env);
        return (FALSE);
    }

    if (fetestexcept(FE_DIVBYZERO | FE_OVERFLOW | FE_INVALID)) {
        if (trace > 0)
            fputs("Model: floating point exception\n\n", trace_stream);
        fesetenv(&env);
        return (FALSE);
    }

    fesetenv(&env);

    *rf = model_interface->rf;
    *jxf = model_interface->jxf;

    copy_vector(model_interface->r, r);

    if (*jxf == TRUE) {
        jx2jxv(model_interface->jx, jx);
        for (i = 0; i < nav; i++)
            for (j = 0; j < MATM(jx); j++) {
                MAT(jx, j, nxv + i) = MAT(model_interface->ja, j, i);
            }
    }

    if (trace > 0) {
        fprintf(trace_stream, "out: rf = %s, jxf = %s\n",
                *rf == TRUE ? "1" : "0", *jxf == TRUE ? "1" : "0");
        if (*rf == TRUE) {
            fputs("[r] =\n", trace_stream);
            pst = tm_setprint(trace_stream, 1, 80, 8, 0);
            print_vector(pst, r);
            tm_endprint(pst);
        }
        if (*jxf == TRUE) {
            fputs("[jx] =\n", trace_stream);
            pst = tm_setprint(trace_stream, 1, 80, 8, 0);
            print_matrix(pst, jx);
            tm_endprint(pst);
        }
    }

    return (TRUE);
}
