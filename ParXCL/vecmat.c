/*
 * ParX - vecmat.c
 * Miscelenious functions on Vector and Matrix types
 * Interface to BLAS and LAPACK
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

#if defined(LINUX) || defined(MSDOS) || defined(WINDOWS) || defined(WIN32) ||  \
    defined(WIN64)
#include "clapack.h"
#include <cblas.h>
#elif defined(OSX)
#include <Accelerate/Accelerate.h>
#endif

/*
 * IMPORTANT:
 * Make sure that the inum and fnum types that are defined in primtype.ht
 * match de integer and double types used by BLAS and LAPACK
 */

/* manage workspace */

#define FWORK_MIN 4096
#define IWORK_MIN 4096

static vector fnum_wrk;     /* global fnum workspace */
static inumvector inum_wrk; /* global inum workspace */

void new_vecmat(inum m, inum n) {
    inum sizef, sizei;
    inum size_svd; /* size required by svd */
    inum size_crt; /* size required by crout */

    size_svd = MAX(3 * MIN(m, n) + MAX(m, n), 5 * MIN(m, n) - 4);

    sizef = MAX(FWORK_MIN, size_svd);
    fnum_wrk = rnew_vector(sizef);

    size_crt = MAX(m, n);

    sizei = MAX(IWORK_MIN, size_crt);
    inum_wrk = rnew_inumvector(sizei);
}

void fre_vecmat(void) {
    rfre_vector(fnum_wrk);
    rfre_inumvector(inum_wrk);
}

/* vector & matrix copy functions */

void copy_vector(vector va, vector vb) {
    inum i;

#ifdef RANGECHECK
    assert(VECN(va) <= VECN(vb));
#endif

    VECN(vb) = VECN(va);
    for (i = 0; i < VECN(va); i++) {
        VEC(vb, i) = VEC(va, i);
    }
}

void copy_matrix(matrix ma, matrix mb) {
    inum c, r;

#ifdef RANGECHECK
    assert(MATM(ma) <= MATM(mb));
    assert(MATN(ma) <= MATN(mb));
#endif

    MATM(mb) = MATM(ma);
    MATN(mb) = MATN(ma);
    for (c = 0; c < MATN(ma); c++) {
        for (r = 0; r < MATM(ma); r++) {
            MAT(mb, r, c) = MAT(ma, r, c);
        }
    }
}

void copy_vec_col(vector v, matrix m, inum c) {
    inum i;

#ifdef RANGECHECK
    assert(VECN(v) == MATM(m));
#endif

    for (i = 0; i < VECN(v); i++) {
        MAT(m, i, c) = VEC(v, i);
    }
}

void copy_vec_row(vector v, matrix m, inum r) {
    inum i;

#ifdef RANGECHECK
    assert(VECN(v) == MATN(m));
#endif

    for (i = 0; i < VECN(v); i++) {
        MAT(m, r, i) = VEC(v, i);
    }
}

void copy_col_vec(matrix m, inum c, vector v) {
    inum i;

#ifdef RANGECHECK
    assert(VECN(v) == MATM(m));
#endif

    for (i = 0; i < MATM(m); i++) {
        VEC(v, i) = MAT(m, i, c);
    }
}

void copy_row_vec(matrix m, inum r, vector v) {
    inum i;

#ifdef RANGECHECK
    assert(VECN(v) == MATN(m));
#endif

    for (i = 0; i < MATN(m); i++) {
        VEC(v, i) = MAT(m, r, i);
    }
}

/************** vector and matrix arithmetic functions *****************/

/* zero vector contents */

void zero_vector(vector v) {
    inum i;

    for (i = 0; i < VECN(v); i++) {
        VEC(v, i) = 0.0;
    }
}

/* transpose a matrix */

void trans_matrix(matrix a, matrix at) {
    inum r, c;

#ifdef RANGECHECK
    assert(MATN(a) == MATM(at));
    assert(MATN(at) == MATM(a));
#endif

    for (c = 0; c < MATN(a); c++) {
        for (r = 0; r < MATM(a); r++) {
            MAT(at, c, r) = MAT(a, r, c);
        }
    }
}

/* calculate the norm of a vector */

