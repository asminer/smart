
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

int addop::GetSums(int a, expr **sums=NULL, int N=0, int offset=0)
{
  DCASSERT(a==0);
  int i;
  for (i=0; i<opnd_count; i++) 
    if (i+offset<N) sums[i+offset] = operands[i];
  return opnd_count;
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

int multop::GetProducts(int a, expr **prods=NULL, int N=0, int offset=0)
{
  DCASSERT(a==0);
  int i;
  for (i=0; i<opnd_count; i++) 
    if (i+offset<N) prods[i+offset] = operands[i];
  return opnd_count;
}


//@}

