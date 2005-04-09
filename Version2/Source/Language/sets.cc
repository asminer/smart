
// $Id$

#include "sets.h"
#include "measures.h"
#include "../Base/memtrack.h"

#include "math.h"

//@Include: sets.h

/** @name sets.cc
    @type File
    @args \ 

   Implementation of simple set stuff.

 */

//@{

//#define UNION_DEBUG

option* index_precision;

// ******************************************************************
// *                                                                *
// *                        set_result class                        *
// *                                                                *
// ******************************************************************

set_result::set_result(int s) : shared_object() 
{
  ALLOC("set_result", sizeof(set_result));
  size = s;
}

set_result::~set_result()
{
  FREE("set_result", sizeof(set_result));
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
  virtual void show(OutputStream &s);
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
  CHECK_RANGE(0, n, Size());
  x.Clear();
  x.ivalue = start + n * inc;
}

int int_interval::IndexOf(const result &x)
{
  if (!x.isNormal()) return -1;
  int i = int((x.ivalue-start)/inc);
  if (i>=Size()) return -1;
  if (i<0) return -1;
  return i;
}

void int_interval::GetOrder(int n, int &i, result &x)
{
  CHECK_RANGE(0, n, Size());
  x.Clear();
  if (inc>0) i = n;
  else i = Size()-n-1;
  x.ivalue = start + i * inc;
}

void int_interval::show(OutputStream &s)
{
  s << "{" << start << ".." << stop << ".." << inc << "}";
}

// ******************************************************************
// *                                                                *
// *                      real_interval  class                      *
// *                                                                *
// ******************************************************************

/**  An interval set of reals.
     These have the form {start..stop..inc}.
 */
class real_interval : public set_result {
  double start, stop, inc;
public:
  real_interval(double s, double e, double i);
  virtual ~real_interval() { }
  virtual void GetElement(int n, result &x);
  virtual int IndexOf(const result &x);
  virtual void GetOrder(int n, int &i, result &x);
  virtual void show(OutputStream &s);
};

real_interval::real_interval(double s, double e, double i) 
  : set_result(int((e-s)/i + 1))
{
  start = s;
  stop = e;
  inc = i;
}

void real_interval::GetElement(int n, result &x)
{
  CHECK_RANGE(0, n, Size());
  x.Clear();
  x.rvalue = start + n * inc;
}

int real_interval::IndexOf(const result &x)
{
  if (!x.isNormal()) return -1;
  int i;
  if (inc>0) 
    i = int( ceil((x.rvalue-start)/inc - 0.5) );
  else
    i = int( ceil((start-x.rvalue)/inc - 0.5) );
  if (i>=Size()) return -1;
  if (i<0) return -1;
  // i is the closest index.
  // make sure start + i*inc is close enough to x
  double delta = fabs( (start + i*inc) - x.rvalue );
  if (delta > index_precision->GetReal()) 
    return -1;  // too far away
  return i;
}

void real_interval::GetOrder(int n, int &i, result &x)
{
  CHECK_RANGE(0, n, Size());
  x.Clear();
  if (inc>0) i = n;
  else i = Size()-n-1;
  x.rvalue = start + i * inc;
}

void real_interval::show(OutputStream &s)
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
  virtual void show(OutputStream &s);
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
  CHECK_RANGE(0, n, Size());
  x.Clear();
  x.ivalue = values[n];
}

