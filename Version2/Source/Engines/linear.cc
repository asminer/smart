
// $Id$

/** Linear solvers
*/

#include "../Base/options.h"
#include "linear.h"

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
// *                             Solvers                             *
// *******************************************************************

// *******************************************************************
// *                           Front  ends                           *
// *******************************************************************

void SSSolve(double *pi, labeled_digraph <float> *Q, float *h,
	     int start, int stop)
{
  if (Verbose.IsActive()) {
    Verbose << "Solving pi Q = 0\n";
    Verbose.flush();
  }
}

void MTTASolve(double *n, labeled_digraph <float> *Q, float *h,
	       sparse_vector <float> *init,
	       int start, int stop)
{
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


