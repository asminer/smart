
#include "assoc.h"
#include "result.h"
#include "dd_front.h"

#include <string.h>  // for memcpy

// ******************************************************************
// *                                                                *
// *                        assoc_op methods                        *
// *                                                                *
// ******************************************************************

const assoc_op** assoc_op::registry = nullptr;

assoc_op::assoc_op(exprman::assoc_opcode o)
{
    code = o;
    registerOp(this);
}

assoc_op::~assoc_op()
{
}

const char* assoc_op::getOp(bool flip, opcode op)
{
    switch (op) {
        case aop_none:  return "no-op";
        case aop_and:   return flip ?  0  : "&";
        case aop_or:    return flip ?  0  : "|";
        case aop_plus:  return flip ? "-" : "+";
        case aop_times: return flip ? "/" : "*";
        case aop_colon: return flip ?  0  : ":";
        case aop_semi:  return flip ?  0  : ";";
        case aop_union: return flip ?  0  : ",";
        default:        return "unknown";
    }
    return "error";  // will never get here, keep compilers happy
}

const char* assoc_op::documentOp(bool flip, opcode op)
{
    switch (op) {
        case aop_and:   return flip ?  0  : "logical and";
        case aop_or:    return flip ?  0  : "logical or";
        case aop_plus:  return flip ? "difference" : "addition";
        case aop_times: return flip ? "division" : "multiplication";
        case aop_colon: return flip ?  0  : "aggregation";
        case aop_semi:  return flip ?  0  : "statement aggregation";
        case aop_union: return flip ?  0  : "set union (inside {})";
        default:        return 0;
    }
    return 0;  // will never get here, keep compilers happy
}

const type* assoc_op::getTypeOf(const type* lt, bool flip, opcode op,
        const type* rt)
{
    const assoc_op* match = nullptr;
    int best_match = -1;
    unsigned num_matches = 0;
    for (const assoc_op* ptr = (code<aop_none) ? registry[code] : nullptr;
            ptr; ptr=ptr->next)
    {
        int d = ptr->getPromoteDistance(flip, lt, rt);
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
        E << "Cannot decide on associative operation: ";
        if (lt) E << *lt;
        else    E << "notype";
        E << " " << getOp(flip, op) << " ";
        if (rt) E << *rt;
        else    E << "notype";
    }

    return match ? match->getExprType(flip, lt, rt) : nullptr;
}


