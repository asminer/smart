
#include "ops_misc.h"
#include "../Streams/streams.h"
#include "../Options/options.h"
#include "exprman.h"
#include "assoc.h"

/**

   Implementation of operator classes, for other types

*/

// ******************************************************************
// *                                                                *
// *                       sequence_op  class                       *
// *                                                                *
// ******************************************************************

class sequence_op : public assoc_op {
  const type* which;
public:
  sequence_op(const type* wh);
  virtual int getPromoteDistance(expr** list, bool* flip, int N) const;
  virtual int getPromoteDistance(bool f, const type* lt, const type* rt) const;
  virtual const type* getExprType(bool f, const type* l, const type* r) const;
};

// ******************************************************************
// *                      sequence_op  methods                      *
// ******************************************************************

sequence_op::sequence_op(const type* wh) : assoc_op(exprman::aop_semi)
{
  which = wh;
  DCASSERT(which);
}

int sequence_op::getPromoteDistance(expr** list, bool* flip, int N) const
{
  for (int i=0; i<N; i++) {
    if (0==list[i])               return -1;
    if (list[i]->Type() != which) return -1;
  }
  return 0;
}

int sequence_op
::getPromoteDistance(bool f, const type* lt, const type* rt) const
{
  if (f)            return -1;
  if (lt != which)  return -1;
  if (rt != which)  return -1;
  return 0;
}

const type* sequence_op
::getExprType(bool f, const type* l, const type* r) const
{
  if (f)          return 0;
  if (l != which) return 0;
  if (r != which) return 0;
  return which;
}


// ******************************************************************
// *                                                                *
// *                         void_seq class                         *
// *                                                                *
// ******************************************************************

/** Sequence of void expressions, separated by semicolon.
    In essence, a block of statements.
 */
class void_seq : public assoc {
public:
  void_seq(const location &W, expr** x, int n);
  virtual bool Print(OutputStream &s, int) const;
  virtual void Compute(traverse_data &x);
protected:
  virtual expr* buildAnother(expr **x, int n) const;
};

// ******************************************************************
// *                        void_seq methods                        *
// ******************************************************************

void_seq::void_seq(const location &W, expr** x, int n)
 : assoc(W, exprman::aop_semi, em->VOID, x, n)
{
}

bool void_seq::Print(OutputStream &s, int d) const
{
  for (int i=0; i<opnd_count; i++) {
    operands[i]->Print(s, d);
  }
  return true;
}

void void_seq::Compute(traverse_data &x)
{
  DCASSERT(0==x.aggregate);
  DCASSERT(x.answer);
  for (int i=0; i<opnd_count; i++) {
    if (x.stopExecution())  return; // an error occurred.
    DCASSERT(operands[i]);
    if (expr_debug.startReport()) {
      expr_debug.report() << "executing: ";
      operands[i]->Print(expr_debug.report(), 0);
      expr_debug.report() << "\n";
      expr_debug.stopIO();
    }
    operands[i]->Compute(x);
  } // for i
}

expr* void_seq::buildAnother(expr **x, int n) const
{
  return new void_seq(Where(), x, n);
}

// ******************************************************************
// *                                                                *
// *                       void_seq_op  class                       *
// *                                                                *
// ******************************************************************

class void_seq_op : public sequence_op {
public:
  void_seq_op(const type* vtype);
  virtual assoc* makeExpr(const location &W, expr** list,
        bool* flip, int N) const;
};

// ******************************************************************
// *                      void_seq_op  methods                      *
// ******************************************************************

void_seq_op::void_seq_op(const type* vtype) : sequence_op(vtype)
{
}

assoc* void_seq_op::makeExpr(const location &W, expr** list,
        bool* flip, int N) const
{
  delete[] flip;
  if (isDefinedForTypes(list, 0, N)) {
    return new void_seq(W, list, N);
  }
  for (int i=0; i<N; i++)  Delete(list[i]);
  delete[] list;
  return 0;
}

// ******************************************************************
// *                                                                *
// *                      next_state_seq class                      *
// *                                                                *
// ******************************************************************

/// Sequence of state change expressions, separated by semicolon.
class next_state_seq : public assoc {
public:
  next_state_seq(const location &W, expr** x, int n);
  virtual bool Print(OutputStream &s, int) const;
  virtual void Compute(traverse_data &x);
  virtual void Traverse(traverse_data &x);
protected:
  virtual expr* buildAnother(expr **x, int n) const;
};

// ******************************************************************
// *                     next_state_seq methods                     *
// ******************************************************************

next_state_seq::next_state_seq(const location &W, expr** x, int n)
 : assoc(W, exprman::aop_semi, em->NEXT_STATE, x, n)
{
}

bool next_state_seq::Print(OutputStream &s, int d) const
{
  for (int i=0; i<opnd_count; i++) {
    if (i) s << "; ";
    operands[i]->Print(s, 0);
  }
  return true;
}

void next_state_seq::Compute(traverse_data &x)
{
  DCASSERT(0==x.aggregate);
  DCASSERT(x.answer);
  DCASSERT(x.current_state);
  DCASSERT(x.next_state);
  for (int i=0; i<opnd_count; i++) {
    DCASSERT(operands[i]);
    operands[i]->Compute(x);
    if (!x.answer->isNormal())  return; // an error occurred.
  }
}

