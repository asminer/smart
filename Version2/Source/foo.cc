
/* A Test of the new smart expression engine */

#include "arrays.h"
#include "functions.h"
#include "operators.h"

internal_func* MakeCond(type t);

array* fib;

/* Test: make a for loop
 */
statement* MakeFor(int n)
{
  expr* zero = MakeConstExpr(0, "foo", 42);
  expr* one = MakeConstExpr(1, "foo", 42);
  expr* two = MakeConstExpr(2, "foo", 42);
  expr* stop = MakeConstExpr(n, "foo", 42);

  expr* intrvl = MakeInterval("foo", 42, zero, stop, Copy(one));
 
  array_index *i = new array_index("foo", 42, INT, "i", intrvl);

  array_index **desc = new array_index*[1];
  desc[0] = i;
  fib = new array("foo", 42, INT, "fib", desc, 1);

  expr** pass = new expr*[3];

  pass[0] = MakeBinaryOp(Copy(i), GT, Copy(one));
  
  expr** ind1 = new expr*[1];
  ind1[0] = MakeBinaryOp(Copy(i), MINUS, Copy(one));
  expr** ind2 = new expr*[1];
  ind2[0] = MakeBinaryOp(Copy(i), MINUS, two);

  expr* fib1 = MakeArrayCall(fib, ind1, 1, "foo", 42);
  expr* fib2 = MakeArrayCall(fib, ind2, 1, "foo", 42);

  pass[1] = MakeBinaryOp(fib1, PLUS, fib2);
  pass[2] = Copy(one);
  
  function* cond = MakeCond(INT);

  expr* right = MakeFunctionCall(cond, pass, 3, "foo", 42);

  statement* assign = MakeArrayAssign(fib, right, "foo", 42);
  statement** block = new statement*[1];
  block[0] = assign;

  statement* floop = MakeForLoop(i, block, 1, "foo", 42);
  return floop;
}

int main()
{
  cout << "Hello world\n";

  int n;
  cout << "Enter n: \n";
  cin >> n;

  statement* buh = MakeFor(n);

  buh->showfancy(0, cout);

  cout << "Executing" << endl;

  buh->Execute();

  cout << "Ok!\n";

  expr** inds = new expr*[1];
  inds[0] = MakeConstExpr(n, "foo", 45);
  expr* test = MakeArrayCall(fib, inds, 1, "foo", 45);

  cout << "Test expression: " << test << "\n";

  cout << "Computing" << endl;

  result answer;
  test->Compute(0, answer);

  if (answer.null) cout << "Got null!\n";
  if (answer.error) cout << "Error: " << answer.error << "\n";

  cout << "Got: " << answer.ivalue << "\n";

  cout << "Exiting\n";
  return 0;
}
