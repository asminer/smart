
#include "normal.h"

#include <math.h>

// ******************************************************************
// *                                                                *
// *                  standard normal distribution                  *
// *                                                                *
// * Code taken from "Discrete Event Simulation: A First Course" by *
// *                 Leemis and Park, Prentice Hall                 *
// ******************************************************************

const double TINY = 1.0e-10;
const double SQRT2PI = 2.506628274631;              /* sqrt(2 * pi) */

double LogGamma(double a)
/* ======================================================================== 
 * LogGamma returns the natural log of the gamma function.
 * NOTE: use a > 0.0 
 *
 * The algorithm used to evaluate the natural log of the gamma function is
 * based on an approximation by C. Lanczos, SIAM J. Numerical Analysis, B,
 * vol 1, 1964.  The constants have been selected to yield a relative error
 * which is less than 2.0e-10 for all positive values of the parameter a.    
 * ======================================================================== 
 */
{ 
   double s[6], sum, temp;
   int    i;

   s[0] =  76.180091729406 / a;
   s[1] = -86.505320327112 / (a + 1.0);
   s[2] =  24.014098222230 / (a + 2.0);
   s[3] =  -1.231739516140 / (a + 3.0);
   s[4] =   0.001208580030 / (a + 4.0);
   s[5] =  -0.000005363820 / (a + 5.0);
   sum  =   1.000000000178;
   for (i = 0; i < 6; i++) 
     sum += s[i];
   temp = (a - 0.5) * log(a + 4.5) - (a + 4.5) + log(SQRT2PI * sum);
   return (temp);
}


double InGamma(double a, double x)
/* ========================================================================
 * Evaluates the incomplete gamma function.
 * NOTE: use a > 0.0 and x >= 0.0
 *
 * The algorithm used to evaluate the incomplete gamma function is based on
 * Algorithm AS 32, J. Applied Statistics, 1970, by G. P. Bhattacharjee.
 * See also equations 6.5.29 and 6.5.31 in the Handbook of Mathematical
 * Functions, Abramowitz and Stegum (editors).  The absolute error is less 
 * than 1e-10 for all non-negative values of x.
 * ========================================================================
 */
{ 
   double t, sum, term, factor, f, g, c[2], p[3], q[3];
   long   n;

   if (x > 0.0)
     factor = exp(-x + a * log(x) - LogGamma(a));
   else
     factor = 0.0;
   if (x < a + 1.0) {                 /* evaluate as an infinite series - */
     t    = a;                        /* A & S equation 6.5.29            */
     term = 1.0 / a;
     sum  = term;
     while (term >= TINY * sum) {     /* sum until 'term' is small */
       t++;
       term *= x / t;
       sum  += term;
     } 
     return (factor * sum);
   }
   else {                             /* evaluate as a continued fraction - */
     p[0]  = 0.0;                     /* A & S eqn 6.5.31 with the extended */
     q[0]  = 1.0;                     /* pattern 2-a, 2, 3-a, 3, 4-a, 4,... */
     p[1]  = 1.0;                     /* - see also A & S sec 3.10, eqn (3) */
     q[1]  = x;
     f     = p[1] / q[1];
     n     = 0;
     do {                             /* recursively generate the continued */
       g  = f;                        /* fraction 'f' until two consecutive */
       n++;                           /* values are small                   */
       if ((n % 2) > 0) {
         c[0] = ((double) (n + 1) / 2) - a;
         c[1] = 1.0;
       }
       else {
         c[0] = (double) n / 2;
         c[1] = x;
       }
       p[2] = c[1] * p[1] + c[0] * p[0];
       q[2] = c[1] * q[1] + c[0] * q[0];
       if (q[2] != 0.0) {             /* rescale to avoid overflow */
         p[0] = p[1] / q[2];
         q[0] = q[1] / q[2];
         p[1] = p[2] / q[2];
         q[1] = 1.0;
         f    = p[1];
       }
     } while ((fabs(f - g) >= TINY) || (q[1] != 1.0));
     return (1.0 - factor * f);
   }
}


double pdfStdNormal(double x)
/* =================================== 
 * NOTE: x can be any value 
 * ===================================
 */
{
   return (exp(- 0.5 * x * x) / SQRT2PI);
}

double cdfStdNormal(double x)
/* =================================== 
 * NOTE: x can be any value 
 * ===================================
 */
{ 
   double t;

   t = InGamma(0.5, 0.5 * x * x);
   if (x < 0.0)
     return (0.5 * (1.0 - t));
   else
     return (0.5 * (1.0 + t));
}

double idfStdNormal(double u)
{
/* 
 * A very accurate approximation of the normal idf due to Odeh & Evans, 
 * J. Applied Statistics, 1974, vol 23, pp 96-97.
 */
  const double p0 = 0.322232431088;     const double q0 = 0.099348462606;
  const double p1 = 1.0;                const double q1 = 0.588581570495;
  const double p2 = 0.342242088547;     const double q2 = 0.531103462366;
  const double p3 = 0.204231210245e-1;  const double q3 = 0.103537752850;
  const double p4 = 0.453642210148e-4;  const double q4 = 0.385607006340e-2;
  double t, p, q, z;

  if (u < 0.5)
    t = sqrt(-2.0 * log(u));
  else
    t = sqrt(-2.0 * log(1.0 - u));
  p   = p0 + t * (p1 + t * (p2 + t * (p3 + t * p4)));
  q   = q0 + t * (q1 + t * (q2 + t * (q3 + t * q4)));
  if (u < 0.5)
    z = (p / q) - t;
  else
    z = t - (p / q);
  return z;
}

