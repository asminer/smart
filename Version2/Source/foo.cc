
/* A Test of the new smart expression engine */

#include "sets.h"


int main()
{
  cout << "Hello world\n";

  expr* start1 = MakeConstExpr(11, "foo", 42);
  expr* stop1 = MakeConstExpr(1, "foo", 42);
  expr* inc1 = MakeConstExpr(-2, "foo", 42);

  expr* start2 = MakeConstExpr(3, "foo", 42);
  expr* stop2 = MakeConstExpr(9, "foo", 42);
  expr* inc2 = MakeConstExpr(3, "foo", 42);

  expr* left = MakeInterval("foo", 42, start1, stop1, inc1);
  expr* right = MakeInterval("foo", 42, start2, stop2, inc2);

  expr* un = MakeUnionOp("foo", 42, left, right);

  cout << "made set expression {" << un << "}\n";

  cout << "Evaluating expression\n";

  result x;
  un->Compute(0, x);

  cout << "Exiting\n";
  return 0;
}
