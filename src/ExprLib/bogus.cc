
#include "bogus.h"
#include "../Streams/streams.h"
#include "exprman.h"

// ******************************************************************
// *                       bogus_expr methods                       *
// ******************************************************************

bogus_expr::bogus_expr(const char* w) : expr(0, -1, (typelist*) 0)
{
  which = w;
}

bool bogus_expr::Print(OutputStream &s, int width) const
{
  s << which;
  return (which[0]!=0);
}

void bogus_expr::PrintType(OutputStream &s) const
{
  s << which;
}

bool bogus_expr::Equals(const shared_object* o) const
{
  if (o==this) return true;
  const bogus_expr* foo = dynamic_cast <const bogus_expr*> (o);
  if (0==foo) return false;
  if (0==which)  return (0==foo->which);
  if (0==foo->which)  return false;
  return 0==strcmp(which, foo->which);
}

void bogus_expr::Traverse(traverse_data &x)
{
  DCASSERT(em);
  if (em->startInternal(__FILE__, __LINE__)) {
    em->noCause();
    em->internal() << "Trying to traverse the " << which << " expression";
    em->stopIO();
  }
}


