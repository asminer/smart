
// $Id$

/** Linear solvers
*/

#include "../Base/options.h"
#include "linear.h"

//#define DEBUG_LINEAR

// *******************************************************************
// *                             Globals                             *
// *******************************************************************

option* Solver;
option* Relaxation;
option* Iters;
option* Precision;

option_const Power_oc("POWER", "Power method");
option_const Jacobi_oc("JACOBI", "Jacobi");
option_const Gauss_oc("GAUSS_SEIDEL", "Gauss-Seidel");
option_const Sor_oc("SOR", "Adaptive successive over-relaxation");

option_const* POWER = &Power_oc;
option_const* JACOBI = &Jacobi_oc;
option_const* GAUSS_SEIDEL = &Gauss_oc;
option_const* SOR = &Sor_oc;

// *******************************************************************
// *                                                                 *
// *                     Solvers for explicit MCs                    *
// *                                                                 *
// *******************************************************************

int SSColJacobi(double *pi, labeled_digraph <float> *Q, float *h, 
              int start, int stop)
{
  if (Verbose.IsActive()) {
    Verbose << "Starting column-wise Jacobi\n";
    Verbose.flush();
  }
  DCASSERT(Q->isTransposed);
  int iters;
  int maxiters = Iters->GetInt();
  double relax = Relaxation->GetReal();
  double one_minus_relax = 1.0 - relax;
  double prec = Precision->GetReal();

  // auxiliary vector
  float* oldpi = new float[stop-start];
  oldpi -= start;

  // start iterations
  for (iters=1; iters<=maxiters; iters++) {
#ifdef DEBUG_LINEAR
    Output << "\tJacobi starting iteration " << iters << "\n";
    Output << "\tcurrent solution vector: [";
    Output.PutArray(pi+start, stop-start);
    Output << "]\n";
    Output.flush();
#endif
    // copy pi into oldpi
    for (int s=start; s<stop; s++) oldpi[s] = pi[s];
    // iteration...
    double maxerror = 0;
    double total = 0;
    int *ci = Q->column_index + Q->row_pointer[start];
    float *v = Q->value + Q->row_pointer[start];
    for (int s=start; s<stop; s++) { 
      // compute new element s
      int *cstop = Q->column_index + Q->row_pointer[s+1];
      double tmp = 0.0;
      while (ci < cstop) {
        tmp += oldpi[ci[0]] * v[0];
        ci++;
        v++;
      }
      // now, tmp is dot product of pi and column s of Q
      tmp *= relax * h[s];
      pi[s] = tmp + one_minus_relax * oldpi[s];
      total += pi[s]; 
      double delta = pi[s] - oldpi[s];
      // if relative precision...
      if (pi[s]) delta /= pi[s];
      if (delta<0) delta = -delta;
      maxerror = MAX(maxerror, delta);
    } // for s
    // normalize
    for (int s=start; s<stop; s++) pi[s] /= total;
#ifdef DEBUG_LINEAR
    Output << "\tJacobi finishing iteration " << iters;
    Output << ", precision is " << maxerror << "\n";
    Output.flush();
#endif
    if (maxerror < prec) break; 
  } // for iters

  // done with aux vector
  oldpi += start;
  delete[] oldpi;

  return iters;
}

int SSRowJacobi(double *pi, labeled_digraph <float> *Q, float *h, 
              int start, int stop)
{
  if (Verbose.IsActive()) {
    Verbose << "Starting row-wise Jacobi\n";
    Verbose.flush();
  }
  DCASSERT(!Q->isTransposed);
  int iters;
  int maxiters = Iters->GetInt();
  double relax = Relaxation->GetReal();
  double one_minus_relax = 1.0 - relax;
  double prec = Precision->GetReal();

  // auxiliary vector
  float* oldpi = new float[stop-start];
  oldpi -= start;

  for (iters=1; iters<=maxiters; iters++) {
#ifdef DEBUG_LINEAR
    Output << "\tJacobi starting iteration " << iters << "\n";
    Output << "\trelaxation value: " << relax << "\n";
    Output << "\tcurrent solution vector: [";
    Output.PutArray(pi+start, stop-start);
    Output << "]\n";
    Output.flush();
#endif
    // copy pi into oldpi, and clear out pi
    for (int s=start; s<stop; s++) {
      oldpi[s] = pi[s];
      pi[s] = 0.0;
    }
    // iteration...
    int *ci = Q->column_index + Q->row_pointer[start];
    float *v = Q->value + Q->row_pointer[start];
    // pi = oldpi * Q
    for (int s=start; s<stop; s++) { 
      double alpha = oldpi[s];

      // pi += alpha * Q[s, all]
     
      int *cstop = Q->column_index + Q->row_pointer[s+1];
      while (ci < cstop) {
        pi[ci[0]] += alpha * v[0];
        ci++;
        v++;
      }
    }
    // finish the iteration
    double maxerror = 0;
    double total = 0;
    for (int s=start; s<stop; s++) {
      pi[s] *= relax * h[s];
      pi[s] += one_minus_relax * oldpi[s];
      total += pi[s]; 
      double delta = pi[s] - oldpi[s];
      // if relative precision...
      if (pi[s]) delta /= pi[s];
      if (delta<0) delta = -delta;
      maxerror = MAX(maxerror, delta);
    } // for s
    // normalize
    for (int s=start; s<stop; s++) pi[s] /= total;
#ifdef DEBUG_LINEAR
    Output << "\tJacobi finishing iteration " << iters;
    Output << ", precision is " << maxerror << "\n";
    Output.flush();
#endif
    if (maxerror < prec) break; 
  } // for iters

  // done with aux vector
  oldpi += start;
  delete[] oldpi;

  return iters;
}