int generic_int_set::IndexOf(const result &x)
{
  if (!x.isNormal()) return -1;
  // binary search through values
  int low = 0, high = Size()-1;
  while (low <= high) {
    int mid = (low+high)/2;
    int i = order[mid];
    CHECK_RANGE(0, i, Size());
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
  CHECK_RANGE(0, n, Size());
  i = order[n];
  CHECK_RANGE(0, i, Size());
  x.Clear();
  x.ivalue = values[i];
}

void generic_int_set::show(OutputStream &s)
{
  s.Put('{');
  s.PutArray(values, Size());
  s.Put('}');
}


// ******************************************************************
// *                                                                *
// *                     generic_real_set class                     *
// *                                                                *
// ******************************************************************

/**  A generic set of reals.
     This can be used to represent the empty set,
     sets of a single element, and arbitrary sets.
 */
class generic_real_set : public set_result {
  /// Values in the set.  Must be sorted.
  double* values;
  /// The order of the values.
  int* order;
public:
  generic_real_set(int s, double* v, int* o);
  virtual ~generic_real_set(); 
  virtual void GetElement(int n, result &x);
  virtual int IndexOf(const result &x);
  virtual void GetOrder(int n, int &i, result &x);
  virtual void show(OutputStream &s);
};

generic_real_set::generic_real_set(int s, double* v, int* o) : set_result(s)
{
  values = v;
  order = o;
}

generic_real_set::~generic_real_set()
{
  delete[] values;
  delete[] order;
}

void generic_real_set::GetElement(int n, result &x)
{
  CHECK_RANGE(0, n, Size());
  x.Clear();
  x.rvalue = values[n];
}

int generic_real_set::IndexOf(const result &x)
{
  if (!x.isNormal()) return -1;
  // binary search through values
  double epsilon = index_precision->GetReal(); 
  int low = 0, high = Size()-1;
  while (low <= high) {
    int mid = (low+high)/2;
    int i = order[mid];
    CHECK_RANGE(0, i, Size());
    if (values[i] > x.rvalue + epsilon) {
      // x is definitely smaller than midpoint
      high = mid-1;
      continue;
    }
    if (values[i]+epsilon < x.rvalue) {
      // x is definitely larger than midpoint
      low = mid+1;
      continue;
    }
    // still here?  x is within epsilon of midpoint; that's equal in our book
    return i;
  }
  return -1;
}

void generic_real_set::GetOrder(int n, int &i, result &x)
{
  CHECK_RANGE(0, n, Size());
  i = order[n];
  CHECK_RANGE(0, i, Size());
  x.Clear();
  x.rvalue = values[i];
}

void generic_real_set::show(OutputStream &s)
{
  s.Put('{');
  s.PutArray(values, Size());
  s.Put('}');
}



// ******************************************************************
// *                                                                *
// *                     generic_void_set class                     *
// *                                                                *
// ******************************************************************

/**  A generic set of voids.
     Used for sets of places, transitions, etc.
     I.e., sets of anything that qualifies as a void type,
     declared within a model.
 */
class generic_void_set : public set_result {
  /// Values in the set.  Sorted by address.
  symbol** values;
  /// The order of the values.
  int* order;
public:
  generic_void_set(int s, symbol** v, int* o);
  virtual ~generic_void_set(); 
  virtual void GetElement(int n, result &x);
  virtual int IndexOf(const result &x);
  virtual void GetOrder(int n, int &i, result &x);
  virtual void show(OutputStream &s);
};

generic_void_set::generic_void_set(int s, symbol** v, int* o) : set_result(s)
{
  values = v;
  order = o;
}

generic_void_set::~generic_void_set()
{
  // TBD: do we need to Delete the symbols?
  delete[] values;
  delete[] order;
}

void generic_void_set::GetElement(int n, result &x)
{
  CHECK_RANGE(0, n, Size());
  x.Clear();
  x.other = values[n];
}

int generic_void_set::IndexOf(const result &x)
{
  if (!x.isNormal()) return -1;
  // binary search through values
  int low = 0, high = Size()-1;
  while (low <= high) {
    int mid = (low+high)/2;
    int i = order[mid];
    CHECK_RANGE(0, i, Size());
    if (values[i] == x.other) return i;
    if (values[i] < x.other) 
      low = mid+1;
    else
      high = mid-1;
  }
  return -1;
}

void generic_void_set::GetOrder(int n, int &i, result &x)
{
  CHECK_RANGE(0, n, Size());
  i = order[n];
  CHECK_RANGE(0, i, Size());
  x.Clear();
  x.other = values[i];
}

void generic_void_set::show(OutputStream &s)
{
  s.Put('{');
  for (int i=0; i<Size(); i++) {
    if (i) s << ", ";
    s << values[i]->Name();
  }
  s.Put('}');
}



// ******************************************************************
// *                                                                *
// *                       int_realset  class                       *
// *                                                                *
// ******************************************************************

/**  A set of reals that was built as a set of integers.
     If we take the union of a set of integers and
     a set of reals, we wrap this around the set of integers.
     That way the types match.
 */
class int_realset : public set_result {
  set_result *intset;
public:
  int_realset(set_result *is);
  virtual ~int_realset(); 
  virtual void GetElement(int n, result &x);
  virtual int IndexOf(const result &x);
  virtual void GetOrder(int n, int &i, result &x);
  virtual void show(OutputStream &s);
};

int_realset::int_realset(set_result *is) : set_result(is->Size())
{
  intset = is;
}

int_realset::~int_realset()
{
  Delete(intset);
}

void int_realset::GetElement(int n, result &x)
{
  DCASSERT(intset);
  intset->GetElement(n, x);
  if (!x.isNormal()) return;
  // convert to real
  x.rvalue = x.ivalue; 
}

int int_realset::IndexOf(const result &x)
{
  result y;
  y.Clear();
  y.ivalue = int(x.rvalue);
  DCASSERT(intset);
  return intset->IndexOf(y);
}

void int_realset::GetOrder(int n, int &i, result &x)
{
  DCASSERT(intset);
  intset->GetOrder(n, i, x);
  if (!x.isNormal()) return;
  // convert to real
  x.rvalue = x.ivalue; 
}

void int_realset::show(OutputStream &s)
{
  s << intset;
}




// ******************************************************************
// ******************************************************************
// **                                                              **
// **                                                              **
// **                                                              **
// **                     Expressions for sets                     **
// **                                                              **
// **                                                              **
// **                                                              **
// ******************************************************************
// ******************************************************************


// ******************************************************************
// *                                                                *
// *                     setexpr_interval class                     *
// *                                                                *
// ******************************************************************

/** Base class for expressions to build set interval.
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
  virtual void ClearCache();
  virtual expr* Substitute(int i);
  virtual int GetSymbols(int i, List <symbol> *syms=NULL);
  virtual void show(OutputStream &s) const;
  virtual Engine_type GetEngine(engineinfo *e);
  virtual expr* SplitEngines(List <measure> *);
protected:
  inline bool ComputeAndCheck(result &s, result &e, result &i, result &x) {
    x.Clear();
    start->Compute(0, s);
    if (s.isNull() || s.isError()) {
      x = s;
      return false;
    }
    stop->Compute(0, e);
    if (e.isNull() || e.isError()) {
      x = e;
      return false;
    }
    inc->Compute(0, i);
    if (i.isNull() || i.isError()) {
      x = i;
      return false;
    }
    // special cases here
    if (s.isInfinity() || e.isInfinity()) {
      // Print error message here
      x.setError();
      return false;
    } 
    return true;
  }
  virtual setexpr_interval* MakeAnother(const char* fn, int line, 
  					expr* s, expr* e, expr* i) = 0;
};

setexpr_interval::setexpr_interval(const char* f, int l, expr* s, expr* e, expr* i) : expr(f, l)
{
  start = s;
  stop = e;
  inc = i;
  DCASSERT(start);
  DCASSERT(stop);
  DCASSERT(inc);
}

setexpr_interval::~setexpr_interval()
{
  Delete(start);
  Delete(stop);
  Delete(inc);
}

void setexpr_interval::ClearCache()
{
  // not sure if this is necessary, but it is correct ;^)
  DCASSERT(start);
  DCASSERT(stop);
  DCASSERT(inc);
  start->ClearCache();
  stop->ClearCache();
  inc->ClearCache();
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
  return MakeAnother(Filename(), Linenumber(), newstart, newstop, newinc);
}

int setexpr_interval::GetSymbols(int i, List <symbol> *syms)
{
  DCASSERT(0==i);
  DCASSERT(start);
  DCASSERT(stop);
  DCASSERT(inc);
  int answer = 0;
  answer = start->GetSymbols(0, syms);
  answer += stop->GetSymbols(0, syms);
  answer += inc->GetSymbols(0, syms);
  return answer;
}

void setexpr_interval::show(OutputStream &s) const
{
  s << start << ".." << stop << ".." << inc;
}

Engine_type setexpr_interval::GetEngine(engineinfo *ei)
{
  Engine_type s,e,i;
  s = start ? start->GetEngine(NULL) : ENG_None;
  e = stop ? stop->GetEngine(NULL) : ENG_None;
  i = inc ? inc->GetEngine(NULL) : ENG_None;
  
  if ((s == ENG_None) && (e == ENG_None) && (i == ENG_None)) {
    if (e) ei->setNone();
    return ENG_None;
  }

  if ((s == ENG_Error) && (e == ENG_Error) && (i == ENG_Error)) {
    if (e) ei->engine = ENG_Error;
    return ENG_Error;
  }

  if (ei) ei->setMixed();
  return ENG_Mixed;
}

expr* setexpr_interval::SplitEngines(List <measure> *mlist)
{
  measure *m;
  Engine_type mine = GetEngine(NULL);
  if (ENG_Error == mine) return NULL;
  if (mine != ENG_Mixed) return Copy(this);

  Engine_type s = start ? start->GetEngine(NULL) : ENG_None;
  expr* newstart;
  switch (s) {
    case ENG_None:
    	newstart = Copy(start);
	break;

    case ENG_Mixed: 
    	newstart = start->SplitEngines(mlist);
	break;

    default:
     	m = new measure(start->Filename(), start->Linenumber(), 
			start->Type(0), NULL);
	m->SetReturn(Copy(start));
	mlist->Append(m);
	newstart = m;
  }

  Engine_type e = stop ? stop->GetEngine(NULL) : ENG_None;
  expr* newstop;
  switch (e) {
    case ENG_None:
    	newstop = Copy(stop);
	break;

    case ENG_Mixed: 
    	newstop = stop->SplitEngines(mlist);
	break;

    default:
     	m = new measure(stop->Filename(), stop->Linenumber(), 
			stop->Type(0), NULL);
	m->SetReturn(Copy(stop));
	mlist->Append(m);
	newstop = m;
  }

  Engine_type i = inc ? inc->GetEngine(NULL) : ENG_None;
  expr* newinc;
  switch (i) {
    case ENG_None:
    	newinc = Copy(inc);
	break;

    case ENG_Mixed: 
    	newinc = inc->SplitEngines(mlist);
	break;

    default:
     	m = new measure(inc->Filename(), inc->Linenumber(), 
			inc->Type(0), NULL);
	m->SetReturn(Copy(inc));
	mlist->Append(m);
	newinc = m;
  }

  return MakeAnother(Filename(), Linenumber(), newstart, newstop, newinc);
}

// ******************************************************************
// *                                                                *
// *                                                                *
// *                     integer set expressions                    *
// *                                                                *
// *                                                                *
// ******************************************************************


// ******************************************************************
// *                   int_setexpr_interval class                   *
// ******************************************************************

/** An expression to build an integer set interval.
 */
class int_setexpr_interval : public setexpr_interval {
public:
  int_setexpr_interval(const char* fn, int line, expr* s, expr* e, expr* i);
  virtual type Type(int i) const;
  virtual void Compute(int i, result &x);
protected:
  virtual setexpr_interval* MakeAnother(const char* fn, int line, 
  					expr* s, expr* e, expr* i);
};

int_setexpr_interval::int_setexpr_interval(const char* f, int l, expr* s, expr* e, expr* i)
 : setexpr_interval(f, l, s, e, i)
{
  DCASSERT(s->Type(0)==INT);
  DCASSERT(e->Type(0)==INT);
  DCASSERT(i->Type(0)==INT);
}

type int_setexpr_interval::Type(int i) const
{
  DCASSERT(0==i);
  return SET_INT;
}

void int_setexpr_interval::Compute(int n, result &x)
{
  DCASSERT(0==n);
  result s,e,i;
  if (!setexpr_interval::ComputeAndCheck(s,e,i,x)) return;
  set_result *xs = NULL;
  int* values;
  int* order;
  if (i.isInfinity() || i.ivalue==0) {
    // that means an interval with just the start element.
    values = new int[1];
    order = new int[1];
    values[0] = s.ivalue;
    order[0] = 0;
    xs = new generic_int_set(1, values, order);
    // print a warning here
  } else
  if (((s.ivalue > e.ivalue) && (i.ivalue>0))
     ||
     ((s.ivalue < e.ivalue) && (i.ivalue<0))) {
    // empty interval
    xs = new generic_int_set(0, NULL, NULL);
    // print a warning here...
  } else {
    // we have an ordinary interval
    xs = new int_interval(s.ivalue, e.ivalue, i.ivalue);
  }
  x.other = xs;
}

setexpr_interval* int_setexpr_interval::MakeAnother(const char* fn, int line, 
  					expr* s, expr* e, expr* i)
{
  return new int_setexpr_interval(fn, line, s, e, i);
}

// ******************************************************************
// *                      intset_element class                      *
// ******************************************************************

/** Used for a single element set.
    (Used when we make those ugly for loops.)
 */
class intset_element : public unary {
public:
  intset_element(const char* fn, int line, expr* x) 
    : unary(fn, line, x) { };
  virtual type Type(int i) const;
  virtual void Compute(int i, result &x);
  virtual void show(OutputStream &s) const;
protected:
  virtual expr* MakeAnother(expr* newopnd) {
    return new intset_element(Filename(), Linenumber(), newopnd);
  }
};

type intset_element::Type(int i) const 
{
  DCASSERT(i==0);
  return SET_INT;
}

void intset_element::Compute(int i, result &x)
{
  DCASSERT(i==0);
  DCASSERT(opnd);
  opnd->Compute(0, x);
  if (x.isError() || x.isNull()) return;
  if (x.isInfinity()) {
    x.setError();
    return;
  }
  int* values = new int[1];
  int* order = new int[1];
  values[0] = x.ivalue;
  order[0] = 0;
   
  set_result *answer = new generic_int_set(1, values, order);
  x.other = answer;
}

void intset_element::show(OutputStream &s) const
{
  s << opnd;
}

// ******************************************************************
// *                       intset_union class                       *
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
  virtual void show(OutputStream &s) const { s << left << ", " << right; }
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
  if (l.isError() || l.isNull()) {
    x = l;
    return;
  }
  set_result *ls = dynamic_cast<set_result*> (l.other);
  DCASSERT(ls);
  result r;
  right->Compute(0, r);
  if (r.isError() || r.isNull()) {
    x = r;
    Delete(ls);
    return;
  }
  set_result *rs = dynamic_cast<set_result*> (r.other);
  DCASSERT(rs);

  // mark the duplicate elements in rs.
  int *rspos = new int[rs->Size()];     // eventually... use a temp buffer
  int lp = 0, rp = 0;
  result lx, rx;
  int ordl, ordr;
  if (lp<ls->Size()) ls->GetOrder(lp, ordl, lx);
  if (rp<rs->Size()) rs->GetOrder(rp, ordr, rx);
  while (lp<ls->Size() && rp<rs->Size()) {
    if (lx.ivalue == rx.ivalue) {
      rspos[ordr] = 0;  // duplicate
      lp++;
      rp++;
      if (lp<ls->Size()) ls->GetOrder(lp, ordl, lx);
      if (rp<rs->Size()) rs->GetOrder(rp, ordr, rx);
    } else if (lx.ivalue < rx.ivalue) {
      // advance lp only
      lp++;
      if (lp<ls->Size()) ls->GetOrder(lp, ordl, lx);
    } else {
      // advance rp only
      rspos[ordr] = 1;  // not duplicate
      rp++;
      if (rp<rs->Size()) rs->GetOrder(rp, ordr, rx);
    }
  }
  // fill the rest of rspos
  while (rp<rs->Size()) {
    rspos[ordr] = 1;
    rp++;
    if (rp<rs->Size()) rs->GetOrder(rp, ordr, rx);
  }

  // we have an array of bits for non-duplicates.
  // translate that into the new positions (by summing).
  for (rp=1; rp<rs->Size(); rp++) rspos[rp] += rspos[rp-1];
  
  // rspos[rs->Size()-1]   is the number of non-duplicates in rs.
  
  int newsize = ls->Size() + rspos[rs->Size()-1];

  set_result *answer;

  if (0==newsize) {
    // left and right must be empty.
    answer = new generic_int_set(0, NULL, NULL);

  } else {
    // we have a non-trivial union.

    // Do a "mergesort"
    int* values = new int[newsize];
    int* order = new int[newsize];
    int optr = 0;
    lp = 0; rp = 0;
    if (lp<ls->Size()) ls->GetOrder(lp, ordl, lx);
    if (rp<rs->Size()) rs->GetOrder(rp, ordr, rx);
    while (lp<ls->Size() && rp<rs->Size()) {
      if (lx.ivalue == rx.ivalue) {
        // this is a duplicate
        order[optr] = ordl;
        optr++;
        values[ordl] = lx.ivalue;
        lp++;
        rp++;
        if (lp<ls->Size()) ls->GetOrder(lp, ordl, lx);
        if (rp<rs->Size()) rs->GetOrder(rp, ordr, rx);
      } else if (lx.ivalue < rx.ivalue) {
        // copy next element from left
        order[optr] = ordl;
        values[ordl] = lx.ivalue;
        optr++;
        lp++;
        if (lp<ls->Size()) ls->GetOrder(lp, ordl, lx);
      } else {
        // copy next element from right
        order[optr] = rspos[ordr] + ls->Size() - 1; // I think this works...
        values[order[optr]] = rx.ivalue;
        optr++;
        rp++;
        if (rp<rs->Size()) rs->GetOrder(rp, ordr, rx);
      }
    }
    // At most one of these loops will go
    while (lp<ls->Size()) {
      order[optr] = ordl;
      values[ordl] = lx.ivalue;
      optr++;
      lp++;
      if (lp<ls->Size()) ls->GetOrder(lp, ordl, lx);
    }
    while (rp<rs->Size()) {
      order[optr] = rspos[ordr] + ls->Size() - 1;
      values[order[optr]] = rx.ivalue;
      optr++;
      rp++;
      if (rp<rs->Size()) rs->GetOrder(rp, ordr, rx);
    }

    answer = new generic_int_set(newsize, values, order);
    
    // eventually... return buffer
    delete[] rspos;
  }

#ifdef UNION_DEBUG
  Output << "Inside set union\n";
  Output << "The union of sets " << ls << " and " << rs << " is " << answer << "\n";
#endif
  
  x.other = answer;
  Delete(ls);
  Delete(rs);
}


