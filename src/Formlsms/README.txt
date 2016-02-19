
# $Id$

A collection of modeling formalism "modules".

Generally speaking, source files have the following suffixes:

"_msr"    Front-end measures for multiple formalisms.
"_form"   For registering formalisms with the expression manager.
"_hlm"    For high-level "compiled" models (derived from hldsm).
"_llm"    For low-level models (derived from lldsm).
"_mdd"    For low-level models, using Meddly (TBD will change)


Specifically we have the following formalism support:

   basic_msr    Basic functions (num_states, num_levels, etc.) for formalisms.
     ctl_msr    Functions for CTL measures.
     dcp_msr    Functions for discrete constraint programming measures.
   stoch_msr    Functions for stochastic models (e.g., avg_ss).

    dcp_form    "Discrete constraint programming" formalism.
    fsm_form    Finite state machine formalism.
     mc_form    Markov chain formalism.
     pn_form    Petri net formalism.
    evm_form    Event-variable model formalism.
    tam_form    Tile assembly model formalism.

    enum_hlm    Enumerated high-level models (e.g., FSMs, MCs)
    dsde_hlm    Discrete-state, discrete-event models.
  noevnt_hlm    High-level models without events.
   phase_hlm    Phase-type models.

   state_llm    Models with reachable states
   graph_llm    Graph-based low-level models; checkable with logics (CTL).
   stoch_llm    Stochastic low-level models, for now Markov chains

     fsm_mdd    Low-level finite state machines, using Meddly.  Will be obsolete soon!

New reachability set stuff and process stuff:
  
    rss_indx    Base class for explicitly stored, indexed sets (rss_enum and rss_expl)
    rss_enum    User enumerated (e.g. within a dtmc model) reachability sets
    rss_expl    Explicitly stored reachability sets
  rss_meddly    Reachability sets stored as an MDD using the library MEDDLY

    rgr_ectl    Base class for reachgraphs that do "explicit" CTL.
   rgr_grlib    Reachability graphs based on the GraphLib data structure.
  rgr_meddly    Reachability graphs stored using MEDDLY

 proc_markov    Base class for stochastic processes that are Markov chains.
  proc_mclib    Markov chains (and reachability graphs) based on the MCLib Markov chain
 proc_meddly    Markov chains stored using MEDDLY


DEAD CODE (remove after debugging and testing)

     fsm_llm    Low-level finite state machines. 
      mc_llm    Low-level Markov chain class. 
   rgr_mclib    Reachability graphs based on the MCLib Markov chain.
