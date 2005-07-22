
// $Id$

#include "sets.h"
#include "measures.h"
#include "../Base/memtrack.h"
#include "../Templates/splay.h"

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
  virtual void GetElement(int n, result *x);
  virtual int IndexOf(const result &x);
  virtual void show(OutputStream &s);
};

int_interval::int_interval(int s, int e, int i) 
  : set_result(int((e-s)/i + 1))
{
  start = s;
  stop = e;
  inc = i;
}

void int_interval::GetElement(int n, result *x)
{
  CHECK_RANGE(0, n, Size());
  DCASSERT(x);
  x->Clear();
  x->ivalue = start + n * inc;
}

int int_interval::IndexOf(const result &x)
{
  if (!x.isNormal()) return -1;
  int i = int((x.ivalue-start)/inc);
  if (i>=Size()) return -1;
  if (i<0) return -1;
  return i;
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
  virtual void GetElement(int n, result *x);
  virtual int IndexOf(const result &x);
  virtual void show(OutputStream &s);
};

real_interval::real_interval(double s, double e, double i) 
  : set_result(int((e-s)/i + 1))
{
  start = s;
  stop = e;
  inc = i;
}

void real_interval::GetElement(int n, result *x)
{
  CHECK_RANGE(0, n, Size());
  DCASSERT(x);
  x->Clear();
  x->rvalue = start + n * inc;
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
  virtual void GetElement(int n, result *x);
  virtual int IndexOf(const result &x);
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

void generic_int_set::GetElement(int n, result *x)
{
  CHECK_RANGE(0, n, Size());
  DCASSERT(x);
  x->Clear();
  x->ivalue = values[n];
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
  virtual void GetElement(int n, result *x);
  virtual int IndexOf(const result &x);
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

void generic_real_set::GetElement(int n, result *x)
{
  CHECK_RANGE(0, n, Size());
  DCASSERT(x);
  x->Clear();
  x->rvalue = values[n];
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
  virtual void GetElement(int n, result *x);
  virtual int IndexOf(const result &x);
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

void generic_void_set::GetElement(int n, result *x)
{
  CHECK_RANGE(0, n, Size());
  DCASSERT(x);
  x->Clear();
  x->other = values[n];
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
  virtual void GetElement(int n, result *x);
  virtual int IndexOf(const result &x);
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

void int_realset::GetElement(int n, result *x)
{
  DCASSERT(intset);
  DCASSERT(x);
  intset->GetElement(n, x);
  if (!x->isNormal()) return;
  // convert to real
  x->rvalue = x->ivalue; 
}

int int_realset::IndexOf(const result &x)
{
  result y;
  y.Clear();
  y.ivalue = int(x.rvalue);
  DCASSERT(intset);
  return intset->IndexOf(y);
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
// *                         unionop  class                         *
// *                                                                *
// ******************************************************************

/**   The base class of union classes.
 
*/  

class unionop : public assoc {
public:
  unionop(const char* fn, int line, expr** x, int n) : assoc(fn, line, x, n) {}
  unionop(const char* fn, int line, expr* l, expr* r) : assoc(fn, line, l, r) {}
  virtual void show(OutputStream &s) const;
  virtual int GetSums(int i, List <expr> *sums=NULL);
};

void unionop::show(OutputStream &s) const 
{ 
  int i;
  for (i=0; i<opnd_count; i++) {
    if (i) s << ", ";
    s << operands[i];
  }  
}

int unionop::GetSums(int a, List <expr> *sums)
{
  DCASSERT(a==0);
  int i;
  int opnds=0;
  for (i=0; i<opnd_count; i++) {
    opnds += operands[i]->GetSums(a, sums);
  }
  return opnds;
}

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
  inline bool ComputeAndCheck(result &s, result &e, result &i, compute_data &x) {
    result* answer = x.answer;
    DCASSERT(answer);
    answer->Clear();
    x.answer = &s;
    start->Compute(x);
    if (s.isNull() || s.isError()) {
      (*answer) = s;
      x.answer = answer;
      return false;
    }
    x.answer = &e;
    stop->Compute(x);
    if (e.isNull() || e.isError()) {
      (*answer) = e;
      x.answer = answer;
      return false;
    }
    x.answer = &i;
    inc->Compute(x);
    x.answer = answer;
    if (i.isNull() || i.isError()) {
      (*answer) = i;
      return false;
    }
    // special cases here
    if (s.isInfinity() || e.isInfinity()) {
      // Print error message here
      answer->setError();
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
  virtual void Compute(compute_data &x);
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

void int_setexpr_interval::Compute(compute_data &x)
{
  DCASSERT(x.answer); 
  DCASSERT(0==x.aggregate);
  DCASSERT(NULL==x.rng_stream);
  DCASSERT(NULL==x.current);
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
  x.answer->other = xs;
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
  virtual void Compute(compute_data &x);
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

void intset_element::Compute(compute_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(opnd);
  opnd->Compute(x);
  if (x.answer->isError() || x.answer->isNull()) return;
  if (x.answer->isInfinity()) {
    x.answer->setError();
    return;
  }
  int* values = new int[1];
  int* order = new int[1];
  values[0] = x.answer->ivalue;
  order[0] = 0;
   
  set_result *answer = new generic_int_set(1, values, order);
  x.answer->other = answer;
}

void intset_element::show(OutputStream &s) const
{
  s << opnd;
}

// ******************************************************************
// *                       intset_union class                       *
// ******************************************************************

/** An associative (yay!) operator to handle the union of integer sets.
    (Used when we make those ugly for loops.)
 */
class intset_union : public unionop {
public:
  intset_union(const char* fn, int line, expr** x, int n) 
   : unionop(fn, line, x, n) {}
  intset_union(const char* fn, int line, expr* l, expr* r)
   : unionop(fn, line, l, r) {}
  virtual type Type(int i) const;
  virtual void Compute(compute_data &x);
protected:
  virtual expr* MakeAnother(expr** newx, int newn) {
    return new intset_union(Filename(), Linenumber(), newx, newn);
  }
};

type intset_union::Type(int i) const
{
  DCASSERT(0==i);
  return SET_INT;
}

void intset_union::Compute(compute_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  x.answer->Clear();
  ArraySplay <int> *ans = new ArraySplay <int>; 
  for (int i=0; i<opnd_count; i++) {
    SafeCompute(operands[i], x);
    if (!x.answer->isNormal()) {
      // bail out
      delete ans;
      return;
    }
#ifdef DEVELOPMENT_CODE
    set_result *s = dynamic_cast<set_result*> (x.answer->other);
    DCASSERT(s);
#else
    set_result *s = (set_result*) x.answer->other;
#endif
    for (int e=0; e<s->Size(); e++) {
      s->GetElement(e, x.answer);
      DCASSERT(x.answer->isNormal());
      ans->AddElement(x.answer->ivalue);
    } 
    Delete(s);
  }
  // prepare result
  x.answer->Clear();
  int newsize = ans->NumNodes();
  int* order = (newsize) ? (new int[newsize]) : NULL;
  ans->FillOrderList(order);
  int* values = ans->Compress();
  delete ans;
  x.answer->other = new generic_int_set(newsize, values, order);
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
  virtual void Compute(compute_data &x);
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

void int2realset::Compute(compute_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(opnd);
  opnd->Compute(x);
  if (x.answer->isNull() || x.answer->isError()) return;
#ifdef DEVELOPMENT_CODE
  set_result* xs = dynamic_cast<set_result*> (x.answer->other);
  DCASSERT(xs);
#else
  set_result* xs = (set_result*) x.answer->other;
#endif
  x.answer->other = new int_realset(xs);
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
  virtual void Compute(compute_data &x);
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

void real_setexpr_interval::Compute(compute_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(NULL==x.rng_stream);
  DCASSERT(NULL==x.current);
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
  x.answer->other = xs;
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
  virtual void Compute(compute_data &x);
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

void realset_element::Compute(compute_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(opnd);
  opnd->Compute(x);
  if (x.answer->isError() || x.answer->isNull()) return;
  if (x.answer->isInfinity()) {
    x.answer->setError();
    return;
  }
  double* values = new double[1];
  int* order = new int[1];
  values[0] = x.answer->rvalue;
  order[0] = 0;
   
  set_result *answer = new generic_real_set(1, values, order);
  x.answer->other = answer;
}

void realset_element::show(OutputStream &s) const
{
  s << opnd;
}

// ******************************************************************
// *                      realset_union  class                      *
// ******************************************************************

/** An assoc operator to handle the union of real sets.
    (Used when we make those ugly for loops.)
 */
class realset_union : public unionop {
public:
  realset_union(const char* fn, int line, expr** x, int n) 
   : unionop(fn, line, x, n) {}
  realset_union(const char* fn, int line, expr* l, expr* r)
   : unionop(fn, line, l, r) {}
  virtual type Type(int i) const;
  virtual void Compute(compute_data &x);
protected:
  virtual expr* MakeAnother(expr** nargs, int ncnt) {
    return new realset_union(Filename(), Linenumber(), nargs, ncnt);
  }
};

type realset_union::Type(int i) const
{
  DCASSERT(0==i);
  return SET_REAL;
}

void realset_union::Compute(compute_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  x.answer->Clear();
  ArraySplay <double> *ans = new ArraySplay <double>; 
  for (int i=0; i<opnd_count; i++) {
    SafeCompute(operands[i], x);
    if (!x.answer->isNormal()) {
      // bail out
      delete ans;
      return;
    }
#ifdef DEVELOPMENT_CODE
    set_result *s = dynamic_cast<set_result*> (x.answer->other);
    DCASSERT(s);
#else
    set_result *s = (set_result*) x.answer->other;
#endif
    for (int e=0; e<s->Size(); e++) {
      s->GetElement(e, x.answer);
      DCASSERT(x.answer->isNormal());
      ans->AddElement(x.answer->rvalue);
    } 
    Delete(s);
  }
  // prepare result
  x.answer->Clear();
  int newsize = ans->NumNodes();
  int* order = (newsize) ? (new int[newsize]) : NULL;
  ans->FillOrderList(order);
  double* values = ans->Compress();
  delete ans;
  x.answer->other = new generic_real_set(newsize, values, order);
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
    DCASSERT(IsASet(st));
    setType = st;
  };
  virtual type Type(int i) const;
  virtual void Compute(compute_data &x);
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

void voidset_element::Compute(compute_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(opnd);
  symbol** values = new symbol*[1];
  int* order = new int[1];
  SafeCompute(opnd, x); 
  DCASSERT(x.answer->isNormal());
  values[0] = dynamic_cast<symbol*> (x.answer->other); 
  DCASSERT(values[0]);
  order[0] = 0;
   
  x.answer->other = new generic_void_set(1, values, order);
}

void voidset_element::show(OutputStream &s) const
{
  s << opnd;
}

// ******************************************************************
// *                      voidset_union  class                      *
// ******************************************************************

/** An assoc operator to handle the union of void sets.
 */
class voidset_union : public unionop {
  type mytype;
public:
  voidset_union(const char* fn, int line, expr** x, int n, type mt) 
    : unionop(fn, line, x, n) { 
    DCASSERT(IsASet(mt));
    mytype = mt;
  };
  voidset_union(const char* fn, int line, expr* l, expr* r, type mt) 
    : unionop(fn, line, l, r) { 
    mytype = mt;
  };
  virtual type Type(int i) const;
  virtual void Compute(compute_data &x);
protected:
  virtual expr* MakeAnother(expr** args, int n) {
    return new voidset_union(Filename(), Linenumber(), args, n, mytype);
  }
};

type voidset_union::Type(int i) const
{
  DCASSERT(0==i);
  return mytype;
}

void voidset_union::Compute(compute_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  x.answer->Clear();
  ArraySplay <void*> *ans = new ArraySplay <void*>; 
  for (int i=0; i<opnd_count; i++) {
    SafeCompute(operands[i], x);
    if (!x.answer->isNormal()) {
      // bail out
      delete ans;
      return;
    }
#ifdef DEVELOPMENT_CODE
    set_result *s = dynamic_cast<set_result*> (x.answer->other);
    DCASSERT(s);
#else
    set_result *s = (set_result*) x.answer->other;
#endif
    for (int e=0; e<s->Size(); e++) {
      s->GetElement(e, x.answer);
      DCASSERT(x.answer->isNormal());
#ifdef DEVELOPMENT_CODE
      symbol *xo = dynamic_cast<symbol*>(x.answer->other);
      DCASSERT(xo);
#else
      symbol *xo = (symbol*) x.answer->other;
#endif
      ans->AddElement(xo);
    } 
    Delete(s);
  }
  // prepare result
  x.answer->Clear();
  int newsize = ans->NumNodes();
  int* order = (newsize) ? (new int[newsize]) : NULL;
  ans->FillOrderList(order);
  symbol** values = (symbol**) ans->Compress();
  delete ans;
  x.answer->other = new generic_void_set(newsize, values, order);
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
  type et = element->Type(0);
  switch (et) {
    case INT:
      return new intset_element(fn, ln, element);

    case REAL:
      return new realset_element(fn, ln, element);

    case STATE:
    case PLACE:
    case TRANS:
      return new voidset_element(fn, ln, element, SetOf(et));

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

    case SET_STATE:
    case SET_PLACE:
    case SET_TRANS:
      return new voidset_union(fn, ln, left, right, left->Type(0));

  }
  return SetOpError("Bad type for set union",fn, ln);
}

expr*  MakeUnionOp(const char *fn, int ln, expr** args, int n)
{
  DCASSERT(args);
  DCASSERT(n>0);
  switch (args[0]->Type(0)) {
    case SET_INT:
      return new intset_union(fn, ln, args, n);

    case SET_REAL:
      return new realset_union(fn, ln, args, n);

    case SET_STATE:
    case SET_PLACE:
    case SET_TRANS:
      return new voidset_union(fn, ln, args, n, args[0]->Type(0));

  }
  return SetOpError("Bad type for set union",fn, ln);
}

expr*  MakeInt2RealSet(const char* fn, int line, expr* intset)
{
  DCASSERT(intset->Type(0) == SET_INT);
  return new int2realset(fn, line, intset);
}

expr* OptimizeUnion(expr* e)
{
  if (NULL==e) return NULL;
  if (ERROR==e) return ERROR;
  DCASSERT(e!=DEFLT);
  DCASSERT(IsASet(e->Type(0)));
  static List <expr> optbuffer(16);
  // Try to split us into sums
  optbuffer.Clear();
  int sumcount = e->GetSums(0, &optbuffer);
  expr* ne;
  if (1==sumcount) {
    ne = optbuffer.Item(0);
  } else {
    // We should have a list of unions
    expr **opnds = optbuffer.Copy();
    // replace with one big union
    ne = MakeUnionOp(e->Filename(), e->Linenumber(), opnds, sumcount);
  }
  Delete(e);
  return ne;
}

//@}