// ******************************************************************
// *                       int2realset  class                       *
// ******************************************************************

/** A typecast expression from int sets to real sets.
 */
class int2realset : public unary {
public:
  int2realset(const char* fn, int line, expr* x) 
    : unary(fn, line, x) { };
  virtual type Type(int i) const;
  virtual void Compute(int i, result &x);
  virtual void show(OutputStream &s) const { s << opnd; }
protected:
  virtual expr* MakeAnother(expr* newx) {
    return new int2realset(Filename(), Linenumber(), newx);
  }
};

type int2realset::Type(int i) const
{
  DCASSERT(0==i);
  return SET_REAL;
}

void int2realset::Compute(int i, result &x)
{
  DCASSERT(0==i);
  DCASSERT(opnd);
  opnd->Compute(0, x);
  if (x.isNull() || x.isError()) return;
  set_result* xs = dynamic_cast<set_result*> (x.other);
  DCASSERT(xs);
  set_result* answer = new int_realset(xs);
  x.other = answer;
}

// ******************************************************************
// *                                                                *
// *                                                                *
// *                      real  set expressions                     *
// *                                                                *
// *                                                                *
// ******************************************************************


// ******************************************************************
// *                  real_setexpr_interval  class                  *
// ******************************************************************

/** An expression to build an real set interval.
 */
