
// $Id$

#include "sets.h"
//@Include: sets.h

/** @name sets.cc
    @type File
    @args \ 

   Implementation of simple set stuff.

 */

//@{

// ******************************************************************
// *                                                                *
// *                        set_result class                        *
// *                                                                *
// ******************************************************************

set_result::set_result(int s)
{
  size = s;
}

set_result::~set_result()
{
  // nothing to do
}

// ******************************************************************
// *                                                                *
// *                       int_interval class                       *
// *                                                                *
// ******************************************************************

/**  An interval set of integers.
     These have the form {start..stop..inc}.
 */
class int_interval : public set_result {
  int start, stop, inc;
public:
  int_interval(int s, int e, int i);
  virtual ~int_interval() { }
  virtual void GetElement(int n, result &x);
  virtual int IndexOf(const result &x);
  virtual void GetOrder(int n, int &i, result &x);
  virtual void show(ostream &s);
};

int_interval::int_interval(int s, int e, int i) 
  : set_result(int((e-s)/i + 1))
{
  start = s;
  stop = e;
  inc = i;
}

void int_interval::GetElement(int n, result &x)
{
  DCASSERT(n>=0);
  DCASSERT(n<Size());
  x.Clear();
  x.ivalue = start + n * inc;
}

int int_interval::IndexOf(const result &x)
{
  if (x.error || x.null || x.infinity) return -1;
  int i = int((x.ivalue-start)/inc);
  if (i>=Size()) return -1;
  if (i<0) return -1;
  return i;
}

void int_interval::GetOrder(int n, int &i, result &x)
{
  DCASSERT(n>=0);
  DCASSERT(n<Size());
  x.Clear();
  if (inc>0) i = n;
  else i = Size()-n-1;
  x.ivalue = start + i * inc;
}

void int_interval::show(ostream &s)
{
  s << "{" << start << ".." << stop << ".." << inc << "}";
}

// ******************************************************************
// *                                                                *
// *                     generic_int_set  class                     *
// *                                                                *
// ******************************************************************

/**  A generic set of integers.
     This can be used to represent the empty set,
     sets of a single element, and arbitrary sets.
 */
class generic_int_set : public set_result {
  /// Values in the set.  Must be sorted.
  int* values;
  /// The order of the values.
  int* order;
public:
  generic_int_set(int s, int* v, int* o);
  virtual ~generic_int_set(); 
  virtual void GetElement(int n, result &x);
  virtual int IndexOf(const result &x);
  virtual void GetOrder(int n, int &i, result &x);
  virtual void show(ostream &s);
};

generic_int_set::generic_int_set(int s, int* v, int* o) : set_result(s)
{
  values = v;
  order = o;
}

generic_int_set::~generic_int_set()
{
  delete[] values;
  delete[] order;
}

void generic_int_set::GetElement(int n, result &x)
{
  DCASSERT(n>=0);
  DCASSERT(n<Size());
  x.Clear();
  x.ivalue = values[n];
}

int generic_int_set::IndexOf(const result &x)
{
  if (x.error || x.null || x.infinity) return -1;
  // binary search through values
  int low = 0, high = Size()-1;
  while (low <= high) {
    int mid = (low+high)/2;
    int i = order[mid];
    DCASSERT(i>=0);
    DCASSERT(i<Size());
    if (values[i] == x.ivalue) return i;
    if (values[i] < x.ivalue) 
      low = mid+1;
    else
      high = mid-1;
  }
  return -1;
}

void generic_int_set::GetOrder(int n, int &i, result &x)
{
  DCASSERT(n>=0);
  DCASSERT(n<Size());
  i = order[n];
  DCASSERT(i>=0);
  DCASSERT(i<Size());
  x.Clear();
  x.ivalue = values[i];
}

void generic_int_set::show(ostream &s)
{
  s << "{";
  int i;
  if (Size()) {
    s << values[0];
    for (i=1; i<Size(); i++) s << ", " << values[i];
  }
  s << "}";
}


// ******************************************************************
// *                                                                *
// *                     setexpr_interval class                     *
// *                                                                *
// ******************************************************************

/** An expression to build an integer set interval.
    Those have the form {start..stop..inc}.
    If any of those are null, then we build a null set.
 */
class setexpr_interval : public expr {
protected:
  expr* start;
  expr* stop;
  expr* inc;
public:
  setexpr_interval(const char* fn, int line, expr* s, expr* e, expr* i);
  virtual ~setexpr_interval();
  virtual type Type(int i) const;
  virtual void Compute(int i, result &x);
  virtual expr* Substitute(int i);
  virtual int GetSymbols(int i, symbol **syms=NULL, int N=0, int offset=0);
  virtual void show(ostream &s) const;
};

setexpr_interval::setexpr_interval(const char* f, int l, expr* s, expr* e, expr* i) : expr(f, l)
{
  start = s;
  stop = e;
  inc = i;
}

setexpr_interval::~setexpr_interval()
{
  Delete(start);
  Delete(stop);
  Delete(inc);
}

type setexpr_interval::Type(int i) const
{
  DCASSERT(0==i);
  return SET_INT;
}