int SSGaussSeidel(double *pi, labeled_digraph <float> *Q, float *h, 
                  int start, int stop)
{
  if (Verbose.IsActive()) {
    Verbose << "Starting Gauss-Seidel\n";
    Verbose.flush();
  }
  DCASSERT(Q->isTransposed);
  int iters;
  int maxiters = Iters->GetInt();
  double relax = Relaxation->GetReal();
  double one_minus_relax = 1.0 - relax;
  double prec = Precision->GetReal();

  // start iterations
  for (iters=1; iters<=maxiters; iters++) {
#ifdef DEBUG_LINEAR
    Output << "\tGauss-Seidel starting iteration " << iters << "\n";
    Output << "\tcurrent solution vector: [";
    Output.PutArray(pi+start, stop-start);
    Output << "]\n";
    Output.flush();
#endif
    double maxerror = 0;
    double total = 0;
    int *ci = Q->column_index + Q->row_pointer[start];
    float *v = Q->value + Q->row_pointer[start];
    for (int s=start; s<stop; s++) { 
      // compute new element s
      int *cstop = Q->column_index + Q->row_pointer[s+1];
      double newpi = 0.0;
      while (ci < cstop) {
        newpi += pi[ci[0]] * v[0];
        ci++;
        v++;
      }
      // now, newpi is dot product of pi and column s of Q
      newpi *= relax * h[s];
      newpi += one_minus_relax * pi[s];
      double delta = newpi - pi[s];
      // if relative precision...
      if (newpi) delta /= newpi;
      if (delta<0) delta = -delta;
      maxerror = MAX(maxerror, delta);
      total += (pi[s] = newpi);
    } // for s
    // normalize
    for (int s=start; s<stop; s++) pi[s] /= total;
#ifdef DEBUG_LINEAR
    Output << "\tGauss-Seidel finishing iteration " << iters;
    Output << ", precision is " << maxerror << "\n";
    Output.flush();
#endif
    if (maxerror < prec) break; 
  } // for iters
  return iters;
}



// *******************************************************************
// *                           Front  ends                           *
// *******************************************************************

void SSSolve(double *pi, labeled_digraph <float> *Q, float *h,
	     int start, int stop)
{
  DCASSERT(start < stop);
  if (Verbose.IsActive()) {
    Verbose << "Solving pi Q = 0\n";
    Verbose.flush();
  }
  int count = 0;
  if (Solver->GetEnum()==JACOBI) {
    if (Q->isTransposed) count = SSColJacobi(pi, Q, h, start, stop);
    else count = SSRowJacobi(pi, Q, h, start, stop);
  } else if (Solver->GetEnum()==GAUSS_SEIDEL) {
    if (Q->isTransposed) count = SSGaussSeidel(pi, Q, h, start, stop);
    else {
      Error.Start();
      Error << "Gauss-Seidel requires matrix stored by columns\n";
      Error.Stop();
      count = 0;
    }
  } else {
	Internal.Start(__FILE__, __LINE__);
	Internal << "Bad value for option  " << Solver << "\n";
 	Internal.Stop();
  } 
  if (Verbose.IsActive()) {
    Verbose << "Finished, " << count << " iterations\n";
    Verbose.flush();
  }
}

void MTTASolve(double *n, labeled_digraph <float> *Q, float *h,
	       sparse_vector <float> *init,
	       int start, int stop)
{
  DCASSERT(start < stop);
  if (Verbose.IsActive()) {
    Verbose << "Solving n Q = -init\n";
    Verbose.flush();
  }
}

void InitLinear()
{
  // Solver option
  option_const **solverlist = new option_const*[4];
  // alphabetical order here
  solverlist[0] = GAUSS_SEIDEL;
  solverlist[1] = JACOBI;
  solverlist[2] = POWER;
  solverlist[3] = SOR;
  Solver = MakeEnumOption("Solver", "Numerical method to use when solving linear systems", solverlist, 4, JACOBI);
  AddOption(Solver);

  // Relaxation option  
  Relaxation = MakeRealOption("Relaxation", "(Initial) Relaxation parameter for linear solvers", 1.0, 0.0, 2.0);
  AddOption(Relaxation);

  // MaxIters option
  Iters = MakeIntOption("MaxIters", "Maximum number of iterations for linear solvers", 1000, 1, 2000000000);
  AddOption(Iters);

  // Precision option
  Precision = MakeRealOption("Precision", "Desired precision for linear solvers", 1e-5, 0, 1);
  AddOption(Precision);
}


