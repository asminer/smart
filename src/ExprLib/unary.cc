
#include "unary.h"
#include "exprman.h"
#include "result.h"
#include "dd_front.h"

// ******************************************************************
// *                                                                *
// *                        unary_op methods                        *
// *                                                                *
// ******************************************************************

unary_op::unary_op(exprman::unary_opcode o)
{
  opcode = o;
}

unary_op::~unary_op()
{
}

// ******************************************************************
// *                                                                *
// *                         unary  methods                         *
// *                                                                *
// ******************************************************************

unary::unary(const char* fn, int line, const type* t, expr *x)
 : expr(fn, line, t)
{
  DCASSERT(em);
  DCASSERT(em->isOrdinary(x));
  opnd = x;
}

unary::~unary()
{
  Delete(opnd);
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
::negop(const char* F, int L, exprman::unary_opcode oc, const type* t, expr* x)
 : unary(F, L, t, x)
{
  opcode = oc;
}

bool negop::Print(OutputStream &s, int) const 
{
  s << em->getOp(opcode);
  DCASSERT(opnd);
  opnd->Print(s, 0);
  return true;
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
        x.ddlib->buildUnary(opcode, x.answer->getPtr(), dd);
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

