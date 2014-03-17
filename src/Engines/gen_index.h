
// $Id$

/*
    Explicit generation template functions based on state indexes.

    I.e., explicit generation engine for any data structure
    that allows us to immediately index states as they are discovered.
*/





/**
    Generate reachability graph from a discrete-event high-level model.
    This is a template function.

    @param  debug Debugging channel
    @param  dsm   High-level model

    @param  rg    State sets and reachability graph.
                  The following methods are required.

                  bool addVanishing(shared_state*, long &index);
                  bool addTangible(shared_state*, long &index);
                  bool hasUnexploredVanishing();
                  bool hasUnexploredTangible();
                  long getUnexploredVanishing(shared_state*);
                  long getUnexploredTangible(shared_state*);
                  void clearVanishing();
                  bool statesOnly();
                  subengine::error addInitial(long init_index);
                  subengine::error addEdge(long from_index, long to_index);

    @throws An appropriate error code

*/
template <class RG>
void generateIndexedRG(named_msg &debug, dsde_hlm &dsm, RG &rg)
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
      long ind;
      bool newinit;
      if (xans.getBool()) {
        newinit = rg.addVanishing(curr_st, ind);
      } else {
        newinit = rg.addTangible(curr_st, ind);
        if (!rg.statesOnly()) {
          rg.addInitial(ind);
        }
      }
      if (debug.startReport()) {
        debug.report() << "Adding initial";
        if (xans.getBool()) {
          debug.report() << " vanishing state# ";
        } else {
          debug.report() << " tangible  state# ";
        }
        debug.report().Put(ind, 4);
        debug.report() << " : ";
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
    long from_ind = -1;
    bool current_is_vanishing = false;

    // Combined tangible + vanishing explore loop!
    for (;;) {
      long exp_index;
  
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
        exp_index = rg.getUnexploredVanishing(curr_st);
        current_is_vanishing = true;
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
          exp_index = rg.getUnexploredTangible(curr_st);
          from_ind = exp_index;
        } else {
          break;
        }
      }
      if (debug.startReport()) {
        debug.report() << "Exploring";
        if (current_is_vanishing) {
          debug.report() << " vanishing state# ";
        } else {
          debug.report() << " tangible  state# ";
        }
        debug.report().Put(exp_index, 4);
        debug.report() << " : ";
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
            dsm.OutOfBoundsError(xans);
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
        long newindex;
        bool next_is_new;
        bool next_is_vanishing;
        if ( (next_is_vanishing = xans.getBool()) ) {
          next_is_new = rg.addVanishing(next_st, newindex);
        } else {
          next_is_new = rg.addTangible(next_st, newindex);
        }
        if (newindex < 0) {
          if (dsm.StartError(0)) {
            switch (newindex) {
              case -1:
                dsm.SendError("Couldn't find reachable state");
                break;
              case -3:
                dsm.SendError("Stack overflow");
                break;
              default:
                dsm.SendError("Out of memory");
            } // switch
            dsm.SendError(" during process generation");
            dsm.DoneError();
          } // if starterror
          if (-1 == newindex)   throw subengine::Engine_Failed;
          else                  throw subengine::Out_Of_Memory;
        } // if newindex < 0

        //
        // Debug info
        //
        if (debug.startReport()) {
          debug.report() << "\t via event " << t->Name() << " to ";
          if (next_is_vanishing) {
            debug.report() << " vanishing state #";
          } else {
            debug.report() << " tangible  state #";
          }
          debug.report().Put(newindex, 4);
          debug.report() << " : ";
          next_st->Print(debug.report(), 0);
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
        if (from_ind < 0) { // exploring from initial vanishing
          rg.addInitial(newindex);
        } else {
          rg.addEdge(from_ind, newindex);
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

                  bool addVanishing(shared_state*, long &index);
                  bool addTangible(shared_state*, long &index);
                  bool hasUnexploredVanishing();
                  bool hasUnexploredTangible();
                  long getUnexploredVanishing(shared_state*);
                  long getUnexploredTangible(shared_state*);
                  subengine::error eliminateVanishing();
                  bool statesOnly();
                  subengine::error addInitialVanishing(long index, double wt);
                  subengine::error addInitialTangible(long index, double wt);
                  subengine::error addTTEdge(long from, long to, double wt);
                  subengine::error addTVEdge(long from, long to, double wt);
                  subengine::error addVTEdge(long from, long to, double wt);
                  subengine::error addVVEdge(long from, long to, double wt);

    @throws An appropriate error code

*/
template <class SMP>
void generateIndexedSMP(named_msg &debug, dsde_hlm &dsm, SMP &smp)
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
      double wt = dsm.GetInitialState(i, curr_st);
      dsm.checkVanishing(x);
      if (!xans.isNormal()) {
        if (dsm.StartError(0)) {
          dsm.SendError("Couldn't determine vanishing / tangible");
          dsm.DoneError();
        }
        throw subengine::Engine_Failed;
      }
      long ind;
      bool newinit;
      if (xans.getBool()) {
        newinit = smp.addVanishing(curr_st, ind);
        if (!smp.statesOnly()) smp.addInitialVanishing(ind, wt);
      } else {
        newinit = smp.addTangible(curr_st, ind);
        if (!smp.statesOnly()) smp.addInitialTangible(ind, wt);
      }
      if (debug.startReport()) {
        debug.report() << "Adding initial";
        if (xans.getBool()) {
          debug.report() << " vanishing state# ";
        } else {
          debug.report() << " tangible  state# ";
        }
        debug.report().Put(ind, 4);
        debug.report() << " : ";
        curr_st->Print(debug.report(), 0);
        debug.report() << "\n";
        debug.stopIO();
      }
      if (!newinit) continue;
      dsm.checkAssertions(x);
      if (0==xans.getBool()) {
        throw subengine::Assertion_Failure;
        break;
      }
    } // for i

    //
    // Done with initial states, start exploration loop
    //
    bool current_is_vanishing = false;
    double weight = 0;

    // Combined tangible + vanishing explore loop!
    for (;;) {
      long exp_index;

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
      if (smp.hasUnexploredVanishing()) {
        // explore next vanishing
        exp_index = smp.getUnexploredVanishing(curr_st);
        current_is_vanishing = true;
      } else {
        // No unexplored vanishing; safe to trash them, if desired
        if (current_is_vanishing) { // assumes we want to eliminate vanishing...
          if (debug.startReport()) {
            debug.report() << "Eliminating vanishing states\n";
            debug.stopIO();
          }
          smp.eliminateVanishing();
        }
        current_is_vanishing = false;
        // find next tangible to explore; if none, break out
        if (smp.hasUnexploredTangible()) {
          exp_index = smp.getUnexploredTangible(curr_st);
        } else {
          break;
        }
      }
      if (debug.startReport()) {
        debug.report() << "Exploring";
        if (current_is_vanishing) {
          debug.report() << " vanishing state# ";
        } else {
          debug.report() << " tangible  state# ";
        }
        debug.report().Put(exp_index, 4);
        debug.report() << " : ";
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
        continue;
      }


      // 
      // Traverse enabled events
      //
      for (int e=0; e<enabled.Length(); e++) {
        model_event* t = enabled.Item(e);
        DCASSERT(t);
        DCASSERT(t->hasFiringType(model_event::Immediate) == current_is_vanishing);
        if (0==t->getNextstate())
          continue;  // firing is "no-op", don't bother

        //
        // t is enabled, fire and get new state
        //
        next_st->fillFrom(curr_st);
        t->getNextstate()->Compute(x);
        if (!xans.isNormal()) {
          if (dsm.StartError(0)) {
            dsm.SendError("Bad next-state expression for event ");
            dsm.SendError(t->Name());
            dsm.SendError(" during process generation");
            dsm.OutOfBoundsError(xans);
            dsm.DoneError();
          }
          throw subengine::Engine_Failed;
          break;
        }


        //
        // get the firing weight (if necessary)
        //
        if (!smp.statesOnly()) {
          if (current_is_vanishing) {
            if (enabled.Length()>1) {
              SafeCompute(t->getWeight(), x);
            } else {
              xans.setReal(1.0);
            } // if enabled.Length
          } else {
            x.which = traverse_data::ComputeExpoRate;
            SafeComputeExpoRate(t->getDistribution(), x);
            x.which = traverse_data::Compute;
          } // if current_is_vanishing
  
          // check result
          if (xans.isNormal() && xans.getReal() > 0.0) {
            weight = xans.getReal();
          } else {
            if (dsm.StartError(0)) {
              dsm.SendError("Bad value ");
              dsm.SendRealError(xans);
              if (current_is_vanishing) {
                dsm.SendError(" for weight of event ");
              } else {
                dsm.SendError(" for rate of event ");
              }
              dsm.SendError(t->Name());
              dsm.DoneError();
            }
            throw subengine::Engine_Failed;
            break;
          }

        } // if not states only


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
          break;
        }

        //
        // add reached state to appropriate set
        //
        long newindex;
        bool next_is_new;
        bool next_is_vanishing;
        if ( (next_is_vanishing = xans.getBool()) ) {
          next_is_new = smp.addVanishing(next_st, newindex);
        } else {
          next_is_new = smp.addTangible(next_st, newindex);
        }
        if (newindex < 0) {
          if (dsm.StartError(0)) {
            switch (newindex) {
              case -1:
                dsm.SendError("Couldn't find reachable state");
                break;
              case -3:
                dsm.SendError("Stack overflow");
                break;
              default:
                dsm.SendError("Out of memory");
            } // switch
            dsm.SendError(" during process generation");
            dsm.DoneError();
          } // if starterror
          if (-1==newindex)   throw subengine::Engine_Failed;
          else                throw subengine::Out_Of_Memory;
        } // if newindex < 0

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

        if (smp.statesOnly()) continue;

        //
        // Add appropriate edge
        //
        if (current_is_vanishing) {
            if (next_is_vanishing)
                smp.addVVEdge(exp_index, newindex, weight);
            else
                smp.addVTEdge(exp_index, newindex, weight);
        } else {
            if (next_is_vanishing)
                smp.addTVEdge(exp_index, newindex, weight);
            else
                smp.addTTEdge(exp_index, newindex, weight);
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

