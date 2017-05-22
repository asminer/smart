
#include "intervals.h"
#include "exprman.h"

// ******************************************************************
// *                                                                *
// *                     interval_point methods                     *
// *                                                                *
// ******************************************************************

const type* interval_point::reals = 0;

interval_point::interval_point()
{
  setUnknown();
}

void interval_point::setFrom(const result &v, const type* st) 
{
  if (v.isNormal()) {
    status = normal_closed;
    if (st->getBaseType() == reals) {
      value = v.getReal();
    } else {
      value = v.getInt();
    }
    return;
  }
  if (v.isInfinity()) {
    status = infinity_closed;
    value = v.signInfinity();
    return;
  }
  if (v.isUnknown()) {
    setUnknown();
  } else {
    setNull();
  }
}

void Minimum(interval_point &c, const interval_point &a, const interval_point &b, bool left)
{
  // ordinary case
  if (a.isNormal() && b.isNormal()) {
    if (a.getValue() < b.getValue()) {
      c = a;
      return;
    }
    if (b.getValue() < a.getValue()) {
      c = b;
      return;
    }
    // equal, check for inclusion or not
    if (left) 
      c.setNormal(a.contains() || b.contains(), a.getValue());
    else 
      c.setNormal(a.contains() && b.contains(), a.getValue());
    return;
  }

  // a or b is null, keep it null.
  if (a.isNull() || b.isNull()) {
    c.setNull();
    return;
  }
  DCASSERT(! a.isNull());
  DCASSERT(! b.isNull());
 
  // are they both infinity?
  if (a.isInfinity() && b.isInfinity()) {
    if (a.getSign() < b.getSign()) {
      c = a;
      return;
    }
    if (b.getSign() < a.getSign()) {
      c = b;
      return;
    }
    // equal, check for inclusion or not
    if (left)
      c.setInfinity(a.contains() || b.contains(), a.getSign());
    else
      c.setInfinity(a.contains() && b.contains(), a.getSign());
    return;
  }

  // is a infinity?  (if so, b is finite)
  if (a.isInfinity()) {
    if (a.getSign() < 0)
      c = a;    // -oo < b for sure
    else
      c = b;    // b < oo for sure
    return;
  }

  // is b infinity?  (if so, a is finite)
  if (b.isInfinity()) {
    if (b.getSign() < 0)
      c = b;    // -oo < a for sure
    else
      c = a;    // a < oo for sure
    return;
  }

  // I think everything else is "unknown"
  c.setUnknown();
}

void Maximum(interval_point &c, const interval_point &a, const interval_point &b, bool left)
{
  // ordinary case
  if (a.isNormal() && b.isNormal()) {
    if (a.getValue() < b.getValue()) {
      c = b;
      return;
    }
    if (b.getValue() < a.getValue()) {
      c = a;
      return;
    }
    // equal, check for inclusion or not
    if (left)
      c.setNormal(a.contains() && b.contains(), a.getValue());
    else
      c.setNormal(a.contains() || b.contains(), a.getValue());
    return;
  }
 
  // a or b is null, keep it null.
  if (a.isNull() || b.isNull()) {
    c.setNull();
    return;
  }
  DCASSERT(! a.isNull());
  DCASSERT(! b.isNull());
 
  // are they both infinity?
  if (a.isInfinity() && b.isInfinity()) {
    if (a.getSign() < b.getSign()) {
      c = b;
      return;
    }
    if (b.getSign() < a.getSign()) {
      c = a;
      return;
    }
    // equal, check for inclusion or not
    if (left)
      c.setInfinity(a.contains() && b.contains(), a.getSign());
    else
      c.setInfinity(a.contains() || b.contains(), a.getSign());
    return;
  }

  // is a infinity?  (b is finite)
  if (a.isInfinity()) {
    if (a.getSign() < 0)
      c = b;    // -oo < b for sure
    else
      c = a;    // b < oo for sure
    return;
  }

  // is b infinity?  (a is finite)
  if (b.isInfinity()) {
    if (b.getSign() < 0)
      c = a;    // -oo < a for sure
    else
      c = b;    // a < oo for sure
    return;
  }

  // I think everything else is "unknown"
  c.setUnknown();
}



// ******************************************************************
// *                                                                *
// *                    interval_object  methods                    *
// *                                                                *
// ******************************************************************

const type* interval_object::reals = 0;

interval_object::interval_object() : shared_object()
{
}

interval_object::interval_object(const result &x, const type* st)
 : shared_object()
{
  left.setFrom(x, st);
  right = left;
}

interval_object::~interval_object()
{
}

bool interval_object::Print(OutputStream &s, int width) const
{
  DCASSERT(reals);
  result x;
  left.getAsResult(x);
  if (left.contains()) s.Put('['); else s.Put('(');
  reals->print(s, x, 0, -1);
  s << ", ";
  right.getAsResult(x);
  reals->print(s, x, 0, -1);
  if (right.contains()) s.Put(']'); else s.Put(')');
  return true;
}

bool interval_object::Equals(const shared_object *o) const
{
  const interval_object* i = dynamic_cast <const interval_object*> (o);
  if (0==i) return false;
  return  (left == i->left) && (right == i->right);
}

// ******************************************************************
// *                                                                *
// *                           Front  end                           *
// *                                                                *
// ******************************************************************

void InitIntervals(const exprman* em)
{
  if (0==em) return;
  interval_point::reals = em->REAL;
  interval_object::reals = em->REAL;
}


void computeUnion(interval_object &c, const interval_object &a, const interval_object &b)
{
  Minimum(c.Left(), a.Left(), b.Left(), true);
  Maximum(c.Right(), a.Right(), b.Right(), false);
}

void computeMinimum(interval_object &c, const interval_object &a, const interval_object &b)
{
  Minimum(c.Left(), a.Left(), b.Left(), true);
  Minimum(c.Right(), a.Right(), b.Right(), false);
}

void computeMaximum(interval_object &c, const interval_object &a, const interval_object &b)
{
  Maximum(c.Left(), a.Left(), b.Left(), true);
  Maximum(c.Right(), a.Right(), b.Right(), false);
}


void sortLeft(interval_object* &a, interval_object* &b)
{
  DCASSERT(a);
  DCASSERT(b);
  interval_point c;
  Minimum(c, a->Left(), b->Left(), true);
  if (c == a->Left()) {
    // already sorted
  } else {
    SWAP(a, b);
  }
}

void sortRight(interval_object* &a, interval_object* &b)
{
  DCASSERT(a);
  DCASSERT(b);
  interval_point c;
  Minimum(c, a->Right(), b->Right(), false);
  if (c == a->Right()) {
    // already sorted
  } else {
    SWAP(a, b);
  }
}
