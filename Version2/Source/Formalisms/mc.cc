
// $Id$

#include "mc.h"

#include "../Base/api.h"
#include "../Language/api.h"


// ******************************************************************
// *                                                                *
// *                       markov_model class                       *
// *                                                                *
// ******************************************************************

/** Smart support for the Markov chain "formalism".
*/
class markov_model : public model {
  int states;
public:
  markov_model(const char* fn, int line, type t, char*n, 
  		formal_param **pl, int np);

  virtual ~markov_model();

  // Required for models:

  virtual model_var* MakeModelVar(const char *fn, int l, type t, char* n);

  virtual void InitModel();
  virtual void FinalizeModel(result &);
  virtual state_model* BuildStateModel();
};

// ******************************************************************
// *                      markov_model methods                      *
// ******************************************************************

markov_model::markov_model(const char* fn, int line, type t, char*n, 
  formal_param **pl, int np) : model(fn, line, t, n, pl, np)
{
  Output << "Created (empty) model " << n << "\n";
  Output.flush();
  states = -1;
}

markov_model::~markov_model()
{
}

model_var* markov_model::MakeModelVar(const char *fn, int l, type t, char* n)
{
  states++;
  return NULL;
}

void markov_model::InitModel()
{
  states = 0;
}

void markov_model::FinalizeModel(result &x)
{
  Output << "MC has " << states << " states?\n";
  Output.flush();

  x.Clear();
  x.notFreeable();
  x.other = this;
}

state_model* markov_model::BuildStateModel()
{
  return NULL;
}

// ******************************************************************
// *                                                                *
// *                        Global front-end                        *
// *                                                                *
// ******************************************************************

model* MakeMarkovChain(type t, char* id, formal_param **pl, int np,
			const char* fn, int line)
{
  return new markov_model(fn, line, t, id, pl, np);
}


