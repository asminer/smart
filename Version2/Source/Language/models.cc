
// $Id$

#include "models.h"
//@Include: models.h

/** @name models.cc
    @type File
    @args \ 

   Implementation of (language support for) models.

 */

//@{

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
  InitModel();
  for (i=0; i<num_stmts; i++) {
    stmt_block[i]->Execute();
  }
  FinalizeModel(last_build);
  x = last_build;

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

//@}

