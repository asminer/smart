
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

        void* addVanishing(shared_state*, bool &isnew, subengine::error &e);
        void* addTangible(shared_state*, bool &isnew, subengine::error &e);

        bool hasUnexploredVanishing();
        bool hasUnexploredTangible();

        void* getUnexploredVanishing(shared_state*, subengine::error &e);
        void* getUnexploredTangible(shared_state*, subengine::error &e);

        subengine::error addInitial(void*);
        subengine::error addTTedge(void* from, void* to);

        void clearVanishing();
        bool statesOnly();


    @return An appropriate error code

*/
template <class RG>
subengine::error
generateUnindexedRG(named_msg &debug, dsde_hlm &dsm, RG &rg)
{
  subengine::error status = subengine::Success;
  
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
      status = subengine::Engine_Failed;
      break;
    }
    bool newinit;
    if (xans.getBool()) {
      rg.addVanishing(curr_st, newinit, status);
      DCASSERT(subengine::Success == status);
    } else {
      void* tindex = rg.addTangible(curr_st, newinit, status);
      DCASSERT(subengine::Success == status);
      if (newinit && !rg.statesOnly()) {
        if ( (status = rg.addInitial(tindex)) ) break;
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
      status = subengine::Assertion_Failure;
      break;
    }
  } // for i

  //
  // Done with initial states, start exploration loop
  //
  bool current_is_vanishing = false;

  // Combined tangible + vanishing explore loop!
  void* this_index = 0;
  while (subengine::Success == status) {
    //
    // Check for signals
    //
    if (debug.caughtTerm()) {
      if (dsm.StartError(0)) {
        dsm.SendError("Process construction prematurely terminated");
        dsm.DoneError();
      }
      status = subengine::Terminated;
      continue;
    }

    //
    // Get next state to explore, with priority to vanishings.
    //
    if (rg.hasUnexploredVanishing()) {
      // explore next vanishing
      current_is_vanishing = true;
      rg.getUnexploredVanishing(curr_st, status);
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
        this_index = rg.getUnexploredTangible(curr_st, status);
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
      status = subengine::Engine_Failed;
      continue;
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
        status = subengine::Engine_Failed;
        break;
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
        status = subengine::Engine_Failed;
        break;
      }

      //
      // add reached state to appropriate set
      //
      bool next_is_new;
      bool next_is_vanishing;
      void* next_index;
      if ( (next_is_vanishing = xans.getBool()) ) {
        next_index = rg.addVanishing(next_st, next_is_new, status);
      } else {
        next_index = rg.addTangible(next_st, next_is_new, status);
      }
      if (status) break;

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
          status = subengine::Assertion_Failure;
          break;
        }
      } // if next_is_new

      if (rg.statesOnly() || next_is_vanishing) continue;

      //
      // Add edge to RG
      //
      if (this_index) {
        status = rg.addTTedge(this_index, next_index);
      } else {
        status = rg.addInitial(next_index);
      }
    } // for e

  } // while !error

  //
  // Cleanup
  //
  Nullify(x.current_state);
  Nullify(x.next_state);
  return status;
}





/**
    Generate reachability graph from a discrete-event high-level model.
    This is a template function.

    @param  debug Debugging channel
    @param  dsm   High-level model

    @param  smp   State sets and semi-Markov process.
                  The following methods are required.

                  bool addVanishing(shared_state*, subengine::error &e);
                  bool addTangible(shared_state*, subengine::error &e);
                  bool hasUnexploredVanishing();
                  bool hasUnexploredTangible();
                  subengine::error getUnexploredVanishing(shared_state*);
                  subengine::error getUnexploredTangible(shared_state*);
                  void clearVanishing();
                  bool statesOnly();
                  subengine::error addInitialVanishing(shared_state*, double wt);
                  subengine::error addInitialTangible(shared_state*, double wt);
                  subengine::error addTTEdge(shared_state* f, shared_state* t, double wt);
                  subengine::error addTVEdge(shared_state* f, shared_state* t, double wt);
                  subengine::error addVTEdge(shared_state* f, shared_state* t, double wt);
                  subengine::error addVVEdge(shared_state* f, shared_state* t, double wt);

    @return An appropriate error code

*/
template <class SMP>
subengine::error
generateUnindexedSMP(named_msg &debug, dsde_hlm &dsm, SMP &smp)
{
  return subengine::Engine_Failed;
}

