
#include "sets.h"
#include "../Streams/streams.h"
#include "../Options/options.h"
#include "result.h"
#include "type.h"
#include "../include/heap.h"

#include <math.h>

// #define DEBUG_MEM

// ******************************************************************
// *                                                                *
// *                       shared_set methods                       *
// *                                                                *
// ******************************************************************

shared_set::shared_set(long s) : shared_object()
{
  size = s;
}

// ******************************************************************
// *                                                                *
// *                     set_converter  methods                     *
// *                                                                *
// ******************************************************************

set_converter::element_convert::element_convert() 
{ 
}

set_converter::element_convert::~element_convert() 
{ 
}



set_converter::set_converter(const element_convert &ec, shared_set *is)
 : shared_set(is->Size()), EC(ec)
{
  oldset = is;
  DCASSERT(oldset);
}

set_converter::~set_converter()
{
  Delete(oldset);
}

void set_converter::GetElement(long n, result &x) const
{
  DCASSERT(oldset);
  oldset->GetElement(n, x);
  EC.convert(x);
}

long set_converter::IndexOf(const result &x) const
{
  result y = x;
  DCASSERT(oldset);
  EC.revert(y);
  return oldset->IndexOf(y);
}

bool set_converter::Print(OutputStream &s, int) const
{
  DCASSERT(oldset);
  return oldset->Print(s, 0);
}

bool set_converter::Equals(const shared_object* o) const
{
  const set_converter* foo = dynamic_cast <const set_converter*> (o);
  if (0==foo) return false;
  return oldset->Equals(foo->oldset);
}


// ******************************************************************
// *                                                                *
// *                        int_ivlset class                        *
// *                                                                *
// ******************************************************************

/**  An interval set of integers.
     These have the form {start..stop..inc}.
 */
class int_ivlset : public shared_set {
  long start, stop, inc;
public:
  int_ivlset(long s, long e, long i);
  virtual void GetElement(long n, result &x) const;
  virtual long IndexOf(const result &x) const;
  virtual bool Print(OutputStream &s, int) const;
  virtual bool Equals(const shared_object* o) const;
};

// ******************************************************************
// *                       int_ivlset methods                       *
// ******************************************************************

int_ivlset::int_ivlset(long s, long e, long i)
  : shared_set(long((e-s)/i + 1))
{
  start = s;
  stop = e;
  inc = i;
  DCASSERT(inc);
}

void int_ivlset::GetElement(long n, result &x) const
{
 // CHECK_RANGE(0, n, Size()); 
 //Incorrect Range check; To be corrected
  x.setInt(start + n * inc);
}

long int_ivlset::IndexOf(const result &x) const
{
  if (!x.isNormal()) return -1;
  long delta = x.getInt() - start;
  if (delta % inc) return -1;
  long i = long((x.getInt()-start)/inc);
  if (i>=Size()) return -1;
  if (i<0) return -1;
  return i;
}

bool int_ivlset::Print(OutputStream &s, int) const
{
  if (1==inc) {
    s << "{" << start << ".." << stop << "}";
  } else {
    s << "{" << start << ".." << stop << ".." << inc << "}";
  }
  return true;
}

bool int_ivlset::Equals(const shared_object* o) const
{
  const int_ivlset* foo = dynamic_cast <const int_ivlset*> (o);
  if (0==foo) return false;
  if (start != foo->start) return false;
  if (stop != foo->stop) return false;
  return (inc == foo->inc);
}

// ******************************************************************
// *                                                                *
// *                       real_ivlset  class                       *
// *                                                                *
// ******************************************************************

/**  An interval set of reals.
     These have the form {start..stop..inc}.
 */
class real_ivlset : public shared_set {
  double start, stop, inc;
  const type* realtype;
public:
  real_ivlset(const type* rt, double s, double e, double i);
  virtual void GetElement(long n, result &x) const;
  virtual long IndexOf(const result &x) const;
  virtual bool Print(OutputStream &s, int) const;
  virtual bool Equals(const shared_object* o) const;
};

// ******************************************************************
// *                      real_ivlset  methods                      *
// ******************************************************************

real_ivlset::real_ivlset(const type* rt, double s, double e, double i)
  : shared_set(long((e-s)/i + 1))
{
  realtype = rt;
  start = s;
  stop = e;
  inc = i;
  DCASSERT(inc);
}

void real_ivlset::GetElement(long n, result &x) const
{
  CHECK_RANGE(0, n, Size());
  x.setReal(start + n * inc);
}

long real_ivlset::IndexOf(const result &x) const
{
  if (!x.isNormal()) return -1;
  long i;
  if (inc>0) 
    i = long( ceil((x.getReal()-start)/inc - 0.5) );
  else
    i = long( ceil((start-x.getReal())/inc - 0.5) );
  if (i>=Size()) return -1;
  if (i<0) return -1;
  // i is the closest index.
  // make sure start + i*inc is close enough to x
  result r(start + i*inc);
  DCASSERT(realtype);
  if (0== realtype->compare(x, r))  return i;
  return -1;
}

