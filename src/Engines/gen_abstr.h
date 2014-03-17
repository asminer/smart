
// $Id$

/*
    Explicit generation template functions NOT based on state indexes.

    I.e., explicit generation engine for data structures
    that do not index states (at least, not immediately).
*/





/**
    Generate reachability graph from a discrete-event high-level model.
    This is a template function.

    @param  debug Debugging channel
    @param  dsm   High-level model

    @param  rg    State sets and reachability graph.
                  The following methods are required.
                  Note the "void*" should be some type of
                  unique state identifier (can be a shared_state).

        void* addVanishing(shared_state*, bool &isnew);
        void* addTangible(shared_state*, bool &isnew);

        bool hasUnexploredVanishing();
        bool hasUnexploredTangible();

        void* getUnexploredVanishing(shared_state*);
        void* getUnexploredTangible(shared_state*);

        void addInitial(void*);
        void addTTedge(void* from, void* to);

        void clearVanishing();
        bool statesOnly();


    @throws An appropriate error code

*/
template <class RG>
void generateUnindexedRG(named_msg &debug, dsde_hlm &dsm, RG &rg)
{
  // allocate temporary states
  shared_state* curr_st = new shared_state(&dsm);
  shared_state* next_st = new shared_state(&dsm);

  // allocate list of enabled events
  List <model_event> enabled;

  // set up traverse data and such
  traverse_data x(traverse_data::Compute);
  result xans;

  x.answer = &xans;
  x.current_state = curr_st;
  x.next_state = next_st;

  try {

    //
    // Find and insert the initial states
    //
    for (int i=0; i<dsm.NumInitialStates(); i++) {
      dsm.GetInitialState(i, curr_st);
      dsm.checkVanishing(x);
      if (!xans.isNormal()) {
        if (dsm.StartError(0)) {
          dsm.SendError("Couldn't determine vanishing / tangible");
          dsm.DoneError();
        }
        throw subengine::Engine_Failed;
      }
      bool newinit;
      if (xans.getBool()) {
        rg.addVanishing(curr_st, newinit);
      } else {
        void* tindex = rg.addTangible(curr_st, newinit);
        if (newinit && !rg.statesOnly()) {
          rg.addInitial(tindex);
        }
      }
      if (debug.startReport()) {
        debug.report() << "Adding initial";
        if (xans.getBool()) {
          debug.report() << " vanishing state: ";
        } else {
          debug.report() << " tangible  state: ";
        }
        curr_st->Print(debug.report(), 0);
        debug.report() << "\n";
        debug.stopIO();
      }
      if (!newinit) continue;
      dsm.checkAssertions(x);
      if (0==xans.getBool()) {
        throw subengine::Assertion_Failure;
      }
    } // for i

    //
    // Done with initial states, start exploration loop
    //
    bool current_is_vanishing = false;

    // Combined tangible + vanishing explore loop!
    void* this_index = 0;
    for (;;) {
      //
      // Check for signals
      //
      if (debug.caughtTerm()) {
        if (dsm.StartError(0)) {
          dsm.SendError("Process construction prematurely terminated");
          dsm.DoneError();
        }
        throw subengine::Terminated;
      }

      //
      // Get next state to explore, with priority to vanishings.
      //
      if (rg.hasUnexploredVanishing()) {
        // explore next vanishing
        current_is_vanishing = true;
        rg.getUnexploredVanishing(curr_st);
      } else {
        // No unexplored vanishing; safe to trash them, if desired
        if (current_is_vanishing) { // assumes we want to eliminate vanishing...
          if (debug.startReport()) {
            debug.report() << "Eliminating vanishing states\n";
            debug.stopIO();
          }
          rg.clearVanishing();
        }
        current_is_vanishing = false;
        // find next tangible to explore; if none, break out
        if (rg.hasUnexploredTangible()) {
          this_index = rg.getUnexploredTangible(curr_st);
        } else {
          break;
        }
      }
      if (debug.startReport()) {
        debug.report() << "Exploring";
        if (current_is_vanishing) {
          debug.report() << " vanishing state: ";
        } else {
          debug.report() << " tangible  state: ";
        }
        curr_st->Print(debug.report(), 0);
        debug.report() << "\n";
        debug.stopIO();
      }

      //
      // Make enabling list
      //
      if (current_is_vanishing)   dsm.makeVanishingEnabledList(x, &enabled);
      else                        dsm.makeTangibleEnabledList(x, &enabled);
      if (!x.answer->isNormal()) {
        DCASSERT(1 == enabled.Length());
        if (dsm.StartError(0)) {
          dsm.SendError("Bad enabling expression for event ");
          dsm.SendError(enabled.Item(0)->Name());
          dsm.SendError(" during process generation");
          dsm.DoneError();
        }
        throw subengine::Engine_Failed;
      }

      // 
      // Traverse enabled events
      //
      for (int e=0; e<enabled.Length(); e++) {
        model_event* t = enabled.Item(e);
        DCASSERT(t);
        DCASSERT(t->actsLikeImmediate() == current_is_vanishing);
        /*
        if (0==t->getNextstate())
          continue;  // firing is "no-op", don't bother
        */

        //
        // t is enabled, fire and get new state
        //
        next_st->fillFrom(curr_st);
        if (t->getNextstate()) {
          t->getNextstate()->Compute(x);
        }
        if (!xans.isNormal()) {
          if (dsm.StartError(0)) {
            dsm.SendError("Bad next-state expression for event ");
            dsm.SendError(t->Name());
            dsm.SendError(" during process generation");
            dsm.DoneError();
          }
          throw subengine::Engine_Failed;
        }

        //
        // determine if the reached state is tangible or vanishing
        //
        SWAP(x.current_state, x.next_state);
        dsm.checkVanishing(x);
        SWAP(x.current_state, x.next_state);
        if (!xans.isNormal()) {
          if (dsm.StartError(0)) {
            dsm.SendError("Couldn't determine vanishing / tangible");
            dsm.DoneError();
          }
          throw subengine::Engine_Failed;
        }

        //
        // add reached state to appropriate set
        //
        bool next_is_new;
        bool next_is_vanishing;
        void* next_index;
        if ( (next_is_vanishing = xans.getBool()) ) {
          next_index = rg.addVanishing(next_st, next_is_new);
        } else {
          next_index = rg.addTangible(next_st, next_is_new);
        }

        //
        // Debug info
        //
        if (debug.startReport()) {
          debug.report() << "\t via event " << t->Name() << " to ";
          if (next_is_vanishing) {
            debug.report() << " vanishing state: ";
          } else {
            debug.report() << " tangible  state: ";
          }
          next_st->Print(debug.report(), 0);
          if (next_is_new) debug.report() << " (new)";
          debug.report() << "\n";
          debug.stopIO();
        }

        //
        // check assertions
        //
        if (next_is_new) {
          SWAP(x.current_state, x.next_state);
          dsm.checkAssertions(x);
          SWAP(x.current_state, x.next_state);
          if (0==xans.getBool()) {
            throw subengine::Assertion_Failure;
          }
        } // if next_is_new

        if (rg.statesOnly() || next_is_vanishing) continue;

        //
        // Add edge to RG
        //
        if (this_index) {
          rg.addTTedge(this_index, next_index);
        } else {
          rg.addInitial(next_index);
        }
      } // for e

    } // infinite loop

    //
    // Cleanup
    //
    Nullify(x.current_state);
    Nullify(x.next_state);
  } // try

  catch (subengine::error e) {
    //
    // Cleanup
    //
    Nullify(x.current_state);
    Nullify(x.next_state);

    throw e;
  }
}





/**
    Generate reachability graph from a discrete-event high-level model.
    This is a template function.

    @param  debug Debugging channel
    @param  dsm   High-level model

    @param  smp   State sets and semi-Markov process.
                  The following methods are required.

                  bool addVanishing(shared_state*);
                  bool addTangible(shared_state*);
                  bool hasUnexploredVanishing();
                  bool hasUnexploredTangible();
                  void getUnexploredVanishing(shared_state*);
                  void getUnexploredTangible(shared_state*);
                  void clearVanishing();
                  bool statesOnly();
                  void addInitialVanishing(shared_state*, double wt);
                  void addInitialTangible(shared_state*, double wt);
                  void addTTEdge(shared_state* f, shared_state* t, double wt);
                  void addTVEdge(shared_state* f, shared_state* t, double wt);
                  void addVTEdge(shared_state* f, shared_state* t, double wt);
                  void addVVEdge(shared_state* f, shared_state* t, double wt);

    @return An appropriate error code

*/
template <class SMP>
void generateUnindexedSMP(named_msg &debug, dsde_hlm &dsm, SMP &smp)
{
  throw  subengine::Engine_Failed;
}

