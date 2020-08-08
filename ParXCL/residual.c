/*
 * ParX - residual.c
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "distance.h"
#include "error.h"
#include "parx.h"
#include "residual.h"
#include "vecmat.h"

/************************ global variables *****************************/

static inum nc; /* number of constraints */
static inum nr; /* number of residuals */
static inum nx; /* number of variable x */
static inum na; /* number of variable a */
static inum np; /* number of variable p */

static inum_list xtrans; /* x transformation vector */
static inum_list ptrans; /* p transformation vector */

static vector x_scale; /* scaling vector for x and Jacx */
static vector p_scale; /* scaling vector for p and Jacp */

static vector p_low; /* un-scaled vector of lower bounds */
static vector p_up;  /* un-scaled vector of upper bounds */

static procedure model_code;   /* model code pointer */
static moddat model_interface; /* model interface structure */

static matrix jacp;   /* reduced Jacobian matrix */
static matrix jacp_s; /* reduced Jacobian matrix */
static vector facp;   /* row buffer Jacp */

static matrix jacx;   /* reduced and scaled Jacobian matrix */
static matrix jacx_s; /* reduced and scaled Jacobian matrix */
static vector facx;   /* row buffer Jacx */

static matrix jaca; /* auxiliary Jacobian matrix */
static vector faca; /* row buffer Jaca */

static vector x_ref;  /* current reference point */
static vector dist;   /* distance from model hyperplane */
static vector aux;    /* auxiliary variables */
static vector lambda; /* Lagrange multipliers */

static matrix c_trans_l; /* constraint space transformation matrix */
static matrix c_trans_x; /* constraint space transformation matrix */
static vector c_scale;   /* constraint space scaling values */

static inum maxiter; /* maximum number of iterations per point */
#define IT_FAC 100   /* factor for calculation of maxiter */

static inum model_calls_r;  /* number of model residual evaluations */
static inum model_calls_jx; /* number of model Jacx evaluations */
static inum model_calls_jp; /* number of model Jacp evaluations */

static fnum tolerance; /* modes tolerance factor == relative model accuracy */

/***********************************************************************/

static boolean eval_model(boolean rf, boolean jxf, boolean jpf, inum trace);

/***********************************************************************/

/* setup residual function */

boolean new_residual(modres mr,   /* model result structure */
                     moddat mi,   /* model interface structure */
                     fnum prec,   /* relative precision */
                     fnum tol,    /* modes tolerance factor */
                     vector atol, /* absolute tolerance of aux. variables */
                     inum *nup    /* number of unknown parameters */
) {
    boolvector xf, pf; /* element flags */
    inum i;

    tolerance = tol;

    nc = nx = np = nr = 0;

    /* find variable x and setup transformation table */

    xtrans = new_inum_list();
    xf = mi->xf;

    for (i = 0; i < mr->nx; i++) {
        switch (VEC(mr->xstat, i)) {
        case MEAS: /* variable x */
        case CALC:
        case SWEEP:
            append_inum_list(xtrans, i);
            VEC(xf, i) = TRUE;
            nx++;
            break;
        case FACT: /* fixed x */
        case STIM:
            VEC(xf, i) = FALSE;
            break;
        case UNKN: /* unknown variable is as yet illegal */
        default:
            fre_inum_list(xtrans);
            errcode = UNKN_VAR_SERR;
            error("ext");
            return (FALSE);
        }
    }

    /* find variable p and setup transformation table */

    ptrans = new_inum_list();
    pf = mi->pf;

    for (i = 0; i < mr->np; i++) {
        if (VEC(mr->pstat, i) == UNKN) {
            append_inum_list(ptrans, i);
            VEC(pf, i) = TRUE;
            np++;
        } else
            VEC(pf, i) = FALSE;
    }

    *nup = np;

    if ((nx == 0) || (np == 0)) { /* no problem to solve */
        fre_inum_list(xtrans);
        fre_inum_list(ptrans);
        errcode = (nx == 0) ? NO_VAR_SERR : NO_PAR_SERR;
        error("ext");
        return (FALSE);
    }

    p_scale = rnew_vector(np);
    p_low = rnew_vector(np);
    p_up = rnew_vector(np);

    /* setup model evaluation interface */

    model_code = mr->model;

    model_interface = mi;

    /* setup buffer variables */

    nc = mr->nr;
    na = mr->na;
    nr = nc - na;

    jacp = rnew_matrix(nc, np);
    jacp_s = new_sub_matrix(jacp);
    sub_matrix(jacp, 0, 0, nr, np, jacp_s);
    facp = rnew_vector(np);

    jacx = rnew_matrix(nc, nx);
    jacx_s = new_sub_matrix(jacx);
    sub_matrix(jacx, 0, 0, nr, nx, jacx_s);
    facx = rnew_vector(nx);
    faca = rnew_vector(na);

    jaca = rnew_matrix(nc, na);

    lambda = rnew_vector(nc);
    dist = rnew_vector(nx);
    aux = rnew_vector(na);

    x_ref = rnew_vector(nx);
    x_scale = rnew_vector(nx);

    c_scale = rnew_vector(nr);
    c_trans_x = rnew_matrix(nr, nx);
    c_trans_l = rnew_matrix(nr, nr);

    /* setup distance function */

    maxiter = IT_FAC * (nx + na);

    new_distance(nc, nx, na, prec, tol, atol);

    return (TRUE);
}

