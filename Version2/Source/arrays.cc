
// $Id$

#include "arrays.h"
//@Include: arrays.h

/** @name arrays.cc
    @type File
    @args \ 

   Implementation of user-defined arrays.

 */

//@{

// ******************************************************************
// *                                                                *
// *                      array_index  methods                      *
// *                                                                *
// ******************************************************************

array_index::array_index(const char *fn, int line, type t, char *n, expr *v) 
  : symbol(fn, line, t, n)
{
  values = v;
  current = NULL;
}

array_index::~array_index()
{
  Delete(values);
  Delete(current);
}

void array_index::Compute(int i, result &x)
{
  DCASSERT(i==0);
  x.Clear();
  if (NULL==current) {
    x.null = true;
    // set error condition?
    return;
  }
  current->GetElement(index, x);
}

void array_index::show(ostream &s) const
{
  s << Name();
}




// ******************************************************************
// *                                                                *
// *                         array  methods                         *
// *                                                                *
// ******************************************************************

array::array(const char* fn, int line, type t, char* n, array_index **il, int dim)
  : symbol(fn, line, t, n)
{
  index_list = il;
  dimension = dim;
  descriptor = NULL;
}
 
array::~array()
{
  // Does this *ever* get called?
  if (index_list) {
    int i;
    for (i=0; i<dimension; i++) Delete(index_list[i]);
    delete[] index_list;
  }
  // delete descriptor here...
}

void array::SetCurrentReturn(constfunc *retvalue)
{
  int i;
  array_desc *prev = NULL;
  array_desc *curr = descriptor;
  int lastindex = 0;
  for (i=0; i<dimension; i++) {
    if (NULL==curr) {
      curr = new array_desc(index_list[i]->CopyCurrent());
      if (prev) prev->down[lastindex] = curr;
      else descriptor = curr;
    }
    lastindex = index_list[i]->Index();
    prev = curr;
    curr = (array_desc*) curr->down[lastindex];
  }
  if (prev->down[lastindex]) {
    // we already have a value...
    cerr << "Internal error: array reassignment?\n";
    exit(0);
  }
  prev->down[lastindex] = retvalue;
}

void array::Compute(expr **il, result &x)
{
  x.Clear();
  int i;
  array_desc *ptr = descriptor;
  for (i=0; i<dimension; i++) {
    result y;
    il[i]->Compute(0, y);
    if (y.null) {
      x.null = true;
      return;
    }
    if (y.error) {
      // Trace the error here?
      x.error = y.error;
      return;
    }
    if (NULL==ptr) {
      // error?  
      x.null = true;
      return;
    }
    int ndx = ptr->values->IndexOf(y);
    if (ndx<0) {
      // range error
      x.error = CE_OutOfRange;
      // print something?
      return;
    }
    ptr = (array_desc*) ptr->down[ndx];
  }
  x.other = ptr;
}

void array::Sample(long &seed, expr **il, result &x)
{
  x.Clear();
  int i;
  array_desc *ptr = descriptor;
  for (i=0; i<dimension; i++) {
    result y;
    il[i]->Sample(seed, 0, y);
    if (y.null) {
      x.null = true;
      return;
    }
    if (y.error) {
      // Trace the error here?
      x.error = y.error;
      return;
    }
    if (NULL==ptr) {
      // error?  
      x.null = true;
      return;
    }
    int ndx = ptr->values->IndexOf(y);
    if (ndx<0) {
      // range error
      x.error = CE_OutOfRange;
      // print something?
      return;
    }
    ptr = (array_desc*) ptr->down[ndx];
  }
  x.other = ptr;
}




// ******************************************************************
// *                                                                *
// *                         acall  methods                         *
// *                                                                *
// ******************************************************************

acall::acall(const char *fn, int line, array *f, expr **p, int np)
  : expr(fn, line)
{
  func = f;
  pass = p;
  numpass = np;
}

acall::~acall()
{
  // does this ever get called?
  // don't delete func
  int i;
  for (i=0; i<numpass; i++) Delete(pass[i]);
  delete[] pass;
}

type acall::Type(int i) const
{
  DCASSERT(0==i);
  return func->Type(0);
}

void acall::Compute(int i, result &x)
{
  DCASSERT(0==i);
  func->Compute(pass, x);
  if (x.null) return;
  if (x.error) return;  // print message?
  constfunc* foo = (constfunc*) x.other;
  foo->Compute(0, x);
}

void acall::Sample(long &seed, int i, result &x)
{
  DCASSERT(0==i);
  func->Sample(seed, pass, x);
  if (x.null) return;
  if (x.error) return;  // print message?
  constfunc* foo = (constfunc*) x.other;
  foo->Sample(seed, 0, x);
}

expr* acall::Substitute(int i)
{
  // implement this!
  return NULL;
}

int acall::GetSymbols(int i, symbol **syms, int N, int offset)
{
  // implement this, too
  return 0;
}

void acall::show(ostream &s) const
{
  if (func->Name()==NULL) return; // hidden?
  s << func->Name();
  DCASSERT(numpass>0);
  s << "[" << pass[0];
  int i;
  for (i=0; i<numpass; i++) s << ", " << pass[i];
  s << "]";
}


//@}

