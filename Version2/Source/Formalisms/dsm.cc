
// $Id$

#include "dsm.h"

#include "../States/reachset.h"
#include "../Chains/procs.h"
#include "../Base/memtrack.h"

//#define DEBUG_PRIO

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
  prio_list = NULL;
  prio_length = 0;
}

event::~event()
{
  Delete(enabling);
  Delete(nextstate);
  Delete(distro);
  delete[] prio_list;
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
  rg = NULL;
  mc = NULL;
  prio_cycle = false;
  OrderEventsByPriority();
}

state_model::~state_model()
{
  FREE("state_model", sizeof(state_model));
  delete statespace;
  delete rg;
  delete mc;
  // trash the events
  for (int e=0; e<num_events; e++) {
    Delete(event_data[e]);  // Delete, in case they are shared
  }
  delete[] event_data;
}

void state_model::OrderEventsByPriority()
{
#ifdef DEBUG_PRIO
  Output << "Reordering events by priority rules.  Current events:\n";
#endif
  // First, for each event, count the number that have priority over it
  // and store in "misc" field
  int e;
  for (e=0; e<num_events; e++) event_data[e]->misc = 0;
  for (e=0; e<num_events; e++) {
    event* ee = event_data[e];
#ifdef DEBUG_PRIO
    Output << "\t" << ee << " has priority over:\n";     
#endif
    for (int i=0; i<ee->prio_length; i++) {
      DCASSERT(ee->prio_list);
      ee->prio_list[i]->misc++;
#ifdef DEBUG_PRIO
      Output << "\t\t" << ee->prio_list[i] << "\n";
#endif
    }
#ifdef DEBUG_PRIO
    Output.flush();
#endif
  }
  // Re-fill the event array.
  // Once an event has "misc" count of 0, it can go in the array.
  // When an event is put into the array, decrement all of its "children"
  // Algorithm is worst case O(n^2); O(n) if there is no priority structure
  for (e=0; e<num_events; e++) {
    int fz;
    for (fz=e; fz<num_events; fz++) if (0==event_data[fz]->misc) break;
    if (fz>=num_events) {
      // no zeroes  <==>  priority cycle
      Error.StartModel(Name(), Filename(), Linenumber());
      Error << "Priority cycle exists somewhere in events:\n";
      for (int show=e; show<num_events; show++) 
        Error << "\t" << event_data[show] << "\n";
      Error.Stop();
      prio_cycle = true;
      return;
    }
    // Put fz here if different from e
    if (fz!=e) SWAP(event_data[fz], event_data[e]);
    // decrement children
    event* ee = event_data[e];
    for (int i=0; i<ee->prio_length; i++) {
      DCASSERT(ee->prio_list);
      ee->prio_list[i]->misc--;
    }
  }
#ifdef DEBUG_PRIO
  Output << "Successfully reordered events, new order:\n";
  for (e=0; e<num_events; e++) 
    Output << "\t" << event_data[e] << "\n";
  Output.flush();
#endif
}

int state_model::GetConstantStateSize() const
{
  Internal.Start(__FILE__, __LINE__);
  Internal << "Request for state size, for a model with variable-sized states";
  Internal.Stop();
  return 0; // keep compiler happy
}

bool state_model::GetEnabledList(const state &current, List <event> *enabled)
{
  enabled->Clear();
  // misc field: are we enabled.  1=yes, -1=no, 0=don't know yet
  int e;
  for (e=0; e<num_events; e++) event_data[e]->misc = 0;

  result x;
  // check in order, correct for priority
  for (e=0; e<num_events; e++) {
    event *t = event_data[e];
    if (t->misc<0) continue;
    t->isEnabled()->Compute(current, 0, x);
    if (!x.isNormal()) {
      Error.StartModel(Name(), Filename(), Linenumber());
      if (x.isUnknown()) 
	Error << "Unknown if event " << t << " is enabled";
      else
	Error << "Bad enabling expression for event " << t;
      Error.Stop();
      return false;
    }
    if (!x.bvalue) {
      t->misc = -1;
      continue;
    }
    // event is enabled, disable all in priority list
    t->misc = 1;
    enabled->Append(t);
    for (int j=0; j<t->prio_length; j++) {
      DCASSERT(t->prio_list);
      t->prio_list[j]->misc = -1;
    } // for j
  } // for e
  return true;
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


