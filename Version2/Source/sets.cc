
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

class int_interval : public set_result {
  int start, stop, inc;
public:
  int_interval(int s, int e, int i);
  virtual ~int_interval() { }
  virtual void GetElement(int n, result &x);
  virtual int IndexOf(const result &x);
  virtual void GetOrder(int n, int &i, result &x);
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

// ******************************************************************
// *                                                                *
// *                     generic_int_set  class                     *
// *                                                                *
// ******************************************************************

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

// ******************************************************************
// *                                                                *
// *           Global functions  to build set expressions           *
// *                                                                *
// ******************************************************************

expr*  MakeInterval(expr* start, expr* stop, expr* inc)
{
  return NULL;
}

//@}

