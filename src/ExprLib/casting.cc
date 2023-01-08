
#include "casting.h"
#include "sets.h"
#include "unary.h"
#include "assoc.h"
#include "bogus.h"
#include "intervals.h"
#include "../Utils/init_opts.h"


// ******************************************************************
// *                                                                *
// *                        typeconv methods                        *
// *                                                                *
// ******************************************************************

general_conv* typeconv::general_list = nullptr;
specific_conv* typeconv::promote_list = nullptr;
specific_conv* typeconv::cast_list = nullptr;
warning_msg typeconv::promote_arg;

typeconv::typeconv()
{
}

typeconv::~typeconv()
{
}

//
// Statics
//

int typeconv::getPromoteDistance(const type* t1, const type* t2)
{
    if (!t1 || !t2) return -1;
    if (t1 == t2) return 0;

    const general_conv* g = findGeneral(t1, t2);
    if (g) return g->getDistance(t1, t2);

    const specific_conv* s = promote_list;
    findPair(s, g, t1, t2);
    if (!s) return -1;
    if (g) {
        const type* tmid = s->promotesTo(t1);
        return s->getDistance(t1) + g->getDistance(tmid, t2);
    } else {
        return s->getDistance(t1);
    }
}

const type* typeconv::getLeastCommonType(const type* a, const type* b)
{
    if (!a || !b) return nullptr;

    // Common and easy case
    if (a==b)     return a;

    //
    // Determine least set, proc, and modifiers required
    //
    bool is_set = a->isASet() || b->isASet();
    bool proc = a->hasProc() || b->hasProc();
    modifier m = MAX( a->getModifier(), b->getModifier() );

    //
    // Determine the base type to use
    //
    const simple_type* ba = a->getBaseType();
    const simple_type* bb = b->getBaseType();
    const type* t = nullptr;

    if (ba == bb) {
        // Another common and easy case
        t = ba;
    } else {
        int a2b = getPromoteDistance(ba, bb);
        int b2a = getPromoteDistance(bb, ba);
        if (a2b >= 0 && b2a >= 0) {
            // Can this happen?
            // If so, take the lesser promotion distance
            DCASSERT(a2b != b2a);
            if (a2b < b2a) {
                t = bb;
            } else {
                t = ba;
            }
        }
        if (a2b >= 0 && b2a < 0) {
            t = bb;
        }
        if (a2b < 0 && b2a >= 0) {
            t = ba;
        }
    }

    //
    // Add set, proc, and modifiers as required
    //
    t = type::procMod(proc, m, t);
    if (is_set && t) {
        t = t->getSetOfThis();
    }
    return t;
}

bool typeconv::isCastable(const type* t1, const type* t2)
{
    if (!t1 || !t2) return false;
    if (t1 == t2) return true;

    const general_conv* g = findGeneral(t1, t2);
    if (g) return true;

    const specific_conv* s = promote_list;
    findPair(s, g, t1, t2);
    if (s) return true;
    s = cast_list;
    findPair(s, g, t1, t2);
    return s;
}

inline expr* applyConversions(const general_conv* g, const specific_conv* s,
        const location &W, expr* e, const type* newt)
{
    const type* midt = s->promotesTo(e->Type());
    DCASSERT(midt);
    if (!g || !g->requiresConversion(midt, newt)) {
        return s->convert(W, e, newt);
    }
    return g->convert(W, s->convert(W, e, midt), newt);
}

expr* typeconv::castExpr(bool promote_only, const location &W,
        const type* newt, expr* e)
{
    if (bogus_expr::orNull(e)) return e;

    if (!newt) {
        Delete(e);
        return bogus_expr::makeError();
    }

    const type* oldt = e->Type();
    DCASSERT(oldt);

    if (0==getPromoteDistance(oldt, newt)) return e;

    const general_conv* g = findGeneral(oldt, newt);
    if (g) return g->convert(W, e, newt);

    const specific_conv* s = promote_list;
    findPair(s, g, oldt, newt);
    if (s) {
        return applyConversions(g, s, W, e, newt);
    }

    if (!promote_only) {
        s = cast_list;
        findPair(s, g, oldt, newt);
        if (s) {
            return applyConversions(g, s, W, e, newt);
        }
    }

    //
    // Can't convert; return error
    //
    Delete(e);
    return bogus_expr::makeError();
}

