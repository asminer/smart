
// $Id$

#include <math.h>
#include <stdlib.h>

// for namespace and such

#include "mclib.h"

// ******************************************************************
/*
  The following algorithm to compute the Poisson PDF is
  from Fox & Glynn, "Computing Poisson Probabilities",
  Communications of the ACM, 31 (4) April 1988, pp 440-445.

  Currently: reckless disregard for underflow or overflow.
*/

const double TAU = 1.0e-30;
const double OMEGA = 1.0e+30;
const double sqrt2pi = sqrt(2.0*M_PI);

int PoissonRight(double lambda, double epsilon)
{
  if (lambda <= 0) return 0;

  // Fox-Glynn Page 443, Column 1 Rule 2 
  //
  if (lambda < 400.0) {  
    lambda = 400.0;
  }
  double a_lambda = (1.0+1.0/lambda)*exp(1.0/16.0)*sqrt(2.0);

  double sqrt2lam = sqrt(2.0*lambda);
  
  for (int k = 3; ; k++) {
    double d_k_lambda = 1.0/(1.0-exp((-2.0/9.0)*(k*sqrt2lam)+1.5));
    double p = a_lambda*d_k_lambda*exp(-k*k/2.0)/(k*sqrt2pi);
    if (p < epsilon/2.0) {
      return int(ceil(lambda+k*sqrt2lam)+1.5)+1;
    }
  }
}

int PoissonLeft(double lambda, double epsilon)
{
  if (lambda < 25) return 0;
  double sqrtlam = sqrt(lambda);
  double b_lambda = (1.0+1.0/lambda)*exp(1.0/(8.0*lambda));
  for (int k = 1; ; k++) { 
    if (b_lambda*exp(-k*k/2.0)/(k*sqrt2pi) < epsilon/2.0) {
      int L = int(floor(lambda-k*sqrtlam-1.5));
      if (L < 0) L = 0;
      return L;
    }
  }  
}

double* MCLib::computePoissonPDF(double lambda, double epsilon, int &L, int &R)
{
  R = PoissonRight(lambda, epsilon);
  L = PoissonLeft(lambda, epsilon);

  double* pdf = (double*) malloc((R-L+1)*sizeof(double));
  double* pdfshift = pdf - L;

  // Straight from page 442
  int m = int(lambda);
  pdfshift[m] = OMEGA/(1e10*(R-L+1));
  // Down
  for (int j=m; j>L; j--) {
    pdfshift[j-1] = (j/lambda)*pdfshift[j];
  }
  // Up (simplified)
  for (int j=m+1; j<=R; j++) {
    // double qty = lambda/j;
    pdfshift[j] = (lambda / j) * pdfshift[j-1];
    if (pdfshift[j] < TAU) {
      R = j;
    } 
  }
  // Get total; add small terms first
  double total = 0.0;
  int l = L;
  int r = R;
  while (l < r) {
    if (pdfshift[l] <= pdfshift[r]) {
      total += pdfshift[l];
      l++;
    } else {
      total += pdfshift[r];
      r--;
    }
  } // while
  total += pdfshift[l];
  // Normalize
  for (int j=L; j<=R; j++)
  pdfshift[j] /= total;

  return pdf;
}

