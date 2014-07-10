
// $Id$

#ifndef FSM_LLM_H
#define FSM_LLM_H

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
class checkable_lldsm;
class exprman;

namespace StateLib {
  class state_db;
}

// ******************************************************************
// *                                                                *
// *                           Front  end                           *
// *                                                                *
// ******************************************************************

void InitFSMLibs(exprman* em);

checkable_lldsm* 
MakeEnumeratedFSM(LS_Vector &init, model_enum* ss, GraphLib::digraph* rg);

// Start an explicit FSM
checkable_lldsm* StartExplicitFSM(StateLib::state_db* ss);

// Get the states from an explicit FSM (or 0 on error)
StateLib::state_db* GrabExplicitFSMStates(lldsm* rg);

// Finish an explicit FSM
void FinishExplicitFSM(lldsm* rs, LS_Vector &init, GraphLib::digraph* rg);

#endif

