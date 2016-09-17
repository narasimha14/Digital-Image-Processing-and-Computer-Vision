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


#include "LinearAlgebra.h"
#include "MatrixOperations.h"
#include "../../external/WinGsl/WinGsl.h"
#include "Utilities/Math.h"  // blepo_ex::Min()
#include <vector>
#include <stdio.h>  // FILE, fopen, ...


// -------------------- all includes must go before these lines ------------------
#if defined(DEBUG) && defined(WIN32) && !defined(NO_MFC)
#include <afxwin.h>
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
// -------------------- all code must go after these lines -----------------------


// ================> begin local functions (available only to this translation unit)
namespace
{
unsigned int* iConstIteratorToPtr(std::vector<unsigned int>::const_iterator t)
{
	const unsigned int& a = *t;
	return const_cast<unsigned int*>( &a );
}

using namespace blepo;

// Note:  gsl_matrix structure has six elements:
//
// typedef struct 
//    {
//      size_t size1;  // number of rows
//      size_t size2;  // number of columns
//      size_t tda;    // size of row as laid out in memory 
//                        (this allows submatrices; set it equal to size2 if entire matrix is used)
//      double * data;  // pointer to actual data
//      gsl_block * block;  // pointer to block that contains data, if any
//      int owner;  // 1 means deallocate block when struct is deallocated; 0 otherwise
//    } gsl_matrix;

// Note:  constness is not as enforced as it could be.  To better enforce constness, split
// this class into two:  one called GslMat and the other called ConstGslMat.  Each should have
// only one constructor:  GslMat(MatDbl* mat) and ConstGslMat(const MatDbl* mat).  Moreover,
// ConstGslMat should not have an operator gsl_matrix*() conversion but rather only the const
// version.  GslMat, on the other hand, can safely have both.  Note, however, that the const_cast
// is unavoidable no matter what we do; it arises from the design of GSL's gsl_matrix itself.
// See the discussion in the manual.
struct GslMat // : public gsl_matrix
{
public:
  GslMat(blepo::MatDbl* mat) { Setup(mat); }
  GslMat(const blepo::MatDbl* mat) { Setup(mat); }
  operator const gsl_matrix*() const { return &m_mat; }
  operator gsl_matrix*() { return &m_mat; }

private:
  void Setup(const blepo::MatDbl* mat)
  {
    m_mat.size1 = mat->Height();
    m_mat.size2 = mat->Width();
    m_mat.tda = mat->Width();
    m_mat.data = const_cast<double*>( mat->Begin() );
    m_mat.block = &m_block;
    m_mat.owner = 0;
    m_block.size = m_mat.size1 * m_mat.size2;
    m_block.data = m_mat.data;
  }
  gsl_matrix m_mat;
  gsl_block m_block;
};

// See comments regarding constness for GslMat above.
struct GslVec //: public gsl_vector
{
public:
  GslVec(blepo::MatDbl* mat) { Setup(mat); }
  GslVec(const blepo::MatDbl* mat) { Setup(mat); }
  operator const gsl_vector*() const { return &m_vec; }
  operator gsl_vector*() { return &m_vec; }

private:
  void Setup(const blepo::MatDbl* mat) 
  {
    assert(mat->Width() == 1);
    m_vec.size = mat->Height();
    m_vec.stride = 1;
    m_vec.data = const_cast<double*>( mat->Begin() );
    m_vec.block = &m_block;
    m_vec.owner = 0;
    m_block.size = m_vec.size;
    m_block.data = m_vec.data;
  }
  gsl_vector m_vec;
  gsl_block m_block;
};

/**
  Computes LU decomposition of a square matrix.
  'lu' is both input and output (n x n); for output, the
    diagonal and upper triangular part contain U, while the
    lower part contains L (with implicit ones on the diagonal)
  'perm' is output, holds the permutation matrix in compact form
    PA = LU, where A is the original matrix
  'sign' is output, holds the sign of the permutation.
*/
void gslLu(MatDbl* lu, std::vector<unsigned int>* perm, int* sign)
{
  const int m = lu->Height(), n = lu->Width();
  if (m != n)  BLEPO_ERROR("Matrix must be square for LU decomposition");
  perm->resize(m);
  gsl_permutation p;
  GslMat glu(lu);
  p.size = m;
  p.data = iConstIteratorToPtr( perm->begin() );
  gsl_linalg_LU_decomp(glu, &p, sign);
}

/** 
  Unpacks the L, U, and P matrices from gslLu result
*/
void gslLuUnpack(const MatDbl& lu, const std::vector<unsigned int>& perm, MatDbl* ell, MatDbl* ewe, MatDbl* pee)
{
  const int m = lu.Height(), n = lu.Width();
  assert(m == n);
  assert(m == perm.size());
  // convert L and U
  ell->Reset(m, m);
  ewe->Reset(m, m);
  MatDbl::ConstIterator ii = lu.Begin();
  MatDbl::Iterator ll = ell->Begin();
  MatDbl::Iterator uu = ewe->Begin();
  int x, y;
  for (y=0 ; y<m ; y++)
  {
    for (x=0 ; x<m ; x++)
    {
      if (x > y)
      {
        *ll++ = 0;
        *uu++ = *ii++;
      }
      else if (x == y)
      {
        *ll++ = 1;
        *uu++ = *ii++;
      }
      else
      {
        *ll++ = *ii++;
        *uu++ = 0;
      }
    }
  }
  // convert P
  // (Note:  There appears to be a typo in the GSL manual, where it says (linalg.texi), 
  //  "The @math{j}-th column of the matrix @math{P} is given by the
  //  @math{k}-th column of the identity matrix, where @math{k = p_j} the
  //  @math{j}-th element of the permutation vector."  The first instance of "column"
  //  should in fact be "row".)
  pee->Reset(m, m);
  Set(pee, 0);
  for (int i=0 ; i<m ; i++) (*pee)(perm[i], i) = 1;
}

/**
  Solves linear system Ax=b using prior LU decomposition of the square matrix A.
  'lu' is the LU decomposition of A, and 'perm' is the resulting permutation matrix,
  computed prior to calling this function.  'b' is the vector on the rhs of the equation.
  'x' is the output.
*/
void gslLuSolve(const MatDbl& lu, const std::vector<unsigned int>& perm, const MatDbl& b, MatDbl* x)
{
  const int m = lu.Height(), n = lu.Width();
  assert(m == n);
  assert(m == perm.size());
  x->Reset(m);
  gsl_permutation p;
  GslMat glu(&lu);
  GslVec gx(x), gb(&b);
  p.size = m;
  p.data = iConstIteratorToPtr(  perm.begin() );
  gsl_linalg_LU_solve(glu, &p, gb, gx);

//  const int rows=10, cols=10;
//  const double* m_data = lu.Begin();
//  gsl_permutation p;
//  gsl_block block;
//  gsl_matrix gmat;
//  gsl_vector gx, gb;
//  gmat.size1 = rows;
//  gmat.size2 = cols;
//  gmat.tda = cols;
//  gmat.data = const_cast<double*>(m_data);
//  gmat.block = &block;
//  gmat.owner = 0;
//  gsl_linalg_LU_solve(&gmat, &p, &gx, &gb);
}

/**
  Computes the inverse of a square matrix using its LU decomposition.
*/
void gslLuInverse(const MatDbl& lu, const std::vector<unsigned int>& perm, MatDbl* out)
{
  const int m = lu.Height(), n = lu.Width();
  assert(m == n);
  assert(m == perm.size());
  out->Reset(m, m);
  gsl_permutation p;
  GslMat glu(&lu), gout(out);
  p.size = m;
  p.data = iConstIteratorToPtr(  perm.begin() );
  gsl_linalg_LU_invert(glu, &p, gout);
}

/**
  Computes the determinant of a square matrix using its LU decomposition.
*/
double gslLuDeterminant(const MatDbl& lu, const std::vector<unsigned int>& perm, int sign)
{
  const int m = lu.Height(), n = lu.Width();
  assert(m == n);
  assert(m == perm.size());
  GslMat glu(&lu);
  return gsl_linalg_LU_det(glu, sign);
}

/**
  Computes QR decomposition of a general matrix.
  'qr' is both input and output (m x n); on output, the
    upper triangular part contain R, while the
    lower triangular part, along with 'tau', contain the
    Householder coefficients of Q.  This is the same format
    as LAPACK.
  'tau':  output vector of size min(m,n).
*/
void gslQr(MatDbl* qr, MatDbl* tau)
{
  const int m = qr->Height(), n = qr->Width();
  tau->Reset(blepo_ex::Min(m, n));
  GslMat gqr(qr);
  GslVec gtau(tau);
  gsl_linalg_QR_decomp(gqr, gtau);
}

/**
  Unpacks the Q and R matrices from gslQr result
*/
void gslQrUnpack(const MatDbl& qr, const MatDbl& tau, MatDbl* q, MatDbl* r)
{
  const int m = qr.Height(), n = qr.Width();
  assert(tau.Height() == blepo_ex::Min(m, n) && tau.Width() == 1);
  q->Reset(m, m);
  r->Reset(m, n);
  GslMat gqr(&qr), gq(q), gr(r);
  GslVec gtau(&tau);
  gsl_linalg_QR_unpack(gqr, gtau, gq, gr);
}

/**
  Solve linear equation Ax=b using QR decomposition of A.
*/
void gslQrSolve(const MatDbl& qr, const MatDbl& tau, const MatDbl& b, MatDbl* x)
{
  const int m = qr.Height(), n = qr.Width();
  if (m != n)  BLEPO_ERROR("QR solve requires the matrix to be square");
  assert(tau.Height() == blepo_ex::Min(m, n) && tau.Width() == 1);
  x->Reset(n);
  GslMat gqr(&qr);
  GslVec gtau(&tau), gb(&b), gx(x);
  gsl_linalg_QR_solve(gqr, gtau, gb, gx);
}

/**
  Solve linear equation Ax=b using QR decomposition of A, where Q and R have been unpacked.
*/
void gslQrSolveUnpack(const MatDbl& q, const MatDbl& r, const MatDbl& b, MatDbl* x)
{
  const int m = q.Height(), n = r.Width();
  assert(m == q.Width() && m == r.Height());
  assert(b.Height() == m && b.Width() == 1);
  x->Reset(n);
  GslMat gq(&q), gr(&r);
  GslVec gb(&b), gx(x);
  gsl_linalg_QR_QRsolve(gq, gr, gb, gx);
}

/**
  Solve overdetermined linear equation Ax=b using QR decomposition of A, by minimizing
  least squares difference.  'x' and 'residue' are outputs.
*/
void gslQrSolveLeastSquares(const MatDbl& qr, const MatDbl& tau, const MatDbl& b, MatDbl* x, MatDbl* residue)
{
  const int m = qr.Height(), n = qr.Width();
  assert(tau.Height() == blepo_ex::Min(m, n) && tau.Width() == 1);
  x->Reset(n);
  residue->Reset(m);
  GslMat gqr(&qr);
  GslVec gtau(&tau), gb(&b), gx(x), gres(residue);
  gsl_linalg_QR_lssolve(gqr, gtau, gb, gx, gres);
}

/**
  Computes Q v (i.e., multiplies the vector v by Q) using
  the QR decomposition of a matrix.  This is a fast implementation that does
  not require reconstructing the full matrix Q.
*/
void gslQrApplyQ(const MatDbl& qr, const MatDbl& tau, MatDbl* v)
{
  const int m = qr.Height(), n = qr.Width();
  assert(tau.Height() == blepo_ex::Min(m, n) && tau.Width() == 1);
  if (v->Height() != m || v->Width() != 1)  BLEPO_ERROR("Invalid dimensions");
  GslMat gqr(&qr);
  GslVec gtau(&tau), gv(v);
  gsl_linalg_QR_Qvec(gqr, gtau, gv);
}

/**
  Solve linear equation Ax=b using QR decomposition of A, assuming that bb = Q^T b has
  already been computed.
*/
void gslQrRSolve(const MatDbl& qr, const MatDbl& bb, MatDbl* x)
{
  const int m = qr.Height(), n = qr.Width();
  x->Reset(n);
  GslMat gqr(&qr);
  GslVec gbb(&bb), gx(x);
  gsl_linalg_QR_Rsolve (gqr, gbb, gx);
}

/**
  Computes Q^T v (i.e., multiplies the vector v by the transpose of Q) using
  the QR decomposition of a matrix.  This is a fast implementation that does
  not require reconstructing the full matrix Q.
*/
void gslQrApplyQTranspose(const MatDbl& qr, const MatDbl& tau, MatDbl* v)
{
  const int m = qr.Height(), n = qr.Width();
  assert(tau.Height() == blepo_ex::Min(m, n) && tau.Width() == 1);
  if (v->Height() != m || v->Width() != 1)  BLEPO_ERROR("Invalid dimensions");
  GslMat gqr(&qr);
  GslVec gtau(&tau), gv(v);
  gsl_linalg_QR_QTvec(gqr, gtau, gv);
}

/**
  Computes the singular value decomposition (SVD) of a matrix A = U S V^T.
    'u' is both input and output; on input it contains the matrix A, on
      output it contains the matrix U.
    's' contains the singular values in compact vector form.  These singular
      values can be placed into a diagonal matrix using Diag() to form S.
    'v' contains the matrix V.
  The dimensions of A are m x n (m rows, n columns)
  If A is tall (m >= n, more rows than columns):   'u' (m x n); 's' (n x 1); 'v' (n x n)    
  Else A is wide (m < n, more columns than rows): 'u' (m x m); 's' (m x 1); 'v' (n x m)    
*/
void gslSvd(MatDbl* u, MatDbl* s, MatDbl* v)
{ 
  if (u->Width() <= u->Height())
  { // A is tall (m >= n, more rows than columns)
    const int m = u->Height(), n = u->Width();
    s->Reset(n);
    v->Reset(n, n);
    MatDbl tmp(n);
    GslMat gu(u), gv(v);
    GslVec gs(s), gtmp(&tmp);
    gsl_linalg_SV_decomp(gu, gv, gs, gtmp);
  }
  else
  { // A is wide (m < n, more columns than rows)
    // for some reason gsl_lingalg_SV_decomp complains if 'u' is wide, so we have to transpose first.
    // Recall that the SVD of the transpose is the transpose of the SVD:
    //    A = U S V^T  <==>  A^T = V S U^T
    // so we just need to swap U and V
    *u = Transpose(*u);
    const int m = u->Height(), n = u->Width();
    s->Reset(n);
    v->Reset(n, n);
    MatDbl tmp(n);
    GslMat gu(u), gv(v);
    GslVec gs(s), gtmp(&tmp);
    gsl_linalg_SV_decomp(gu, gv, gs, gtmp);
    MatDbl tmp2 = *u;  // swap U and V
    *u = *v;
    *v = tmp2;
  }
}

/**
  Computes the SVD using a modified algorithm that is much faster for tall
  matrices (m >> n).
*/
void gslSvdTall(MatDbl* u, MatDbl* s, MatDbl* v)
{ 
  const int m = u->Height(), n = u->Width();
  s->Reset(n);
  v->Reset(n, n);
  MatDbl tmp1(n, n), tmp2(n);
  GslMat gu(u), gv(v), gtmp1(&tmp1);;
  GslVec gs(s), gtmp2(&tmp2);
  gsl_linalg_SV_decomp_mod(gu, gtmp1, gv, gs, gtmp2);
}

/**
  Solve linear equation Ax=b using SVD of A.
*/
void gslSvdSolve(const MatDbl& u, const MatDbl& s, const MatDbl& v, const MatDbl& b, MatDbl* x)
{
  const int m = u.Height(), n = u.Width();
  assert(s.Height() == n && s.Width() == 1);
  assert(v.Height() == n && v.Width() == n);
  assert(b.Height() == m && b.Width() == 1);
  x->Reset(n);
  GslMat gu(&u), gv(&v);
  GslVec gs(&s), gb(&b), gx(x);
  gsl_linalg_SV_solve(gu, gv, gs, gb, gx);
}

/**
  Computes the eigenvalues of a real symmetric matrix.
  'a' contains the matrix on input and is destroyed during the computation.
  'eigenvalues' is output, contains the unsorted eigenvalues
*/
void gslEigenvalues(MatDbl* a, MatDbl* eigenvalues)
{
  const int m = a->Height(), n = a->Width();
  if (m != n)  BLEPO_ERROR("Matrix must be square to compute eigenvalues");
  eigenvalues->Reset(n);
  GslMat ga(a);
  GslVec gval(eigenvalues);
  gsl_eigen_symm_workspace* gtmp = gsl_eigen_symm_alloc(n);
  gsl_eigen_symm(ga, gval, gtmp);
  gsl_eigen_symm_free(gtmp);
}

/**
  Computes the eigenvalues and eigenvectors of a real symmetric matrix.
  'a' contains the matrix on input and is destroyed during the computation.
  'eigenvalues' is output, contains the unsorted eigenvalues
*/
void gslEigen(MatDbl* a, MatDbl* eigenvalues, MatDbl* eigenvectors)
{
  const int m = a->Height(), n = a->Width();
  if (m != n)  BLEPO_ERROR("Matrix must be square to compute eigenvalues");
  eigenvalues->Reset(n);
  eigenvectors->Reset(n, n);
  GslMat ga(a), gvec(eigenvectors);
  GslVec gval(eigenvalues);
  gsl_eigen_symmv_workspace* gtmp = gsl_eigen_symmv_alloc(n);
  gsl_eigen_symmv(ga, gval, gvec, gtmp);
  gsl_eigen_symmv_free(gtmp);
}

/**
  Sorts eigenvalues and eigenvectors according to either the eigenvalues or 
  their absolute values.
*/
void gslSortEigen(MatDbl* eigenvalues, MatDbl* eigenvectors, int sort_type)
{
  const int m = eigenvectors->Height(), n = eigenvectors->Width();
  assert(m == n && eigenvalues->Height() == m && eigenvalues->Width() == 1);
  GslMat gvec(eigenvectors);
  GslVec gval(eigenvalues);
  gsl_eigen_sort_t sort;
  switch (sort_type)
  {
  case 0:  sort = GSL_EIGEN_SORT_VAL_ASC;  break;
  case 1:  sort = GSL_EIGEN_SORT_VAL_DESC;  break;
  case 2:  sort = GSL_EIGEN_SORT_ABS_ASC;  break;
  case 3:
  default: sort = GSL_EIGEN_SORT_ABS_DESC;  break;
  };
  gsl_eigen_symmv_sort(gval, gvec, sort);
}

};
// ================< end local functions

