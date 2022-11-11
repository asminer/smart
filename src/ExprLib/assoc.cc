
#include "assoc.h"
#include "../Streams/streams.h"
#include "result.h"
#include "dd_front.h"

#include <string.h>  // for memcpy

// ******************************************************************
// *                                                                *
// *                        assoc_op methods                        *
// *                                                                *
// ******************************************************************

assoc_op::assoc_op(exprman::assoc_opcode o)
{
  opcode = o;
}

assoc_op::~assoc_op()
{
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

bool flipassoc::Print(OutputStream &s, int) const
{
  s.Put('(');
  if (flip && flip[0]) s << em->getOp(true, opcode);
  operands[0]->Print(s, 0);
  for (int i=1; i<opnd_count; i++) {
    s << em->getOp(flip && flip[i], opcode);
    operands[i]->Print(s, 0);
  }
  s.Put(')');
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
  if (em->startInternal(__FILE__, __LINE__)) {
    em->internal() << "Loss of flip information in associative operator";
    em->stopIO();
  }
  return 0;
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



