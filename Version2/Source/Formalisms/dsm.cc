
// $Id$

#include "dsm.h"

#include "../States/reachset.h"
#include "../Chains/procs.h"
#include "../Base/memtrack.h"

/** @name dsm.cc
    @type File
    @args \ 

  Implementation for Discrete-state model "base class".

*/

//@{

void Delete(state_model *x) 
{
  Delete((expr*) x);
}

// ******************************************************************
// *                                                                *
// *                          event methods                         *
// *                                                                *
// ******************************************************************

event::event(const char* fn, int line, type t, char* n)
 : symbol(fn, line, t, n)
{
  enabling = nextstate = distro = NULL;
}

event::~event()
{
  Delete(enabling);
  Delete(nextstate);
  Delete(distro);
}

void event::setEnabling(expr *e)
{
  DCASSERT(e); // at least should be constant "true"
  DCASSERT(NULL==enabling);
  enabling = e;
  DCASSERT(e->Type(0) == PROC_BOOL);
}

void event::setNextstate(expr *e)
{
  DCASSERT(NULL==nextstate);
  nextstate = e;
  DCASSERT(
    NULL==e || e->Type(0) == PROC_STATE  // add proc_stateset once we're ready
  );
}

void event::setDistribution(expr *e)
{
  DCASSERT(NULL==distro);
  distro = e;
}

// ******************************************************************
// *                                                                *
// *                       state_model methods                      *
// *                                                                *
// ******************************************************************

OutputStream& operator<< (OutputStream &s, state_model *e)
{
  if (e) s << e->Name();
  else s << "null";
  return s;
}

state_model::state_model(const char* fn, int line, type t, char* n,
  event** ed, int ne) : symbol(fn, line, t, n)
{
  ALLOC("state_model", sizeof(state_model));
  event_data = ed;
  num_events = ne;
  // set useful stuff to unknown values
  statespace = NULL;
  proctype = Proc_Unknown;
  mc = NULL;
}

state_model::~state_model()
{
  FREE("state_model", sizeof(state_model));
  delete statespace;
  delete mc;
  // trash the events
  for (int e=0; e<num_events; e++) {
    Delete(event_data[e]);  // Delete, in case they are shared
  }
  delete[] event_data;
}

int state_model::GetConstantStateSize() const
{
  Internal.Start(__FILE__, __LINE__);
  Internal << "Request for state size, for a model with variable-sized states";
  Internal.Stop();
  return 0; // keep compiler happy
}

void state_model::DetermineProcessType()
{
  if (proctype != Proc_Unknown) return;
  
  int e;
  // check if this is a Finite state machine (no distros)
  proctype = Proc_FSM;
  for (e=0; e<num_events; e++) {
    DCASSERT(event_data[e]);
    if (event_data[e]->Distribution()) {
      proctype = Proc_Unknown;
      break;
    }
  } // for e
  if (Proc_FSM == proctype) return;  

  // Not a FSM, now check distributions
  bool all_expos = true;
  for (e=0; e<num_events; e++) {
    switch (event_data[e]->DistroType()) {
      case VOID:
		// some events have no distribution, that's bad
		proctype = Proc_Error;
		return;

      case EXPO:
      case PROC_EXPO:
		// rule some other things out, eventually
		break;

      default:
		all_expos = false;
		break;
    } // switch
  } 

  if (all_expos) {
    proctype = Proc_Ctmc;
    return;
  }

  // there needs to be more here, eventually
  
  proctype = Proc_General;
}

//@}