class real_setexpr_interval : public setexpr_interval {
public:
  real_setexpr_interval(const char* fn, int line, expr* s, expr* e, expr* i);
  virtual type Type(int i) const;
  virtual void Compute(int i, result &x);
protected:
  virtual setexpr_interval* MakeAnother(const char* fn, int line, 
  					expr* s, expr* e, expr* i);
};

real_setexpr_interval::real_setexpr_interval(const char* f, int l, expr* s, expr* e, expr* i)
 : setexpr_interval(f, l, s, e, i)
{
  DCASSERT(s->Type(0)==REAL);
  DCASSERT(e->Type(0)==REAL);
  DCASSERT(i->Type(0)==REAL);
}

type real_setexpr_interval::Type(int i) const
{
  DCASSERT(0==i);
  return SET_REAL;
}

void real_setexpr_interval::Compute(int n, result &x)
{
  DCASSERT(0==n);
  result s,e,i;
  if (!setexpr_interval::ComputeAndCheck(s,e,i,x)) return;
  set_result *xs = NULL;
  double* values;
  int* order;
  if (i.isInfinity() || i.rvalue==0.0) {
    // that means an interval with just the start element.
    values = new double[1];
    order = new int[1];
    values[0] = s.rvalue;
    order[0] = 0;
    xs = new generic_real_set(1, values, order);
    // print a warning here
  } else
  if (((s.rvalue > e.rvalue) && (i.rvalue>0))
     ||
     ((s.rvalue < e.rvalue) && (i.rvalue<0))) {
    // empty interval
    xs = new generic_real_set(0, NULL, NULL);
    // print a warning here...
  } else {
    // we have an ordinary interval
    xs = new real_interval(s.rvalue, e.rvalue, i.rvalue);
  }
  x.other = xs;
}

