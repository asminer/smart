
// $Id$

#include "baseops.h"
//@Include: baseops.h

/** @name baseops.cc
    @type File
    @args \ 

   Implementation of base expression classes for things like addition.

 */

//@{

// ******************************************************************
// *                                                                *
// *                          negop  class                          *
// *                                                                *
// ******************************************************************

void negop::show(ostream &s) const 
{
  type t = Type(0); // sneaky...  this is slow but so's I/O so who cares
  const char* op = "-";
  if ((t==BOOL) || (t==RAND_BOOL) || (t==PROC_BOOL) || (t==PROC_RAND_BOOL))
    op = "!";
  unary_show(s, op);
}

// ******************************************************************
// *                                                                *
// *                          addop  class                          *
// *                                                                *
// ******************************************************************

void addop::show(ostream &s) const 
{
  type t = Type(0); // sneaky...  this is slow but so's I/O so who cares
  const char* op = "+";
  if ((t==BOOL) || (t==RAND_BOOL) || (t==PROC_BOOL) || (t==PROC_RAND_BOOL))
    op = "|";
  assoc_show(s, op);
}

int addop::GetSums(int a, expr **sums, int N, int offset)
{
  DCASSERT(a==0);
  int i;
  int opnds=0;
  for (i=0; i<opnd_count; i++) {
    int count = operands[i]->GetSums(a, sums, N, offset);
    offset += count;
    opnds += count;
  }
  return opnds;
}

// ******************************************************************
// *                                                                *
// *                          multop class                          *
// *                                                                *
// ******************************************************************

void multop::show(ostream &s) const 
{
  type t = Type(0); // sneaky...  this is slow but so's I/O so who cares
  const char* op = "*";
  if ((t==BOOL) || (t==RAND_BOOL) || (t==PROC_BOOL) || (t==PROC_RAND_BOOL))
    op = "&";
  assoc_show(s, op);
}

int multop::GetProducts(int a, expr **prods, int N, int offset)
{
  DCASSERT(a==0);
  int i;
  int opnds=0;
  for (i=0; i<opnd_count; i++) {
    int count = operands[i]->GetProducts(a, prods, N, offset);
    offset += count;
    opnds += count;
  }
  return opnds;
}


//@}

