
// $Id$

#include "api.h"

#include "infinity.h"

void InitLanguage()
{
  CreateRuntimeStack(1024); // Large enough?

  // initialize options
  char* inf = strdup("infinity");
  const char* doc1 = "Output string for infinity";
  infinity_string = MakeStringOption("InfinityString", doc1, inf);
  AddOption(infinity_string);

  // testing... add more options

  option* foo = MakeRealOption("IndexPrecision", 
                "Epsilon for real array index comparisons", 1e-5, 0, 1e100);

  AddOption(foo);

  option* bar = MakeIntOption("Iterations", "Max iterations allowed", 
  				1000, 0, 2000000000);
  AddOption(bar);

  option_const* relative = new option_const("RELATIVE", "Use relative precision");
  option_const* absolute = new option_const("ABSOLUTE", "Use absolute precision");

  option_const** list = new option_const*[2];
  list[0] = absolute;
  list[1] = relative;

  option* test1 = MakeEnumOption("IndexComparison", 
     "Method to compare real array indices", list, 2, relative);

  option* test2 = MakeEnumOption("ConvergeComparison",
     "Method to compare converge values", list, 2, relative);

  AddOption(test1);
  AddOption(test2);

  option_const** list2 = new option_const*[4];
  list2[0] = new option_const("GAUSS_SEIDEL", "Gauss-Seidel");
  list2[1] = new option_const("JACOBI", "Jacobi");
  list2[2] = new option_const("POWER", "Power method");
  list2[3] = new option_const("SOR", "Adaptive successive over-relaxation");

  option* solver = MakeEnumOption("Solver", "Numerical method to use when solving linear systems", list2, 4, list2[1]);

  AddOption(solver);
}
