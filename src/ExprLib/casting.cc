
#include "casting.h"
#include "unary.h"


// ******************************************************************
// *                                                                *
// *                        typeconv methods                        *
// *                                                                *
// ******************************************************************

general_conv* typeconv::general_list = nullptr;
specific_conv* typeconv::promote_list = nullptr;
specific_conv* typeconv::cast_list = nullptr;

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

const type* typeconv::getLeastCommonType(const type* a, const type* b) const
{
  if (!a || !b) return nullptr;
  if (a==b)     return a;

  bool is_set = a->isASet() || b->isASet();
  bool proc = a->hasProc() || b->hasProc();

  modifier m = MAX( a->getModifier(), b->getModifier() );

  const simple_type* ba = a->getBaseType();
  const simple_type* bb = b->getBaseType();

  const type* t = nullptr;
  if (isPromotable(ba, bb)) t = bb;
  if (isPromotable(bb, ba)) t = ba;
  if (t)                    t = t->modifyType(m);
  if (t) if (proc)          t = t->addProc();
  if (t) if (is_set)        t = t->getSetOfThis();
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
        expr* e, const type* newt)
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
        return bogus_expr::getError();

    const type* oldt = e->Type();
    DCASSERT(oldt);

    if (0==getPromoteDistance(oldt, newt)) return e;

    const general_conv* g = findGeneral(oldt, newt);
    if (g) return g->convert(W, e, newt);

    const specific_conv* s = promote_list;
    findPair(s, g, oldt, newt);
    if (s) {
        return applyConversions(g, s, e, newt);
    }

    if (!promote_only) {
        s = cast_list;
        findPair(s, g, oldt, newt);
        if (s) {
            return applyConversions(g, s, e, newt);
        }
    }

    //
    // Can't convert; return error
    //
    Delete(e);
    return bogus_expr::getError();
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

