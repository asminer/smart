
// $Id$

#include "models.h"
#include "measures.h"

//@Include: models.h

/** @name models.cc
    @type File
    @args \ 

   Implementation of (language support for) models.

 */

//@{

// remove this soon...

void Delete(state_model *x)
{
  DCASSERT(0);
}

// ******************************************************************
// *                                                                *
// *                       model_var  methods                       *
// *                                                                *
// ******************************************************************

model_var::model_var(const char* fn, int line, type t, char* n)
 : symbol(fn, line, t, n)
{
  state_index = -1;
  part_index = -1;
}

// ******************************************************************
// *                                                                *
// *                         model  methods                         *
// *                                                                *
// ******************************************************************

bool model::SameParams()
{
  // compare current_params with last_params
  int i;
  for (i=0; i<num_params; i++) {
    if (!Equals(parameters[i]->Type(0), current_params[i], last_params[i]))
      return false;
  }
  return true;
}

model::model(const char* fn, int l, type t, char* n, formal_param **pl, int np)
 : function(fn, l, t, n, pl, np)
{
  stmt_block = NULL;
  num_stmts = 0;
  dsm = NULL;
  // Allocate space to save parameters
  if (np) {
    current_params = new result[np];
    last_params = new result[np];
  } else {
    current_params = last_params = NULL;
  }
  // link the parameters
  int i;
  for (i=0; i<np; i++) {
    pl[i]->LinkUserFunc(&current_params, i);
  }
  last_build.Clear();
  last_build.setNull();
}

model::~model()
{
  // is this ever called?
  Delete(dsm);
  int i;
  for (i=0; i<num_stmts; i++) delete stmt_block[i];
  delete[] stmt_block;
  FreeLast();
  delete[] current_params;
  delete[] last_params;
}

void model::SetStatementBlock(statement **b, int n)
{
  DCASSERT(NULL==stmt_block);
  stmt_block = b;
  num_stmts = n;
}

void model::Compute(expr **p, int np, result &x)
{
  if (NULL==stmt_block) {
    x.setNull();
    return;
  }
  DCASSERT(np <= num_params);
  // compute parameters
  int i;
  for (i=0; i<np; i++) SafeCompute(p[i], 0, current_params[i]);
  for (i=np; i<num_params; i++) current_params[i].setNull();

  // same as last time?
  if (SameParams()) {
    FreeCurrent();
    x = last_build;
    return;
  }

  // nope... rebuild
  Delete(dsm);
  InitModel();
  for (i=0; i<num_stmts; i++) {
    stmt_block[i]->Execute();
  }
  FinalizeModel(last_build);
  x = last_build;
  if (x.other == this) dsm = BuildStateModel();
  else dsm = NULL;

  // save parameters
  FreeLast();
  SWAP(current_params, last_params);  // pointer swap!
}

void model::Sample(Rng &, expr **, int np, result &)
{
  Internal.Start(__FILE__, __LINE__, Filename(), Linenumber());
  Internal << "Attempt to sample a model.";
  Internal.Stop();
}

// ******************************************************************
// *                                                                *
// *                        model_call class                        *
// *                                                                *
// ******************************************************************

/**  An expression to compute a model call.
     That is, this computes expressions of the type
        model(p1, p2, p3).measure;
*/

class model_call : public expr {
protected:
  model *mdl;
  expr **pass;
  int numpass;
  measure *msr;
public:
  model_call(const char *fn, int line, model *m, expr **p, int np, measure *s);
  virtual ~model_call();
  virtual type Type(int i) const;
  virtual void Compute(int i, result &x);
  virtual expr* Substitute(int i);
  virtual int GetSymbols(int i, List <symbol> *syms=NULL);
  virtual void show(OutputStream &s) const;
  virtual Engine_type GetEngine(engineinfo *e);
  virtual expr* SplitEngines(List <measure> *mlist);
};

// ******************************************************************
// *                       model_call methods                       *
// ******************************************************************

model_call::model_call(const char *fn, int l, model *m, expr **p, int np,
measure *s) : expr (fn, l)
{
  mdl = m;
  pass = p;
  numpass = np;
  msr = s;
}

model_call::~model_call()
{
  // don't delete the model
  int i;
  for (i=0; i < numpass; i++) Delete(pass[i]);
  delete[] pass;
}

type model_call::Type(int i) const
{
  DCASSERT(0==i);
  return msr->Type(i);
}

void model_call::Compute(int i, result &x)
{
  result m;
  DCASSERT(0==i);
  DCASSERT(mdl);
  mdl->Compute(pass, numpass, m);
  if (m.isError() || m.isNull()) {
    x = m;
    return;
  }
  DCASSERT(x.other == mdl);

  // compute the measure and stuff here...

  x.setNull();
}

expr* model_call::Substitute(int i)
{
  DCASSERT(0==i);
  if (0==numpass) return Copy(this);

  // substitute each passed parameter
  expr ** pass2 = new expr*[numpass];
  int n;
  for (n=0; n<numpass; n++) pass2[n] = pass[n]->Substitute(0);

  return new model_call(Filename(), Linenumber(), mdl, pass2, numpass, msr);
}

int model_call::GetSymbols(int i, List <symbol> *syms)
{
  DCASSERT(0==i);
  int n;
  int answer = 0;
  for (n=0; n<numpass; n++) {
    answer += pass[n]->GetSymbols(0, syms);
  }
  return answer;
}

void model_call::show(OutputStream &s) const
{
  if (mdl->Name()==NULL) return; // can this happen?
  s << mdl->Name();
  if (numpass) {
    s << "(";
    int i;
    for (i=0; i<numpass; i++) {
      if (i) s << ", ";
      s << pass[i];
    }
    s << ")";
  }
  s << "." << msr;
}

Engine_type model_call::GetEngine(engineinfo *e)
{
  // Since we are outside the model, treat this as NONE
  if (e) e->engine = ENG_None;
  return ENG_None;
}

expr* model_call::SplitEngines(List <measure> *mlist)
{
  return Copy(this);
}

// ******************************************************************
// *                                                                *
// *                                                                *
// *                          Global stuff                          *
// *                                                                *
// *                                                                *
// ******************************************************************

expr* MakeMeasureCall(model *m, expr **p, int np, measure *s, const char *fn, int line)
{
  return new model_call(fn, line, m, p, np, s);
}

//@}

