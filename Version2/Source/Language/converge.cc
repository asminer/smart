
// $Id$

#include "converge.h"
//@Include: converge.h

/** @name converge.cc
    @type File
    @args \ 

   Implementation of all things converge related.

 */

//@{

option* use_current;
option* max_converge_iters;
option* converge_precision;
option* converge_precision_test;

option_const* relative;
option_const* absolute;

//#define DEBUG_CONVERGE

// ******************************************************************
// *                                                                *
// *                         cvgfunc  class                         *
// *                                                                *
// ******************************************************************

cvgfunc::cvgfunc(const char* fn, int line, type t, char* n)
 : constfunc(fn, line, t, n)
{
  current.Clear();
  current.setNull();  // haven't been computed yet
}

void cvgfunc::Compute(int i, result &x)
{
  DCASSERT(i==0);
  x = current;
}

void cvgfunc::ShowHeader(OutputStream &s) const
{
  s << GetType(Type(0)) << " " << Name();
}

// ******************************************************************
// *                                                                *
// *                        guess_stmt class                        *
// *                                                                *
// ******************************************************************

class guess_stmt : public statement {
private:
  cvgfunc* var;
  expr* guess;
public:
  guess_stmt(const char* fn, int line, cvgfunc* var, expr* guess);
  virtual ~guess_stmt();
  virtual void Execute() { }
  virtual void InitialGuess();
  virtual void Affix();
  virtual void show(OutputStream &s) const;
  virtual void showfancy(int dpth, OutputStream &s) const;
};

guess_stmt::guess_stmt(const char* fn, int line, cvgfunc* v, expr* g)
 : statement(fn, line)
{
  var = v;
  guess = g;
  DCASSERT(guess != ERROR);
}

guess_stmt::~guess_stmt()
{
  Delete(guess);
}

void guess_stmt::InitialGuess()
{
  guess->Compute(0, var->current);
#ifdef DEBUG_CONVERGE
  Output << "Initialized " << var << " got ";
  PrintResult(Output, REAL, var->current);
  Output << "\n";
  Output.flush();
#endif
}

void guess_stmt::Affix()
{
  var->state = CS_Computed;
}

void guess_stmt::show(OutputStream &s) const
{
  var->ShowHeader(s);
  s << " guess " << guess;
}

void guess_stmt::showfancy(int depth, OutputStream &s) const
{
  s.Pad(depth);
  show(s);
  s << ";\n";
}

// ******************************************************************
// *                                                                *
// *                       assign_stmt  class                       *
// *                                                                *
// ******************************************************************

class assign_stmt : public statement {
private:
  cvgfunc* var;
  expr* rhs;
  result update;
  bool hasconverged;
  bool was_updated;
public:
  assign_stmt(const char* fn, int line, cvgfunc* var, expr* rhs);
  virtual ~assign_stmt();
  virtual void Execute();
  virtual bool HasConverged();
  virtual void Affix();
  virtual void show(OutputStream &s) const;
  virtual void showfancy(int dpth, OutputStream &s) const;
protected:
  inline void CheckConvergence() {
    // Both null
    if (var->current.isNull() && update.isNull()) {
      hasconverged = true;
      return;
    }
    // one is null
    if (var->current.isNull() || update.isNull()) {
      hasconverged = false;
      return;
    }
    // both infinite (means converged to stop infinite loops)
    if (var->current.isInfinity() && update.isInfinity()) {
      hasconverged = (var->current.ivalue > 0) == (update.ivalue > 0);
      return;
    }
    // one is infinite
    if (var->current.isInfinity() || update.isInfinity()) {
      hasconverged = false;
    }
    // cases that don't make sense:
    if (var->current.isUnknown() || update.isUnknown()) {
      Internal.Start(__FILE__, __LINE__, Filename(), Linenumber());
      Internal << "Don't know value within a converge?";
      Internal.Stop();
    }
    // ok, so we should have 2 reals, check their difference

    double delta = var->current.rvalue - update.rvalue;
    if (converge_precision_test->GetEnum() == relative) {
      if (var->current.rvalue) delta /= var->current.rvalue;
    }
    hasconverged = (ABS(delta) < converge_precision->GetReal());
  }
  inline void Update() {
    CheckConvergence();
    var->current = update;
    was_updated = true;
#ifdef DEBUG_CONVERGE
    Output << "Updated " << var << "\n";
    Output.flush();
#endif
  }
};

assign_stmt::assign_stmt(const char* fn, int line, cvgfunc* v, expr* r)
 : statement(fn, line)
{
  var = v;
  rhs = r;
  DCASSERT(rhs != ERROR);
  DCASSERT(rhs);
  DCASSERT(rhs->Type(0) == REAL);
  was_updated = false;
}

assign_stmt::~assign_stmt()
{
  Delete(rhs);
}