expr* assoc_op::makeExpr(const location &W, opcode op, expr** opnds,
        bool* flip, int N)
{
    // TBD HERE
    bool has_null = false;
    bool has_error = false;
    for (int i=0; i<N; i++) {
        if (0==opnds[i]) {
            has_null = true;
            continue;
        }
        if (isError(opnds[i])) {
            has_error = true;
            continue;
        }
        DCASSERT(isOrdinary(opnds[i]));
    }
    //
    // If there's a null or an error,
    // the whole thing becomes error.
    // The exception is that null is allowed as an aggregate in :
    //
    if ( (op != aop_colon && has_null) || has_error ) {
        for (int i=0; i<N; i++) Delete(opnds[i]);
        delete[] opnds;
        delete[] flip;
        if (op != aop_colon && has_null)  return 0;
        return makeError();
    }

    const assoc_op* match = 0;
    int best_match = -1;
    unsigned num_matches = 0;
    for (const assoc_op* ptr = (op<aop_none) ? registry[op] : nullptr;
            ptr; ptr=ptr->next)
    {
        int d = ptr->getPromoteDistance(opnds, flip, N);
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

    //
    // There should be exactly one match;
    // if not, give some kind of error message
    //
    if (num_matches > 1) {
        // too many matches, this should not happen!
        internal_error E(__FILE__, __LINE__);
        E << "Cannot decide on associative operation: ";
        if (lt) E << *lt;
        else    E << "notype";
        E << " " << getOp(flip, op) << " ";
        if (rt) E << *rt;
        else    E << "notype";
        return nullptr; // irrelevant
    }
    if (!num_matches) {
        typechecking_error E(W);
        E << "Undefined associative operation: ";
        if (opnds[0])   opnds[0]->PrintType(E.stream());
        else            E << *type::null;
        for (int i=1; i<N; i++) {
            bool f = flip ? flip[i] : 0;
            E << " " << getOp(f, op) << " ";
            if (opnds[i]) opnds[i]->PrintType(E.stream());
            else          E << *type::null;
        }
        for (int i=0; i<N; i++)  Delete(opnds[i]);
        delete[] opnds;
        delete[] flip;
        return makeError();
    }

    DCASSERT(1==num_matches);
    DCASSERT(match);
    expr* answer = match->makeExpr(W, opnds, flip, N);
    if (!answer) {
        internal_error E(__FILE__, __LINE__, W);
        E << "Couldn't build associative expression for " << getOp(0, op);
    }
    return answer;
}


void assoc_op::registerOp(assoc_op* op)
{
    if (!registry) {
        registry = new const assoc_op* [aop_none];
        for (unsigned i=0; i<aop_none; i++) {
            registry[i] = nullptr;
        }
    }
    if (!op)    return;
    if (op->getOpcode() >= aop_none) return;
    op->next = registry[op->getOpcode()];
    registry[op->getOpcode()] = op;
}

// ******************************************************************
// *                                                                *
// *                         assoc  methods                         *
// *                                                                *
// ******************************************************************

assoc::assoc(const location &W, exprman::assoc_opcode oc,
  const type* t, expr **x, int n) : expr(W, t)
{
  opnd_count = n;
  operands = x;
  opcode = oc;
}

assoc::assoc(const location &W, exprman::assoc_opcode oc,
  typelist* t, expr **x, int n) : expr(W, t)
{
  opnd_count = n;
  operands = x;
  opcode = oc;
}

assoc::~assoc()
{
  int i;
  for (i=0; i<opnd_count; i++) Delete(operands[i]);
  delete[] operands;
}

void assoc::Traverse(traverse_data &x)
{
  switch (x.which) {
    case traverse_data::Substitute: {
      DCASSERT(x.answer);
      expr** newops = new expr* [opnd_count];
      for (int i=0; i<opnd_count; i++) {
        if (operands[i]) {
          operands[i]->Traverse(x);
          newops[i] = smart_cast <expr*> (Share(x.answer->getPtr()));
        } else {
          newops[i] = 0;
        }
      } // for i
      x.answer->setPtr(MakeAnother(newops, opnd_count));
      return;
    }

    case traverse_data::BuildDD: {
      DCASSERT(x.answer);
      DCASSERT(x.ddlib);
      // compute first opnd
      shared_object* dd = 0;
      if (operands[0])  operands[0]->Traverse(x);
      else              x.answer->setNull();
      if (x.answer->isNormal()) dd = Share(x.answer->getPtr());
      // compute the rest and accumulate as we go
      for (int i=1; i<opnd_count; i++) {
        if (0==dd) break;
        shared_object* tmp = 0;
        if (operands[i]) {
          operands[i]->Traverse(x);
          if (x.answer->isNormal()) tmp = Share(x.answer->getPtr());
        }
        if (tmp) {
          try {
            x.ddlib->buildAssoc(dd, false, opcode, tmp, dd);
          }
          catch (sv_encoder::error e) {
            // error
            Delete(dd);
            dd = 0;
          }
          Delete(tmp);
        } else {
          Delete(dd);
          dd = 0;
        }
      } // for i
      // done, package answer
      if (0==dd) x.answer->setNull();
      else       x.answer->setPtr(dd);
      return;
    }

    case traverse_data::BuildExpoRateDD:
      DCASSERT(x.answer);
      x.answer->setNull();
      return;

    case traverse_data::GetProducts:
      if (x.elist)  x.elist->Append(this);
      DCASSERT(x.answer);
      x.answer->setInt(x.answer->getInt()+1);
      return;

    default:
      for (int i=0; i<opnd_count; i++) {
        DCASSERT(operands[i]);
        operands[i]->Traverse(x);
      }
  }
}

expr* assoc::MakeAnother(expr **newx, int newn)
{
  if (0==newx) return 0;
  // check for null or identity
  bool is_null = false;
  bool is_same = (newn==opnd_count);
  for (int i=0; i<newn; i++) {
    if (0==newx[i]) is_null = true;
    if (newx[i]!=operands[i]) is_same = false;
  }
  if (is_null || is_same) {
    for (int i=0; i<newn; i++) Delete(newx[i]);
    delete[] newx;
  }
  if (is_null) return 0;
  if (is_same) return Share(this);
  return buildAnother(newx, newn);
}

// ******************************************************************
// *                                                                *
// *                       flipassoc  methods                       *
// *                                                                *
// ******************************************************************

flipassoc::flipassoc(const location &W, exprman::assoc_opcode oc,
 const type* t, expr** x, bool* f, int n) : assoc(W, oc, t, x, n)
{
  flip = checkFlip(f, n);
}

flipassoc::~flipassoc()
{
  delete[] flip;
}

void flipassoc::Traverse(traverse_data &x)
{
  switch (x.which) {
    case traverse_data::Substitute: {
      DCASSERT(x.answer);
      expr** newops = new expr* [opnd_count];
      for (int i=0; i<opnd_count; i++) {
        DCASSERT(operands[i]);
        operands[i]->Traverse(x);
        newops[i] = smart_cast <expr*> (Share(x.answer->getPtr()));
        DCASSERT(newops[i]);
      } // for i
      bool* newf;
      if (flip) {
        newf = new bool[opnd_count];
        memcpy(newf, flip, opnd_count*sizeof(bool));
       } else {
        newf = 0;
      }
      x.answer->setPtr(MakeAnother(newops, newf, opnd_count));
      return;
    }

    case traverse_data::BuildDD: {
      DCASSERT(x.answer);
      DCASSERT(x.ddlib);
      // compute first opnd
      shared_object* dd = 0;
      if (operands[0])  operands[0]->Traverse(x);
      else              x.answer->setNull();
      if (x.answer->isNormal()) dd = Share(x.answer->getPtr());
      // compute the rest and accumulate as we go
      for (int i=1; i<opnd_count; i++) {
        if (0==dd) break;
        shared_object* tmp = 0;
        if (operands[i]) {
          operands[i]->Traverse(x);
          if (x.answer->isNormal()) tmp = Share(x.answer->getPtr());
        }
        if (tmp) {
          bool f = flip ? flip[i] : false;
          try {
            x.ddlib->buildAssoc(dd, f, opcode, tmp, dd);
          }
          catch (sv_encoder::error e) {
            // error
            Delete(dd);
            dd = 0;
          }
          Delete(tmp);
        } else {
          Delete(dd);
          dd = 0;
        }
      } // for i
      // done, package answer
      if (0==dd) x.answer->setNull();
      else       x.answer->setPtr(dd);
      return;
    }

    case traverse_data::GetProducts:
      if (x.elist)  x.elist->Append(this);
      DCASSERT(x.answer);
      x.answer->setInt(x.answer->getInt()+1);
      return;

    default:
      for (int i=0; i<opnd_count; i++) {
        DCASSERT(operands[i]);
        operands[i]->Traverse(x);
      }
  }
}

bool flipassoc::Print(std::ostream &s, int) const
{
  s << '(';
  if (flip && flip[0]) s << em->getOp(true, opcode);
  operands[0]->Print(s, 0);
  for (int i=1; i<opnd_count; i++) {
    s << em->getOp(flip && flip[i], opcode);
    operands[i]->Print(s, 0);
  }
  s << ')';
  return true;
}

expr* flipassoc::MakeAnother(expr **newx, bool* newf, int newn)
{
  if (0==newx) {
    delete[] newf;
    return 0;
  }
  newf = checkFlip(newf, newn);
  // check for null or identity
  bool is_null = false;
  bool is_same = (newn==opnd_count);
  for (int i=0; i<newn; i++) {
    if (0==newx[i]) is_null = true;
    if (!is_same) continue;
    bool aflip = flip ? flip[i] : false;
    bool bflip = newf ? newf[i] : false;
    is_same = (newx[i] == operands[i]) && (aflip == bflip);
  }
  if (is_null || is_same) {
    for (int i=0; i<newn; i++) Delete(newx[i]);
    delete[] newx;
    delete[] newf;
  }
  if (is_null) return 0;
  if (is_same) return Share(this);
  return buildAnother(newx, newf, newn);
}

expr* flipassoc::buildAnother(expr** newx, int newn) const
{
    internal_error E(__FILE__, __LINE__);
    E << "Loss of flip information in associative operator";
    return nullptr;
}

// ******************************************************************
// *                                                                *
// *                        summation  class                        *
// *                                                                *
// ******************************************************************

summation::summation(const location &W, exprman::assoc_opcode oc,
  const type* t, expr** x, bool* f, int n)
 : flipassoc(W, oc, t, x, f, n)
{
}

// ******************************************************************
// *                                                                *
// *                         product  class                         *
// *                                                                *
// ******************************************************************

product::product(const location &W, exprman::assoc_opcode oc,
  const type* t, expr** x, bool* f, int n)
 : flipassoc(W, oc, t, x, f, n)
{
}

void product::Traverse(traverse_data &x)
{
  switch (x.which) {
    case traverse_data::GetProducts:
        if (flip) for (int i=0; i<opnd_count; i++) if (flip[i]) {
          flipassoc::Traverse(x);  // overkill, for now...
          return;
        }
        // nothing inverted, easy case
        DCASSERT(x.answer);
        if (x.elist) {
          for (int i=0; i<opnd_count; i++) {
            x.elist->Append(operands[i]);
          }
        }
        x.answer->setInt(x.answer->getInt()+opnd_count);
        return;

    default:
        flipassoc::Traverse(x);
  }
}



