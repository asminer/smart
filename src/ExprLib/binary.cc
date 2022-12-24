
#include "binary.h"
#include "bogus.h"
#include "dd_front.h"

// ******************************************************************
// *                                                                *
// *                       binary_op  methods                       *
// *                                                                *
// ******************************************************************

const binary_op** binary_op::registry = nullptr;

binary_op::binary_op(opcode o)
{
    code = o;
    registerOp(this);
}

binary_op::~binary_op()
{
}

const char* binary_op::getOp(opcode op)
{
    switch (op) {
        case bop_none:    return "no-op";
        case bop_mod:     return "%";
        case bop_diff:    return "\\";
        case bop_implies: return "->";
        case bop_equals:  return "==";
        case bop_nequal:  return "!=";
        case bop_gt:      return ">";
        case bop_ge:      return ">=";
        case bop_lt:      return "<";
        case bop_le:      return "<=";
        case bop_until:   return "U";
        case bop_and:     return "&&";
        default:          return "unknown_op";
    }
    return "error";  // will never get here, keep compilers happy
}

const char* binary_op::documentOp(opcode op)
{
    switch (op) {
        case bop_mod:     return "modulo";
        case bop_diff:    return "set difference";
        case bop_implies: return "implication";
        case bop_equals:  return "equality comparison";
        case bop_nequal:  return "inequality comparison";
        case bop_gt:      return "greater than";
        case bop_ge:      return "greater or equal";
        case bop_lt:      return "less than";
        case bop_le:      return "less or equal";
        default:          return nullptr;
    }
    return nullptr;  // will never get here, keep compilers happy
}

const type* binary_op::getTypeOf(const type* l, opcode code, const type* r)
{
    const binary_op* match = bestMatch(l, code, r);
    return match ? match->getExprType(l, r) : nullptr;
}

expr* binary_op::makeExpr(const location& W, expr* lt, opcode op, expr* rt)
{
    if (bogus_expr::orNull(lt)) {
        Delete(rt);
        return lt;
    }
    if (bogus_expr::orNull(rt)) {
        Delete(lt);
        return rt;
    }

    const binary_op* match = bestMatch(lt->Type(), op, rt->Type());
    if (match) {
        return match->makeExpr(W, lt, rt);
    }

    typechecking_error E(W);
    E << "Undefined binary operation: ";
    lt->PrintType(E.stream());
    E << " " << getOp(op) << " ";
    rt->PrintType(E.stream());
    Delete(lt);
    Delete(rt);
    return bogus_expr::getError();
}

const binary_op* binary_op::bestMatch(const type* l, opcode code, const type* r)
{
    const binary_op* match = nullptr;
    int best_match = -1;
    unsigned num_matches = 0;
    for (const binary_op* ptr = (code<bop_none) ? registry[code] : nullptr;
            ptr; ptr=ptr->next)
    {
        int d = ptr->getPromoteDistance(l, r);
        if (d<0)  continue;
        if (best_match >= 0) {
            if (d > best_match)  continue;
            if (d == best_match) {
                num_matches++;
                continue;
            }
        }
        num_matches = 1;
        best_match = d;
        match = ptr;
    } // for all operations with this code

    if (num_matches > 1) {
        // too many matches, this should not happen!
        internal_error E(__FILE__, __LINE__);
        E << "Cannot decide on binary operation: ";
        if (l)  E << *l;
        else    E << "notype";
        E << " " << getOp(code) << " ";
        if (r)  E << *r;
        else    E << "notype";
        return nullptr;
    }
    return match;
}

void binary_op::registerOp(binary_op* op)
{
    if (!registry) {
        registry = new const binary_op* [bop_none];
        for (unsigned i=0; i<bop_none; i++) {
            registry[i] = nullptr;
        }
    }
    if (!op)    return;
    if (op->getOpcode() >= bop_none) return;
    op->next = registry[op->getOpcode()];
    registry[op->getOpcode()] = op;
}


// ******************************************************************
// *                                                                *
// *                         binary methods                         *
// *                                                                *
// ******************************************************************

