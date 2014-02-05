
// $Id$

#include "casting.h"
#include "../Streams/streams.h"
#include "unary.h"


// ******************************************************************
// *                                                                *
// *                       base_conv  methods                       *
// *                                                                *
// ******************************************************************

base_conv::base_conv()
{
}

base_conv::~base_conv()
{
}

// ******************************************************************
// *                                                                *
// *                     specific_conv  methods                     *
// *                                                                *
// ******************************************************************

specific_conv::specific_conv(bool c)
{
  is_cast = c;
}

// ******************************************************************
// *                                                                *
// *                        typecast methods                        *
// *                                                                *
// ******************************************************************

typecast::typecast(const char* fn, int line, const type* newt, expr* x)
 : unary(fn, line, newt, x) 
{ 
  silent = false;
}

bool typecast::Print(OutputStream &s, int) const
{
  DCASSERT(opnd);

  bool printed = false;
  if (!silent) {
    DCASSERT(Type());
    s.Put(Type()->getName());
    s.Put('(');
    printed = true;
  }

  if (opnd->Print(s, 0))  printed = true;

  if (!silent) s.Put(')');

  return printed;
}

void typecast::Compute(traverse_data &x)
{
  DCASSERT(opnd);
  opnd->Compute(x);
}

expr* typecast::buildAnother(expr* x) const
{
  return new typecast(Filename(), Linenumber(), Type(), x);
}