namespace blepo 
{

void Svd(const MatDbl& mat, MatDbl* u, MatDbl* s, MatDbl* v)
{
  if (&mat == u || &mat == s || &mat == v)  BLEPO_ERROR("SVD cannot be done inplace");
  *u = mat;
  gslSvd(u, s, v);
}

void Lu(const MatDbl& mat, MatDbl* l, MatDbl* u, MatDbl* p)
{
  if (&mat == l || &mat == u || &mat == p)  BLEPO_ERROR("LU decomposition cannot be done inplace");
  MatDbl lu = mat;
  std::vector<unsigned int> perm;
  int sign;
  gslLu(&lu, &perm, &sign);
  gslLuUnpack(lu, perm, l, u, p);
}

void Inverse(const MatDbl& mat, MatDbl* out)
{
  if (&mat == out)  BLEPO_ERROR("Matrix inverse cannot be done inplace");
  if (mat.Height() != mat.Width())  BLEPO_ERROR("Cannot compute inverse of non-square matrix");
  MatDbl lu = mat;
  std::vector<unsigned int> perm;
  int sign;
  gslLu(&lu, &perm, &sign);
  gslLuInverse(lu, perm, out);
}

MatDbl Inverse(const MatDbl& mat)
{
  MatDbl tmp;
  Inverse(mat, &tmp);
  return tmp;
}

MatFlt Inverse(const MatFlt& mat)
{
  MatFlt inv;
  MatDbl tmp, tmp_inv;
  Convert(mat, &tmp);
  Inverse(tmp, &tmp_inv);
  Convert(tmp_inv, &inv);
  return inv;
}

double Determinant(const MatDbl& mat)
{
  if (mat.Height() != mat.Width())  BLEPO_ERROR("Cannot compute inverse of non-square matrix");
  MatDbl lu = mat;
  std::vector<unsigned int> perm;
  int sign;
  gslLu(&lu, &perm, &sign);
  return gslLuDeterminant(lu, perm, sign);
}

void EigenSymm(const MatDbl& mat, MatDbl* eigenvalues)
{
  if (&mat == eigenvalues)  BLEPO_ERROR("Eigen cannot be done inplace");
  MatDbl tmp = mat;
  gslEigenvalues(&tmp, eigenvalues);
}

void EigenSymm(const MatDbl& mat, MatDbl* eigenvalues, MatDbl* eigenvectors)
{
  if (&mat == eigenvalues || &mat == eigenvectors)  BLEPO_ERROR("Eigen cannot be done inplace");
  MatDbl tmp = mat;
  gslEigen(&tmp, eigenvalues, eigenvectors);
}

void Qr(const MatDbl& mat, MatDbl* q, MatDbl* r)
{
  if (&mat == q || &mat == r)  BLEPO_ERROR("QR factorization cannot be done inplace");
  MatDbl qr = mat, tau;
  gslQr(&qr, &tau);
  gslQrUnpack(qr, tau, q, r);
}

void SolveLinearLuSquare(const MatDbl& a, const MatDbl& b, MatDbl* x)
{
  if (&a == &b || &a == x || &b == x)  BLEPO_ERROR("Least squares cannot be done inplace");
  MatDbl lu = a;
  std::vector<unsigned int> perm;
  int sign;
  gslLu(&lu, &perm, &sign);
  gslLuSolve(lu, perm, b, x);
}

void SolveLinearQrSquare(const MatDbl& a, const MatDbl& b, MatDbl* x)
{
  if (&a == &b || &a == x || &b == x)  BLEPO_ERROR("Least squares cannot be done inplace");
  MatDbl qr = a, tau;
  gslQr(&qr, &tau);
  gslQrSolve(qr, tau, b, x);
}

void SolveLinearQr(const MatDbl& a, const MatDbl& b, MatDbl* x, MatDbl* residue)
{
  if (&a == &b || &a == x || &b == x)  BLEPO_ERROR("Least squares cannot be done inplace");
  MatDbl qr = a, tau;
  gslQr(&qr, &tau);
  gslQrSolveLeastSquares(qr, tau, b, x, residue);
}

void SolveLinearSvd(const MatDbl& a, const MatDbl& b, MatDbl* x)
{
  if (&a == &b || &a == x || &b == x)  BLEPO_ERROR("Least squares cannot be done inplace");
  MatDbl u, s, v;
  Svd(a, &u, &s, &v);
  gslSvdSolve(u, s, v, b, x);
}

void SolveLinear(const MatDbl& a, const MatDbl& b, MatDbl* x)
{
  if (a.Height() == a.Width())
  {
    SolveLinearLuSquare(a, b, x);
  }
  else if (a.Height() > a.Width())
  {
//    SolveLinearSvd(a, b, x);
    MatDbl residue;
    SolveLinearQr(a, b, x, &residue);
  }
  else
  {
    BLEPO_ERROR("Cannot solve an underdetermined linear equation");
  }
}



double Determinant3x3(const MatDbl& mat)
{
  if (mat.Height() != 3)  BLEPO_ERROR("Not a 3x3 matric");
  if (mat.Width() != 3)  BLEPO_ERROR("Not a 3x3 matric");

  return(   mat(0)*mat(4)*mat(8) + mat(1)*mat(5)*mat(6) + mat(2)*mat(3)*mat(7) 
          - mat(0)*mat(5)*mat(7) - mat(1)*mat(3)*mat(8) - mat(2)*mat(4)*mat(6)
        );
}



void Inverse3x3(const MatDbl& mat, MatDbl* out)
{
  if (&mat == out)  BLEPO_ERROR("Matrix inverse cannot be done inplace");
  if (mat.Height() != 3)  BLEPO_ERROR("Not a 3x3 matric");
  if (mat.Width() != 3)  BLEPO_ERROR("Not a 3x3 matric");
    
  out->Reset(3, 3);

  double mat_det = Determinant3x3(mat);
  (*out)(0,0) = (mat(4)*mat(8) - mat(5)*mat(7)) / mat_det;
  (*out)(1,0) = (mat(2)*mat(7) - mat(1)*mat(8)) / mat_det;
  (*out)(2,0) = (mat(1)*mat(5) - mat(2)*mat(4)) / mat_det;

  (*out)(0,1) = (mat(5)*mat(6) - mat(3)*mat(8)) / mat_det;
  (*out)(1,1) = (mat(0)*mat(8) - mat(2)*mat(6)) / mat_det;
  (*out)(2,1) = (mat(2)*mat(3) - mat(0)*mat(5)) / mat_det;

  (*out)(0,2) = (mat(3)*mat(7) - mat(4)*mat(6)) / mat_det;
  (*out)(1,2) = (mat(1)*mat(6) - mat(0)*mat(7)) / mat_det;
  (*out)(2,2) = (mat(0)*mat(4) - mat(1)*mat(3)) / mat_det;

}

};//namespace blepo



