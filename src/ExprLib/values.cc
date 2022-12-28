
#include "values.h"
// #include "exprman.h"
#include "intervals.h"
#include "dd_front.h"

// #define SHOW_IMPLICIT_CAST

// ******************************************************************
// *                                                                *
// *                         value  methods                         *
// *                                                                *
// ******************************************************************

value::value(const location &W, const type* t, const result &x)
 : expr (W, t)
{
  val = x;
}

value::~value()
{
}

bool value::Print(std::ostream &s, int width) const
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

int value::Compare(const shared_object* o) const
{
  if (o==this) return 0;
  const value* foo = dynamic_cast <const value*> (o);
  if (0==foo) return 1;
  if (foo->Type() != Type()) return 1;  // Error out here?
  DCASSERT(Type());
  return Type()->compare(val, foo->val);
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
        if (type::matches(Type(), "real") || type::matches(Type(), "int")) {
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
        if (type::matches(Type(), "real")) {
          if (0.0 == val.getReal())     x.answer->setInfinity(1);
          else                          x.answer->setNull();
          return;
        }
        if (type::matches(Type(), "int")) {
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

        try {
          const type* bt = Type();
          if (bt) bt = bt->getBaseType();
          if (type::matches(bt, "real")) {
            x.ddlib->buildSymbolicConst(val.getReal(), dd);
          } else if (type::matches(bt, "int")) {
            x.ddlib->buildSymbolicConst(val.getInt(), dd);
          } else if (type::matches(bt, "bool")) {
            x.ddlib->buildSymbolicConst(val.getBool(), dd);
          } else {
            internal_error E(__FILE__, __LINE__, Where());
            E << "Unhandled type";
          }
          x.answer->setPtr(dd);
        } // try
        catch (sv_encoder::error e) {
          expr_error E(this, x.answer);
          E << "Error while building constant: ";
          E << sv_encoder::getNameOfError(e);
          Delete(dd);
        } // catch
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