binary::binary(const location &W, binary_op::opcode oc,
 const type* t, expr *l, expr *r)
 : expr(W, t)
{
    left = l;
    right = r;
    opcode = oc;
    DCASSERT(!bogus_expr::orNull(left));
    DCASSERT(!bogus_expr::orNull(right));
}

binary::~binary()
{
    Delete(left);
    Delete(right);
}

void binary::Traverse(traverse_data &x)
{
    switch (x.which) {
        case traverse_data::Substitute: {
            DCASSERT(x.answer);
            left->Traverse(x);
            expr* newleft = smart_cast <expr*> (Share(x.answer->getPtr()));
            right->Traverse(x);
            expr* newright = smart_cast <expr*> (Share(x.answer->getPtr()));
            x.answer->setPtr(MakeAnother(newleft, newright));
            return;
        }

        case traverse_data::GetProducts:
            if (x.elist)  x.elist->Append(this);
            DCASSERT(x.answer);
            x.answer->setInt(x.answer->getInt()+1);
            return;

        case traverse_data::BuildDD: {
            DCASSERT(x.answer);
            DCASSERT(x.ddlib);
            shared_object* ldd = 0;
            shared_object* rdd = 0;
            left->Traverse(x);
            if (x.answer->isNormal()) ldd = Share(x.answer->getPtr());
            right->Traverse(x);
            if (x.answer->isNormal()) rdd = Share(x.answer->getPtr());
            shared_object* dd = x.ddlib->makeEdge(0);
            DCASSERT(dd);
            try {
                x.ddlib->buildBinary(ldd, opcode, rdd, dd);
                x.answer->setPtr(dd);
            }
            catch (sv_encoder::error e) {
                // an error occurred
                Delete(dd);
                x.answer->setNull();
            }
            Delete(ldd);
            Delete(rdd);
            return;
        }

        case traverse_data::BuildExpoRateDD:
            DCASSERT(x.answer);
            x.answer->setNull();
            return;

        default:
            left->Traverse(x);
            right->Traverse(x);
    }
}

bool binary::Print(std::ostream &s, int) const
{
    s << '(';
    left->Print(s, 0);
    s << binary_op::getOp(opcode);
    right->Print(s, 0);
    s << ')';
    return true;
}


// ******************************************************************
// *                                                                *
// *                          modulo class                          *
// *                                                                *
// ******************************************************************

modulo::modulo(const location &W, const type* t, expr* l, expr* r)
 : binary(W, binary_op::bop_mod, t, l, r)
{
}

// ******************************************************************
// *                                                                *
// *                           eqop class                           *
// *                                                                *
// ******************************************************************

eqop::eqop(const location &W, const type* t, expr* l, expr* r)
 : binary(W, binary_op::bop_equals, t, l, r)
{
}

// ******************************************************************
// *                                                                *
// *                          neqop  class                          *
// *                                                                *
// ******************************************************************

neqop::neqop(const location &W, const type* t, expr* l, expr* r)
 : binary(W, binary_op::bop_nequal, t, l, r)
{
}

// ******************************************************************
// *                                                                *
// *                           gtop class                           *
// *                                                                *
// ******************************************************************

gtop::gtop(const location &W, const type* t, expr* l, expr* r)
 : binary(W, binary_op::bop_gt, t, l, r)
{
}

// ******************************************************************
// *                                                                *
// *                           geop class                           *
// *                                                                *
// ******************************************************************

geop::geop(const location &W, const type* t, expr* l, expr* r)
 : binary(W, binary_op::bop_ge, t, l, r)
{
}

// ******************************************************************
// *                                                                *
// *                           ltop class                           *
// *                                                                *
// ******************************************************************

ltop::ltop(const location &W, const type* t, expr* l, expr* r)
 : binary(W, binary_op::bop_lt, t, l, r)
{
}

// ******************************************************************
// *                                                                *
// *                           leop class                           *
// *                                                                *
// ******************************************************************

leop::leop(const location &W, const type* t, expr* l, expr* r)
 : binary(W, binary_op::bop_le, t, l, r)
{
}

