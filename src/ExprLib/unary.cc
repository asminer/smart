
#include "unary.h"
#include "bogus.h"
#include "result.h"
#include "dd_front.h"

// ******************************************************************
// *                                                                *
// *                        unary_op methods                        *
// *                                                                *
// ******************************************************************

const unary_op** unary_op::registry = nullptr;

unary_op::unary_op(opcode oc)
{
    code = oc;
    registerOp(this);
}

unary_op::~unary_op()
{
}

const char* unary_op::getOp(opcode code)
{
    switch (code) {
        case uop_none:      return "no-op";
        case uop_not:       return "!";
        case uop_neg:       return "-";
        case uop_forall:    return "A";
        case uop_exists:    return "E";
        case uop_future:    return "F";
        case uop_globally:  return "G";
        case uop_next:      return "X";
        default:            return "unknown_op";
    }
    return "error";  // will never get here, keep compilers happy
}

const char* unary_op::documentOp(opcode code)
{
    switch (code) {
        case uop_not: return "logical negation";
        case uop_neg: return "numerical negation";
        default:      return nullptr;
    }
    return nullptr;  // will never get here, keep compilers happy
}

const type* unary_op::getTypeOf(opcode code, const type* x)
{
    const unary_op* match = bestMatch(code, x);
    return match ? match->getExprType(x) : nullptr;
}

expr* unary_op::makeExpr(const location &W, opcode code, expr* opnd)
{
    //
    // Deal with special operands
    //
    if (bogus_expr::orNull(opnd)) {
        return opnd;
    }

    const unary_op* match = bestMatch(code, opnd->Type());
    if (match) {
        return match->makeExpr(W, opnd);
    }
    typechecking_error E(W);
    E << "Undefined unary operation: " << getOp(code) << " ";
    opnd->PrintType(E.stream());
    Delete(opnd);
    return bogus_expr::getError();
}

const unary_op* unary_op::bestMatch(opcode code, const type* x)
{
    const unary_op* match = nullptr;
    unsigned num_matches = 0;
    for (const unary_op* ptr = (code<uop_none) ? registry[code] : nullptr;
            ptr; ptr=ptr->next)
    {
        if (! ptr->isDefinedForType(x)) continue;
        match = ptr;
        num_matches++;
    } // for all operations with this code

    if (num_matches > 1) {
        // too many matches, this should not happen!
        internal_error E(__FILE__, __LINE__);
        E << "Cannot decide on unary operation: " << getOp(code) << " ";
        if (x)  E << *x;
        else    E << "notype";
        return nullptr;
    }
    return match;
}

void unary_op::registerOp(unary_op* op)
{
    if (!registry) {
        registry = new const unary_op* [uop_none];
        for (unsigned i=0; i<uop_none; i++) {
            registry[i] = nullptr;
        }
    }
    if (!op)    return;
    if (op->getOpcode() >= uop_none) return;
    op->next = registry[op->getOpcode()];
    registry[op->getOpcode()] = op;
}

// ******************************************************************
// *                                                                *
// *                         unary  methods                         *
// *                                                                *
// ******************************************************************

unary::unary(const location& W, unary_op::opcode oc, const type* t, expr *x)
 : expr(W, t)
{
    code = oc;
    opnd = x;
    DCASSERT(!bogus_expr::orNull(opnd));
}

unary::~unary()
{
    Delete(opnd);
}

bool unary::Print(std::ostream &s, int) const
{
    s << unary_op::getOp(code);
    DCASSERT(opnd);
    opnd->Print(s);
    return true;
}

void unary::Traverse(traverse_data &x)
{
  DCASSERT(opnd);
  DCASSERT(x.answer);
  switch (x.which) {
    case traverse_data::Substitute: {
      opnd->Traverse(x);
      expr* newopnd = smart_cast <expr*> (Share(x.answer->getPtr()));
      x.answer->setPtr(MakeAnother(newopnd));
      return;
    }

    case traverse_data::GetProducts:
      if (x.elist)  x.elist->Append(this);
      DCASSERT(x.answer);
      x.answer->setInt(x.answer->getInt()+1);
      return;

    case traverse_data::BuildExpoRateDD:
      DCASSERT(x.answer);
      x.answer->setNull();
      return;

    default:
      opnd->Traverse(x);
  }
}

// ******************************************************************
// *                                                                *
// *                          negop  class                          *
// *                                                                *
// ******************************************************************

negop
::negop(const location& W, unary_op::opcode oc, const type* t, expr* x)
 : unary(W, oc, t, x)
{
}

void negop::Traverse(traverse_data &x)
{
  DCASSERT(opnd);
  DCASSERT(x.answer);
  switch (x.which) {
    case traverse_data::BuildDD: {
      DCASSERT(x.ddlib);
      opnd->Traverse(x);
      if (!x.answer->isNormal()) return;
      shared_object* dd = x.ddlib->makeEdge(0);
      DCASSERT(dd);
      try {
        x.ddlib->buildUnary(GetOpCode(), x.answer->getPtr(), dd);
        x.answer->setPtr(dd);
      }
      catch (sv_encoder::error e) {
        // there was an error
        // TBD: do we make noise?
        Delete(dd);
        x.answer->setNull();
      }
      return;
    }

    default:
      unary::Traverse(x);
  }
}

// ******************************************************************
// *                                                                *
// *                    unary_temporal_op  class                    *
// *                                                                *
// ******************************************************************

unary_temporal_expr
::unary_temporal_expr(const location &W, unary_op::opcode oc, const type* t, expr* x)
 : unary(W, oc, t, x)
{
}