//
////#if 1  // stb's code
//
//  *u = mat;
//  s->Reset(mat.Width());
//  v->Reset(mat.Width(), mat.Width());
//  MatDbl tmp(mat.Width());
//  GslMat gu(u), gv(v);
//  GslVec gs(s), gtmp(&tmp);
//  gsl_linalg_SV_decomp(gu, gv, gs, gtmp);
//
////#else 1 // Saurabh's code
////  gsl_matrix*  gmata = gsl_matrix_alloc(mat.Rows(), mat.Cols());
////  Blepomat2Gslmat(mat, gmata);
////  gsl_matrix* gmatv = gsl_matrix_alloc(mat.Cols(), mat.Cols());
////  gsl_vector* gvecs = gsl_vector_alloc(mat.Cols());
////  gsl_vector* gvecwork = gsl_vector_alloc(mat.Cols()); 
////  gsl_linalg_SV_decomp(gmata, gmatv, gvecs, gvecwork);
////  Gslmat2Blepomat(*gmata, u);
////  Gslvect2Blepomatdiag(*gvecs, s);
////  Gslmat2Blepomat(*gmatv, v);
////  gsl_matrix_free(gmata);
////  gsl_matrix_free(gmatv);
////  gsl_vector_free(gvecs);
////  gsl_vector_free(gvecwork);
////#endif
//}

