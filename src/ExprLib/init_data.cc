
#include "init_data.h"
#include "../Options/options.h"
#include "../Options/optman.h"
#include "../Utils/strings.h"
#include "../Utils/initializer.h"

#include "exprman.h"
#include "type.h"
#include "casting.h"
#include "sets.h"
#include "intervals.h"

#include "ops_bool.h"
#include "ops_int.h"
#include "ops_real.h"
#include "ops_set.h"
#include "ops_misc.h"

#include <errno.h>

//#define REQUIRES_PRECOMPUTING

// #define DEBUG_REAL_TYPE

// ******************************************************************
// *                                                                *
// *                                                                *
// *                     Promotions and casting                     *
// *                                                                *
// *                                                                *
// ******************************************************************

// ******************************************************************
// *                                                                *
// *                         null2any class                         *
// *                                                                *
// ******************************************************************

/// This class handles type "promotion" rules of the form null -> X.
class null2any : public general_conv {
public:
  null2any();
  virtual int getDistance(const type* src, const type* dest) const;
  virtual bool requiresConversion(const type*, const type*) const {
    return false;
  }
  virtual expr* convert(const location &W, expr* e, const type* t) const {
    return new typecast(W, t, e);
  }
};

null2any::null2any() : general_conv()
{
}

int null2any::getDistance(const type* src, const type* dest) const
{
  DCASSERT(src != dest);
  DCASSERT(em);
  if (src != type::null) return -1;
  return RANGE_EXPAND;
}

// ******************************************************************
// *                                                                *
// *                         elem2set class                         *
// *                                                                *
// ******************************************************************

/// This class handles type promotion rules of the form X -> {X}.
class elem2set : public general_conv {

    class converter : public typecast {
    public:
      converter(const location &W, const type* nt, expr* x);
      virtual void Compute(traverse_data &x);
    protected:
      virtual expr* buildAnother(expr* x) const {
        return new converter(Where(), Type(), x);
      }
    };

public:
  elem2set();
  virtual int getDistance(const type* src, const type* dest) const;
  virtual bool requiresConversion(const type* src, const type* dest) const {
    return true;
  }
  virtual expr* convert(const location &W, expr* e, const type* d) const {
    return new converter(W, d, e);
  }
};

elem2set::converter::converter(const location &W, const type* nt, expr* x)
 : typecast(W, nt, x)
{
  DCASSERT(x);
  DCASSERT(x->Type());
  DCASSERT(nt);
  DCASSERT(nt->getSetElemType() == x->Type());
}

void elem2set::converter::Compute(traverse_data &x)
{
  DCASSERT(opnd);
  DCASSERT(x.answer);
  opnd->Compute(x);
  if (x.answer->isInfinity() || x.answer->isNormal()) {
    shared_set* s = MakeSingleton(opnd->Type(), *x.answer);
    x.answer->setPtr(s);
  }
}

elem2set::elem2set() : general_conv()
{
}

int elem2set::getDistance(const type* src, const type* dest) const
{
  DCASSERT(src);
  DCASSERT(dest);
  if (src->getSetOfThis() == dest) return MAKE_SET;
  return -1;
}

// ******************************************************************
// *                                                                *
// *                     formalism2model  class                     *
// *                                                                *
// ******************************************************************

/// This class handles type "promotion" rules of the form formalism -> MODEL.
class formalism2model : public general_conv {
public:
  formalism2model();
  virtual int getDistance(const type* src, const type* dest) const;
  virtual bool requiresConversion(const type*, const type*) const {
    return false;
  }
  virtual expr* convert(const location &, expr* src, const type*) const {
    return src;
  }
};

formalism2model::formalism2model() : general_conv()
{
}

int formalism2model::getDistance(const type* src, const type* dest) const
{
  DCASSERT(src != dest);
  DCASSERT(em);
  if (!dest->matches("model")) return -1;
  DCASSERT(src);
  return src->isAFormalism() ? 0 : -1;
}

// ******************************************************************
// *                                                                *
// *                       precomp_add  class                       *
// *                                                                *
// ******************************************************************

/** This class handles changes in modifiers that require "precomputation".
    Currently those are
         x -> rand x
         x -> proc x
         x -> proc rand x
      ph x -> proc ph x
*/
class precomp_add : public general_conv {