fnum norm_vector(vector v) {
    fnum norm;
    inum n, incx;
    fnum *dx;

    if (VECN(v) == 1) { /* quick return */
        return (fabs(VEC(v, 0)));
    }

    n = VECN(v);
    dx = VECA(v);
    incx = 1;
    norm = cblas_dnrm2(n, dx, incx);
    return (norm);
}

/* inner product of two vectors */

fnum inp_vector(vector a, vector b) {
    inum n, incx, incy;
    fnum *dx, *dy;
    fnum inpr;

#ifdef RANGECHECK
    assert(VECN(a) == VECN(b));
#endif

    if (VECN(a) == 1) { /* quick return */
        return (VEC(a, 0) * VEC(b, 0));
    }

    n = VECN(a);
    dx = VECA(a);
    dy = VECA(b);
    incx = incy = 1;
    inpr = cblas_ddot(n, dx, incx, dy, incy);
    return (inpr);
}

/* multiply a vector by a matrix, c = A x b */

void mul_mat_vec(matrix a, vector b, vector c) {
    enum CBLAS_ORDER order;
    enum CBLAS_TRANSPOSE trans;
    inum m, n, lda, incx, incy;
    fnum *fa, *fx, *fy, alpha, beta;

#ifdef RANGECHECK
    assert(MATN(a) == VECN(b));
    assert(MATM(a) == VECN(c));
#endif

    order = CblasColMajor;
    trans = CblasNoTrans;
    fa = MATA(a);
    m = MATM(a);
    n = MATN(a);
    lda = MATSM(a);
    fx = VECA(b);
    fy = VECA(c);
    incx = 1;
    incy = 1;
    alpha = 1.0;
    beta = 0.0;

    (void)cblas_dgemv(order, trans, m, n, alpha, fa, lda, fx, incx, beta, fy,
                      incy);
}

/* multiply a vector by the transposed of a matrix, c = A_t x b */

void mul_matt_vec(matrix a, vector b, vector c) {
    enum CBLAS_ORDER order;
    enum CBLAS_TRANSPOSE trans;
    inum m, n, lda, incx, incy;
    fnum *fa, *fx, *fy, alpha, beta;

#ifdef RANGECHECK
    assert(MATM(a) == VECN(b));
    assert(MATN(a) == VECN(c));
#endif

    order = CblasColMajor;
    trans = CblasTrans;
    fa = MATA(a);
    m = MATM(a);
    n = MATN(a);
    lda = MATSM(a);
    fx = VECA(b);
    fy = VECA(c);
    incx = 1;
    incy = 1;
    alpha = 1.0;
    beta = 0.0;

    (void)cblas_dgemv(order, trans, m, n, alpha, fa, lda, fx, incx, beta, fy,
                      incy);
}

/* multiply a matrix with a matrix, C = A x B */

void mul_mat_mat(matrix a, matrix b, matrix c) {
    enum CBLAS_ORDER order;
    enum CBLAS_TRANSPOSE transa, transb;
    inum m, n, k, lda, ldb, ldc;
    fnum *fa, *fb, *fc, alpha, beta;

#ifdef RANGECHECK
    assert(MATN(a) == MATM(b));
    assert(MATM(a) == MATM(c));
    assert(MATN(b) == MATN(c));
#endif

    order = CblasColMajor;
    transa = transb = CblasNoTrans;
    fa = MATA(a);
    fb = MATA(b);
    fc = MATA(c);
    m = MATM(c);
    n = MATN(c);
    k = MATM(b);
    lda = MATSM(a);
    ldb = MATSM(b);
    ldc = MATSM(c);
    alpha = 1.0;
    beta = 0.0;

    (void)cblas_dgemm(order, transa, transb, m, n, k, alpha, fa, lda, fb, ldb,
                      beta, fc, ldc);
}

/* multiply a matrix with the transposed of a matrix, C = A_t x B */

