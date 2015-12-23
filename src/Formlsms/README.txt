
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

    dsde_hlm    Discrete-state, discrete-event models.
  noevnt_hlm    High-level models without events.
   phase_hlm    Phase-type models.

   check_llm    Abstract base class: models that are "checkable" (CTL).
     fsm_llm    Low-level finite state machines.
      mc_llm    Low-level Markov chain class.
   stoch_llm    Abstract base class: stochastic models.

     fsm_mdd    Low-level finite state machines, using Meddly.

Stuff in progress:
  
         rss    Abstract base class for reachability sets, used by fsm & mc
    rss_enum    Enumerated (explicitly declared) reachability sets
