
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
      mc_llm    Low-level Markov chain class. (TBD: STILL NEEDED?)
   stoch_llm    Stochastic low-level models, for now Markov chains

     fsm_mdd    Low-level finite state machines, using Meddly.  Will be obsolete soon!

TBD: Kill or severly shrink mc_llm

New reachability set stuff and process stuff:
  
    rss_indx    Base class for explicitly stored, indexed sets (rss_enum and rss_expl)
    rss_enum    User enumerated (e.g. within a dtmc model) reachability sets
    rss_expl    Explicitly stored reachability sets
  rss_meddly    Stored as an MDD using the library MEDDLY

    rgr_ectl    Base class for reachgraphs that do "explicit" CTL.
   rgr_grlib    Reachability graphs based on the GraphLib data structure.

 proc_markov    Base class for stochastic processes that are Markov chains.
  proc_mclib    Markov chains (and reachability graphs) based on the MCLib Markov chain

TBD:

  rgr_meddly    Maybe split?  monolithic, etc. as separate classes?  
                Abstract base class to collect options?

     mc_expl    
   mc_meddly    TBD - can we extend the MCLib to include support for
                "abstract" MCs, kinda like the LSLib does?

DEAD CODE (remove after debugging and testing)

     fsm_llm    Low-level finite state machines. 
   rgr_mclib    Reachability graphs based on the MCLib Markov chain.
