
// $Id$

#include "ssgen.h"

option* StateSpace;

const int num_ss_options = 2;

option_const debug_ss("DEBUG", "Use splay tree and display states as they are generated");
option_const splay_ss("SPLAY", "Splay tree");

bool DebugReachset(state_model *dsm)
{
  if (Verbose.IsActive()) {
    Verbose << "Starting reachability set generation in debug mode\n";
    Verbose.flush();
  }
  return false;
}

bool SplayReachset(state_model *dsm)
{
  if (Verbose.IsActive()) {
    Verbose << "Starting reachability set generation using Splay tree\n";
    Verbose.flush();
  }
  return false;
}

bool BuildReachset(state_model *dsm)
{
  const option_const* ss_option = StateSpace->GetEnum();
  if (ss_option == &debug_ss)
    return DebugReachset(dsm); 
  if (ss_option == &splay_ss)
    return SplayReachset(dsm); 

  Internal.Start(__FILE__, __LINE__);
  Internal << "StateSpace option " << ss_option << " not handled";
  Internal.Stop();
  return false;
}

//#define DEBUG

void InitSSGen()
{
#ifdef DEBUG
  Output << "Initializing state space generation options\n";
#endif
  // StateSpace option
  option_const **sslist = new option_const*[num_ss_options];
  // these must be alphabetical
  sslist[0] = &debug_ss;
  sslist[1] = &splay_ss;
  StateSpace = MakeEnumOption("StateSpace", "Algorithm and data structure to use for state space generation", sslist, num_ss_options, &splay_ss);
  AddOption(StateSpace);
}

