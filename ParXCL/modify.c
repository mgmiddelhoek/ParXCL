/*
 * ParX - modify.c
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

#include "parx.h"
#include "vecmat.h"
#include "prob.h"
#include "objectiv.h"
#include "modify.h"

/* Probability criterion for Chi square test */
#define CHICRIT	0.5

/* MODES test if fit is close enough */

boolean proximity(
                  vector res, /* residuals */
                  vector s_val, /* singular values */
                  inum ng, /* number of equations per point */
                  inum rank, /* dimension of the parameter space */
                  opttype opt, /* optimization type */
                  fnum tol, /* required tolerance */
                  fnum *pmc, /* previous value of maximum consistency */
                  inum trace /* trace level */
) {
    inum no; /* number of data points */
    fnum ssq; /* sum of squares */
    fnum rssq; /* root of sum of squares */
    fnum chi_2; /* chi-square */
    fnum spread; /* spread */
    fnum var; /* variance */
    fnum cond; /* condition number */
    fnum consist; /* consistency of the parameter set */
    fnum maxcons; /* consistency in best direction */
    inum fr; /* degrees of freedom */
    fnum prob; /* probability */
    fnum eps; /* residual for a single point */
    inum i, j;
    
    no = VECN(res) / ng;
    
    fr = no - rank;
    
    if (fr <= 0) {
        if (trace >= 1) {
            fprintf(trace_stream, "\nInsufficient data points remaining\n");
        }
        
        return (TRUE);
    }
    
    rssq = norm_vector(res);
    ssq = rssq * rssq;
    chi_2 = ssq;
    
    if (no != 0) {
        var = ssq / no;
    } else {
        var = 0.0;
    }
    spread = sqrt(var);
    prob = chi2(chi_2, fr);
    
    cond = VEC(s_val, 0) / VEC(s_val, rank - 1);
    
    for (consist = 1.0, i = 0; i < rank; i++) {
        consist *= fabs(rssq / VEC(s_val, i));
    }
    consist = pow(consist, 1.0 / ((double) rank));
    
    if (rank >= 1) {
        maxcons = fabs(rssq / VEC(s_val, 0));
    } else {
        maxcons = 0.0;
    }
    
    if (opt != CONSIST) {
        *pmc = maxcons;
    }
    
    if (trace >= 1) {
        fprintf(trace_stream, "\nNumber of selected data points = %ld\n\n", (long) no);
        fprintf(trace_stream, "Rank        = %ld\n", (long) rank);
        fprintf(trace_stream, "Sum of sq.  = %.9e\n", ssq);
        fprintf(trace_stream, "Variance    = %.9e\n", var);
        fprintf(trace_stream, "Spread      = %.9e\n", spread);
        fprintf(trace_stream, "Chi2 prob.  = %.9e\n", prob);
        fprintf(trace_stream, "Condition   = %.9e\n", cond);
        fprintf(trace_stream, "Consistency = %.9e\n", consist);
        fprintf(trace_stream, "Max. Cons.  = %.9e\n\n", maxcons);
    }
    
    switch (opt) {
            
        case MODES:
            if (spread > 1.0) {
                return (FALSE);
            }
            return (TRUE);
            
        case STRICT:
            for (i = 0; i < VECN(res); i += ng) {
                for (j = 0, eps = 0.0; j < ng; j++) {
                    eps += VEC(res, (i + j)) * VEC(res, (i + j));
                }
                eps = sqrt(eps);
                if (eps > 1.0) {
                    return (FALSE);
                }
            }
            return (TRUE);
            
        case CHISQ:
            if (prob < CHICRIT) {
                return (FALSE);
            }
            return (TRUE);
            
        case CONSIST:
            if ((*pmc >= maxcons) || (spread > 1.0)) {
                *pmc = maxcons;
                return (FALSE);
            }
            return (TRUE);
            
        case BESTFIT:
        default:
            break;
    }
    
    return (TRUE);
}

/* modify the data set by one point */