void mul_matt_mat(matrix a, matrix b, matrix c) {
    enum CBLAS_ORDER order;
    enum CBLAS_TRANSPOSE transa, transb;
    inum m, n, k, lda, ldb, ldc;
    fnum *fa, *fb, *fc, alpha, beta;

#ifdef RANGECHECK
    assert(MATM(a) == MATM(b));
    assert(MATN(a) == MATM(c));
    assert(MATN(b) == MATN(c));
#endif

    order = CblasColMajor;
    transa = CblasTrans;
    transb = CblasNoTrans;
    fa = MATA(a);
    fb = MATA(b);
    fc = MATA(c);
    m = MATM(c);
    n = MATN(c);
    k = MATM(b);
    lda = MATSM(a);
    ldb = MATSM(b);
    ldc = MATSM(c);
    alpha = 1.0;
    beta = 0.0;

    (void)cblas_dgemm(order, transa, transb, m, n, k, alpha, fa, lda, fb, ldb,
                      beta, fc, ldc);
}

/* singular value decomposition and matrix transformation, return rank */

inum svd(matrix a,  /* input matrix */
         matrix u,  /* A = U . D . Vt , may be matrixNIL or A */
         vector s,  /* s = diagonal of D */
         matrix vt, /* may be matrixNIL or A */
         fnum tol) /* tolerance for s = 0, if < 0 then use machine prec. */ {
    char jobu, jobvt;
    inum m, n, lda, ldu, ldvt, info;
    fnum *fa, *fs, *fu, *fvt;
    fnum *fwork;
    inum lwork;

    inum i, rank;

    if (a == matrixNIL) {
        return (FAIL);
    }
    m = MATM(a);
    n = MATN(a);
    lda = MATSM(a);
    fa = MATA(a);

    if (s == vectorNIL) {
        return (FAIL);
    }
    fs = VECA(s);
    if (u == vt) {
        return (FAIL);
    }

    if (u == matrixNIL) { /* U not required */
        jobu = 'N';
        ldu = 1;
        fu = NULL;
    } else {
        if (u == a) { /* return U in A */
            jobu = 'O';
            ldu = 1;
            fu = NULL;
        } else {
            jobu = 'S';
            ldu = MATSM(u);
            fu = MATA(u);
        }
    }

    if (vt == matrixNIL) { /* Vt not required */
        jobvt = 'N';
        ldvt = 1;
        fvt = NULL;
    } else {
        if (vt == a) { /* return Vt in A */
            jobvt = 'O';
            ldvt = 1;
            fvt = NULL;
        } else {
            jobvt = 'S';
            ldvt = MATSM(vt);
            fvt = MATA(vt);
        }
    }

    lwork = VECN(fnum_wrk);
    fwork = VECA(fnum_wrk);

    info = 0;

    (void)dgesvd_(&jobu, &jobvt, &m, &n, fa, &lda, fs, fu, &ldu, fvt, &ldvt,
                  fwork, &lwork, &info);

    if (info != 0) {
        return (FAIL);
    }

    /* determine rank */

    if (VECN(s) == 1) {
        rank = (VEC(s, 0) == 0.0) ? 0 : 1;
    } else {
        tol = (tol <= 0.0) ? FNUM_EPS : tol;
        tol *= fabs(VEC(s, 0));

        for (rank = 0, i = 0; i < VECN(s); i++) {
            if (fabs(VEC(s, i)) < tol) {
                break;
            }
            rank++;
        }
    }

    return (rank);
}

/* solve linear set of equations: A x = b , for a general matrix */

boolean crout(matrix a, vector x, vector b) {
    inum n, nrhs, lda, ldb, info;
    fnum *fa, *fb;
    inum *ipiv;
    fnum m;

#ifdef RANGECHECK
    assert(MATM(a) == MATN(a));
    assert(MATN(a) == VECN(b));
    assert(MATN(a) == VECN(x));
#endif

    if (VECN(x) == 1) { /* use short cut */
        if ((m = MAT(a, 0, 0)) == 0.0) {
            return (FALSE);
        }
        VEC(x, 0) = VEC(b, 0) / m;
        return (TRUE);
    }

    if (b != x) {
        copy_vector(b, x);
    }

    n = VECN(b);
    nrhs = 1;
    fa = MATA(a);
    lda = MATSM(a);
    fb = VECA(x);
    ldb = n;

    if (VECN(inum_wrk) < n) { /* not enough workspace */
        return (FALSE);
    }
    ipiv = VECA(inum_wrk);

    info = 0;

    (void)dgesv_(&n, &nrhs, fa, &lda, ipiv, fb, &ldb, &info);

    if (info != 0) {
        return (FALSE);
    }

    return (TRUE);
}

