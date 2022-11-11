
#include "iterators.h"
#include "../Streams/streams.h"
#include "../Options/options.h"
#include "result.h"
#include "exprman.h"

// ******************************************************************
// *                                                                *
// *                        iterator methods                        *
// *                                                                *
// ******************************************************************

iterator::iterator(const location &W, const type* t, char *n, expr *v)
  : symbol(W, t, n)
{
  values = v;
  current = 0;
}

iterator::~iterator()
{
  Delete(values);
  Delete(current);
}

void iterator::Compute(traverse_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  // x.answer->Clear();
  if (NULL==current) {
    x.answer->setNull();
    // set error condition?
    return;
  }
  current->GetElement(index, x.answer[0]);
}

void iterator::PrintAll(OutputStream &s) const
{
  s.Put(Name());
  s.Put(" in {");
  if (values) values->Print(s, 0);
  else        s.Put("null");
  s.Put('}');
}

void iterator::ComputeCurrent(traverse_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  result* answer = x.answer;
  result foo;
  x.answer = &foo;
  SafeCompute(values, x);
  x.answer = answer;

  current = Share( smart_cast <shared_set*> (foo.getPtr()) );

  if (expr_debug.startReport()) {
    expr_debug.report() << "computed set: ";
    if (current)  current->Print(expr_debug.report(), 0);
    else          expr_debug.report() << "null";
    expr_debug.report() << "\n";
    expr_debug.stopIO();
  }
}

void iterator::ShowAssignment(OutputStream &s) const
{
  DCASSERT(Type());
  result foo;
  current->GetElement(index, foo);
  s << Name() << "=";
  Type()->print(s, foo);
}