setexpr_interval* real_setexpr_interval::MakeAnother(const char* fn, int line, 
  					expr* s, expr* e, expr* i)
{
  return new real_setexpr_interval(fn, line, s, e, i);
}

// ******************************************************************
// *                     realset_element  class                     *
// ******************************************************************

/** Used for a single element set.
    (Used when we make those ugly for loops.)
 */
class realset_element : public unary {
public:
  realset_element(const char* fn, int line, expr* x) 
    : unary(fn, line, x) { };
  virtual type Type(int i) const;
  virtual void Compute(int i, result &x);
  virtual void show(OutputStream &s) const;
protected:
  virtual expr* MakeAnother(expr* newopnd) {
    return new realset_element(Filename(), Linenumber(), newopnd);
  }
};

type realset_element::Type(int i) const 
{
  DCASSERT(i==0);
  return SET_REAL;
}

void realset_element::Compute(int i, result &x)
{
  DCASSERT(i==0);
  DCASSERT(opnd);
  opnd->Compute(0, x);
  if (x.isError() || x.isNull()) return;
  if (x.isInfinity()) {
    x.setError();
    return;
  }
  double* values = new double[1];
  int* order = new int[1];
  values[0] = x.rvalue;
  order[0] = 0;
   
  set_result *answer = new generic_real_set(1, values, order);
  x.other = answer;
}

