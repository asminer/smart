
// $Id$

#include "mc.h"

#include "../Base/api.h"
#include "../Language/api.h"
#include "../Main/tables.h"
#include "../States/reachset.h"

#include "dsm.h"


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

class markov_dsm : public state_model {
  char** statenames;
  int numstates;
public:
  /** Constructor.
	@param	sn	Array of state names
	@param	ns	Number of states
  */
  markov_dsm(char** sn, int ns);
  virtual ~markov_dsm();

  // required stuff:

  virtual void ShowState(OutputStream &s, const state &x);
  virtual void ShowEventName(OutputStream &s, int e);

/*
  virtual int NumInitialStates() const;
  virtual void GetInitialState(int n, state &s) const;

  virtual expr* EnabledExpr(int e);
  virtual expr* NextStateExpr(int e);
  virtual expr* EventDistribution(int e);
*/

};

// ******************************************************************
// *                       markov_dsm methods                       *
// ******************************************************************

markov_dsm::markov_dsm(char** sn, int ns) : state_model(1)
{
  statenames = sn;
  numstates = ns;

  statespace = new reachset;
  statespace->CreateEnumerated(numstates);
}

markov_dsm::~markov_dsm()
{
}

void markov_dsm::ShowState(OutputStream &s, const state &x)
{
  // check state legality and range here...
  s << statenames[x.Read(0).ivalue];
}

void markov_dsm::ShowEventName(OutputStream &s, int e)
{
  DCASSERT(e < NumEvents());
  DCASSERT(e>=0);

  // Is there something better to do here?
  s << "Markov chain";
}

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

