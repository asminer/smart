
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
  char op = '-';
  if ((t==BOOL) || (t==RAND_BOOL) || (t==PROC_BOOL) || (t==PROC_RAND_BOOL))
    op = '!';
  s << op << opnd;
}

// ******************************************************************
// *                                                                *
// *                          addop  class                          *
// *                                                                *
// ******************************************************************

void addop::show(ostream &s) const 
{
  type t = Type(0); // sneaky...  this is slow but so's I/O so who cares
  char opnd = '+';
  if ((t==BOOL) || (t==RAND_BOOL) || (t==PROC_BOOL) || (t==PROC_RAND_BOOL))
    opnd = '|';
  s << "(" << left << opnd << right << ")";
}

int addop::GetSums(int i, expr **sums=NULL, int N=0, int offset=0)
{
  DCASSERT(i==0);
  int answer = 0;
  if (left) {
    answer = left->GetSums(0, sums, N, offset);
  } else {
    answer = 1;
    if (offset<N) sums[offset] = NULL;
  }
  if (right) {
    answer += right->GetSums(0, sums, N, offset+answer);
  } else {
    if (offset+answer<N) sums[offset+answer] = NULL;
    answer++;
  }
  return answer;
}

// ******************************************************************
// *                                                                *
// *                          subop  class                          *
// *                                                                *
// ******************************************************************

void subop::show(ostream &s) const 
{
  s << "(" << left << "-" << right << ")";
}

// ******************************************************************
// *                                                                *
// *                          multop class                          *
// *                                                                *
// ******************************************************************

void multop::show(ostream &s) const 
{
  type t = Type(0); // sneaky...  this is slow but so's I/O so who cares
  char opnd = '*';
  if ((t==BOOL) || (t==RAND_BOOL) || (t==PROC_BOOL) || (t==PROC_RAND_BOOL))
    opnd = '&';
  s << "(" << left << opnd << right << ")";
}

int multop::GetProducts(int i, expr **prods=NULL, int N=0, int offset=0)
{
  DCASSERT(i==0);
  int answer = 0;
  if (left) {
    answer = left->GetProducts(0, prods, N, offset);
  } else {
    answer = 1;
    if (offset<N) prods[offset] = NULL;
  }
  if (right) {
    answer += right->GetProducts(0, prods, N, offset+answer);
  } else {
    if (offset+answer<N) prods[offset+answer] = NULL;
    answer++;
  }
  return answer;
}

// ******************************************************************
// *                                                                *
// *                          divop  class                          *
// *                                                                *
// ******************************************************************

void divop::show(ostream &s) const 
{
  s << "(" << left << "/" << right << ")";
}

// ******************************************************************
// *                                                                *
// *                        consteqop  class                        *
// *                                                                *
// ******************************************************************

void consteqop::show(ostream &s) const 
{
  s << "(" << left << "==" << right << ")";
}

// ******************************************************************
// *                                                                *
// *                        constneqop class                        *
// *                                                                *
// ******************************************************************

void constneqop::show(ostream &s) const 
{
  s << "(" << left << "!=" << right << ")";
}

// ******************************************************************
// *                                                                *
// *                        constgtop  class                        *
// *                                                                *
// ******************************************************************

void constgtop::show(ostream &s) const 
{
  s << "(" << left << ">" << right << ")";
}

// ******************************************************************
// *                                                                *
// *                        constgeop  class                        *
// *                                                                *
// ******************************************************************

void constgeop::show(ostream &s) const 
{
  s << "(" << left << ">=" << right << ")";
}

// ******************************************************************
// *                                                                *
// *                        constltop  class                        *
// *                                                                *
// ******************************************************************

void constltop::show(ostream &s) const 
{
  s << "(" << left << "<" << right << ")";
}

// ******************************************************************
// *                                                                *
// *                        constleop  class                        *
// *                                                                *
// ******************************************************************

void constleop::show(ostream &s) const 
{
  s << "(" << left << "<=" << right << ")";
}

// ******************************************************************
// *                                                                *
// *                           eqop class                           *
// *                                                                *
// ******************************************************************

void eqop::show(ostream &s) const 
{
  s << "(" << left << "==" << right << ")";
}

// ******************************************************************
// *                                                                *
// *                          neqop  class                          *
// *                                                                *
// ******************************************************************

void neqop::show(ostream &s) const 
{
  s << "(" << left << "!=" << right << ")";
}

// ******************************************************************
// *                                                                *
// *                           gtop class                           *
// *                                                                *
// ******************************************************************

void gtop::show(ostream &s) const 
{
  s << "(" << left << ">" << right << ")";
}

// ******************************************************************
// *                                                                *
// *                           geop class                           *
// *                                                                *
// ******************************************************************

void geop::show(ostream &s) const 
{
  s << "(" << left << ">=" << right << ")";
}

// ******************************************************************
// *                                                                *
// *                           ltop class                           *
// *                                                                *
// ******************************************************************

void ltop::show(ostream &s) const 
{
  s << "(" << left << "<" << right << ")";
}

// ******************************************************************
// *                                                                *
// *                           leop class                           *
// *                                                                *
// ******************************************************************

void leop::show(ostream &s) const 
{
  s << "(" << left << "<=" << right << ")";
}

//@}

