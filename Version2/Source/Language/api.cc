
// $Id$

#include "api.h"

#include "infinity.h"

void SetStackSize(void *x, const char* f, int l)
{
  int newsize = ((int*)x)[0];
  if ((newsize<0) || (newsize>2000000000)) {
    Warning.Start(f, l);
    Warning << "Option #StackSize set out of range";
    Warning.Stop();
    return;
  }
  bool ok = ResizeRuntimeStack(newsize);
  if (!ok) {
    Warning.Start(f, l);
    Warning << "Attempt to set option #StackSize failed, ignoring";
    Warning.Stop();
  }
  // remove soon
  Output << "Successfully resized stack\n";
}

void InitLanguage()
{
  option *o;
  // initialize options

  // InfinityString
  char* inf = strdup("infinity");
  const char* doc1 = "Output string for infinity";
  infinity_string = MakeStringOption("InfinityString", doc1, inf);
  AddOption(infinity_string);

  // StackSize
  const char* ssdoc = "Size of run-time stack to use for function calls";
  o = MakeActionOption(INT, "StackSize", "1024", ssdoc, "[0..2000000000]", 
                       SetStackSize, NULL);
  AddOption(o);
  CreateRuntimeStack(1024); // Large enough?

  InitConvergeOptions();  // see converge.cc
  


  // testing... add more options

  index_precision = MakeRealOption("IndexPrecision", 
                "Epsilon for real array index comparisons", 1e-5, 0, 1e100);

  AddOption(index_precision);

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

  AddOption(test1);

  option_const** list2 = new option_const*[4];
  list2[0] = new option_const("GAUSS_SEIDEL", "Gauss-Seidel");
  list2[1] = new option_const("JACOBI", "Jacobi");
  list2[2] = new option_const("POWER", "Power method");
  list2[3] = new option_const("SOR", "Adaptive successive over-relaxation");

  option* solver = MakeEnumOption("Solver", "Numerical method to use when solving linear systems", list2, 4, list2[1]);

  AddOption(solver);
}
