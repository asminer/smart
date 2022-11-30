
#include "bogus.h"
#include "exprman.h"

// ******************************************************************
// *                       bogus_expr methods                       *
// ******************************************************************

bogus_expr::bogus_expr(const char* w)
    : expr( location::NOWHERE(), (typelist*) 0)
{
  which = w;
}

bool bogus_expr::Print(std::ostream &s, int width) const
{
  s << which;
  return (which[0]!=0);
}

void bogus_expr::PrintType(std::ostream &s) const
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
    internal_error E(__FILE__, __LINE__);
    E << "Trying to traverse the " << which << " expression";
}