    class converter : public typecast {
      result cached;
      bool precomputed;
    public:
      converter(const location &W, const type* nt, expr* x);
      virtual void Compute(traverse_data &x);
      virtual void Traverse(traverse_data &x);
    protected:
      virtual expr* buildAnother(expr* x) const {
        return new converter(Where(), Type(), x);
      }
    };

public:
  precomp_add();
  virtual int getDistance(const type* src, const type* dest) const;
  virtual bool requiresConversion(const type*, const type*) const {
    return true;
  }
  virtual expr* convert(const location &W, expr* e, const type* t) const {
    return new converter(W, t, e);
  }
};

precomp_add::converter
 ::converter(const location &W, const type* nt, expr* x)
 : typecast(W, nt, x)
{
  precomputed = false;
  cached.setNull();
  silent = true;
}

void precomp_add::converter::Compute(traverse_data &x)
{
  if (!precomputed) {
#ifdef REQUIRES_PRECOMPUTING
    if (em->startInternal(__FILE__, __LINE__)) {
      em->causedBy(this);
      em->internal() << "Expression not precomputed: ";
      Print(em->internal(), 0);
      em->stopIO();
    }
    exit(1);
#endif
    SafeCompute(opnd, x);
  } else {
    x.answer[0] = cached;
  }
}

void precomp_add::converter::Traverse(traverse_data &x)
{
  switch (x.which) {
    case traverse_data::PreCompute:
        x.which = traverse_data::Compute;
        x.answer = &cached;
        SafeCompute(opnd, x);
        precomputed = true;
        x.which = traverse_data::PreCompute;
        return;

    case traverse_data::FindRange: {
        x.which = traverse_data::Compute;
        SafeCompute(opnd, x);
        interval_object *range = new interval_object(*x.answer, Type(0));
        x.answer->setPtr(range);
        x.which = traverse_data::FindRange;
        return;
    }

    default:
        typecast::Traverse(x);
        return;
  }
}


precomp_add::precomp_add() : general_conv()
{
}

int precomp_add::getDistance(const type* src, const type* dest) const
{
  DCASSERT(src != dest);
  DCASSERT(em);
  if (src->hasProc()) return -1;
  if (src->isASet()) return -1;
  if (dest->isASet()) return -1;
  if (src->getBaseType() != dest->getBaseType()) return -1;

  int d = 0;
  if (dest->hasProc()) d += MAKE_PROC;

  if (src->getModifier() == DETERM) {
    switch (dest->getModifier()) {
      case DETERM:  return d;
      case RAND:    return d + MAKE_RAND;
      default:      return -1;
    } // switch
  }

  if (src->getModifier() != PHASE) return -1;
  if (dest->getModifier() != PHASE) return -1;
  return d;
}


// ******************************************************************
// *                                                                *
// *                       noop_addproc class                       *
// *                                                                *
// ******************************************************************

/** This class handles additions of "proc" that do not require anything.
    These are currently
      rand x -> proc rand x
      proc x -> proc rand x
*/
class noop_addproc : public general_conv {
public:
  noop_addproc();
  virtual int getDistance(const type* src, const type* dest) const;
  virtual bool requiresConversion(const type*, const type*) const {
    return false;
  }
  virtual expr* convert(const location &W, expr* e, const type* t) const {
    return new typecast(W, t, e);
  }
};

noop_addproc::noop_addproc() : general_conv()
{
}

int noop_addproc::getDistance(const type* src, const type* dest) const
{
  DCASSERT(src != dest);
  DCASSERT(em);
  if (src->isASet()) return -1;
  if (dest->isASet()) return -1;
  if (!dest->hasProc()) return -1;
  if (dest->getModifier() != RAND) return -1;
  if (src->getBaseType() != dest->getBaseType()) return -1;

  int d = 0;
  if (!src->hasProc())  d = MAKE_PROC;

  switch (src->getModifier()) {
    case DETERM:    return MAKE_RAND + d;
    case RAND:      return d;
    default:        return -1;
  } // switch
}


// ******************************************************************
// *                                                                *
// *                         int2real class                         *
// *                                                                *
// ******************************************************************


/** Type promotions from int to real.
    Note this also handles {int} to {real}.
*/
class int2real : public specific_conv {

    class converter : public typecast {
    public:
      converter(const location &W, const type* nt, expr* x);
      virtual void Compute(traverse_data &x);
    protected:
      virtual expr* buildAnother(expr* x) const {
        return new converter(Where(), Type(), x);
      }
    };

    class setconv : public typecast {

        class int2real_convert : public set_converter::element_convert {
        public:
          virtual void convert(result &x) const {
            if (x.isNormal()) x.setReal(x.getInt());
          }
          virtual void revert(result &x) const {
            if (x.isNormal()) x.setInt(long(x.getReal()));
          }
        };