//void Lu(const MatDbl& mat, MatDbl* permutation, MatDbl* lu)
//{
//  const int m = mat.Height(), n = mat.Width();
//  if (m != n)  BLEPO_ERROR("Matrix must be square for LU decomposition");
//  gsl_permutation p(m);
//  *lu = mat;
//  GslMat glu(lu);
//  int sign;
//  gsl_linalg_LU_decomp(lu, &p, &sign);
//  // convert permutation
//  permutation->Reset(m, m);
//  Set(&permutation, 0);
//  for (int i=0 ; i<m ; i++) permutation(i, p.data[i]) = 1;
//}


//void Qr(const MatDbl& mat, MatDbl* q, MatDbl* r)
//{
//
//
//}

//void SolveLinearEquation(const MatDbl& a, const MatDbl& b, MatDbl* x)
//{
////  GslMat u(&a), v(&a);
////  GslVec s(&a), bb(&b), xx(&x);
////  gsl_linalg_SV_solve(u, v, s, bb, xx);
//}

//void SolveLinearEquation(const MatDbl& lower, const MatDbl& upper, const MatDbl& b, MatDbl* r)
//{
//}


//void Inverse(const MatDbl& mat, MatDbl* out)
//{
//  if (mat.Width() != mat.Height())  BLEPO_ERROR("Cannot compute the inverse of a non-square matrix");
//  const int n = mat.Width();
//  MatDbl u, s, v, tmp;
//  out->Reset(n, n);
//  Svd(mat, &u, &s, &v);
//  Transpose(&u);
//  for (int i=0 ; i<n ; i++)  s(i) = 1.0 / s(i);
//  MatrixMultiply(v, s, &tmp);  // I don't think this will work, since s is a vector
//  MatrixMultiply(tmp, u, out);
//}






