
#include "trinary.h"
#include "bogus.h"

// ******************************************************************
// *                                                                *
// *                       trinary_op methods                       *
// *                                                                *
// ******************************************************************

trinary_op::trinary_op(opcode o)
{
    code = o;
    registerOp(this);
}

trinary_op::~trinary_op()
{
}

const char* trinary_op::getFirst(opcode op)
{
    switch (op) {
        case top_none:      return "no-op";
        case top_interval:  return "..";
        case top_ite:       return "?";
        default:            return "unknown";
    }
    return "error";
}

const char* trinary_op::getSecond(opcode op)
{
    switch (op) {
        case top_none:      return "no-op";
        case top_interval:  return "..";
        case top_ite:       return ":";
        default:            return "unknown";
    }
    return "error";
}

const char* trinary_op::documentOp(opcode op)
{
    switch (op) {
        case top_interval:  return "intervals (within {})";
        case top_ite:       return "if-then-else";
        default:            return nullptr;
    }
    return nullptr;
}

const type* trinary_op::getTypeOf(opcode op, const type* left,
        const type* middle, const type* right)
{
    const trinary_op* match = bestMatch(op, left, middle, right);
    return match ? match->getExprType(left, middle, right) : nullptr;
}

expr* trinary_op::makeExpr(const location& W, opcode op, expr* l,
        expr* m, expr* r)
{
    if (bogus_expr::orNull(l)) {
        Delete(m);
        Delete(r);
        return l;
    }
    if (bogus_expr::orNull(m)) {
        Delete(l);
        Delete(r);
        return m;
    }
    if (bogus_expr::orNull(r)) {
        Delete(l);
        Delete(m);
        return r;
    }

    const trinary_op* match = bestMatch(op, l->Type(), m->Type(), r->Type());
    if (match) {
        return match->makeExpr(W, l, m, r);
    }

    typechecking_error E(W);
    E << "Undefined trinary operation: ";
    l->PrintType(E.stream());
    E << " " << getFirst(op) << " ";
    m->PrintType(E.stream());
    E << " " << getSecond(op) << " ";
    r->PrintType(E.stream());
    Delete(l);
    Delete(m);
    Delete(r);
    return bogus_expr::makeError();
}

const trinary_op* trinary_op::bestMatch(opcode code, const type* left,
            const type* middle, const type* right)
{
    const trinary_op* match = 0;
    int best_match = -1;
    unsigned num_matches = 0;
    for (const trinary_op* ptr = (code<top_none) ? registry[code] : nullptr;
            ptr; ptr=ptr->next)
    {
        int d = ptr->getPromoteDistance(left, middle, right);
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
        E << "Cannot decide on trinary operation: ";
        if (left)   E << *left;
        else        E << "notype";
        E << " " << getFirst(code) << " ";
        if (middle) E << *middle;
        else        E << "notype";
        E << " " << getSecond(code) << " ";
        if (right)  E << *right;
        else        E << "notype";
        return nullptr;
    }
    return match;
}

void trinary_op::registerOp(trinary_op* op)
{
    if (!registry) {
        registry = new const trinary_op* [top_none];
        for (unsigned i=0; i<top_none; i++) {
            registry[i] = nullptr;
        }
    }
    if (!op)    return;
    if (op->getOpcode() >= top_none) return;
    op->next = registry[op->getOpcode()];
    registry[op->getOpcode()] = op;
}


// ******************************************************************
// *                                                                *
// *                        trinary  methods                        *
// *                                                                *
// ******************************************************************

trinary::trinary(const location &W, const type* t, expr* l, expr* m, expr* r)
 : expr(W, t)
{
    left = l;
    middle = m;
    right = r;
    DCASSERT(!bogus_expr::orNull(left));
    DCASSERT(!bogus_expr::orNull(middle));
    DCASSERT(!bogus_expr::orNull(right));
}

trinary::~trinary()
{
    Delete(left);
    Delete(middle);
    Delete(right);
}

void trinary::Traverse(traverse_data &x)
{
    switch (x.which) {
        case traverse_data::Substitute: {
            expr* newl = 0;
            if (left) {
                left->Traverse(x);
                newl = smart_cast <expr*> (Share(x.answer->getPtr()));
            }
            expr* newm = 0;
            if (middle) {
                middle->Traverse(x);
                newm = smart_cast <expr*> (Share(x.answer->getPtr()));
            }
            expr* newr = 0;
            if (right) {
                right->Traverse(x);
                newr = smart_cast <expr*> (Share(x.answer->getPtr()));
            }
            x.answer->setPtr(MakeAnother(newl, newm, newr));
            return;
        }

        case traverse_data::GetProducts:
            if (x.elist)  x.elist->Append(this);
            DCASSERT(x.answer);
            x.answer->setInt(x.answer->getInt()+1);
            return;

        default:
            if (left)   left->Traverse(x);
            if (middle) middle->Traverse(x);
            if (right)  right->Traverse(x);
    }
}

