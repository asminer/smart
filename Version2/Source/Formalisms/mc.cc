
// $Id$

#include "mc.h"

#include "../Base/api.h"
#include "../Language/api.h"
#include "../Main/tables.h"
#include "../States/reachset.h"

#include "dsm.h"

//#define DEBUG_MC

// ******************************************************************
// *                                                                *
// *                        markov_dsm  class                       *
// *                                                                *
// ******************************************************************

/** A discrete-state model (internel representation) for Markov chains.
*/
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

  virtual int NumInitialStates() const;
  virtual void GetInitialState(int n, state &s) const;

  virtual expr* EnabledExpr(int e) { return NULL; } // fix later
  virtual expr* NextStateExpr(int e) { return NULL; } // fix later
  virtual expr* EventDistribution(int e) { return NULL; } // fix later

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

int markov_dsm::NumInitialStates() const
{
  // fill this in later
  return 0;
}

void markov_dsm::GetInitialState(int n, state &s) const
{
  // fill this in later
  s[0].ivalue = 0;
}

// ******************************************************************
// *                                                                *
// *                       markov_model class                       *
// *                                                                *
// ******************************************************************

/** Smart support for the Markov chain "formalism".
    I.e., front-end stuff for Markov chain formalism.
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
  statelist = NULL; 
  statenames = NULL;
  numstates = 0;
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
  statenames = NULL;
  numstates = 0;
}

void markov_model::FinalizeModel(result &x)
{
  numstates = statelist->Length();
  statenames = statelist->MakeArray();
  delete statelist;
  statelist = NULL;
#ifdef DEBUG_MC
  Output << "MC has " << numstates << " states?\n";
  int i;
  for (i=0; i<numstates; i++) {
    Output << "\t" << statenames[i] << "\n";
  }
  Output.flush();
#endif

  x.Clear();
  x.notFreeable();
  x.other = this;
}

state_model* markov_model::BuildStateModel()
{
  return new markov_dsm(statenames, numstates);
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

