/*
 * ParX - clapack.h
 * Interface to LAPACK
 *
 * Copyright (c) 2012 M.G.Middelhoek <martin@middelhoek.com>
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

#ifndef LAPACK_H
#define LAPACK_H

#include "primtype.h"

#ifdef __cplusplus
extern "C" {
#endif

inum dgesvd_(char *jobu, char *jobvt, inum *m, inum *n, fnum *a, inum *lda,
             fnum *s, fnum *u, inum *ldu, fnum *vt, inum *ldvt, fnum *work,
             inum *lwork, inum *info);

inum dgesv_(inum *n, inum *nrhs, fnum *a, inum *lda, inum *ipiv, fnum *b,
            inum *ldb, inum *info);

inum dsysv_(char *uplo, inum *n, inum *nrhs, fnum *a, inum *lda, inum *ipiv,
            fnum *b, inum *ldb, fnum *work, inum *lwork, inum *info);

inum dposv_(char *uplo, inum *n, inum *nrhs, fnum *a, inum *lda, fnum *b,
            inum *ldb, inum *info);

#ifdef __cplusplus
}
#endif

#endif /* LAPACK_H */
