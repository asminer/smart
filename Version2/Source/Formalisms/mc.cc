
// $Id$

#include "mc.h"

#include "../Base/api.h"
#include "../Language/api.h"
#include "../Main/tables.h"
#include "../States/reachset.h"
#include "../Templates/sparsevect.h"
#include "../Templates/graphs.h"

#include "dsm.h"

#define DEBUG_MC

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
  sparse_vector <float> *initial;
public:
  labeled_digraph <float> *wdgraph;
  markov_model(const char* fn, int line, type t, char*n, 
  		formal_param **pl, int np);

  virtual ~markov_model();

  void AddInitial(int state, double weight, const char *fn, int line);
  void AddArc(int fromstate, int tostate, double weight, const char *fn, int line);

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
  initial = new sparse_vector <float>(2);
  wdgraph = new labeled_digraph <float>;
  wdgraph->ResizeNodes(4);
  wdgraph->ResizeEdges(4);
}

markov_model::~markov_model()
{
  delete initial;
  delete wdgraph;
}

void markov_model::AddInitial(int state, double weight, const char* fn, int line)
{
  int e = initial->BinarySearchIndex(state);
  if (e<0) {
    initial->SortedAppend(state, weight);
    return;
  }
  // Duplicate entry, give a warning
  Warning.Start(fn, line);
  Warning << "Ignoring duplicate initial probability for \n\tstate ";
  Warning << statelist->Item(state) << " in Markov chain " << Name();
  Warning.Stop();
}

void markov_model::AddArc(int fromstate, int tostate, double weight, const char *fn, int line)
{
  float f = weight;
  if (wdgraph->AddEdgeInOrder(fromstate, tostate, f) == wdgraph->NumEdges()-1) 
    return;

  // Duplicate entry, give a warning
  Warning.Start(fn, line);
  Warning << "Summing duplicate arc from \n\tstate ";
  Warning << statelist->Item(fromstate) << " to " << statelist->Item(tostate);
  Warning << " in Markov chain " << Name();
  Warning.Stop();
}