void fre_residual(void) /* free globals */ {
    fre_inum_list(xtrans);
    fre_inum_list(ptrans);
    rfre_vector(p_scale);
    rfre_vector(p_low);
    rfre_vector(p_up);

    rfre_vector(lambda);
    rfre_vector(dist);
    rfre_vector(aux);

    rfre_matrix(jacx);
    fre_sub_matrix(jacx_s);
    rfre_vector(facx);
    rfre_vector(faca);

    rfre_matrix(jacp);
    fre_sub_matrix(jacp_s);
    rfre_vector(facp);

    rfre_matrix(jaca);

    rfre_vector(x_ref);
    rfre_vector(x_scale);

    rfre_vector(c_scale);
    rfre_matrix(c_trans_x);
    rfre_matrix(c_trans_l);

    fre_distance();
}

/* un-scale distance vector for printing */

static void unscale_x(vector ds, vector norm, vector d) {
    inum index;
    inum i;
    fnum s;

    for (i = 0; i < LSTS(xtrans); i++) {
        index = LST(xtrans, i);
        s = VEC(norm, i);
        VEC(d, index) = s * VEC(ds, i);
    }
}

/* add the distance vector to the x vector after scaling */

static void add_dist_x(vector d, vector norm, vector x) {
    inum index;
    inum i;
    fnum s;

    for (i = 0; i < LSTS(xtrans); i++) {
        index = LST(xtrans, i);
        s = VEC(norm, i);
        VEC(x, index) += s * VEC(d, i);
    }
}

/* reorder the columns and scale the Jx matrix */

static void T_jx_jxv(matrix jx, vector norm, matrix jxv) {
    inum c, r;
    inum cindex;
    fnum s;

    for (c = 0; c < LSTS(xtrans); c++) {
        cindex = LST(xtrans, c);
        s = VEC(norm, c);
        for (r = 0; r < MATM(jxv); r++) {
            MAT(jxv, r, c) = s * MAT(jx, r, cindex);
        }
    }
}

/* transpose and scale the elements of the p vector */

static void T_p_pv(vector p, vector norm, vector pv) {
    inum i;
    inum index;
    fnum s;

    for (i = 0; i < LSTS(ptrans); i++) {
        index = LST(ptrans, i);
        s = VEC(norm, i);
        VEC(pv, i) = VEC(p, index) / s;
    }
}

