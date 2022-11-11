
#include "binary.h"
#include "dd_front.h"

// ******************************************************************
// *                                                                *
// *                       binary_op  methods                       *
// *                                                                *
// ******************************************************************

binary_op::binary_op(exprman::binary_opcode o)
{
  opcode = o;
}

binary_op::~binary_op()
{
}

// ******************************************************************
// *                                                                *
// *                         binary methods                         *
// *                                                                *
// ******************************************************************

binary::binary(const location &W, exprman::binary_opcode oc,
 const type* t, expr *l, expr *r)
 : expr(W, t)
{
  left = l;
  right = r;
  opcode = oc;
  DCASSERT(em);
  DCASSERT(em->isOrdinary(left));
  DCASSERT(em->isOrdinary(right));
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

bool binary::Print(OutputStream &s, int) const
{
  s.Put('(');
  left->Print(s, 0);
  s.Put(em->getOp(opcode));
  right->Print(s, 0);
  s.Put(')');
  return true;
}


// ******************************************************************
// *                                                                *
// *                          modulo class                          *
// *                                                                *
// ******************************************************************

modulo::modulo(const location &W, const type* t, expr* l, expr* r)
 : binary(W, exprman::bop_mod, t, l, r)
{
}

// ******************************************************************
// *                                                                *
// *                           eqop class                           *
// *                                                                *
// ******************************************************************

eqop::eqop(const location &W, const type* t, expr* l, expr* r)
 : binary(W, exprman::bop_equals, t, l, r)
{
}

// ******************************************************************
// *                                                                *
// *                          neqop  class                          *
// *                                                                *
// ******************************************************************

neqop::neqop(const location &W, const type* t, expr* l, expr* r)
 : binary(W, exprman::bop_nequal, t, l, r)
{
}

// ******************************************************************
// *                                                                *
// *                           gtop class                           *
// *                                                                *
// ******************************************************************

gtop::gtop(const location &W, const type* t, expr* l, expr* r)
 : binary(W, exprman::bop_gt, t, l, r)
{
}

// ******************************************************************
// *                                                                *
// *                           geop class                           *
// *                                                                *
// ******************************************************************

geop::geop(const location &W, const type* t, expr* l, expr* r)
 : binary(W, exprman::bop_ge, t, l, r)
{
}

// ******************************************************************
// *                                                                *
// *                           ltop class                           *
// *                                                                *
// ******************************************************************

ltop::ltop(const location &W, const type* t, expr* l, expr* r)
 : binary(W, exprman::bop_lt, t, l, r)
{
}

// ******************************************************************
// *                                                                *
// *                           leop class                           *
// *                                                                *
// ******************************************************************

leop::leop(const location &W, const type* t, expr* l, expr* r)
 : binary(W, exprman::bop_le, t, l, r)
{
}

