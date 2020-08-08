/*
 * ParX - objectiv.c
 * Objective function for MODES
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

#include "error.h"
#include "objectiv.h"
#include "parx.h"
#include "residual.h"
#include "vecmat.h"

/**************************** global variables *************************/

static xgroup xg_in;   /* active measurements group */
static xgroup xg_out;  /* inactive measurements group */
static xgroup xg_fail; /* failed measurement group */
static xset *xsindex;  /* reverse index table to xset's */
static inum np;        /* number of parameters */
static inum nr;        /* number of residuals */
static inum neq;       /* number of equations */

static vector m_res;  /* memory for residual vector */
static matrix m_jacp; /* memory for Jacobian matrix */
static vector res;    /* residual vector */
static matrix jacp;   /* Jacobian matrix */

/***********************************************************************/

boolean objective(vector p,       /* parameter values */
                  boolean rf,     /* residual flag */
                  vector *rp,     /* pointer to residual vector */
                  boolean jf,     /* Jacobian flag */
                  matrix *jpp,    /* pointer to Jacobian matrix */
                  boolean modify, /* allow modify point set */
                  boolean all,    /* evaluate objective for all points */
                  inum *npoints,  /* remaining number of data points */
                  inum *mc_r,  /* total number of model residual evaluations */
                  inum *mc_jx, /* total number of model Jx evaluations */
                  inum *mc_jp, /* total number of model Jp evaluations */
                  inum trace   /* trace level */
) {
    xset xs;     /* current measurement set */
    vector subr; /* sub residual vector */
    matrix subj; /* sub Jacobian matrix */
    matrix s;    /* scaling matrix */
    boolean sf;  /* scaling matrix flag */

    inum lmc_r;  /* local number of model residual evaluations */
    inum lmc_jx; /* local number of model Jx evaluations */
    inum lmc_jp; /* local number of model Jp evaluations */

    inum xi, xn; /* point counter */
    boolean ok;
    inum grp;
    inum i;
    TMPRINTSTATE *pst;

    if ((rf == FALSE) && (jf == FALSE)) { /* easy question */
        return (TRUE);
    }

    if (trace >= 1) {
        fputs("\nobjective function evaluation.\nparameters:\n", trace_stream);
        pst = tm_setprint(trace_stream, 0, 80, 8, 0);
        print_vector(pst, p);
        tm_endprint(pst);
        fputc('\n', trace_stream);
    }

    lmc_r = lmc_jx = lmc_jp = 0; /* reset counters */

    neq = nr * xg_in->n; /* total number of equations */

    if (all == TRUE) {
        neq += nr * xg_out->n;
        neq += nr * xg_fail->n;
    }

    subr = vectorNIL;

    if (rf == TRUE) { /* allocate sub residual vector */
        sub_vector(m_res, 0, neq, res);
        *rp = res;
        subr = new_sub_vector(res);
    }

    subj = matrixNIL;

    if (jf == TRUE) { /* allocate sub Jacobian matrix */
        sub_matrix(m_jacp, 0, 0, neq, np, jacp);
        *jpp = jacp;
        subj = new_sub_matrix(jacp);
    }

    sf = FALSE;
    s = matrixNIL;

    if (trace >= 3) { /* return scaling matrix for debugging */
        sf = TRUE;
        s = rnew_matrix(nr, nr);
    }

    ok = TRUE;
    xn = xg_in->n;

    for (grp = 0; grp <= (all == TRUE ? 2 : 0); grp++) {

        switch (grp) {
        case 0:
            xs = xg_in->g;
            break;
        case 1:
            xs = xg_out->g;
            break;
        default:
            xs = xg_fail->g;
            break;
        }

        for (xi = 0, i = 0; xs != xsetNIL;) {

            if (trace >= 2) {
                fprintf(trace_stream, "point %ld of %ld\n", (long)(xi + 1),
                        (long)xn);
                pst = tm_setprint(trace_stream, 0, 80, 8, 0);
                print_xset(pst, xs);
                tm_endprint(pst);
            }

            if (rf == TRUE) { /* set sub residual */
                sub_vector(res, i, nr, subr);
            }

            if (jf == TRUE) { /* set sub Jacobian */
                sub_matrix(jacp, i, 0, nr, np, subj);
            }

            *(xsindex + xi) = xs; /* setup xsindex to point */

            ok = residual(xs, p, rf, subr, jf, subj, sf, s, &lmc_r, &lmc_jx,
                          &lmc_jp, trace - 2);

            if (ok == FALSE) { /* failed */

                xs->res = -1.0; /* no valid residual */

                if (trace >= 2) {
                    fputs("residual calculation failed\n", trace_stream);
                }

                if (all == TRUE) { /* ignore point and go on */
                    xs = xs->next;
                    ok = TRUE;
                    continue;
                }

                if (modify == FALSE) { /* not allowed to remove failed point */
                    xn = xi;
                    break;
                }

                /* remove failed data point */

                xs = xs->next;
                xn = remove_data_point(xi, FGROUP, trace - 1);
                ok = TRUE;

                continue; /* next point */
            }

            /* return norm of residual vector in xset */
            xs->res = norm_vector(subr);

            xs = xs->next;
            xi++;
            i += nr; /* next point */

            if (trace >= 2) {
                fputs("residual:\n", trace_stream);
                pst = tm_setprint(trace_stream, 0, 80, 8, 0);

                if (rf == TRUE) {
                    fputs("[d]:\n", trace_stream);
                    print_vector(pst, subr);
                }
                if (jf == TRUE) {
                    fputs("[Jp]:\n", trace_stream);
                    print_matrix(pst, subj);
                }
                if (sf == TRUE) {
                    fputs("[S]:\n", trace_stream);
                    print_matrix(pst, s);
                }
                tm_endprint(pst);
                fputc('\n', trace_stream);
            }
        }
    }

    /* format result */

    *npoints = xn;
    neq = nr * xn; /* total number of equations */

    if (rf == TRUE) {
        sub_vector(m_res, 0, neq, res);
        *rp = res;

        fre_sub_vector(subr);
    }

    if (jf == TRUE) {
        sub_matrix(m_jacp, 0, 0, neq, np, jacp);
        *jpp = jacp;

        fre_sub_matrix(subj);
    }

    if (sf == TRUE) {
        rfre_matrix(s);
    }

    /* add evaluation count */

    *mc_r += lmc_r;
    *mc_jx += lmc_jx;
    *mc_jp += lmc_jp;

    if (trace >= 1) {

        fputc('\n', trace_stream);
        pst = tm_setprint(trace_stream, 0, 80, 8, 0);

        if (rf == TRUE) {
            fputs("[residuals] =\n", trace_stream);
            print_vector(pst, *rp);
        }
        if (jf == TRUE) {
            fputs("[Jacobian_p] =\n", trace_stream);
            print_matrix(pst, *jpp);
        }
        tm_endprint(pst);

        fprintf(trace_stream, "model evaluations: f %ld, Jx %ld, Jp %ld\n\n",
                (long)lmc_r, (long)lmc_jx, (long)lmc_jp);
    }

    return (ok);
}

