
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
  virtual void Guess();
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

void guess_stmt::Guess()
{
  guess->Compute(0, var->current);
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
  bool isfixed;
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

    double delta = ABS(var->current.rvalue - update.rvalue);
    // if relative difference then...
    if (var->current.rvalue) delta /= var->current.rvalue;
    // ...endif
    // 1e-5 should be an option
    hasconverged = (delta < 1e-5);
  }
  inline void Update() {
    CheckConvergence();
    var->current = update;
    was_updated = true;
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
  isfixed = false;
  was_updated = false;
}

assign_stmt::~assign_stmt()
{
  Delete(rhs);
}

void assign_stmt::Execute()
{
  DCASSERT(!isfixed);
  rhs->Compute(0, update);
  was_updated = false;
  if (use_current->GetBool()) Update();
}

bool assign_stmt::HasConverged()
{
  DCASSERT(!isfixed);
  if (!was_updated) Update();
  return hasconverged;
}

void assign_stmt::Affix()
{
  isfixed = true;
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
  virtual void InitialGuess();
  virtual bool HasConverged();
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
  int j;
  for (j=0; j<blocksize; j++)
    block[j]->Execute();
}

void converge_stmt::InitialGuess()
{
  int j;
  for (j=0; j<blocksize; j++)
    block[j]->InitialGuess();
}

bool converge_stmt::HasConverged()
{
  bool hc = true;
  int j;
  for (j=0; j<blocksize; j++)
    if (!block[j]->HasConverged()) hc = false;
  return hc;
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

//@}


