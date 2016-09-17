/* Copyright (c) 2004,2005 Clemson University.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details..
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

// This file requires
//    - all the wingsl header files to be in the include directory
//    - wingsl.lib, wingsl.dll to be in the link/executable directory
// Standard Blepo setup already accomplished this (via the directories in VC++ under tools -> options)

/**
	Basic Linear Algera functionality like SVD QR etc. This library is
	wrapper around WinGsl which is (modification of GNU's GSL project. for Visual C++).
	This class has some private member function which take care of chores like transformation
	from GSL matrix to gnu matrix and vice versa. 
*/

#ifndef __BLEPO_LINEARALGEBRA_H__
#define __BLEPO_LINEARALGEBRA_H__

#include "Matrix.h"

namespace blepo {

/// Singular value decomposition (mat = u * Diag(s) * Transpose(v))
/// @param 'mat' is m x n input matrix
/// @param 'u' is m x n
/// @param 's' is n x 1 (vector containing singular values)
/// @param 'v' is n x n
void Svd(const MatDbl& mat, MatDbl* u, MatDbl* s, MatDbl* v);
/// LU decomposition (p * mat = l * u)
void Lu(const MatDbl& mat, MatDbl* l, MatDbl* u, MatDbl* p);
/// Compute inverse of square matrix
void Inverse(const MatDbl& mat, MatDbl* out);
/// Compute determinant of square matrix
double Determinant(const MatDbl& mat);
/// Compute eigenvalues of square, symmetric matrix
/// (ignores upper triangular elements)
void EigenSymm(const MatDbl& mat, MatDbl* eigenvalues);
/// Compute eigenvalues and eigenvectors of square, symmetric matrix
/// (ignores upper triangular elements)
void EigenSymm(const MatDbl& mat, MatDbl* eigenvalues, MatDbl* eigenvectors);
/// QR factorization (mat = q * r)
void Qr(const MatDbl& mat, MatDbl* q, MatDbl* r);
/// Solve linear equation (Ax = b), where A is an m x n matrix, and x and b
/// are vectors.  If the system of equations is overdetermined (m > n), then
/// the least squares solution is produced.
void SolveLinear(const MatDbl& a, const MatDbl& b, MatDbl* x);

// convenient, inefficient way
MatDbl Inverse(const MatDbl& mat);

// Inverse for MatFlt by converting to and from MatDbl
MatFlt Inverse(const MatFlt& mat);

double  Determinant3x3(const MatDbl& mat);
void    Inverse3x3(const MatDbl& mat, MatDbl* out);

/// Solve linear equation (Ax = b), where A is a matrix, and
/// x and b are vectors.  These are four different implementations (using LU, 
/// QR, or SVD) and should produce the same results.  The first two are
/// restricted to square matrices, while the final two can handle overdetermined
/// systems.
void SolveLinearLuSquare(const MatDbl& a, const MatDbl& b, MatDbl* x);
void SolveLinearQrSquare(const MatDbl& a, const MatDbl& b, MatDbl* x);
void SolveLinearQr(const MatDbl& a, const MatDbl& b, MatDbl* x, MatDbl* residue);
void SolveLinearSvd(const MatDbl& a, const MatDbl& b, MatDbl* x);

// Computes Cholesky decomposition 
// input:  'a' is a square matrix
// output:  'out' is a lower triangular matrix such that Transpose(out) * (out) = a
void CholeskyDecomp(const MatDbl& a, MatDbl* out);



//  void Qr(const MatDbl& mat, MatDbl* q, MatDbl* r);
//  void SolveLinearEquation(const MatDbl& a, const MatDbl& b, MatDbl* x);
//  void SolveLinearEquation(const MatDbl& lower, const MatDbl& upper, const MatDbl& b, MatDbl* r);
//  void Eigen(const MatDbl& mat, MatDbl* eigenvalues, MatDbl* eigenvectors);

};//namespace blepo

#endif //__BLEPO_LINEARALGEBRA_H__