/* solve linear set of equations: A x = b , for a symmetric matrix */

boolean solvesym_v(matrix a, vector x, vector b) {
    char uplo;
    inum n, nrhs, lda, ldb, info;
    fnum *fa, *fb, *work;
    inum *ipiv, lwork;

    fnum m;

#ifdef RANGECHECK
    assert(MATM(a) == MATN(a));
    assert(MATN(a) == VECN(b));
    assert(MATN(a) == VECN(x));
#endif

    if (VECN(x) == 1) { /* use short cut */
        if ((m = MAT(a, 0, 0)) == 0.0) {
            return (FALSE);
        }
        VEC(x, 0) = VEC(b, 0) / m;
        return (TRUE);
    }

    if (b != x) {
        copy_vector(b, x);
    }

    uplo = 'U';
    n = VECN(b);
    nrhs = 1;
    fa = MATA(a);
    lda = MATSM(a);
    fb = VECA(x);
    ldb = n;

    if (VECN(inum_wrk) < n) { /* not enough workspace */
        return (FALSE);
    }
    ipiv = VECA(inum_wrk);
    work = VECA(fnum_wrk);
    lwork = VECN(fnum_wrk);

    info = 0;

    (void)dsysv_(&uplo, &n, &nrhs, fa, &lda, ipiv, fb, &ldb, work, &lwork,
                 &info);

    if (info != 0) {
        return (FALSE);
    }

    return (TRUE);
}

/* solve linear set of equations: A X = B , for a symmetric matrix */

boolean solvesym_m(matrix a, matrix x, matrix b) {
    char uplo;
    inum n, nrhs, lda, ldb, info;
    fnum *fa, *fb, *work;
    inum *ipiv, lwork;

    fnum m;
    inum i;

#ifdef RANGECHECK
    assert(MATM(a) == MATN(a));
    assert(MATM(a) == MATM(b));
    assert(MATM(a) == MATM(x));
    assert(MATN(x) == MATN(b));
#endif

    if (MATM(a) == 1) { /* use short cut */
        if ((m = MAT(a, 0, 0)) == 0.0) {
            return (FALSE);
        }
        for (i = 0; i < MATN(x); i++) {
            MAT(x, 0, i) = MAT(b, 0, i) / m;
        }
        return (TRUE);
    }

    if (b != x) {
        copy_matrix(b, x);
    }

    uplo = 'U';
    n = MATM(a);
    nrhs = MATN(x);
    fa = MATA(a);
    lda = MATSM(a);
    fb = MATA(x);
    ldb = MATSM(x);

    if (VECN(inum_wrk) < n) { /* not enough workspace */
        return (FALSE);
    }
    ipiv = VECA(inum_wrk);
    work = VECA(fnum_wrk);
    lwork = VECN(fnum_wrk);

    info = 0;

    (void)dsysv_(&uplo, &n, &nrhs, fa, &lda, ipiv, fb, &ldb, work, &lwork,
                 &info);

    if (info != 0) {
        return (FALSE);
    }

    return (TRUE);
}

/* solve linear set of equations: A X = B , for a positive def. matrix */

boolean cholesky(matrix a, matrix x, matrix b) {
    char uplo;
    inum n, nrhs, lda, ldb, info;
    fnum *fa, *fb;

    fnum m;
    inum i;

#ifdef RANGECHECK
    assert(MATM(a) == MATN(a));
    assert(MATM(a) == MATM(b));
    assert(MATM(a) == MATM(x));
    assert(MATN(x) == MATN(b));
#endif

    if (MATM(a) == 1) {
        if ((m = MAT(a, 0, 0)) == 0.0) {
            return (FALSE);
        }
        for (i = 0; i < MATN(x); i++) {
            MAT(x, 0, i) = MAT(b, 0, i) / m;
        }
        return (TRUE);
    }

    if (b != x) {
        copy_matrix(b, x);
    }

    uplo = 'U';
    n = MATM(a);
    nrhs = MATN(x);
    fa = MATA(a);
    lda = MATSM(a);
    fb = MATA(x);
    ldb = MATSM(x);

    info = 0;

    (void)dposv_(&uplo, &n, &nrhs, fa, &lda, fb, &ldb, &info);

    if (info != 0) {
        return (FALSE);
    }

    return (TRUE);
}