void assign_stmt::Execute()
{
  DCASSERT(var->state != CS_Computed);
  rhs->Compute(0, update);
#ifdef DEBUG_CONVERGE
  Output << "Computed " << var << " got ";
  PrintResult(Output, REAL, var->current);
  Output << "\n";
  Output.flush();
#endif
  was_updated = false;
  if (use_current->GetBool()) Update();
}

bool assign_stmt::HasConverged()
{
  DCASSERT(var->state != CS_Computed);
  if (!was_updated) Update();
  return hasconverged;
}

void assign_stmt::Affix()
{
  var->state = CS_Computed;
}

void assign_stmt::show(OutputStream &s) const
{
  var->ShowHeader(s);
  s << " := " << rhs;
}

void assign_stmt::showfancy(int depth, OutputStream &s) const
{
  s.Pad(depth);
  show(s);
  s << ";\n";
}

// ******************************************************************
// *                                                                *
// *                      converge_stmt  class                      *
// *                                                                *
// ******************************************************************

class converge_stmt : public statement {
  statement **block;
  int blocksize;
public:
  converge_stmt(const char* fn, int line, statement** b, int bs);
  virtual ~converge_stmt();

  virtual void Execute();
  // virtual void InitialGuess();
  // virtual bool HasConverged();
  virtual void Affix();
  virtual void show(OutputStream &s) const;
  virtual void showfancy(int depth, OutputStream &s) const;
};

converge_stmt::converge_stmt(const char* fn, int line, statement** b, int bs)
 : statement(fn, line)
{
  block = b;
  blocksize = bs;
}

converge_stmt::~converge_stmt()
{
  int j;
  for (j=0; j<blocksize; j++) delete block[j];
  delete[] block;
}

void converge_stmt::Execute()
{
  int iters, j;
  // do guesses
  for (j=0; j<blocksize; j++) block[j]->InitialGuess();
  
  for (iters=max_converge_iters->GetInt(); iters; iters--) {
    
    // execute block
    for (j=0; j<blocksize; j++) block[j]->Execute();

    // have we converged yet?
    bool hc = true;
    for (j=0; j<blocksize; j++) 
      if (!block[j]->HasConverged()) hc = false;

    if (hc) break;
  }

  if (0==iters) {
    Warning.Start(Filename(), Linenumber());
    Warning << "converge did not attain precision ";
    Warning << converge_precision->GetReal() << " after ";
    Warning << max_converge_iters->GetInt() << " iterations";
    Warning.Stop();
  }
}

void converge_stmt::Affix() 
{
  int j;
  for (j=0; j<blocksize; j++)
    block[j]->Affix();
}

void converge_stmt::show(OutputStream &s) const
{
  s << "converge";
}

void converge_stmt::showfancy(int depth, OutputStream &s) const
{
  int j;
  s.Pad(depth);
  s << "converge {\n";
  for (j=0; j<blocksize; j++) {
    block[j]->showfancy(depth+2, s);
  }
  s.Pad(depth);
  s << "}\n";
}


// ******************************************************************
// *                                                                *
// *                                                                *
// *                          Global stuff                          *
// *                                                                *
// *                                                                *
// ******************************************************************

cvgfunc* MakeConvergeVar(type t, char* id, const char* file, int line)
{
  DCASSERT(t==REAL);
  return new cvgfunc(file, line, t, id);
}

statement* MakeGuessStmt(cvgfunc* v, expr* g, const char* fn, int line)
{
  v->state = CS_HasGuess;
  return new guess_stmt(fn, line, v, g);
}

statement* MakeAssignStmt(cvgfunc* v, expr* r, const char* fn, int line)
{
  v->state = CS_Defined;
  return new assign_stmt(fn, line, v, r);
}

statement* MakeConverge(statement** block, int bsize, const char* fn, int ln)
{
  return new converge_stmt(fn, ln, block, bsize);
}

void InitConvergeOptions()
{
  // UseCurrent
  const char* ucdoc = "Should converge variables be updated immediately";
  use_current = MakeBoolOption("UseCurrent", ucdoc, true);
  AddOption(use_current);

  // MaxConvergeIters
  const char* mci = "Maximum number of iterations of a converge";
  max_converge_iters = MakeIntOption("MaxConvergeIters", mci, 1000, 1, 2000000000);
  AddOption(max_converge_iters);

  // ConvergePrecision
  const char* cp = "Desired precision for converge values";
  converge_precision = MakeRealOption("ConvergePrecision", cp, 1e-5, 1e-100, 1);
  AddOption(converge_precision);

  // ConvergePrecisionTest
  absolute = new option_const("ABSOLUTE", "Use absolute precision");
  relative = new option_const("RELATIVE", "Use relative precision");
  option_const** list = new option_const*[2];
  list[0] = absolute;
  list[1] = relative;

  const char* cpt = "Comparison to use for convergence test of converge values";

  converge_precision_test 
    = MakeEnumOption("ConvergePrecisionTest", cpt, list, 2, relative);
  AddOption(converge_precision_test);
}

//@}


