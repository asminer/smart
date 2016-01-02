
// $Id:$

#include "../ExprLib/mod_vars.h"
#include "../ExprLib/exprman.h"
#include "rgr.h"

reachgraph::reachgraph()
{
}

reachgraph::~reachgraph()
{
}

bool reachgraph::Print(OutputStream &s, int width) const
{
  // Required for shared object, but will we ever call it?
  s << "reachgraph (why is it printing?)";
  return true;
}

bool reachgraph::Equals(const shared_object* o) const
{
  return (this == o);
}