void realset_element::show(OutputStream &s) const
{
  s << opnd;
}

// ******************************************************************
// *                      realset_union  class                      *
// ******************************************************************

/** A binary operator to handle the union of two real sets.
    (Used when we make those ugly for loops.)
 */
class realset_union : public binary {
public:
  realset_union(const char* fn, int line, expr* l, expr* r) 
    : binary(fn, line, l, r) { };
  virtual type Type(int i) const;
  virtual void Compute(int i, result &x);
  virtual void show(OutputStream &s) const { s << left << ", " << right; }
protected:
  virtual expr* MakeAnother(expr* newleft, expr* newright) {
    return new realset_union(Filename(), Linenumber(), newleft, newright);
  }
};

type realset_union::Type(int i) const
{
  DCASSERT(0==i);
  return SET_REAL;
}

void realset_union::Compute(int i, result &x)
{
  DCASSERT(0==i);
  DCASSERT(left);
  DCASSERT(right);
  x.Clear();
  result l;
  left->Compute(0, l); 
  if (l.isError() || l.isNull()) {
    x = l;
    return;
  }
  set_result *ls = dynamic_cast<set_result*> (l.other);
  DCASSERT(ls);
  result r;
  right->Compute(0, r);
  if (r.isError() || r.isNull()) {
    x = r;
    Delete(ls);
    return;
  }
  set_result *rs = dynamic_cast<set_result*> (r.other);
  DCASSERT(rs);

  // mark the duplicate elements in rs.
  int *rspos = new int[rs->Size()];     // eventually... use a temp buffer
  int lp = 0, rp = 0;
  result lx, rx;
  int ordl, ordr;
  if (lp<ls->Size()) ls->GetOrder(lp, ordl, lx);
  if (rp<rs->Size()) rs->GetOrder(rp, ordr, rx);
  while (lp<ls->Size() && rp<rs->Size()) {
    if (lx.rvalue == rx.rvalue) {
      // Eventually: use IndexPrecision or something for epsilon...
      rspos[ordr] = 0;  // duplicate
      lp++;
      rp++;
      if (lp<ls->Size()) ls->GetOrder(lp, ordl, lx);
      if (rp<rs->Size()) rs->GetOrder(rp, ordr, rx);
    } else if (lx.rvalue < rx.rvalue) {
      // advance lp only
      lp++;
      if (lp<ls->Size()) ls->GetOrder(lp, ordl, lx);
    } else {
      // advance rp only
      rspos[ordr] = 1;  // not duplicate
      rp++;
      if (rp<rs->Size()) rs->GetOrder(rp, ordr, rx);
    }
  }
  // fill the rest of rspos
  while (rp<rs->Size()) {
    rspos[ordr] = 1;
    rp++;
    if (rp<rs->Size()) rs->GetOrder(rp, ordr, rx);
  }

  // we have an array of bits for non-duplicates.
  // translate that into the new positions (by summing).
  for (rp=1; rp<rs->Size(); rp++) rspos[rp] += rspos[rp-1];
  
  // rspos[rs->Size()-1]   is the number of non-duplicates in rs.
  
  int newsize = ls->Size() + rspos[rs->Size()-1];

  set_result *answer;

  if (0==newsize) {
    // left and right must be empty.
    answer = new generic_real_set(0, NULL, NULL);

  } else {
    // we have a non-trivial union.

    // Do a "mergesort"
    double* values = new double[newsize];
    int* order = new int[newsize];
    int optr = 0;
    lp = 0; rp = 0;
    if (lp<ls->Size()) ls->GetOrder(lp, ordl, lx);
    if (rp<rs->Size()) rs->GetOrder(rp, ordr, rx);
    while (lp<ls->Size() && rp<rs->Size()) {
      if (lx.rvalue == rx.rvalue) {
        // this is a duplicate
        order[optr] = ordl;
        optr++;
        values[ordl] = lx.rvalue;
        lp++;
        rp++;
        if (lp<ls->Size()) ls->GetOrder(lp, ordl, lx);
        if (rp<rs->Size()) rs->GetOrder(rp, ordr, rx);
      } else if (lx.rvalue < rx.rvalue) {
        // copy next element from left
        order[optr] = ordl;
        values[ordl] = lx.rvalue;
        optr++;
        lp++;
        if (lp<ls->Size()) ls->GetOrder(lp, ordl, lx);
      } else {
        // copy next element from right
        order[optr] = rspos[ordr] + ls->Size() - 1; // I think this works...
        values[order[optr]] = rx.rvalue;
        optr++;
        rp++;
        if (rp<rs->Size()) rs->GetOrder(rp, ordr, rx);
      }
    }
    // At most one of these loops will go
    while (lp<ls->Size()) {
      order[optr] = ordl;
      values[ordl] = lx.rvalue;
      optr++;
      lp++;
      if (lp<ls->Size()) ls->GetOrder(lp, ordl, lx);
    }
    while (rp<rs->Size()) {
      order[optr] = rspos[ordr] + ls->Size() - 1;
      values[order[optr]] = rx.rvalue;
      optr++;
      rp++;
      if (rp<rs->Size()) rs->GetOrder(rp, ordr, rx);
    }

    answer = new generic_real_set(newsize, values, order);
    
    // eventually... return buffer
    delete[] rspos;
  }

#ifdef UNION_DEBUG
  Output << "Inside set union\n";
  Output << "The union of sets " << ls << " and " << rs << " is " << answer << "\n";
#endif
  
  x.other = answer;
  Delete(ls);
  Delete(rs);
}