  static int2real_convert foo;

    public:
      setconv(const location &W, const type* nt, expr* x);
      virtual void Compute(traverse_data &x);
    protected:
      virtual expr* buildAnother(expr* x) const {
        return new setconv(Where(), Type(), x);
      }
    };

public:
  int2real();
  virtual int getDistance(const type* src) const {
    DCASSERT(src);
    if (!type::matches(src->getBaseType(), "int"))  return -1;
    if (src->getModifier() == PHASE)    return -1;  // different rule.
    return SIMPLE_CONV;
  }
  virtual const type* promotesTo(const type* src) const;
  virtual expr* convert(const location &, expr*, const type*) const;
};

int2real::setconv::int2real_convert int2real::setconv::foo;

int2real::converter::converter(const location &W, const type* nt, expr* x)
 : typecast(W, nt, x)
{
}

void int2real::converter::Compute(traverse_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(opnd);
  opnd->Compute(x);
  if (x.answer->isNormal()) {
    x.answer->setReal(x.answer->getInt());
  }
}

int2real::setconv::setconv(const location &W, const type* nt, expr* x)
 : typecast(W, nt, x)
{
}

void int2real::setconv::Compute(traverse_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(opnd);
  opnd->Compute(x);
  if (x.answer->isNull()) return;

  shared_set* xs = smart_cast<shared_set*> (Share(x.answer->getPtr()));
  DCASSERT(xs);
  x.answer->setPtr(new set_converter(foo, xs));
}



int2real::int2real() : specific_conv(false)
{
}

const type* int2real::promotesTo(const type* src) const
{
  DCASSERT(src);
  DCASSERT(type::matches(src->getBaseType(), "int"));
  DCASSERT(src->getModifier() != PHASE);

  const type* dest = type::find(src->isASet(),
          src->hasProc(), src->getModifier(), "real");

  DCASSERT(dest);
  return dest;
}

expr* int2real::convert(const location &W, expr* e, const type* nt) const
{
  if (nt->isASet())  return new setconv(W, nt, e);
  return new converter(W, nt, e);
}

// ******************************************************************
// *                                                                *
// *                         real2int class                         *
// *                                                                *
// ******************************************************************

/** Type casting from real to int.
    Note this also handles {real} to {int}.
*/
class real2int : public specific_conv {

    class converter : public typecast {
    public:
      converter(const location &W, const type* nt, expr* x);
      virtual void Compute(traverse_data &x);
    protected:
      virtual expr* buildAnother(expr* x) const {
        return new converter(Where(), Type(), x);
      }
    };

public:
  real2int();
  virtual int getDistance(const type* src) const {
    DCASSERT(src);
    if (!type::matches(src->getBaseType(), "real")) return -1;
    return SIMPLE_CONV;
  }
  virtual const type* promotesTo(const type* src) const;
  virtual expr* convert(const location &, expr*, const type*) const;
};

real2int::converter::converter(const location &W, const type* nt, expr* x)
 : typecast(W, nt, x)
{
}

void real2int::converter::Compute(traverse_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(opnd);
  opnd->Compute(x);
  if (x.answer->isNormal()) {
    x.answer->setInt(long(x.answer->getReal()));
  }
}

real2int::real2int() : specific_conv(true)
{
}

const type* real2int::promotesTo(const type* src) const
{
  DCASSERT(src);
  DCASSERT(type::matches(src->getBaseType(), "real"));

  const type* dest = type::find(src->isASet(),
          src->hasProc(), src->getModifier(), "int");

  DCASSERT(dest);
  return dest;
}

expr* real2int::convert(const location &W, expr* e, const type* nt) const
{
  // TBD: {real} to {int}
  return new converter(W, nt, e);
}


// ******************************************************************
// *                                                                *
// *                                                                *
// *                           Front  end                           *
// *                                                                *
// *                                                                *
// ******************************************************************

void InitTypes(exprman* em)
{
  if (0==em)  return;

  // Type changes
  em->registerConversion( new null2any        );
  em->registerConversion( new formalism2model );
  em->registerConversion( new precomp_add     );
  em->registerConversion( new noop_addproc    );
  em->registerConversion( new elem2set        );
  em->registerConversion( new int2real        );
  em->registerConversion( new real2int        );

  // Operators
  InitBooleanOps(em);
  InitIntegerOps(em);
  InitRealOps(em);
  InitSetOps(em);
  InitMiscOps(em);
}

