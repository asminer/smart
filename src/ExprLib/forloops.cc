
#include "forloops.h"
#include "iterators.h"
#include "../Options/options.h"
#include "result.h"
#include "bogus.h"
#include "casting.h"
#include <stdlib.h>

// ******************************************************************
// *                                                                *
// *                         forstmt  class                         *
// *                                                                *
// ******************************************************************

/**  A statement used for for-loops.
     Multiple dimensions can be handled in a single statement.
 */

class forstmt : public expr {
  iterator** index;
  int dimension;
  expr* block;
public:
  forstmt(const location &W, iterator** i, int d, expr* b);
  virtual ~forstmt();

  virtual bool Print(std::ostream &s, int) const;
  virtual void Compute(traverse_data &x);
  virtual void Traverse(traverse_data &x);

protected:
  void Compute(int d, traverse_data &x);
  void Traverse(int d, traverse_data &x);

  void ShowAssignments(std::ostream &s) const;
  inline void DebugIteration(const char* msg) const {
    if (!expr_debug.start())  return;
    expr_debug << msg;
    for (int i=0; i<dimension; i++) {
      if (i) expr_debug << ", ";
      index[i]->ShowAssignment(expr_debug.stream());
    }
    expr_debug.stop();
  }
};


// ******************************************************************
// *                        forstmt  methods                        *
// ******************************************************************

forstmt::forstmt(const location &W, iterator** i, int d, expr* b)
  : expr(W, STMT)
{
  index = i;
  dimension = d;
  block = b;
  if (block) {
    traverse_data x(traverse_data::Block);
    block->Traverse(x);
  }
}

forstmt::~forstmt()
{
#ifdef DEBUG_MEM
  printf("destroying forstmt: 0x%x\n", unsigned(this));
  printf("\tblock: 0x%x\n", unsigned(block));
  printf("\tindex: 0x%x\n", unsigned(index));
#endif
  Delete(block);
  for (int j=0; j<dimension; j++)   Delete(index[j]);
  free(index);
}

bool forstmt::Print(std::ostream &s, int w) const
{
    s << padding(w) << "for (";
    index[0]->PrintAll(s);
    for (int d=1; d<dimension; d++) {
        s << ", ";
        index[d]->PrintAll(s);
    }
    s << ") {\n";
    block->Print(s, w+2);
    s << padding(w) << "}\n";
    return true;
}

void forstmt::Compute(traverse_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  if (x.stopExecution()) return;
  Compute(0, x);
}

void forstmt::Traverse(traverse_data &x)
{
  switch (x.which) {
    case traverse_data::PreCompute:
    case traverse_data::Guess:
    case traverse_data::Update:
    case traverse_data::Affix:
        Traverse(0, x);
        return;

  // symbols? substitute?

    default:
        block->Traverse(x);
  };
}

void forstmt::Compute(int d, traverse_data &x)
{
  if (d>=dimension) {
    // execute block
    DebugIteration("for loop: executing with ");
    block->Compute(x);
  } else {
    // Loop this dimension
    index[d]->ComputeCurrent(x);
    if (!index[d]->FirstIndex()) return;  // empty loop
    do {
      Compute(d+1, x);
    } while (index[d]->NextValue());
    index[d]->DoneCurrent();
  }
}

void forstmt::Traverse(int d, traverse_data &x)
{
  if (d>=dimension) {
    // execute block
    block->Traverse(x);
  } else {
    // Loop this dimension
    index[d]->ComputeCurrent(x);
    if (!index[d]->FirstIndex()) return;  // empty loop
    do {
      Traverse(d+1, x);
    } while (index[d]->NextValue());
    index[d]->DoneCurrent();
  }
}

void forstmt::ShowAssignments(std::ostream &s) const
{

}

// ******************************************************************
// *                                                                *
// *                        exprman  methods                        *
// *                                                                *
// ******************************************************************


expr* expr::makeForLoop(const location &W, symbol** iters, int dim, expr* stmt)
{
    if (bogus_expr::orNull(stmt)) {
        return Share(stmt);
    }
    DCASSERT(type::matches(stmt->Type(), "void"));
#ifdef DEVELOPMENT_CODE
    for (int i=0; i<dim; i++) {
        iterator* foo = dynamic_cast <iterator*> (iters[i]);
        DCASSERT(foo);
    }
#endif
    expr* x = new forstmt(W, (iterator**) iters, dim, stmt);
    if (x->OK())  return x;
    if (x->hadError()) {
        Delete(x);
        return bogus_expr::makeError();
    }
    Delete(x);
    return nullptr;
}