// ******************************************************************
// *                                                                *
// *                                                                *
// *                      void  set expressions                     *
// *                                                                *
// *                                                                *
// ******************************************************************

// ******************************************************************
// *                     voidset_element  class                     *
// ******************************************************************

/** Used for a single element set.  
    This is the "base case" for all void sets.
 */
class voidset_element : public unary {
  type setType;
public:
  voidset_element(const char* fn, int line, expr* x, type st) 
    : unary(fn, line, x) { 
    CHECK_RANGE(FIRST_SET, st, 1+LAST_SET); 
    setType = st;
  };
  virtual type Type(int i) const;
  virtual void Compute(int i, result &x);
  virtual void show(OutputStream &s) const;
protected:
  virtual expr* MakeAnother(expr* newopnd) {
    return new voidset_element(Filename(), Linenumber(), newopnd, setType);
  }
};

type voidset_element::Type(int i) const 
{
  DCASSERT(i==0);
  return setType;
}

void voidset_element::Compute(int i, result &x)
{
  DCASSERT(i==0);
  DCASSERT(opnd);
  symbol** values = new symbol*[1];
  int* order = new int[1];
  SafeCompute(opnd, 0, x); 
  DCASSERT(x.isNormal()); // I'm *pretty* sure the alternative is impossible
  values[0] = dynamic_cast<symbol*> (x.other); 
  DCASSERT(values[0]);
  order[0] = 0;
   
  set_result *answer = new generic_void_set(1, values, order);
  x.other = answer;
}

void voidset_element::show(OutputStream &s) const
{
  s << opnd;
}

// ******************************************************************
// *                      voidset_union  class                      *
// ******************************************************************

/** A binary operator to handle the union of two void sets.
 */
class voidset_union : public binary {
public:
  voidset_union(const char* fn, int line, expr* l, expr* r) 
    : binary(fn, line, l, r) { 
    DCASSERT(l); 
    DCASSERT(r); 
  };
  virtual type Type(int i) const;
  virtual void Compute(int i, result &x);
  virtual void show(OutputStream &s) const { s << left << ", " << right; }
protected:
  virtual expr* MakeAnother(expr* newleft, expr* newright) {
    return new voidset_union(Filename(), Linenumber(), newleft, newright);
  }
};

type voidset_union::Type(int i) const
{
  DCASSERT(0==i);
  DCASSERT(left);
  return left->Type(i);
}