expr* typeconv::promoteExpr(expr* e, bool prc, bool rnd, const expr* fp)
{
    if (bogus_expr::orNull(e)) return e;
    if (!fp) {
        Delete(e);
        return nullptr;
    }
    if (fp->NumComponents() != e->NumComponents()) {
        return bogus_expr::makeError();
    }

    //
    // Make sure the requested promotion is possible
    //

    bool changetype = false;
    int nc = e->NumComponents();
    for (int i=0; i<nc; i++) {
        const type* fpt = fp->Type(i);
        DCASSERT(fpt);
        if (rnd) fpt = fpt->modifyType(RAND);
        DCASSERT(fpt);
        if (prc) fpt = fpt->addProc();
        DCASSERT(fpt);
        int d = getPromoteDistance(e->Type(i), fpt);
        if (d<0) return bogus_expr::makeError();
        if (d>0) changetype = true;
    }

    //
    // Fast and easy case: no promotion(s) needed
    //

    if (!changetype) {
        return e;
    }

    //
    // Actually change the type
    //

    if (promote_arg.start(e->Where())) {
      promote_arg << "Promoting argument ";
      e->Print(promote_arg.stream(), 0);
      promote_arg << " to type ";
      fp->PrintType(promote_arg.stream());
      promote_arg.stop();
    }

    //
    // Common and easier case: the expression has only one aggregate
    //

    if (1==nc) {
        const type* prf = type::procMod(prc, rnd ? RAND : DETERM, fp->Type());
        DCASSERT(prf);
        return castExpr(true, e->Where(), prf, e);
    }

    //
    // Promote each aggregate as necessary
    //
    expr** newagg = new expr*[nc];
    for (int i=0; i<nc; i++) {
        const type* prf = type::procMod(prc, rnd ? RAND : DETERM, fp->Type(i));
        DCASSERT(prf);
        expr* thisagg = Share(e->GetComponent(i));
        if (!thisagg) {
            newagg[i] = thisagg;
        } else {
            newagg[i] = castExpr(true, e->Where(), prf, e);
            DCASSERT(!bogus_expr::orNull(newagg[i]));
        }
    }

    //
    // Build the new aggregate
    //
    expr* f = assoc_op::makeExpr(e->Where(), assoc_op::aop_colon,
                newagg, nullptr, nc);
    Delete(e);
    return f;
}



//
// Private helpers
//

void typeconv::registerConv(general_conv* c)
{
    if (!c) return;
    c->next = general_list;
    general_list = c;
}

void typeconv::registerConv(specific_conv* c)
{
    if (!c) return;
    if (c->isPromotion()) {
        c->next = promote_list;
        promote_list = c;
    } else {
        c->next = cast_list;
        cast_list = c;
    }
}

/*
 *  Find the best general conversion from old type to new type.
 *  Returns null if we can't get there with just a general conversion.
 */
const general_conv* typeconv::findGeneral(const type* oldt, const type* newt)
{
    DCASSERT(oldt);
    DCASSERT(newt);
    const general_conv* match = nullptr;
    int best = -1;
    unsigned count = 0;
    for (const general_conv* ptr = general_list; ptr; ptr=ptr->next) {
        int d = ptr->getDistance(oldt, newt);
        if (d<0) continue;
        if ((best<0) || (d<best)) {
            best = d;
            match = ptr;
            count = 1;
            continue;
        }
        if (d==best) ++count;
    }
    if (count>1) {
        internal_error E(__FILE__, __LINE__);
        E << "More than one general conversion for " << *oldt;
        E << " -> " << *newt;
        return nullptr;
    }
    return match;
}

