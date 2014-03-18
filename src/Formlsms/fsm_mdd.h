
// $Id$

#ifndef FSM_MDD_H
#define FSM_MDD_H

class lldsm;
class checkable_lldsm;
class meddly_states;

// ******************************************************************
// *                                                                *
// *                           Front  end                           *
// *                                                                *
// ******************************************************************

// Start a Meddly FSM
checkable_lldsm* StartMeddlyFSM(meddly_states* ss);

// Get the states from a Meddly FSM (or 0 on error)
meddly_states* GrabMeddlyFSMStates(lldsm* fsm);

// Finish a Meddly FSM
void FinishMeddlyFSM(lldsm* fsm, bool pot);


#endif

