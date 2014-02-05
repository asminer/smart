
// $Id$

#include "values.h"
#include "../Streams/streams.h"
#include "exprman.h"
#include "intervals.h"
#include "dd_front.h"

// #define SHOW_IMPLICIT_CAST

// ******************************************************************
// *                                                                *
// *                         value  methods                         *
// *                                                                *
// ******************************************************************

value::value(const char* fn, int line, const type* t, const result &x)
 : expr (fn, line, t)
{
  val = x;
}

value::~value()
{
}

bool value::Print(OutputStream &s, int width) const
{
  const type* t = Type();
  DCASSERT(t);
  const simple_type* bt = t->getBaseType();
#ifdef SHOW_IMPLICIT_CAST
  if (bt != t) {
    s << t->getName() << "(";
    bt->show(s, val);
    s << ")";
  } else 
#endif
    bt->show(s, val);

  return true;
}

bool value::Equals(const shared_object* o) const
{
  if (o==this) return true;
  const value* foo = dynamic_cast <const value*> (o);
  if (0==foo) return false;
  if (foo->Type() != Type()) return false;
  DCASSERT(Type());
  return Type()->equals(val, foo->val);
}

void value::Compute(traverse_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  x.answer[0] = val;
}

void value::Traverse(traverse_data &x)
{
  switch (x.which) {
    case traverse_data::Substitute:
        DCASSERT(x.answer);
        x.answer->setPtr(Share(this));
        return;

    case traverse_data::PreCompute:
    case traverse_data::GetSymbols:
    case traverse_data::GetVarDeps:
    case traverse_data::GetMeasures:
        return;

    case traverse_data::FindRange:
        DCASSERT(x.answer);
        if (em->REAL == Type() || em->INT == Type()) {
          interval_object* foo = new interval_object;
          foo->Left().setFrom(val, Type());
          foo->Right().setFrom(val, Type());
          x.answer->setPtr(foo);
          return;
        }
        x.answer->setNull();
        return;

    case traverse_data::ComputeExpoRate:
        DCASSERT(x.answer);
        if (em->REAL == Type()) {
          if (0.0 == val.getReal())     x.answer->setInfinity(1);
          else                          x.answer->setNull();
          return;
        }
        if (em->INT == Type()) {
          if (0 == val.getInt())        x.answer->setInfinity(1);
          else                          x.answer->setNull();
          return;
        }
        x.answer->setNull();
        return;

    case traverse_data::BuildDD: {
        DCASSERT(x.answer);
        DCASSERT(x.ddlib);
        shared_object* dd = x.ddlib->makeEdge(0);

        sv_encoder::error e = sv_encoder::Success;
        const type* bt = Type();
        if (bt) bt = bt->getBaseType();
        if (em->REAL == bt) {
          e = x.ddlib->buildSymbolicConst(val.getReal(), dd);
        } else if (em->INT == bt) {
          e = x.ddlib->buildSymbolicConst(val.getInt(), dd);
        } else if (em->BOOL == bt) {
          e = x.ddlib->buildSymbolicConst(val.getBool(), dd);
        } else {
          if (em->startInternal(__FILE__, __LINE__)) {
            em->causedBy(this);
            em->internal() << "Unhandled type\n";
            em->stopIO();
          }
        }
        if (e) {
          if (em->startError()) {
            em->causedBy(this);
            em->cerr() << "Error while building constant: ";
            em->cerr() << sv_encoder::getNameOfError(e);
            em->stopIO();
          }
          Delete(dd);
          x.answer->setNull();
        } else {
          x.answer->setPtr(dd);
        }
        return;
    }
  
    case traverse_data::BuildExpoRateDD:
      DCASSERT(x.answer);
      x.answer->setNull();
      return;


    default:
      expr::Traverse(x);
  }
}

