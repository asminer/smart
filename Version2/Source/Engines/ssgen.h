
// $Id$

#ifndef SSGEN_H
#define SSGEN_H

#include "../Formalisms/dsm.h"
#include "../States/flatss.h"
#include "../States/trees.h"

// Necessary for ssmcgen.cc

extern option* StateStorage;
extern option_const debug_ss;
extern option_const redblack_ss;
extern option_const splay_ss;

/** Useful helper function.
    Compress and attach the reachability set to the state model.
    If an error occurred, use states = NULL.
*/
void CompressAndAffix(state_model* dsm, state_array* states, binary_tree* tree);

/** 	Build the reachability set for a state model.

        This will be added to the state model itself.
	If there was a problem, a special "error" reachability
	set will be set for the model.
	(This allows errors to propogate in a healty way.)

*/
void 	BuildReachset(state_model *dsm);

/// Initialize options for reachability set generation.
void InitSSGen();

#endif
