
#include "trinary.h"
#include "exprman.h"

// ******************************************************************
// *                                                                *
// *                       trinary_op methods                       *
// *                                                                *
// ******************************************************************

trinary_op::trinary_op(exprman::trinary_opcode o)
{
  opcode = o;
}

trinary_op::~trinary_op()
{
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
  DCASSERT(em->isOrdinary(left));
  DCASSERT(em->isOrdinary(middle));
  DCASSERT(em->isOrdinary(right));
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

