
/* A Test of the new smart expression engine */

#include "functions.h"
#include "operators.h"
#include "initfuncs.h"

#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>

internal_func* MakeCond(type t);  // Defined in initfuncs.cc

function* MakeFib()
{

  formal_param **pl = new formal_param*[1];
  pl[0] = new formal_param("foo", 42, INT, "n");
  user_func *f = new user_func("foo.sm", 42, INT, "fib", pl, 1);

  internal_func *cond = MakeCond(INT);
  
  expr *one = MakeConstExpr(1, "foo", 42);
  expr *two = MakeConstExpr(2, "foo", 42);

  expr **pass1 = new expr*[1];
  pass1[0] = MakeBinaryOp(pl[0], MINUS, one, "foo", 42);
  expr *rec1 = new fcall("foo", 42, f, pass1, 1);

  expr **pass2 = new expr*[1];
  pass2[0] = MakeBinaryOp(pl[0], MINUS, two, "foo", 42);
  expr *rec2 = new fcall("foo", 42, f, pass2, 1);

  expr **pass3 = new expr*[3];
  pass3[0] = MakeBinaryOp(pl[0], GT, Copy(one), "foo", 42);
  pass3[1] = MakeBinaryOp(rec1, PLUS, rec2, "foo", 42);
  pass3[2] = Copy(one);

  expr *foo = new fcall("foo", 42, cond, pass3, 3);

  f->SetReturn(foo);
  return f;
}

expr* MakeCall(function *f, int i)
{
  expr **pp = new expr*[1];
  pp[0] = MakeConstExpr(i, "foo", 99);
  fcall *e = new fcall("foo", 99, f, pp, 1);
  return e;
}


int main()
{
  cout << "Hello world\n";
  CreateRuntimeStack(1024);
  cout << "Run-time stack is up\n";

  function *f = MakeFib();
  cout << "Made fibonacci function:\n";
  cout << f << "\n";

  int n;
  cout << "Value of n?\n";
  cin >> n;
  expr *call = MakeCall(f, n);
  cout << "Made function call:\n";
  cout << call << "\n";

  cout << "Evaluating expression\n";
  rusage current;
  double start, stop;
  int test = getrusage(RUSAGE_SELF, &current);
  start = current.ru_utime.tv_sec + current.ru_utime.tv_usec/ 1000000.0;

  result x;
  call->Compute(0, x);

  test = getrusage(RUSAGE_SELF, &current);
  stop = current.ru_utime.tv_sec + current.ru_utime.tv_usec/ 1000000.0;

  cout << "done in " << stop-start << " seconds\n";
 
  cout << " Got : " << x.ivalue << "\n";
  
  DestroyRuntimeStack();
  cout << "Run-time stack is gone\n";
  cout << "Exiting\n";
  return 0;
}