/*
 * Find the best specific + general conversion to apply
 * to get from old type to new type.
 *  @param  list    On input, list of specific conversions to check
 *                  (promotions or casts).
 *                  On output, the best specific conversion to apply first,
 *                  or null.
 *  @param  gc      On output, the best general conversion to apply,
 *                  or null for none.
 *
 *  @param  oldt    Original type
 *  @param  newt    Desired type
 */
void typeconv::findPair(const specific_conv* &list, const general_conv* &gc,
        const type* oldt, const type* newt)
{
    DCASSERT(oldt);
    DCASSERT(newt);
    const specific_conv* s_match = nullptr;
    gc = nullptr;
    int best = -1;
    unsigned count = 0;

    for (const specific_conv* ptr = list; ptr; ptr=ptr->next) {
        int sd = ptr->getDistance(oldt);
        if (sd<0) continue;
        if ((best>=0) && (sd>best)) continue;
        const type* midt = ptr->promotesTo(oldt);
        DCASSERT(midt);
        int gd = 0;
        const general_conv* thisgc = nullptr;
        if (midt != oldt) {
            thisgc = findGeneral(midt, newt);
            if (!thisgc) continue;
            gd = thisgc->getDistance(midt, newt);
        }
        int d = sd + gd;
        if ((best<0) || (d<best)) {
            best = d;
            s_match = ptr;
            gc = thisgc;
            count = 1;
            continue;
        }
        if (d==best) ++count;
    }

    if (count>1) {
        internal_error E(__FILE__, __LINE__);
        E << "More than one specific+general conversion for " << *oldt;
        E << " -> " << *newt;
    }
    list = s_match;
}

// ******************************************************************
// *                                                                *
// *                      general_conv methods                      *
// *                                                                *
// ******************************************************************

general_conv::general_conv()
{
    registerConv(this);
}

// ******************************************************************
// *                                                                *
// *                     specific_conv  methods                     *
// *                                                                *
// ******************************************************************

specific_conv::specific_conv(bool c)
{
    is_cast = c;
    registerConv(this);
}

// ******************************************************************
// *                                                                *
// *                        typecast methods                        *
// *                                                                *
// ******************************************************************

typecast::typecast(const location &W, const type* newt, expr* x)
 : unary(W, unary_op::uop_none, newt, x)
{
    silent = false;
}

bool typecast::Print(std::ostream &s, int) const
{
    DCASSERT(opnd);

    bool printed = false;
    if (!silent) {
        DCASSERT(Type());
        s << *Type() << '(';
        printed = true;
    }

    if (opnd->Print(s, 0))  printed = true;

    if (!silent) s << ')';

    return printed;
}

void typecast::Compute(traverse_data &x)
{
    DCASSERT(opnd);
    opnd->Compute(x);
}

expr* typecast::buildAnother(expr* x) const
{
    return new typecast(Where(), Type(), x);
}

// ******************************************************************
// ******************************************************************
// **                                                              **
// **                                                              **
// **                       Promotion  rules                       **
// **                                                              **
// **                                                              **
// ******************************************************************
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
// ******************************************************************
// **                                                              **
// **                                                              **
// **                        Initialization                        **
// **                                                              **
// **                                                              **
// ******************************************************************
// ******************************************************************

class casting_init : public initializer {
    public:
        casting_init();
    protected:
        virtual void execute();
};
static casting_init the_casting_initializer;

casting_init::casting_init() : initializer(__FILE__, 1, 2)
{
    builds_resource(0, "casts");
    needs_resource(1, "types"); // not sure how important this is
    needs_resource(2, "Warning");
}

void casting_init::execute()
{
    //
    // Promotion warning
    //
    initialize_msg(typeconv::promote_arg,
        "promote_args",
        "When arguments are automatically promoted in a function call",
        get_object(2, "Warning")
    );
    typeconv::promote_arg.Deactivate();

    //
    // Build and register type conversion rules
    //
    new null2any;
    new formalism2model;
    new precomp_add;
    new noop_addproc;
    new elem2set;
    new int2real;
    new real2int;
}