model_var* markov_model::MakeModelVar(const char *fn, int l, type t, char* n)
{
  int ndx = statelist->Length();
  statelist->Append(n);
  wdgraph->AddNode();
  model_var* s = new model_var(fn, l, t, n);
  s->SetIndex(ndx);
#ifdef DEBUG_MC
  Output << "\tModel " << Name() << " created state " << n << " index " << ndx << "\n"; 
  Output.flush();
#endif
  return s;
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
  Output << "\tMC " << Name() << " has " << numstates << " states\n";
  int i;
  for (i=0; i<numstates; i++) {
    Output << "\t" << statenames[i] << "\n";
  }
  Output << "\tInitial weights:\n";
  for (i=0; i<initial->NumNonzeroes(); i++) {
    Output << "\t" << statenames[initial->index[i]];
    Output << " : " << initial->value[i] << "\n"; 
  }
  Output.flush();
  // wdgraph->Transpose();
  wdgraph->ConvertToStatic();
  Output << "Markov chain itself:\n";
  for (i=0; i<numstates; i++) {
    wdgraph->ShowNodeList(Output, i);
    Output.flush();
  }
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


// ********************************************************
// *                         init                         *
// ********************************************************

void compute_mc_init(expr **pp, int np, result &x)
{
  DCASSERT(np>1);
  DCASSERT(pp);

  markov_model *mc = dynamic_cast<markov_model*>(pp[0]);
  DCASSERT(mc);

#ifdef DEBUG_MC
  Output << "\tInside init for model " << mc << "\n";
#endif

  x.Clear();
  int i;
  for (i=1; i<np; i++) {
#ifdef DEBUG_MC
    Output << "\tparameter " << i << " is " << pp[i] << "\n";
    Output << "\t state ";
#endif
    SafeCompute(pp[i], 0, x);
#ifdef DEBUG_MC
    PrintResult(Output, INT, x);
    Output << "\n\t value ";
#endif
    int index = x.ivalue;
    // check for errors here...

    SafeCompute(pp[i], 1, x);
#ifdef DEBUG_MC
    PrintResult(Output, REAL, x);
    Output << "\n";
#endif
    double weight = x.rvalue;
    // again with the errors

    mc->AddInitial(index, weight, pp[i]->Filename(), pp[i]->Linenumber());
  }

#ifdef DEBUG_MC
  Output << "\tExiting init for model " << mc << "\n";
  Output.flush();
#endif

  x.setNull();
}

void Add_init(PtrTable *fns)
{
  const char* helpdoc = "Sets the initial state(s) with probabilities for a Markov Chain model.";

  formal_param **pl = new formal_param*[2];
  pl[0] = new formal_param(MARKOV, "m");
  type *tl = new type[2];
  tl[0] = STATE;
  tl[1] = REAL;
  pl[1] = new formal_param(tl, 2, "pair");
  internal_func *p = new internal_func(VOID, "init", 
	compute_mc_init, NULL,
	pl, 2, 1, helpdoc);  // parameter 1 repeats...
  p->setWithinModel();
  InsertFunction(fns, p);
}

// ********************************************************
// *                         arcs                         *
// ********************************************************

void compute_mc_arcs(expr **pp, int np, result &x)
{
  DCASSERT(np>1);
  DCASSERT(pp);
  markov_model *mc = dynamic_cast<markov_model*>(pp[0]);
  DCASSERT(mc);

#ifdef DEBUG_MC
  Output << "\tInside arcs for model " << mc << "\n";
#endif


  x.Clear();
  int i;
  for (i=1; i<np; i++) {
#ifdef DEBUG_MC
    Output << "\tparameter " << i << " is " << pp[i] << "\n";
    Output << "\t from state ";
#endif
    SafeCompute(pp[i], 0, x);
#ifdef DEBUG_MC
    PrintResult(Output, INT, x);
    Output << "\n\t to state ";
#endif
    int fromstate = x.ivalue;
    // check for errors here...

    SafeCompute(pp[i], 1, x);
#ifdef DEBUG_MC
    PrintResult(Output, INT, x);
    Output << "\n\t weight ";
#endif
    int tostate = x.ivalue;

    SafeCompute(pp[i], 2, x);
#ifdef DEBUG_MC
    PrintResult(Output, REAL, x);
    Output << "\n";
#endif
    double weight = x.rvalue;
    // again with the errors

    mc->AddArc(fromstate, tostate, weight, pp[i]->Filename(), pp[i]->Linenumber());
  }

#ifdef DEBUG_MC
  Output << "\tExiting arcs for model " << mc << "\n";
  Output.flush();
#endif

  x.setNull();
}

void Add_arcs(PtrTable *fns)
{
  const char* helpdoc = "Adds a set of arcs to the Markov chain";

  formal_param **pl = new formal_param*[2];
  pl[0] = new formal_param(MARKOV, "m");
  type *tl = new type[3];
  tl[0] = STATE;
  tl[1] = STATE;
  tl[2] = REAL;
  pl[1] = new formal_param(tl, 3, "triple");
  internal_func *p = new internal_func(VOID, "arcs", 
	compute_mc_arcs, NULL,
	pl, 2, 1, helpdoc);  // parameter 1 repeats...
  p->setWithinModel();
  InsertFunction(fns, p);
}

// ********************************************************
// *                        instate                       *
// ********************************************************

// A Proc function!
void compute_mc_instate(const state &m, expr **pp, int np, result &x)
{
  DCASSERT(np==2);
  DCASSERT(pp);
  /*
  model *m = dynamic_cast<model*> (pp[0]);
  DCASSERT(m);
  */
  // debugging
  Output << "Checking instate\n";
  Output.flush();

  x.Clear();
  SafeCompute(pp[1], 0, x);

  // error checking here...

  Output << "\tgot param: ";
  PrintResult(Output, INT, x);
  Output << "\n";
  Output.flush();

  Output << "\tcurrent state: ";
  PrintResult(Output, INT, m.Read(0));
  Output << "\n";
  Output.flush();

  // error checking here for m

  if (x.ivalue == m.Read(0).ivalue) {
    x.bvalue = true;
  } else {
    x.bvalue = false;
  }
}

void Add_instate(PtrTable *fns)
{
  const char* helpdoc = "Returns true if the Markov chain is in the specified state";

  formal_param **pl = new formal_param*[2];
  pl[0] = new formal_param(MARKOV, "m");
  pl[1] = new formal_param(STATE, "s");
  internal_func *p = new internal_func(PROC_BOOL, "instate", 
	compute_mc_instate, NULL,
	pl, 2, helpdoc);  
  p->setWithinModel();
  InsertFunction(fns, p);
}

// ********************************************************
// *                          test                        *
// ********************************************************

#include "../Engines/sccs.h"

// A hook for testing things
void compute_mc_test(expr **pp, int np, result &x)
{
  DCASSERT(np==2);
  DCASSERT(pp);
  markov_model *mc = dynamic_cast<markov_model*> (pp[0]);
  DCASSERT(mc);
  result term;
  SafeCompute(pp[1], 0, term);

  if (term.bvalue)
	Output << "Computing terminal sccs for Markov chain " << mc << "\n";
  else
  	Output << "Computing sccs for Markov chain " << mc << "\n";
  Output.flush();
  digraph *foo = mc->wdgraph;
  unsigned long* mapping = new unsigned long[foo->NumNodes()];
  int i;
  for (i=0; i<foo->NumNodes(); i++) mapping[i] = 0;

  x.Clear();
  if (term.bvalue)
  	x.ivalue = ComputeTSCCs(foo, mapping); 
  else
  	x.ivalue = ComputeSCCs(foo, mapping); 

  Output << "Done, node vector is: [";
  Output.PutArray(mapping, foo->NumNodes());
  Output << "]\n";
  Output.flush();
  
  delete[] mapping;
}

void Add_test(PtrTable *fns)
{
  formal_param **pl = new formal_param*[2];
  pl[0] = new formal_param(MARKOV, "m");
  pl[1] = new formal_param(BOOL, "dummy");
  internal_func *p = new internal_func(INT, "test",
	compute_mc_test, NULL, pl, 2, NULL);
  p->setWithinModel();
  InsertFunction(fns, p);
}

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
  Add_init(t);
  Add_arcs(t);

  Add_instate(t);

  Add_test(t);
}