static void T_pv_p(vector pv, vector norm, vector p) {
    inum i;
    inum index;
    fnum s;

    for (i = 0; i < LSTS(ptrans); i++) {
        index = LST(ptrans, i);
        s = (norm == vectorNIL) ? 1.0 : VEC(norm, i);
        VEC(p, index) = s * VEC(pv, i);
    }
}

/* transpose and scale the columns of the Jp matrix */

static void T_jp_jpv(matrix jp, vector norm, matrix jpv) {
    inum c, r;
    inum cindex;
    fnum s;

    for (c = 0; c < LSTS(ptrans); c++) {
        cindex = LST(ptrans, c);
        s = VEC(norm, c);
        for (r = 0; r < MATM(jpv); r++) {
            MAT(jpv, r, c) = s * MAT(jp, r, cindex);
        }
    }
}

/* scale parameter vector for printing */

void scale_p(vector p, /* un-scaled p */
             vector ps /* scaled p */
) {
    inum i;
    fnum s;

    for (i = 0; i < VECN(ps); i++) {
        s = 1.0 / VEC(p_scale, i);
        VEC(ps, i) = s * VEC(p, i);
    }
}

/* un-scale parameter vector for printing */

void unscale_p(vector ps, /* scaled p */
               vector p   /* un-scaled p */
) {
    inum i;
    fnum s;

    for (i = 0; i < VECN(p); i++) {
        s = VEC(p_scale, i);
        VEC(p, i) = s * VEC(ps, i);
    }
}

/* allocate the parameter vectors */

void new_pvar(pset ps,      /* parameter set with upper and lower bounds */
              vector *pval, /* scaled values */
              vector *plow, /* scaled lower bounds */
              vector *pup   /* scaled upper bounds */
) {
    inum i;

    copy_vector(ps->val, model_interface->p);

    *pval = rnew_vector(np);
    *plow = rnew_vector(np);
    *pup = rnew_vector(np);

    for (i = 0; i < np; i++) {
        VEC(p_scale, i) = 1.0; /* first just transpose */
    }

    T_p_pv(ps->val, p_scale, *pval);
    T_p_pv(ps->lb, p_scale, p_low);
    T_p_pv(ps->ub, p_scale, p_up);

    set_p_scale(*pval, *plow, *pup, matrixNIL); /* use scaling */
}

/* read and free the parameter vectors */

void fre_pvar(vector pval, /* scaled parameter values */
              vector plow, /* scaled parameter precision */
              vector pup,  /* redundancy indicator */
              pset ps      /* output parameter set */
) {
    inum i;
    inum index;
    fnum s;

    for (i = 0; i < LSTS(ptrans); i++) {
        index = LST(ptrans, i);
        s = VEC(p_scale, i);
        VEC(ps->val, index) = s * VEC(pval, i);

        if (VEC(pup, i) > 0.0) { /* redundant parameter */
            VEC(ps->lb, index) = INF;
            VEC(ps->ub, index) = INF;
        } else {
            VEC(ps->lb, index) = s * VEC(plow, i);
            VEC(ps->ub, index) = s * VEC(plow, i);
        }
    }

    rfre_vector(pval);
    rfre_vector(plow);
    rfre_vector(pup);
}

/* set the scale for the parameters */

void set_p_scale(vector val, /* parameter values */
                 vector lb,  /* lower bounds */
                 vector ub,  /* upper bounds */
                 matrix jacp /* Jacobian_p matrix */
) {
    inum i, j;
    fnum sn, so, p, l, u;

    for (i = 0; i < np; i++) {

        so = VEC(p_scale, i);

        p = VEC(val, i);

        if ((fabs(p / INF) * fabs(so / INF)) >= 1.0) {
            p = INF * SIGN(p) * SIGN(so);
        } else {
            p *= so; /* un-scale p */
        }

        l = VEC(p_low, i);
        u = VEC(p_up, i);

        if ((l == 0.0) || (u == 0.0) || (SIGN(l) != SIGN(u))) {
            sn = fabs(u - l); /* use static scaling */
        } else {              /* use dynamic scaling */
            sn = MAX(l, p);
            sn = MIN(sn, u);
        }

        if (sn == 0.0) { /* don't scale */
            sn = 1.0;
        }

        VEC(p_scale, i) = sn;
        VEC(val, i) = p / sn;
        VEC(lb, i) = l / sn;
        VEC(ub, i) = u / sn;

        /* rescale Jacobian matrix if available */

        sn /= so;

        if (jacp != matrixNIL) {
            for (j = 0; j < MATM(jacp); j++) {
                MAT(jacp, j, i) *= sn;
            }
        }
    }
}

