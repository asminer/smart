
// $Id$

#include "mcgen.h"
#include "../Chains/procs.h"
#include "../States/reachset.h"
#include "../Base/timers.h"

option* MarkovStorage;

const int num_mc_options = 2;

option_const sparse_mc("SPARSE", "\aSparse, row-wise or column-wise storage");
option_const kronecker_mc("KRONECKER", "\aUsing Kronecker algebra");

inline bool IllegalRateCheck(result &x, state_model *dsm, event *t)
{
  if (!x.isNormal() || x.rvalue <= 0) {
    Error.StartModel(dsm->Name(), dsm->Filename(), dsm->Linenumber());
    if (x.isInfinity()) 
      Error << "Infinite rate";
    else if (x.isUnknown()) 
      Error << "Unknown rate";
    else if (x.isError() || x.isNull())
      Error << "Undefined rate";
    else 
      Error << "Illegal rate (" << x.rvalue << ")";

    Error << " for event " << t << " during CTMC generation";
    Error.Stop();
    return true;
  }  // if error
  return false;
}

template <class REACHSET>
bool GenerateCTMC(state_model *dsm, REACHSET *S, labeled_digraph<float>* mc)
{
  DCASSERT(dsm);
  DCASSERT(dsm->NumEvents() > 0);
  DCASSERT(S);

  bool error = false;
  int e;


  // allocate temporary (full) states
  DCASSERT(dsm->UsesConstantStateSize());
  int stsize = dsm->GetConstantStateSize();
  state current, reached;
  AllocState(current, stsize);
  AllocState(reached, stsize);

  result x;
  x.Clear();
  // compute the constant rates once before starting; use -1 if not const.
  float* rates = new float[dsm->NumEvents()];
  for (e=0; e<dsm->NumEvents(); e++) {
    event* t = dsm->GetEvent(e);
    if (EXPO==t->DistroType()) {
      SafeCompute(t->Distribution(), 0, x);
      if (IllegalRateCheck(x, dsm, t)) {
        // bail out
	delete[] rates;
	return false;
      }
      rates[e] = x.rvalue;
    } else {
      rates[e] = -1.0;
    }
  } 
  
  // ok, build the ctmc
  int from; 
  mc->ResizeNodes(S->NumStates());
  for (from=0; from < S->NumStates(); from++) {
    mc->AddNode();
  }
  mc->ResizeEdges(4);
  for (from=0; from < S->NumStates(); from++) {
    S->GetState(from, current);
    // what is enabled?
    for (e=0; e<dsm->NumEvents(); e++) {
      event* t = dsm->GetEvent(e);
      DCASSERT(t);
      if (NULL==t->getNextstate())
        continue;  // firing is "no-op", don't bother

      t->isEnabled()->Compute(current, 0, x);
      if (!x.isNormal()) {
	Error.StartModel(dsm->Name(), dsm->Filename(), dsm->Linenumber());
	if (x.isUnknown()) 
	  Error << "Unknown if event " << t << " is enabled";
	else
	  Error << "Bad enabling expression for event " << t;
	Error << " during CTMC generation";
	Error.Stop();
	error = true;
	break;
      }
      if (!x.bvalue) continue;  // event is not enabled

      // e is enabled, fire and get new state

      // set reached = current
      S->GetState(from, reached);
      // do the firing
      t->getNextstate()->NextState(current, reached, x); 
      if (!x.isNormal()) {
	Error.StartModel(dsm->Name(), dsm->Filename(), dsm->Linenumber());
	Error << "Bad next-state expression during CTMC generation";
	Error.Stop();
	error = true;
	break;
      }

      // Which state is reached?
      int to = S->FindState(reached);
      if (to<0) {
        Internal.Start(__FILE__, __LINE__, dsm->Filename(), dsm->Linenumber());
        Internal << "Couldn't find reachable state ";
        dsm->ShowState(Internal, reached);
        Internal << " in reachability set\n";
        Internal.Stop();
      }

      // determine the rate
      if (rates[e]<0) {
	// we must have a PROC_EXPO distribution
        SafeCompute(t->Distribution(), current, 0, x);
        if (IllegalRateCheck(x, dsm, t)) error = true;
	else mc->AddEdgeInOrder(from, to, x.rvalue);
      } else {
        mc->AddEdgeInOrder(from, to, rates[e]);
      }

    } // for e
    if (error) break;
  } // for from

  // cleanup
  FreeState(reached);
  FreeState(current);
  delete[] rates;
  return !error; 
}

void CompressAndAffix(state_model* dsm, labeled_digraph<float> *mc)
{
  if (NULL==mc) {
    dsm->mc = new markov_chain(NULL);
    dsm->mc->CreateError();
    return;
  }
  dsm->mc = new markov_chain(NULL);
  dsm->mc->CreateExplicit(new classified_chain<float>(mc));
}

void SparseCTMC(state_model *dsm)
{
  DCASSERT(NULL==dsm->mc);
  if (Verbose.IsActive()) {
    Verbose << "Starting CTMC generation using sparse storage\n";
    Verbose.flush();
  }

  labeled_digraph<float>* mc = NULL;
  bool ok = false;

  // Build CTMC based on state space type
  switch (dsm->statespace->Storage()) {
    case RT_None:
    case RT_Error:
	break;

    case RT_Explicit:
        mc = new labeled_digraph<float>;
	ok = GenerateCTMC(dsm, dsm->statespace->Explicit(), mc);
	break;
			
    default:
	Internal.Start(__FILE__, __LINE__);
	Internal << "Unhandled storage type for CTMC generation\n";
	Internal.Stop();
  }

  if (Verbose.IsActive()) {
    Verbose << "Done generating CTMC; classifying and compressing\n";
    Verbose.flush();
  }

  // An error occurred during generation
  if (!ok) {
    delete mc;
    mc = NULL;
  } else {

    // "generate" initial probability vector here...

    // transpose if necessary
    if (!MatrixByRows->GetBool()) mc->Transpose();
  }

  // attach to model
  CompressAndAffix(dsm, mc);
}

// *******************************************************************
// *                           Front  ends                           *
// *******************************************************************

void BuildCTMC(state_model *dsm)
{
  if (NULL==dsm) return;
  DCASSERT(dsm->statespace);
  if (dsm->mc) return;  // already built

  const option_const* mc_option = MarkovStorage->GetEnum();

  timer watch;

  watch.Start();
  if (mc_option == &sparse_mc)		SparseCTMC(dsm); 
  watch.Stop();

  if (NULL==dsm->mc) {
    // we didn't do anything...
    Internal.Start(__FILE__, __LINE__);
    Internal << "MarkovStorage option " << mc_option << " not handled";
    Internal.Stop();
  }

  if (Verbose.IsActive()) {
    Verbose << "Generation took " << watch << "\n";
    Verbose.flush();
  }
}

//#define DEBUG

void InitMCGen()
{
#ifdef DEBUG
  Output << "Initializing Markov chain generation options\n";
#endif
  // StateStorage option
  option_const **mclist = new option_const*[num_mc_options];
  // these must be alphabetical
  mclist[0] = &kronecker_mc;
  mclist[1] = &sparse_mc;
  MarkovStorage = MakeEnumOption("MarkovStorage", "Algorithm and data structure to use for Markov chain generation", mclist, num_mc_options, &sparse_mc);
  AddOption(MarkovStorage);
}

