
#include "distros.h"

#include <string.h>
#include <math.h>

// ******************************************************************
// *                                                                *
// *                    discrete_pdf  methods                       *
// *                                                                *
// ******************************************************************

discrete_pdf::discrete_pdf()
{
  Left=-1;
  Right=-2;
  probs = 0;
  probs_shifted = 0;
  prob_infinity = 0;
}

discrete_pdf::discrete_pdf(const discrete_pdf &P)
{
  Left = P.Left;
  Right = P.Right;
  prob_infinity = P.prob_infinity;

  if (0==P.probs) {
    probs = 0;
    probs_shifted = 0;
  } else {
    probs_shifted = new double[Right-Left+1];
    memcpy(probs_shifted, P.probs_shifted, (Right-Left+1) * sizeof(double));
    probs = probs_shifted - Left;
  }
}

discrete_pdf::discrete_pdf(long L, long R, double* shifted, double infinity)
{
  probs = 0;
  probs_shifted = 0;
  reset(L, R, shifted, infinity);
}

discrete_pdf::~discrete_pdf()
{
  delete[] probs_shifted;
}

void discrete_pdf::reset(long L, long R, double* shifted, double infinity)
{
  delete[] probs_shifted;

  Left = L;
  Right = R;
  probs_shifted = shifted;
  probs = probs_shifted - Left;
  prob_infinity = infinity;
}

void discrete_pdf::copyFromAndTruncate(const double* pdf, long size, 
  double infinity)
{
  prob_infinity = infinity;

  delete[] probs_shifted;

  // 
  // Determine left truncation point
  //
  for (Left=0; Left<size; Left++) {
    if (pdf[Left]) break;
  }
  if (Left>=size) {
    // Special and easy case:
    // whole array is zero.  
    Left = -1;
    Right = -2;
    probs = 0;
    probs_shifted = 0;
    return;
  }

  //
  // Determine right truncation point
  //
  for (Right=size-1; Right>Left; Right--) {
    if (pdf[Right]) break;
  }

  //
  // Allocate array and copy over
  //
  probs_shifted = new double[Right-Left+1];
  memcpy(probs_shifted, pdf+Left, (Right-Left+1) * sizeof(double));
  probs = probs_shifted - Left;
}

// ******************************************************************
// *                                                                *
// *                    discrete_cdf  methods                       *
// *                                                                *
// ******************************************************************

discrete_cdf::discrete_cdf()
{
  Left=-1;
  Right=-2;
  probs = 0;
  probs_shifted = 0;
}

discrete_cdf::~discrete_cdf()
{
  delete[] probs_shifted;
}

void discrete_cdf::setFromPDF(const discrete_pdf &P)
{
  Left = P.Left;
  Right = P.Right;

  if (0==P.probs) {
    probs = 0;
    probs_shifted = 0;
  } else {
    probs_shifted = new double[Right-Left+1];
    memcpy(probs_shifted, P.probs_shifted, (Right-Left+1) * sizeof(double));
    probs = probs_shifted - Left;

    // convert to CDF using F(i) = F(i-1) + f(i)
    for (long i=Left+1; i<=Right; i++) {
      probs[i] += probs[i-1];
    }
  }
}

// ******************************************************************
// *                                                                *
// *                   discrete_1mcdf  methods                      *
// *                                                                *
// ******************************************************************

discrete_1mcdf::discrete_1mcdf()
{
  Left=-1;
  Right=-2;
  probs = 0;
  probs_shifted = 0;
}

discrete_1mcdf::~discrete_1mcdf()
{
  delete[] probs_shifted;
}

void discrete_1mcdf::setFromPDF(const discrete_pdf &P)
{
  Left = P.Left;
  Right = P.Right;

  if (0==P.probs) {
    probs = 0;
    probs_shifted = 0;
  } else {
    probs_shifted = new double[Right-Left+1];
    memcpy(probs_shifted, P.probs_shifted, (Right-Left+1) * sizeof(double));
    probs = probs_shifted - Left;

    // convert to  1-CDF, Prob(X > i)
    for (long i=Left; i<Right; i++) {
      probs[i] = probs[i+1];
    }
    probs[Right] = 0;
    for (long i=Right-1; i>=Left; i--) {
      probs[i] += probs[i+1];
    }
  }
}


// ******************************************************************
// *                                                                *
// *                    Poisson  distribution                       *
// *                                                                *
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

// ******************************************************************

long PoissonRight(double lambda, double epsilon)
{
  if (lambda <= 0) return 0;

  // Fox-Glynn Page 443, Column 1 Rule 2 
  //
  if (lambda < 400.0) {  
    lambda = 400.0;
  }
  double a_lambda = (1.0+1.0/lambda)*exp(1.0/16.0)*sqrt(2.0);

  double sqrt2lam = sqrt(2.0*lambda);
  
  for (long k = 3; ; k++) {
    double d_k_lambda = 1.0/(1.0-exp((-2.0/9.0)*(k*sqrt2lam)+1.5));
    double p = a_lambda*d_k_lambda*exp(-k*k/2.0)/(k*sqrt2pi);
    if (p < epsilon/2.0) {
      return long(ceil(lambda+k*sqrt2lam)+1.5)+1;
    }
  }
}

// ******************************************************************

long PoissonLeft(double lambda, double epsilon)
{
  if (lambda < 25) return 0;
  double sqrtlam = sqrt(lambda);
  double b_lambda = (1.0+1.0/lambda)*exp(1.0/(8.0*lambda));
  for (long k = 1; ; k++) { 
    if (b_lambda*exp(-k*k/2.0)/(k*sqrt2pi) < epsilon/2.0) {
      long L = long(floor(lambda-k*sqrtlam-1.5));
      if (L < 0) L = 0;
      return L;
    }
  }  
}

// ******************************************************************

void computePoissonPDF(double lambda, double epsilon, discrete_pdf &P)
{
  long R = PoissonRight(lambda, epsilon);
  long L = PoissonLeft(lambda, epsilon);

  double* shifted_pdf = new double[R-L+1];
  double* pdf = shifted_pdf - L;

  // Straight from page 442
  long m = long(lambda);
  pdf[m] = OMEGA/(1e10*(R-L+1));
  // Down
  for (long j=m; j>L; j--) {
    pdf[j-1] = (j/lambda)*pdf[j];
  }
  // Up (simplified)
  for (long j=m+1; j<=R; j++) {
    // double qty = lambda/j;
    pdf[j] = (lambda / j) * pdf[j-1];
    if (pdf[j] < TAU) {
      R = j;
    } 
  }
  // Get total; add small terms first
  double total = 0.0;
  long l = L;
  long r = R;
  while (l < r) {
    if (pdf[l] <= pdf[r]) {
      total += pdf[l];
      l++;
    } else {
      total += pdf[r];
      r--;
    }
  } // while
  total += pdf[l];
  // Normalize

  for (int j=L; j<=R; j++) {
    pdf[j] /= total;
  }

  P.reset(L, R, shifted_pdf, 0);
}



