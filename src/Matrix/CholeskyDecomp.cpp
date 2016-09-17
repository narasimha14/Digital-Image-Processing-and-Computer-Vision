/***************************************************
This file takes in matrix A and decomposes it
to an upper and lower triangular matrix.  Returns
the lower triangular matrix. 

  Bug:  Code assumes that matrix is symmetric, positive semi-definite.
        Needs to check for this to be safe.

  @author Brian Peasley
***************************************************/

#include "Matrix.h"
#include "MatrixOperations.h" // Set
#include <math.h> // sqrt

namespace blepo
{

void CholeskyDecomp(const MatDbl& a, MatDbl* out)
{
  int n = a.Height();
  assert(out);
  assert(a.Width() == a.Height());  // only works for square matrices
  if (!out)  return;
  out->Reset(n, n);
  Set(out, 0);
  MatDbl& L = *out;
  double sum;
  
  
  for (int i = 0 ; i < n ; i++)
  {
    sum = 0.0;
    for (int k = 0 ; k < i ; k++)
    {
      sum += L(k,i) * L(k,i);
    }
    
    L(i,i) = sqrt(a(i,i) - sum);
    
    for (int j = i+1 ; j < n ; j++)
    {
      sum = 0;
      for (int k = 0 ; k < i ; k++)
      {
        sum += L(k,i) * L(k,j);
      }
      
      L(i, j) = ( 1 / L(i,i) ) * ( a(j,i) - sum );
    }
  }
}

}; // blepo