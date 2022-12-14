
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

inline bool include_cmp(bool acont, bool bcont, bool left)
{
    // If interval points are equal,
    // compare based on inclusion.

    if (left) {
        // [x < (x
        if (acont) {
            return bcont ? 0 : -1;
        } else {
            return bcont ? +1 : 0;
        }
    } else {
        // x) < x]
        if (acont) {
            return bcont ? 0 : +1;
        } else {
            return bcont ? -1 : 0;
        }
    }
}

//
// Return 1  if this <  p
// Return -1 if this >  p
// Return 0  if this == p
// Return 2, -2 if we can't compare mathematically but can for storage
int interval_point::compare(const interval_point &p, bool left) const
{
    // ordinary case
    if (isNormal() && p.isNormal()) {
        if (getValue() < p.getValue()) {
            return -1;
        }
        if (p.getValue() < getValue()) {
            return +1;
        }
        // equal; compare based on inclusion
        return include_cmp(contains(), p.contains(), left);
    }

    // are they both infinity?
    if (isInfinity() && p.isInfinity()) {
        if (getSign() < p.getSign()) {
            return -1;
        }
        if (p.getSign() < getSign()) {
            return +1;
        }
        // equal, check for inclusion or not
        return include_cmp(contains(), p.contains(), left);
    }

    //
    // Is a infinity?
    if (isInfinity() && p.isNormal()) {
        if (getSign() < 0)    return -1;  // -oo < p
        else                    return +1;  // p < oo
    }
    //
    // Is p infinity?
    if (p.isInfinity() && isNormal()) {
        if (p.getSign() < 0)    return +1;  // -oo < a
        else                    return -1;  // a < oo
    }

    // Remaining cases are not mathematically comparable.
    // But we can compare for other purposes (i.e., checking
    // for uniqueness, arbitrary ordering, etc.)

    if (isNull()) {
       if (p.isNull()) return 0;
       else            return -2;
    }
    if (p.isNull()) return +2;

    if (isUnknown()) {
        if (p.isUnknown())  return 0;
        else                return -2;
    }
    if (p.isUnknown()) return +2;

    // Can we get here?
    DCASSERT(0);
    return 0;
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
    // a or b is null, keep it null.
    if (a.isNull() || b.isNull()) {
        c.setNull();
        return;
    }

    // a or b is unknown, keep it unknown.
    if (a.isUnknown() || b.isUnknown()) {
        c.setUnknown();
        return;
    }

    // Can do ordinary comparison.
    if (a.compare(b, left) <= 0) {
        c = a;
    } else {
        c = b;
    }
}

void Maximum(interval_point &c, const interval_point &a, const interval_point &b, bool left)
{
    // a or b is null, keep it null.
    if (a.isNull() || b.isNull()) {
        c.setNull();
        return;
    }

    // a or b is unknown, keep it unknown.
    if (a.isUnknown() || b.isUnknown()) {
        c.setUnknown();
        return;
    }

    // Can do ordinary comparison.
    if (a.compare(b, left) >= 0) {
        c = a;
    } else {
        c = b;
    }
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

bool interval_object::Print(std::ostream &s, int width) const
{
    DCASSERT(reals);
    result x;
    left.getAsResult(x);
    s << (left.contains() ? '[' : '(');
    reals->print(s, x, 0, -1);
    s << ", ";
    right.getAsResult(x);
    reals->print(s, x, 0, -1);
    s << (right.contains() ? ']' : ')');
    return true;
}

int interval_object::Compare(const shared_object *o) const
{
    if (this == o) return 0;
    const interval_object* i = dynamic_cast <const interval_object*> (o);
    if (!i) return 1;

    int cmp = left.compare(i->left, true);
    if (cmp) return cmp;
    return right.compare(i->right, false);
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