/* Residual function */

boolean residual(xset xs,     /* set of measurements */
                 vector pv,   /* variable parameters */
                 boolean rf,  /* residual request flag */
                 vector r,    /* residual vector */
                 boolean jpf, /* Jacobian request flag */
                 matrix jp,   /* Jacobian matrix */
                 boolean sf,  /* scaling matrix request flag */
                 matrix s,    /* scaling matrix */
                 inum *mc_r,  /* number of model residual evaluations */
                 inum *mc_jx, /* number of model Jacobian_x evaluations */
                 inum *mc_jp, /* number of model Jacobian_p evaluations */
                 inum trace   /* trace level */
) {
    inum rank; /* rank from singular value decomposition */
    fnum f, piv;
    boolean b;
    inum ncl;
    inum i, c, a;
    inum rs, rd, rp;
    TMPRINTSTATE *pst;

    model_calls_r = *mc_r;
    model_calls_jx = *mc_jx;
    model_calls_jp = *mc_jp;

    T_pv_p(pv, p_scale, model_interface->p); /* substitute parm. values */

    copy_vector(xs->val, x_ref);
    zero_vector(xs->delta);

    for (i = 0; i < VECN(x_scale); i++) {
        VEC(x_scale, i) =
            MAX(MAX(fabs(VEC(xs->err, i)), fabs(tolerance * VEC(xs->val, i))),
                fabs(VEC(xs->abserr, i)));
    }

    /* we have no initial values for dist and lambda */

    zero_vector(dist);   /* zero distance */
    zero_vector(lambda); /* zero Lagrange estimate */
    zero_vector(aux);    /* zero auxiliary variables */

    b = distance(dist, aux, lambda, jacx, jaca, maxiter, trace - 1);

    *mc_r = model_calls_r;
    *mc_jx = model_calls_jx;
    *mc_jp = model_calls_jp;

    if (b == FALSE) {

        if (trace >= 1) {
            fprintf(trace_stream, "point %ld: distance evaluation failed\n",
                    (long)(xs->id));
        }
        return (FALSE);
    }

    /* return distance vector in xset */
    unscale_x(dist, x_scale, xs->delta);

    copy_vector(x_ref, model_interface->x); /* goto nearest point */
    copy_vector(aux, model_interface->a);   /* goto nearest point */
    add_dist_x(dist, x_scale, model_interface->x);

    b = eval_model(FALSE, FALSE, jpf, trace - 1);

    *mc_r = model_calls_r;
    *mc_jx = model_calls_jx;
    *mc_jp = model_calls_jp;

    if (b == FALSE) {

        if (trace >= 1) {
            fprintf(trace_stream, "point %ld: model Jp evaluation failed\n",
                    (long)(xs->id));
        }
        return (FALSE);
    }

    if (jpf == TRUE) {

        T_jp_jpv(model_interface->jp, p_scale, jacp);
    }

    /* reduce the number of equations */

    if (trace >= 2) {
        pst = tm_setprint(trace_stream, 0, 80, 8, 0);
        fputs("\naux reduction (initial):\n", trace_stream);
        fputs("[Jx] =\n", trace_stream);
        print_matrix(pst, jacx);
        fputs("[Ja] =\n", trace_stream);
        print_matrix(pst, jaca);
        fputs("[Jp] =\n", trace_stream);
        print_matrix(pst, jacp);
        tm_endprint(pst);
    }

    for (ncl = nc, a = 0; a < na; a++) { /* for all aux. variables */

        for (piv = 0.0, rs = 0, rp = 0; rs < ncl; rs++) { /* find pivot */
            f = MAT(jaca, rs, a);
            if (fabs(f) > fabs(piv)) {
                piv = f;
                rp = rs;
            }
        }
        if (piv == 0.0) {
            if (trace >= 1) {
                fprintf(trace_stream,
                        "point %ld: row reduction failed for aux. var %ld\n",
                        (long)(xs->id), (long)a);
            }
            return (FALSE);
        }

        /* remove aux. rows from Jacx, Jaca and Jacp */

        for (c = 0; c < nx; c++) {
            VEC(facx, c) = MAT(jacx, rp, c) / piv;
        }

        for (c = 0; c < na; c++) {
            VEC(faca, c) = MAT(jaca, rp, c) / piv;
        }

        for (c = 0; (jpf == TRUE) && (c < np); c++) {
            VEC(facp, c) = MAT(jacp, rp, c) / piv;
        }

        for (rd = 0, rs = 0; rd < ncl - 1; rd++, rs++) {
            if (rs == rp) { /* skip pivot row */
                rs++;
            }
            f = MAT(jaca, rs, a);
            for (c = 0; c < nx; c++) {
                MAT(jacx, rd, c) = MAT(jacx, rs, c) - f * VEC(facx, c);
            }
            for (c = 0; c < na; c++) {
                MAT(jaca, rd, c) = MAT(jaca, rs, c) - f * VEC(faca, c);
            }
            for (c = 0; (jpf == TRUE) && (c < np); c++) {
                MAT(jacp, rd, c) = MAT(jacp, rs, c) - f * VEC(facp, c);
            }
        }
        ncl--;

        if (trace >= 2) {
            pst = tm_setprint(trace_stream, 0, 80, 8, 0);
            fprintf(trace_stream, "\naux reduction (%ld):\n", (long)ncl);
            fputs("[Jx] =\n", trace_stream);
            print_matrix(pst, jacx);
            fputs("[Ja] =\n", trace_stream);
            print_matrix(pst, jaca);
            fputs("[Jp] =\n", trace_stream);
            print_matrix(pst, jacp);
            tm_endprint(pst);
        }
    }

    rank = svd(jacx_s, c_trans_l, c_scale, c_trans_x, -1.0);

    if (rank != MATM(jacx_s)) {

        if (trace >= 1) {
            fprintf(trace_stream,
                    "point %ld: decomposition failed, rank = %ld\n",
                    (long)xs->id, (long)rank);
        }
        return (FALSE);
    }

    /* copy to r and jp and scale */

    if (rf == TRUE) {
        mul_mat_vec(c_trans_x, dist, r);
    }

    if (jpf == TRUE) {
        mul_matt_mat(c_trans_l, jacp_s, jp);
        for (i = 0; i < MATM(jp); i++) {
            f = -VEC(c_scale, i);
            if (f == 0.0) {
                f = 0.0;
            } else {
                f = 1.0 / f;
            }
            for (c = 0; c < MATN(jp); c++) {
                MAT(jp, i, c) *= f;
            }
        }
    }

    if (sf == TRUE) /* transpose scale matrix */
        for (i = 0; i < MATM(s); i++) {
            f = VEC(c_scale, i);
            if (f == 0.0) {
                f = 0.0;
            } else {
                f = 1.0 / f;
            }
            for (c = 0; c < MATN(s); c++) {
                MAT(s, i, c) = f * MAT(c_trans_l, c, i);
            }
        }

    return (TRUE);
}