////private util functions
//void  Gslmat2Blepomat(const gsl_matrix& gmat, MatDbl *mat)
//{
//		
//	int i ,j;
//	int Rows = gmat.size1;
//	int Cols = gmat.size2;
//	mat->Reset(Cols, Rows);
//	for (i=0;i< Rows;i++)
//	{
//		for(j=0;j< Cols;j++)
//		{
//			(*mat)(j,i) = gsl_matrix_get(&gmat,i,j);
//		}
//	}
//}
//
//void Blepomat2Gslmat(const MatDbl& mat , gsl_matrix* gmat)
//{
//	//this function assumes the user has already allocalte memory for both type of matrices
//	//this assumption is made as user would be responsible for allocating and freeing the memory
//	int i ,j;//simply for looping
//	int Rows = mat.Rows();
//	int Cols = mat.Cols();
//	for (i=0;i< Rows;i++)
//	{
//		for(j=0;j< Cols;j++)
//		{
//			gsl_matrix_set(gmat,i,j,mat(j,i));
//		}
//	}
//	
//	
//}
//
//void Gslvect2Blepomat(const gsl_vector& gvec, MatDbl* mat)
//{
//	int Rows = gvec.size;
//	int Cols=1, i;
//	mat->Reset(Cols, Rows);
//	
//	for (i=0; i<Rows; i++)
//	{
//		(*mat)(i) = gsl_vector_get(&gvec,i);
//	}		
//}
//	
//void Gslvect2Blepomatdiag(const gsl_vector& gvec, MatDbl* mat)
//{
//	int Rows = gvec.size;
//	int i,j;
//	mat->Reset(Rows,Rows);
//	for (i=0;i< Rows;i++)
//	{
//		for(j=0;j< Rows;j++)
//		{
//			(*mat)(i,j)=0;
//		}
//	}
//	for (i=0; i<Rows; i++)
//	{
//		(*mat)(i,i)=gsl_vector_get(&gvec,i);
//	}	
//	
//}

	