bool real_ivlset::Print(OutputStream &s, int) const
{
  s << "{" << start << ".." << stop << ".." << inc << "}";
  return true;
}

bool real_ivlset::Equals(const shared_object* o) const
{
  const real_ivlset* foo = dynamic_cast <const real_ivlset*> (o);
  if (0==foo) return false;
  if (start != foo->start) return false;
  if (stop != foo->stop) return false;
  return (inc == foo->inc);
}

// ******************************************************************
// *                                                                *
// *                          objset class                          *
// *                                                                *
// ******************************************************************

/**  A generic set of objects.
     This can be used to represent the empty set,
     sets of a single element, and arbitrary sets.
 */
class objset : public shared_set {
  /// Type of the values.
  const type* item_type;
  /// Values in the set.
  result* values;
  /// The order of the values.
  long* order;
public:
  objset(const type* t, long s, result* v, long* o);
  virtual ~objset(); 
  virtual void GetElement(long n, result& x) const;
  virtual long IndexOf(const result &x) const;
  virtual bool Print(OutputStream &s, int) const;
  virtual bool Equals(const shared_object* o) const;
};

// ******************************************************************
// *                         objset methods                         *
// ******************************************************************

objset::objset(const type* t, long s, result* v, long* o)
 : shared_set(s)
{
#ifdef DEBUG_MEM
  printf("Creating   objset 0x%x\n", unsigned(this));
#endif
  DCASSERT(t);
  item_type = t;
  values = v;
  order = o;
#ifdef DEVELOPMENT_CODE
  for (long i=1; i<s; i++) {
    if (t->compare(values[order[i]], values[order[i-1]]) > 0) continue;
    DCASSERT(0);
  }
#endif
}

objset::~objset()
{
#ifdef DEBUG_MEM
  printf("Destroying objset 0x%x\n", unsigned(this));
  printf("\tvalues: 0x%x\n", unsigned(values));
  printf("\torder : 0x%x\n", unsigned(order));
#endif
  for (int i=0; i<Size(); i++)  values[i].deletePtr();
  delete[] values;
  delete[] order;
}

void objset::GetElement(long n, result &x) const
{
  CHECK_RANGE(0, n, Size());
  x = values[n];
}

long objset::IndexOf(const result &x) const
{
  // binary search through values
  long low = 0, high = Size()-1;
  while (low <= high) {
    long mid = (low+high)/2;
    long i = order[mid];
    CHECK_RANGE(0, i, Size());
    int cmp = item_type->compare(values[i], x);
    if (0==cmp)   return i;
    if (cmp < 0)  low = mid+1;
    else          high = mid-1;
  }
  return -1;
}

bool objset::Print(OutputStream &s, int) const
{
  s.Put('{');
  for (long i=0; i<Size(); i++) {
    if (i) s.Put(", ");
    item_type->print(s, values[i]);
  }
  s.Put('}');
  return true;
}

bool objset::Equals(const shared_object* o) const
{
  const objset* foo = dynamic_cast <const objset*> (o);
  if (0==foo) return false;
  if (Size() != foo->Size()) return false;
  if (item_type != foo->item_type) return false;
  for (int i=0; i<Size(); i++) {
    if (item_type->compare(values[order[i]], foo->values[foo->order[i]]))
    return false;
  }
  return true;
}

// ******************************************************************
// *                                                                *
// *                        makeorder  class                        *
// *                                                                *
// ******************************************************************

class makeorder {
  const type* valtype;
  long size;
  result* values;
  long* order;
public:
  makeorder(const type* t, long s, result* v, long* o) {
    DCASSERT(t);
    valtype = t;
    size = s;
    values = v;
    order = o;
  }
  inline int Compare(long i, long j) const {
    CHECK_RANGE(0, i, size);
    CHECK_RANGE(0, j, size);
    return valtype->compare(values[order[i]], values[order[j]]);
  }
  inline void Swap(long i, long j) {
    CHECK_RANGE(0, i, size);
    CHECK_RANGE(0, j, size);
    SWAP(order[i], order[j]);
  }
};

// ******************************************************************
// *                                                                *
// *                           Front  end                           *
// *                                                                *
// ******************************************************************

shared_set* MakeSingleton(const type* t, const result& r)
{
  result* items = new result[1];
  long* order = new long[1];
  items[0].constructFrom(r);
  order[0] = 0;
  return new objset(t, 1, items, order);
}

shared_set* MakeRangeSet(long start, long stop, long inc)
{
  return new int_ivlset(start, stop, inc);
}

shared_set* MakeRangeSet(const type* rt, double start, double stop, double inc)
{
  return new real_ivlset(rt, start, stop, inc);
}

shared_set* MakeSet(const type* t, long s, result* v, long* o)
{
  return new objset(t, s, v, o);
}

shared_set* MakeSet(const type* t, long s, result* v)
{
  DCASSERT(t);
  if (0==s) return new objset(t, 0, 0, 0);
  DCASSERT(v);
  long* order = new long[s];
  for (long i=0; i<s; i++) order[i] = i;
  makeorder foo(t, s, v, order);
  HeapSortAbstract(&foo, s);
  return new objset(t, s, v, order);
}