/* evaluate the model constraints in xm + dx, for distance determination */

boolean ext_constraints(vector dxv,  /* difference variable x */
                        vector auxv, /* auxiliary variable a */
                        boolean rf,  /* request flag */
                        vector rv,   /* model equation residual */
                        boolean jxf, /* request flag */
                        matrix jxv,  /* Jacobian x */
                        matrix jav,  /* Jacobian a */
                        inum trace   /* trace level */
) {
    if ((rf == FALSE) && (jxf == FALSE)) { /* no information needed */
        return (TRUE);
    }

    /* substitute variable x and add dx */

    copy_vector(x_ref, model_interface->x);
    copy_vector(auxv, model_interface->a);
    add_dist_x(dxv, x_scale, model_interface->x);

    if (eval_model(rf, jxf, FALSE, trace) == FALSE) {
        return (FALSE);
    }

    if (rf == TRUE) {
        copy_vector(model_interface->r, rv);
    }

    if (jxf == TRUE) {
        T_jx_jxv(model_interface->jx, x_scale, jxv);
        copy_matrix(model_interface->ja, jav);
    }

    return (TRUE);
}

/* evaluate the model constraints */

boolean eval_model(boolean rf, boolean jxf, boolean jpf, inum trace) {
    boolean rc;
    TMPRINTSTATE *pst;
    fenv_t env; /* floating point environment */

    if (trace >= 1) {
        pst = tm_setprint(trace_stream, 0, 80, 8, 0);
        fputs("\n| model call input:\n", trace_stream);
        fputs("[p] =\n", trace_stream);
        print_vector(pst, model_interface->p);
        fputs("[x] =\n", trace_stream);
        print_vector(pst, model_interface->x);
        fputs("[a] =\n", trace_stream);
        print_vector(pst, model_interface->a);
        fputs("[c] =\n", trace_stream);
        print_vector(pst, model_interface->c);
        fputs("[f] =\n", trace_stream);
        print_vector(pst, model_interface->f);
        tm_endprint(pst);
    }

    if (rf == TRUE) {
        model_calls_r++;
    }
    if (jxf == TRUE) {
        model_calls_jx++;
    }
    if (jpf == TRUE) {
        model_calls_jp++;
    }

    model_interface->rf = rf;
    model_interface->jxf = jxf;
    model_interface->jpf = jpf;

    feholdexcept(&env); /* store current environment */

    rc = (*model_code)(model_interface); /* call model */

    if (rc == FALSE) {
        if (trace > 0) {
            fputs("Model: illegal return\n\n", trace_stream);
        }
        fesetenv(&env); /* restore environment */
        return (FALSE);
    }

    if (trace >= 1) {
        fputs("| model call output:\n", trace_stream);
        pst = tm_setprint(trace_stream, 0, 80, 8, 0);
        if (model_interface->rf == TRUE) {
            fputs("[r] =\n", trace_stream);
            print_vector(pst, model_interface->r);
        }
        if (model_interface->jxf == TRUE) {
            fputs("[jx] =\n", trace_stream);
            print_boolvector(pst, model_interface->xf);
            print_matrix(pst, model_interface->jx);
            fputs("[ja] =\n", trace_stream);
            print_matrix(pst, model_interface->ja);
        }
        if (model_interface->jpf == TRUE) {
            fputs("[jp] =\n", trace_stream);
            print_boolvector(pst, model_interface->pf);
            print_matrix(pst, model_interface->jp);
        }
        tm_endprint(pst);
    }

    if ((rf != model_interface->rf) || (model_interface->jxf != jxf) ||
        (jpf != model_interface->jpf)) {

        if (trace >= 1) {
            fputs("Model: incomplete return\n", trace_stream);
        }
        fesetenv(&env); /* restore environment */
        return (FALSE);
    }

    /* test for floating point exception flags */
    if (fetestexcept(FE_DIVBYZERO | FE_OVERFLOW | FE_INVALID)) {
        if (trace >= 1) {
            fputs("Model: floating point exception\n", trace_stream);
        }
        fesetenv(&env); /* restore environment */
        return (FALSE);
    }

    if (trace >= 1) {
        fputc('\n', trace_stream);
    }

    fesetenv(&env); /* restore environment */
    return (TRUE);
}