void voidset_union::Compute(int i, result &x)
{
  DCASSERT(0==i);
  DCASSERT(left);
  DCASSERT(right);
  x.Clear();
  result l;
  left->Compute(0, l); 
  if (l.isError() || l.isNull()) {
    x = l;
    return;
  }
  set_result *ls = dynamic_cast<set_result*> (l.other);
  DCASSERT(ls);
  result r;
  right->Compute(0, r);
  if (r.isError() || r.isNull()) {
    x = r;
    Delete(ls);
    return;
  }
  set_result *rs = dynamic_cast<set_result*> (r.other);
  DCASSERT(rs);

  // mark the duplicate elements in rs.
  int *rspos = new int[rs->Size()];     // eventually... use a temp buffer
  int lp = 0, rp = 0;
  result lx, rx;
  int ordl, ordr;
  if (lp<ls->Size()) ls->GetOrder(lp, ordl, lx);
  if (rp<rs->Size()) rs->GetOrder(rp, ordr, rx);
  while (lp<ls->Size() && rp<rs->Size()) {
    if (lx.other == rx.other) {
      rspos[ordr] = 0;  // duplicate
      lp++;
      rp++;
      if (lp<ls->Size()) ls->GetOrder(lp, ordl, lx);
      if (rp<rs->Size()) rs->GetOrder(rp, ordr, rx);
    } else if (lx.other < rx.other) {
      // advance lp only
      lp++;
      if (lp<ls->Size()) ls->GetOrder(lp, ordl, lx);
    } else {
      // advance rp only
      rspos[ordr] = 1;  // not duplicate
      rp++;
      if (rp<rs->Size()) rs->GetOrder(rp, ordr, rx);
    }
  }
  // fill the rest of rspos
  while (rp<rs->Size()) {
    rspos[ordr] = 1;
    rp++;
    if (rp<rs->Size()) rs->GetOrder(rp, ordr, rx);
  }

  // we have an array of bits for non-duplicates.
  // translate that into the new positions (by summing).
  for (rp=1; rp<rs->Size(); rp++) rspos[rp] += rspos[rp-1];
  
  // rspos[rs->Size()-1]   is the number of non-duplicates in rs.
  
  int newsize = ls->Size() + rspos[rs->Size()-1];

  set_result *answer;

  if (0==newsize) {
    // left and right must be empty.
    answer = new generic_void_set(0, NULL, NULL);

  } else {
    // we have a non-trivial union.

    // Do a "mergesort"
    symbol** values = new symbol*[newsize];
    int* order = new int[newsize];
    int optr = 0;
    lp = 0; rp = 0;
    if (lp<ls->Size()) ls->GetOrder(lp, ordl, lx);
    if (rp<rs->Size()) rs->GetOrder(rp, ordr, rx);
    while (lp<ls->Size() && rp<rs->Size()) {
      if (lx.other == rx.other) {
        // this is a duplicate
        order[optr] = ordl;
        optr++;
        values[ordl] = dynamic_cast<symbol*>(lx.other);
        DCASSERT(values[ordl]);
        lp++;
        rp++;
        if (lp<ls->Size()) ls->GetOrder(lp, ordl, lx);
        if (rp<rs->Size()) rs->GetOrder(rp, ordr, rx);
      } else if (lx.other < rx.other) {
        // copy next element from left
        order[optr] = ordl;
        values[ordl] = dynamic_cast<symbol*>(lx.other);
        DCASSERT(values[ordl]);
        optr++;
        lp++;
        if (lp<ls->Size()) ls->GetOrder(lp, ordl, lx);
      } else {
        // copy next element from right
        order[optr] = rspos[ordr] + ls->Size() - 1; // I think this works...
        values[order[optr]] = dynamic_cast<symbol*>(rx.other);
        DCASSERT(values[order[optr]]);
        optr++;
        rp++;
        if (rp<rs->Size()) rs->GetOrder(rp, ordr, rx);
      }
    }
    // At most one of these loops will go
    while (lp<ls->Size()) {
      order[optr] = ordl;
      values[ordl] = dynamic_cast<symbol*>(lx.other);
      DCASSERT(values[ordl]);
      optr++;
      lp++;
      if (lp<ls->Size()) ls->GetOrder(lp, ordl, lx);
    }
    while (rp<rs->Size()) {
      order[optr] = rspos[ordr] + ls->Size() - 1;
      values[order[optr]] = dynamic_cast<symbol*>(rx.other);
      DCASSERT(values[order[optr]]);
      optr++;
      rp++;
      if (rp<rs->Size()) rs->GetOrder(rp, ordr, rx);
    }

    answer = new generic_void_set(newsize, values, order);
    
    // eventually... return buffer
    delete[] rspos;
  }

#ifdef UNION_DEBUG
  Output << "Inside set union\n";
  Output << "The union of sets " << ls << " and " << rs << " is " << answer << "\n";
#endif
  
  x.other = answer;
  Delete(ls);
  Delete(rs);
}



// ******************************************************************
// *                                                                *
// *                                                                *
// *           Global functions  to build set expressions           *
// *                                                                *
// *                                                                *
// ******************************************************************

expr* SetOpError(const char* what, const char *fn, int ln)
{
  // shouldn't ever get here
  Internal.Start(__FILE__, __LINE__, fn, ln);
  Internal << what;
  Internal.Stop();
  return ERROR;
}


expr*  MakeInterval(const char *fn, int ln, expr* start, expr* stop, expr* inc)
{
  switch (start->Type(0)) {
    case INT: 
      return new int_setexpr_interval(fn, ln, start, stop, inc);

    case REAL:
      // not done yet
      return new real_setexpr_interval(fn, ln, start, stop, inc);
  }
  return SetOpError("Bad interval type",fn, ln);
}

expr*  MakeElementSet(const char *fn, int ln, expr* element)
{
  switch (element->Type(0)) {
    case INT:
      return new intset_element(fn, ln, element);

    case REAL:
      return new realset_element(fn, ln, element);

    case PLACE:
      return new voidset_element(fn, ln, element, SET_PLACE);

    case TRANS:
      return new voidset_element(fn, ln, element, SET_TRANS);
  }
  return SetOpError("Bad element set type",fn, ln);
}

expr*  MakeUnionOp(const char *fn, int ln, expr* left, expr* right)
{
  DCASSERT(left);
  DCASSERT(right);
  DCASSERT(left->Type(0) == right->Type(0));
  switch (left->Type(0)) {
    case SET_INT:
      return new intset_union(fn, ln, left, right);

    case SET_REAL:
      return new realset_union(fn, ln, left, right);

    case SET_PLACE:
    case SET_TRANS:
      return new voidset_union(fn, ln, left, right);
  }
  return SetOpError("Bad type for set union",fn, ln);
}

expr*  MakeInt2RealSet(const char* fn, int line, expr* intset)
{
  DCASSERT(intset->Type(0) == SET_INT);
  return new int2realset(fn, line, intset);
}

//@}

