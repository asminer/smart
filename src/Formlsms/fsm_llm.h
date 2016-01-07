
// $Id$

#ifndef FSM_LLM_H
#define FSM_LLM_H

class exprman;

// ******************************************************************
// *                                                                *
// *                           Front  end                           *
// *                                                                *
// ******************************************************************

void InitFSMLibs(exprman* em);

#ifndef INITIALIZERS_ONLY

#include "graph_llm.h"

//
//
// NEW CODE NOT 100% WORKING YET FROM HERE
//
//

/**
    Start a FSM when only the states are known.
    Edges must be added later with FinishGenericFSM().
*/
graph_lldsm* StartFSM(state_lldsm::reachset* rss);  

/**
    Finish a FSM.
    It must have been started by StartGenericFSM().
*/
void FinishFSM(graph_lldsm* rs, graph_lldsm::reachgraph* rgr);

//
//
// OLD CODE TO BE RETIRED FROM HERE
//
//

namespace GraphLib {
  class digraph;
};

namespace StateLib {
  class state_db;
};

struct LS_Vector;
class model_enum;
class expl_rss_only;
class lldsm;
graph_lldsm* 
MakeEnumeratedFSM(LS_Vector &init, model_enum* ss, GraphLib::digraph* rg);

// Start an explicit FSM
graph_lldsm* StartExplicitFSM(StateLib::state_db* ss);

// Get the states from an explicit FSM (or 0 on error)
StateLib::state_db* GrabExplicitFSMStates(lldsm* rg);

// Finish an explicit FSM
void FinishExplicitFSM(lldsm* rs, LS_Vector &init, GraphLib::digraph* rg);

#endif  // INITIALIZERS_ONLY

#endif

