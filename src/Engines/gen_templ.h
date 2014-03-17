
// $Id$

/*
    Explicit generation template functions.

    States are "indexed" by a unique identifier.
*/


/**
    Generate reachability graph from a discrete-event high-level model.
    This is a template function, where parameter RG is a generic
    RG class (see required methods, below) and UID is the type of the
    unique identifier per states.


    @param  debug Debugging channel
    @param  dsm   High-level model

    @param  rg    State sets and reachability graph.
                  The following methods are required.

                  bool add(bool isVanishing, const shared_state*, UID &id);
                  bool hasUnexploredVanishing();
                  bool hasUnexploredTangible();
                  UID  getUnexploredVanishing(shared_state*);
                  UID  getUnexploredTangible(shared_state*);
                  void clearVanishing(named_msg &debug);
                  bool statesOnly();
                  void addInitial(const UID id);
                  void addEdge(UID from, UID to);
                  void show(OutputStream &s, bool isVanishing, const UID id, const shared_state* st);
                  UID  illegalID();

    @throws An appropriate error code

*/
template <class RG, typename UID>
void generateRGt(named_msg &debug, dsde_hlm &dsm, RG &rg)
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
      UID id;
      bool newinit = rg.add(xans.getBool(), curr_st, id);
      if (!xans.getBool()) {
        rg.addInitial(id);
      }
      if (debug.startReport()) {
        debug.report() << "Adding initial ";
        rg.show(debug.report(), xans.getBool(), id, curr_st);
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
    UID from_id;
    rg.makeIllegalID(from_id);
    bool valid_from = false;
    bool current_is_vanishing = false;

    // Combined tangible + vanishing explore loop!
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
      UID exp_id;
      if (rg.hasUnexploredVanishing()) {
        // explore next vanishing
        exp_id = rg.getUnexploredVanishing(curr_st);
        current_is_vanishing = true;
      } else {
        // No unexplored vanishing; safe to trash them, if desired
        if (current_is_vanishing) { // assumes we want to eliminate vanishing...
          rg.clearVanishing(debug);
        }
        current_is_vanishing = false;
        // find next tangible to explore; if none, break out
        if (rg.hasUnexploredTangible()) {
          from_id = exp_id = rg.getUnexploredTangible(curr_st);
          valid_from = true;
        } else {
          break;  // done exploring!
        }
      }
      if (debug.startReport()) {
        debug.report() << "Exploring ";
        rg.show(debug.report(), current_is_vanishing, exp_id, curr_st);
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
        bool next_is_new;
        UID next_id;
        bool next_is_vanishing = xans.getBool();
        next_is_new = rg.add(next_is_vanishing, next_st, next_id);

        //
        // Debug info
        //
        if (debug.startReport()) {
          debug.report() << "\t via event " << t->Name() << " to ";
          rg.show(debug.report(), next_is_vanishing, next_id, next_st);
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
        if (valid_from) {
          rg.addEdge(from_id, next_id);
        } else {
          // Must be exploring an initial vanishing state
          rg.addInitial(next_id);
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
    Generate semi-Markov process (vanishing chain) 
    from a discrete-event high-level model.
    This is a template function, where parameter SMP is a generic
    SMP class (see required methods, below) and UID is the type of the
    unique identifier per states.


    @param  debug Debugging channel
    @param  dsm   High-level model

    @param  rg    State sets and semi-Markov process.
                  The following methods are required.

                  bool add(bool isVanishing, const shared_state*, UID &id);
                  bool hasUnexploredVanishing();
                  bool hasUnexploredTangible();
                  UID  getUnexploredVanishing(shared_state*);
                  UID  getUnexploredTangible(shared_state*);
                  void eliminateVanishing(named_msg &debug);
                  bool statesOnly();
                  void addInitial(bool isVanishing, const UID id, double wt);
                  void addTTEdge(UID from, UID to, double wt);
                  void addTVEdge(UID from, UID to, double wt);
                  void addVTEdge(UID from, UID to, double wt);
                  void addVVEdge(UID from, UID to, double wt);
                  void show(OutputStream &s, bool isVanishing, const UID id, const shared_state* st);
                  UID  illegalID();

    @throws An appropriate error code

*/
template <class SMP, typename UID>
void generateSMPt(named_msg &debug, dsde_hlm &dsm, SMP &smp)
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
      UID id;
      bool newinit = smp.add(xans.getBool(), curr_st, id);
      smp.addInitial(xans.getBool(), id, wt);
      if (debug.startReport()) {
        debug.report() << "Adding initial ";
        smp.show(debug.report(), xans.getBool(), id, curr_st);
        debug.report() << " wt " << wt << "\n";
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
      UID from_id;

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
        from_id = smp.getUnexploredVanishing(curr_st);
        current_is_vanishing = true;
      } else {
        // No unexplored vanishing; safe to trash them, if desired
        if (current_is_vanishing) { // assumes we want to eliminate vanishing...
          smp.eliminateVanishing(debug);
        }
        current_is_vanishing = false;
        // find next tangible to explore; if none, break out
        if (smp.hasUnexploredTangible()) {
          from_id = smp.getUnexploredTangible(curr_st);
        } else {
          break;  // done exploring!
        }
      }
      if (debug.startReport()) {
        debug.report() << "Exploring ";
        smp.show(debug.report(), current_is_vanishing, from_id, curr_st);
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
        }

        //
        // add reached state to appropriate set
        //
        UID to_id;
        bool next_is_new;
        bool next_is_vanishing = xans.getBool();
        next_is_new = smp.add(next_is_vanishing, next_st, to_id);

        //
        // Debug info
        //
        if (debug.startReport()) {
          debug.report() << "\t via event " << t->Name();
          if (!smp.statesOnly()) debug.report() << " (" << weight << ")";
          debug.report() << " to ";
          smp.show(debug.report(), next_is_vanishing, to_id, next_st);
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

        if (smp.statesOnly()) continue;

        //
        // Add appropriate edge
        //
        if (current_is_vanishing) {
            if (next_is_vanishing)
                smp.addVVEdge(from_id, to_id, weight);
            else
                smp.addVTEdge(from_id, to_id, weight);
        } else {
            if (next_is_vanishing)
                smp.addTVEdge(from_id, to_id, weight);
            else
                smp.addTTEdge(from_id, to_id, weight);
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