boolean new_objective(numblock numb, /* numerical data block */
                      fnum prec,     /* relative precision */
                      fnum tol,      /* modes tolerance factor */
                      inum *maxeq,   /* maximum number of equations */
                      inum *ngroup   /* number of equations per point */
) {
    /* test if data is available */

    if (numb->x->n == 0) {
        errcode = NO_DATA_SERR;
        error("ext");
        return (FALSE);
    }

    /* setup the model constants and flags */

    copy_vector(numb->c->val, numb->modi->c);
    copy_vector(numb->f->val, numb->modi->f);

    /* setup residual function */

    if (new_residual(numb->mod, numb->modi, prec, tol, numb->a->val, &np) ==
        FALSE) {
        return (FALSE);
    }

    xg_in = numb->x;
    xg_out = new_xgroup(UGROUP, 0, new_xset_list());
    xg_fail = new_xgroup(FGROUP, 0, new_xset_list());

    xsindex = TM_MALLOC(xset *, xg_in->n * sizeof(xset));

    nr = numb->mod->nr - numb->mod->na; /* number of residuals per point */
    neq = nr * xg_in->n;                /* total number of equations */

    m_res = rnew_vector(neq);
    res = new_sub_vector(m_res);

    m_jacp = rnew_matrix(neq, np);
    jacp = new_sub_matrix(m_jacp);

    *maxeq = neq;
    *ngroup = nr;

    /* setup vecmat library */

    new_vecmat(neq, np); /* maximum problem size */

    return (TRUE);
}

void fre_objective(numblock numb) {
    TM_FREE(xsindex);

    fre_sub_vector(res);
    rfre_vector(m_res);

    fre_sub_matrix(jacp);
    rfre_matrix(m_jacp);

    if (xg_in->n != 0)
        numb->x = xg_in;

    if (xg_out->n != 0) {
        numb->x = append_xgroup_list(numb->x, xg_out);
    } else {
        rfre_xgroup(xg_out);
    }

    if (xg_fail->n != 0) {
        numb->x = append_xgroup_list(numb->x, xg_fail);
    } else {
        rfre_xgroup(xg_fail);
    }

    fre_residual();

    fre_vecmat();
}

/* move a data point from the active to an inactive group */

inum remove_data_point(inum n,    /* xsindex of point to be moved */
                       inum g,    /* target group */
                       inum trace /* trace level */
) {
    TMPRINTSTATE *pst;

    if (n == 0) { /* first point in the list */
        xg_in->g = (xsindex[n])->next;
    } else {
        (xsindex[n - 1])->next = (xsindex[n])->next;
    }

    xg_in->n--;

    switch (g) {
    case UGROUP: /* move point to unselected group */
        (xsindex[n])->next = xg_out->g;
        xg_out->g = xsindex[n];
        xg_out->n++;
        break;
    case FGROUP: /* move point to failed group */
        (xsindex[n])->next = xg_fail->g;
        xg_fail->g = xsindex[n];
        xg_fail->n++;
        break;
    }

    if (trace >= 1) {
        fprintf(trace_stream, "Removing data point: %ld\n",
                (long)(xsindex[n]->id));
        pst = tm_setprint(trace_stream, 0, 80, 8, 0);
        print_vector(pst, xsindex[n]->val);
        tm_endprint(pst);
    }

    return (xg_in->n);
}
