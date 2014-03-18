
// $Id$

#ifndef MC_MDD_H
#define MC_MDD_H

class lldsm;
class markov_lldsm;
class meddly_states;
class meddly_varoption;
class shared_ddedge;

// ******************************************************************
// *                                                                *
// *                           Front  end                           *
// *                                                                *
// ******************************************************************

// Start a Meddly MC
markov_lldsm* StartMeddlyMC(meddly_states* ss);

// Get the states from a Meddly MC (or 0 on error)
meddly_states* GrabMeddlyMCStates(lldsm* mc);

// Finish a Meddly MC
void FinishMeddlyMC(lldsm* mc, shared_ddedge* potproc, bool pot);


#endif

