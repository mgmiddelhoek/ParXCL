/*
 * ParX - vecmat.h
 * Miscelenious functions on Vector and Matrix types
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

#ifndef __VECMAT_H
#define __VECMAT_H

#include "primtype.h"

/*********** manage workspace for vector and matrix functions **********/

extern void new_vecmat(inum m, inum n);
extern void fre_vecmat(void);

/*********** miscelenious functions on vector and matrix types *********/

extern void copy_vector(vector va, vector vb);
extern void copy_matrix(matrix ma, matrix mb);
extern void copy_vec_col(vector v, matrix m, inum c);
extern void copy_vec_row(vector v, matrix m, inum r);
extern void copy_col_vec(matrix m, inum c, vector v);
extern void copy_row_vec(matrix m, inum r, vector v);

/************** vector and matrix arithmetic functions *****************/

extern void zero_vector(vector v);
extern void trans_matrix(matrix a, matrix at);
extern fnum norm_vector(vector v);
extern fnum inp_vector(vector a, vector b);
extern void mul_mat_vec(matrix a, vector b, vector c);
extern void mul_matt_vec(matrix a, vector b, vector c);
extern void mul_mat_mat(matrix a, matrix b, matrix c);
extern void mul_matt_mat(matrix a, matrix b, matrix c);
extern inum svd(matrix a, matrix u, vector s, matrix vt, fnum tol);
extern boolean crout(matrix a, vector x, vector b);
extern boolean solvesym_v(matrix a, vector x, vector b);
extern boolean solvesym_m(matrix a, matrix x, matrix b);
extern boolean cholesky(matrix a, matrix x, matrix b);

#endif