void next_state_seq::Traverse(traverse_data &x)
{
  if (x.which != traverse_data::GetProducts) {
    assoc::Traverse(x);
    return;
  }
  if (x.elist) {
    for (int i=0; i<opnd_count; i++) {
      x.elist->Append(operands[i]);
    }
  }
  x.answer->setInt(x.answer->getInt()+opnd_count);
}

expr* next_state_seq::buildAnother(expr **x, int n) const
{
  return new next_state_seq(this->Where(), x, n);
}

// ******************************************************************
// *                                                                *
// *                    next_state_seq_op  class                    *
// *                                                                *
// ******************************************************************

class next_state_seq_op : public sequence_op {
public:
  next_state_seq_op(const type* the_type);
  virtual assoc* makeExpr(const location &W, expr** list,
        bool* flip, int N) const;
};

// ******************************************************************
// *                   next_state_seq_op  methods                   *
// ******************************************************************

next_state_seq_op::next_state_seq_op(const type* the_type)
 : sequence_op(the_type)
{
}

assoc* next_state_seq_op::makeExpr(const location &W, expr** list,
        bool* flip, int N) const
{
  delete[] flip;
  if (isDefinedForTypes(list, 0, N)) {
    return new next_state_seq(W, list, N);
  }
  for (int i=0; i<N; i++)  Delete(list[i]);
  delete[] list;
  return 0;
}

// ******************************************************************
// *                                                                *
// *                        aggregates class                        *
// *                                                                *
// ******************************************************************

/**   The class used to aggregate expressions.

      We are derived from assoc, but most of the
      provided functions of assoc must be overloaded
      (because of the special nature of aggregates).
*/

class aggregates : public assoc {
public:
  /// Constructor.
  aggregates(const location &W, expr **x, int nc);
  virtual expr* GetComponent(int i);
  virtual void Compute(traverse_data &x);
  virtual void Traverse(traverse_data &x);
  virtual bool Print(OutputStream &s, int) const;
protected:
  virtual assoc* buildAnother(expr **, int) const;
};


// ******************************************************************
// *                       aggregates methods                       *
// ******************************************************************

aggregates::aggregates(const location &W, expr **x, int nc)
 : assoc (W, exprman::aop_colon, (typelist*) 0, x, nc)
{
  DCASSERT(nc>0);
  // determine the type
  typelist* tl = new typelist(nc);
  for (int i=0; i<nc; i++) {
    tl->SetItem(i, em->SafeType(x[i]));
  }
  SetType(tl);
}

expr* aggregates::GetComponent(int i)
{
  CHECK_RANGE(0, i, opnd_count);
  return operands[i];
}

void aggregates::Compute(traverse_data &x)
{
  DCASSERT(x.answer);
  int i = x.aggregate;
  CHECK_RANGE(0, i, opnd_count);
  x.aggregate = 0;
  SafeCompute(operands[i], x);
  x.aggregate = i;  // just in case the caller needs it
}

void aggregates::Traverse(traverse_data &x)
{
  int i = x.aggregate;
  CHECK_RANGE(0, i, opnd_count);
  x.aggregate = 0;
  if (operands[i]) operands[i]->Traverse(x);
  x.aggregate = i;  // just in case the caller needs it
}

bool aggregates::Print(OutputStream &s, int) const
{
  operands[0]->Print(s, 0);
  for (int i=1; i<opnd_count; i++) {
    s.Put(':');
    if (operands[i])  operands[i]->Print(s, 0);
    else              s.Put("null");
  }
  return true;
}

assoc* aggregates::buildAnother(expr **, int) const
{
  if (em->startInternal(__FILE__, __LINE__)) {
    em->causedBy(this);
    em->internal() << "call to aggregates::buildAnother";
    em->stopIO();
  }
  return 0;
}

// ******************************************************************
// *                                                                *
// *                        aggreg_op  class                        *
// *                                                                *
// ******************************************************************

class aggreg_op : public assoc_op {
public:
  aggreg_op();
  virtual int getPromoteDistance(expr** list, bool* flip, int N) const;
  virtual int getPromoteDistance(bool f, const type* lt, const type* rt) const;
  virtual const type* getExprType(bool f, const type* l, const type* r) const;
  virtual assoc* makeExpr(const location &W, expr** list,
        bool* flip, int N) const;
};

// ******************************************************************
// *                       aggreg_op  methods                       *
// ******************************************************************

aggreg_op::aggreg_op() : assoc_op(exprman::aop_colon)
{
}

int aggreg_op::getPromoteDistance(expr** list, bool* flip, int N) const
{
  return 0;  // ANY aggregation is allowed
}

int aggreg_op::getPromoteDistance(bool f, const type* lt, const type* rt) const
{
  if (f)  return -1;
  return 0;
}

const type* aggreg_op::getExprType(bool f, const type* l, const type* r) const
{
  // caller should know better!
  return 0;
}

assoc* aggreg_op::makeExpr(const location &W, expr** list,
        bool* flip, int N) const
{
  delete[] flip;
  return new aggregates(W, list, N);
}


// ******************************************************************
// *                                                                *
// *                           Front  end                           *
// *                                                                *
// ******************************************************************

void InitMiscOps(exprman* em)
{
  if (0==em)  return;
  em->registerOperation( new void_seq_op(em->VOID)              );
  em->registerOperation( new next_state_seq_op(em->NEXT_STATE)  );
  em->registerOperation( new aggreg_op                          );
}


