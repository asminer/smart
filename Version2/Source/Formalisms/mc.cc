
// $Id$

#include "mc.h"

#include "../Base/api.h"
#include "../Language/api.h"
#include "../Main/tables.h"


// ******************************************************************
// *                                                                *
// *                       markov_model class                       *
// *                                                                *
// ******************************************************************

/** Smart support for the Markov chain "formalism".
*/
class markov_model : public model {
  List <char> *statelist;
  char** statenames;
  int numstates;
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
  statelist = NULL; 
}

markov_model::~markov_model()
{
}

model_var* markov_model::MakeModelVar(const char *fn, int l, type t, char* n)
{
  statelist->Append(n);
  return NULL;
}

void markov_model::InitModel()
{
  statelist = new List <char> (16);
}

void markov_model::FinalizeModel(result &x)
{
  numstates = statelist->Length();
  statenames = statelist->MakeArray();
  delete statelist;
  statelist = NULL;
  Output << "MC has " << numstates << " states?\n";
  int i;
  for (i=0; i<numstates; i++) {
    Output << "\t" << statenames[i] << "\n";
  }
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
// *                        markov_dsm  class                       *
// *                                                                *
// ******************************************************************

// ******************************************************************
// *                                                                *
// *                      MC-specific functions                     *
// *                                                                *
// ******************************************************************

// ******************************************************************
// *                                                                *
// *                        Global front-ends                       *
// *                                                                *
// ******************************************************************

model* MakeMarkovChain(type t, char* id, formal_param **pl, int np,
			const char* fn, int line)
{
  return new markov_model(fn, line, t, id, pl, np);
}

void InitMCModelFuncs(PtrTable *t)
{
}