void setexpr_interval::Compute(int n, result &x)
{
  DCASSERT(0==n);
  DCASSERT(start);
  DCASSERT(stop);
  DCASSERT(inc);
  x.Clear();
  result s,e,i;
  start->Compute(0, s);
  if (s.null || s.error) {
    x = s;
    return;
  }
  stop->Compute(0, e);
  if (e.null || e.error) {
    x = e;
    return;
  }
  inc->Compute(0, i);
  if (i.null || i.error) {
    x = i;
    return;
  }
  // special cases here
  if (s.infinity || e.infinity) {
    // Print error message here
    x.error = CE_Undefined;
    return;
  } 
  if (i.infinity || i.ivalue==0) {
    // that means an interval with just the start element.
    e.ivalue = s.ivalue;
    i.ivalue = 1;
  }
  set_result *xs = new int_interval(s.ivalue, e.ivalue, i.ivalue);
  x.other = xs;
}

expr* setexpr_interval::Substitute(int i)
{
  //  Is this even used?  Just in case...
  // 
  DCASSERT(0==i);
  DCASSERT(start);
  DCASSERT(stop);
  DCASSERT(inc);
  // keep copying to a minimum...
  expr* newstart = start->Substitute(0);
  expr* newstop = stop->Substitute(0);
  expr* newinc = inc->Substitute(0);
  // Anything change?
  if ((newstart==start) && (newstop==stop) && (newinc==inc)) {
    // Just copy ourself
    Delete(newstart);
    Delete(newstop);
    Delete(newinc);
    return Copy(this);
  }
  // something changed, make a new one
  return new setexpr_interval(Filename(), Linenumber(), newstart, newstop, newinc);
}

int setexpr_interval::GetSymbols(int i, symbol **syms, int N, int offset)
{
  DCASSERT(0==i);
  DCASSERT(start);
  DCASSERT(stop);
  DCASSERT(inc);
  int answer = 0;
  answer = start->GetSymbols(0, syms, N, offset);
  answer += stop->GetSymbols(0, syms, N, offset+answer);
  answer += inc->GetSymbols(0, syms, N, offset+answer);
  return answer;
}

void setexpr_interval::show(ostream &s) const
{
  s << start << ".." << stop << ".." << inc;
}

// ******************************************************************
// *                                                                *
// *                       intset_union class                       *
// *                                                                *
// ******************************************************************

/** A binary operator to handle the union of two integer sets.
    (Used when we make those ugly for loops.)
 */
class intset_union : public binary {
public:
  intset_union(const char* fn, int line, expr* l, expr* r) 
    : binary(fn, line, l, r) { };
  virtual type Type(int i) const;
  virtual void Compute(int i, result &x);
  virtual void show(ostream &s) const { s << left << ", " << right; }
protected:
  virtual expr* MakeAnother(expr* newleft, expr* newright) {
    return new intset_union(Filename(), Linenumber(), newleft, newright);
  }
};

type intset_union::Type(int i) const
{
  DCASSERT(0==i);
  return SET_INT;
}

void intset_union::Compute(int i, result &x)
{
  DCASSERT(0==i);
  DCASSERT(left);
  DCASSERT(right);
  x.Clear();
  result l;
  left->Compute(0, l); 
  if (l.error || l.null) {
    x = l;
    return;
  }
  set_result *ls = (set_result*) l.other;
  result r;
  right->Compute(0, r);
  if (r.error || r.null) {
    x = r;
    delete ls;
    return;
  }
  set_result *rs = (set_result*) r.other;

  // figure out how many elements in rs are also in ls.
  int duplicates = 0;
  int lp = 0, rp = 0;
  result lx, rx;
  int dumbl, dumbr;
  if (lp<ls->Size() && rp<rs->Size()) {
    ls->GetOrder(lp, dumbl, lx);
    rs->GetOrder(rp, dumbr, rx);
  }
  while (lp<ls->Size() && rp<rs->Size()) {
    if (lx.ivalue == rx.ivalue) {
      duplicates++;
      lp++;
      rp++;
      if (lp<ls->Size() && rp<rs->Size()) {
        ls->GetOrder(lp, dumbl, lx);
        rs->GetOrder(rp, dumbr, rx);
      }
    } else if (lx.ivalue < rx.ivalue) {
      // advance lp only
      lp++;
      if (lp<ls->Size()) ls->GetOrder(lp, dumbl, lx);
    } else {
      // advance rp only
      rp++;
      if (rp<rs->Size()) rs->GetOrder(rp, dumbr, rx);
    }
  }
  int newsize = ls->Size() + rs->Size() - duplicates;
  // We don't allow empty sets, so the union should have at least one element.
  DCASSERT(newsize>0);

  // Do a "mergesort"
  int* values = new int[newsize];
  int* order = new int[newsize];
  int lvptr = 0;
  int rvptr = ls->Size();
  int optr = 0;

  // tricky part here...

  set_result *answer = new generic_int_set(newsize, values, order);

  // temporary...

  cout << "Inside set union\n";
  cout << "The union of sets " << ls << " and " << rs << " is " << answer << "\n";
  
  x.other = answer;
  delete ls;
  delete rs;
}

// ******************************************************************
// *                                                                *
// *           Global functions  to build set expressions           *
// *                                                                *
// ******************************************************************

expr*  MakeInterval(expr* start, expr* stop, expr* inc)
{
  return NULL;
}

expr*  MakeElementSet(expr* element)
{
  return NULL;
}

expr*  MakeUnionOp(expr* left, expr* right)
{
  return NULL;
}

//@}