boolean modify_point_set(
                         vector res, /* residual vector */
                         inum ng, /* number of equations per point */
                         vector sv, /* S, singular values */
                         matrix pt, /* Pt, right hand singular vectors */
                         matrix q, /* Q, left hand singular vectors */
                         inum rank, /* rank of the Jacobian matrix */
                         vector dp, /* corrected step direction */
                         fnum *dc, /* correction on objective function */
                         fnum *res_norm, /* corrected norm of the residual vector */
                         inum *npoints, /* number of data points */
                         vector wrkp, /* workspace, same dimension as dp */
                         vector wrkv, /* workspace, same dimension as res */
                         matrix wrkm, /* workspace, dimensions neq x max(ng,np) */
                         inum trace /* trace level */
) {
    inum g; /* data group */
    fnum inp; /* vector in-product */
    fnum sigma; /* square residual norm */
    fnum dsig; /* delta sigma */
    fnum dsig_max; /* maximum delta sigma */
    fnum own; /* direct delta sigma */
    inum index; /* index of point */
    inum index_max; /* index of maximum */
    inum fr; /* degrees of freedom */
    vector sub_res; /* work space */
    vector sub_wrkv; /* work space */
    matrix sub_wrkm; /* work space */
    boolean b, b0;
    inum i, j, v;
    TMPRINTSTATE *pst;
    
    fr = VECN(res) - rank;
    
    if (fr <= 0) {
        return (FALSE);
    }
    
    if (trace >= 2) {
        fputs("Data point set modification\n", trace_stream);
    }
    
    /* calculate I - (Q x Qt) in blocks of ng x ng */
    
    for (g = 0; g < MATM(q); g += ng) {
        
        for (i = 0; i < ng; i++) {
            for (j = i; j < ng; j++) {
                
                /* row_i x row_j, upto rank */
                
                inp = 0;
                for (v = 0; v < rank; v++) {
                    inp += MAT(q, g + i, v) * MAT(q, g + j, v);
                }
                
                MAT(wrkm, g + i, j) = -inp;
                
                if (j != i) { /* symmetric */
                    MAT(wrkm, g + j, i) = -inp;
                } else {
                    MAT(wrkm, g + j, i) += 1.0;
                }
            }
        }
    }
    
    /* calculate: res x (I - Q x Qt)^-1 x res, in blocks */
    
    /* package wrkm, wrkv and res in sub structures */
    
    sub_res = new_sub_vector(res);
    sub_wrkv = new_sub_vector(wrkv);
    sub_wrkm = new_sub_matrix(wrkm);
    
    b = TRUE;
    dsig_max = 0.0;
    index_max = -1;
    
    if (trace >= 2) {
        
        sigma = norm_vector(res);
        sigma *= sigma;
        
        fprintf(trace_stream, "sumsq = %.*e\n", FNUM_DIG, sigma);
    }
    
    for (g = 0, index = 0; g < MATM(q); g += ng, index++) {
        
        sub_vector(res, g, ng, sub_res);
        sub_vector(wrkv, g, ng, sub_wrkv);
        sub_matrix(wrkm, g, 0, ng, ng, sub_wrkm);
        
        b = solvesym_v(sub_wrkm, sub_wrkv, sub_res);
        
        if (b == FALSE) {
            break;
        }
        
        dsig = inp_vector(sub_res, sub_wrkv);
        own = norm_vector(sub_res);
        own *= own;
        
        if (trace >= 5) {
            fprintf(trace_stream,
                    "%c index %5ld, d_sumsq= %.*e, own = %.*e\n",
                    (dsig > dsig_max) ? '*' : ' ', (long) index,
                    FNUM_DIG, dsig, FNUM_DIG, own);
        }
        
        if (dsig > dsig_max) {
            dsig_max = dsig;
            index_max = index;
        }
    }
    
    if (index_max == -1) { /* no worst point */
        
        fre_sub_vector(sub_res);
        fre_sub_vector(sub_wrkv);
        fre_sub_matrix(sub_wrkm);
        
        return (FALSE);
    }
    
    /* predict dp after removing worst data point */
    
    g = index_max * ng;
    
    /* calculate I - (Q1 x Q1t) */
    
    for (i = 0; i < ng; i++) {
        for (j = i; j < ng; j++) {
            
            /* row_i x row_j, upto rank */
            
            inp = 0;
            for (v = 0; v < rank; v++) {
                inp += MAT(q, g + i, v) * MAT(q, g + j, v);
            }
            
            MAT(wrkm, g + i, j) = -inp;
            
            if (j != i) { /* symmetric */
                MAT(wrkm, g + j, i) = -inp;
            } else {
                MAT(wrkm, g + j, i) += 1.0;
            }
        }
    }
    
    /* calculate: (I - Q1 x Q1t)^-1 x (res1 + Q1 x D x Pt x dp) */
    
    /* package wrkm, wrkv and res in sub structures */
    
    sub_vector(res, g, ng, sub_res);
    sub_vector(wrkv, g, ng, sub_wrkv);
    sub_matrix(wrkm, g, 0, ng, ng, sub_wrkm);
    
    for (i = 0; i < rank; i++) {
        inp = 0.0;
        for (j = 0; j < VECN(dp); j++) {
            inp += MAT(pt, i, j) * VEC(dp, j);
        }
        VEC(wrkp, i) = inp * VEC(sv, i);
    }
    for (i = 0; i < ng; i++) {
        for (j = 0; j < rank; j++) {
            VEC(sub_res, i) += MAT(q, g + i, j) * VEC(wrkp, j);
        }
    }
    
    b0 = solvesym_v(sub_wrkm, sub_wrkv, sub_res);
    
    b = (b0 == FALSE) ? FALSE : b;
    
    if (b == FALSE) {
        
        fre_sub_vector(sub_res);
        fre_sub_vector(sub_wrkv);
        fre_sub_matrix(sub_wrkm);
        
        return (FALSE);
    }
    
    for (i = 0; i < rank; i++) {
        inp = 0.0;
        for (j = 0; j < ng; j++) {
            inp += MAT(q, g + j, i) * VEC(wrkv, g + j);
        }
        VEC(wrkp, i) = inp / VEC(sv, i);
    }
    
    for (i = 0; i < MATN(pt); i++) {
        inp = 0.0;
        for (j = 0; j < rank; j++) {
            inp += MAT(pt, j, i) * VEC(wrkp, j);
        }
        MAT(wrkm, 0, i) = inp;
    }
    
    for (i = 0; i < VECN(dp); i++) {
        VEC(dp, i) += (i < rank) ? MAT(wrkm, 0, i) : 0.0;
        VEC(wrkp, i) = (i < rank) ? MAT(wrkm, 0, i) : 0.0;
    }
    
    if (trace >= 2) {
        pst = tm_setprint(trace_stream, 0, 80, 8, 0);
        fputs("step correction:\n", trace_stream);
        print_vector(pst, wrkp);
        fputs("predicted step direction:\n", trace_stream);
        print_vector(pst, dp);
        tm_endprint(pst);
    }
    
    /* correct residual norm */
    
    zero_vector(sub_res);
    
    *res_norm = norm_vector(res);
    
    *dc = dsig_max;
    
    *npoints = remove_data_point(index_max, UGROUP, trace);
    
    fre_sub_vector(sub_res);
    fre_sub_vector(sub_wrkv);
    fre_sub_matrix(sub_wrkm);
    
    return (TRUE);
}
