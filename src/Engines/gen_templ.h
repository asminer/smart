/*
 Explicit generation template functions.

 States are "indexed" by a unique identifier.
 */
#include "../_StateLib/lchild_rsiblingt.h"

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
template<class RG, typename UID>
void generateRGt(named_msg &debug, dsde_hlm &dsm, RG &rg) {
	// allocate temporary states
	shared_state* curr_st = new shared_state(&dsm);
	shared_state* next_st = new shared_state(&dsm);

	// allocate list of enabled events
	List<model_event> enabled;

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
		for (int i = 0; i < dsm.NumInitialStates(); i++) {
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
			if (!newinit)
				continue;
			dsm.checkAssertions(x);
			if (0 == xans.getBool()) {
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
					dsm.SendError(
							"Process construction prematurely terminated");
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
			if (current_is_vanishing)
				dsm.makeVanishingEnabledList(x, &enabled);
			else
				dsm.makeTangibleEnabledList(x, &enabled);
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
			for (int e = 0; e < enabled.Length(); e++) {
				model_event* t = enabled.Item(e);
				DCASSERT(t);DCASSERT(t->actsLikeImmediate() == current_is_vanishing);

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
						dsm.SendError(
								"Couldn't determine vanishing / tangible");
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
					rg.show(debug.report(), next_is_vanishing, next_id,
							next_st);
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
					if (0 == xans.getBool()) {
						throw subengine::Assertion_Failure;
					}
				} // if next_is_new

				if (rg.statesOnly() || next_is_vanishing)
					continue;

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

		if (debug.startReport()) {
			debug.report() << "Done exploring\n";
			debug.stopIO();
		}

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

template<class CG, typename UID>
int isDeadend(List<model_event> e) {
	return e.Length();

}
template<class CG, typename UID>
bool SharedStateEqual(shared_state*node, shared_state* node1) {
	bool res = true;
	for (int i = 0; i < node->getNumStateVars(); i++) {
		if (node->get(i) != node1->get(i))
			return false;
	}
	return true;
}
template<class CG, typename UID>
bool isRepeated(List<shared_state>* slist, shared_state* node) {
	//printf("\n For Checking: In isRepeated %d", slist->Length());
	shared_state* temp;
	for (int i = 0; i < slist->Length(); i++) {
		bool res = false;
		for (int j = 0; j < slist->Item(i)->getNumStateVars(); j++) {
			if (slist->Item(i)->get(j) != node->get(j)) {
				res = true;
			}
		}
		if (res == false)
			return true;
	}

	return false;
}
template<class CG, typename UID>
void isLessthanNodeUpdate(List<shared_state>* slist, shared_state* node) {

	for (int i = 0; i < slist->Length(); i++) {
		bool res = true;
		for (int j = 0; j < slist->Item(i)->getNumStateVars(); j++) {
			if (slist->Item(i)->omega(j)) {
				if (node->omega(j))
					;
				else {
					res = false;
					break;
				}
			} else {
				if (node->omega(j))
					continue;
				else if (slist->Item(i)->get(j) > node->get(j))
					res = false;
			}

		}
		///update!
		if (res)
			for (int j = 0; j < slist->Item(i)->getNumStateVars(); j++) {
				if (!slist->Item(i)->omega(j))
					if (slist->Item(i)->get(j) < node->get(j))
						node->set_omega(j);
			}
	}

}
template<class CG, typename UID>
List<model_event> calcEnabledTransition(dsde_hlm &dsm, shared_state* curr_st,
		shared_state* next_st, CG &cg, List<model_event> enabled,
		named_msg &debug, bool& current_is_vanishing) {
	traverse_data x(traverse_data::Compute);
	result xans;

	x.answer = &xans;
	x.current_state = curr_st;
	x.next_state = next_st;

	UID from_id;
	cg.makeIllegalID(from_id);
	bool valid_from = false;
	current_is_vanishing = false;
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
		if (cg.hasUnexploredVanishing()) {
			// explore next vanishing
			exp_id = cg.getUnexploredVanishing(curr_st);
			current_is_vanishing = true;
		} else {
			// No unexplored vanishing; safe to trash them, if desired
			if (current_is_vanishing) { // assumes we want to eliminate vanishing...
				cg.clearVanishing(debug);
			}
			current_is_vanishing = false;
			// find next tangible to explore; if none, break out
			if (cg.hasUnexploredTangible()) {
				from_id = exp_id = cg.getUnexploredTangible(curr_st);
				valid_from = true;
			} else {
				break;  // done exploring!
			}
		}
		if (debug.startReport()) {
			debug.report() << "Exploring 1111";
			debug.report() << "\n";
			debug.stopIO();
		}

		//
		// Make enabling list
		//
		if (current_is_vanishing)
			dsm.makeVanishingEnabledList(x, &enabled);
		else
			dsm.makeTangibleEnabledList(x, &enabled);
	}

	return enabled;
}
//Coverability
//TODO need cleanup

template<class CG, typename UID>
lchild_rsiblingt* generateCGT(named_msg &debug, dsde_hlm &dsm, CG &rg,
		List<shared_state>* slist, lchild_rsiblingt* node, bool firstTime,
		shared_state*newcur) {
	if (node->val != NULL) {
		node->PrintState(node);

	}
	shared_state* curr_st = new shared_state(&dsm);
	shared_state* next_st = new shared_state(&dsm);
	if (!firstTime) {
		if (debug.startReport()) {
			debug.report() << "NOT FIRST TIME " << "\n";
			debug.stopIO();
		}

		curr_st->fillFrom(newcur);

		next_st->fillFrom(newcur);
		for (int i = 0; i < newcur->getNumStateVars(); i++) {
			if (newcur->omega(i)) {
				if (debug.startReport()) {
					debug.report() << "OMEGA RECIVED HERE!!!! " << "\n";
					debug.stopIO();
				}

				curr_st->set_omega(i);
				next_st->set_omega(i);
			}
		}
	}
	// allocate list of enabled events
	List<model_event> enabled;

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
		if (firstTime) {
			for (int i = 0; i < dsm.NumInitialStates(); i++) {
				dsm.GetInitialState(i, curr_st);
				dsm.checkVanishing(x);
				if (!xans.isNormal()) {
					if (dsm.StartError(0)) {
						dsm.SendError(
								"Couldn't determine vanishing / tangible");
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
					debug.report() << "COV Adding initial ";
					rg.show(debug.report(), xans.getBool(), id, curr_st);
					debug.report() << "\nThe id is :" << id << "\n";
					debug.stopIO();
				}
				if (!newinit)
					continue;
				dsm.checkAssertions(x);
				if (0 == xans.getBool()) {
					throw subengine::Assertion_Failure;
				}
				node->val=new shared_state(&dsm);
				node->val->fillFrom(curr_st);
				//node->val = curr_st;
				//node->val->fillFrom(curr_st);
				slist->Append(curr_st);
				if (debug.startReport()) {
					debug.report() << "CALLING FIRST TIME " << "\n";
					debug.stopIO();
				}
			}				// for i
		}
		{

			//Not the firstTime.
			//
			// Done with initial states, start exploration loop
			//
			UID from_id;
			rg.makeIllegalID(from_id);
			bool valid_from = false;
			bool current_is_vanishing = false;

			// Combined tangible + vanishing explore loop!
			/*for (;;)*/{
				//
				// Check for signals
				//
				if (debug.caughtTerm()) {
					if (dsm.StartError(0)) {
						dsm.SendError(
								"Process construction prematurely terminated");
						dsm.DoneError();
					}
					throw subengine::Terminated;
				}

				//
				// Get next state to explore, with priority to vanishings.
				//
				UID exp_id;
				bool unexploredvanishflag = false;
				bool unexploredtangibleflag = false;
				if (debug.startReport()) {
					debug.report() << "COv HEre in else part of recursive ";
					debug.report() << "\n";
					debug.stopIO();
				}

				//
				// Make enabling list
				//
				/*
				 TBD fix the next statment
				 */
				bool boolflag[curr_st->getNumStateVars()];
				/*
				 This ^ causes a compile error, because the array
				 dimension is not a constant known at compile time.
				 */
				bool myflag = false;
				for (int i = 0; i < curr_st->getNumStateVars(); i++) {
					if (curr_st->omega(i)) {
						myflag = true;
						boolflag[i] = true;
						curr_st->set_omega(i);

					} else
						boolflag[i] = false;
				}
				if (!myflag) {
					curr_st->Unset_omega();
				}
				if (current_is_vanishing) {
					dsm.makeVanishingEnabledList(x, &enabled);
					if (debug.startReport()) {
						debug.report() << "IS VANISH ";

						debug.report() << "\n";
						debug.stopIO();
					}
				} else {
					dsm.makeTangibleEnabledListCov(x, &enabled, boolflag);
					if (debug.startReport()) {
						debug.report() << "IS TANGIBLE ";

						debug.report() << "\n";
						debug.stopIO();
					}
				}
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
				if (debug.startReport()) {
					debug.report() << "COV number of enabled transition "
							<< enabled.Length();
					debug.report() << "\n";
					debug.stopIO();
				}
				if (enabled.Length() == 0 && !unexploredtangibleflag
						&& !unexploredvanishflag) {
					curr_st->set_type(1);
					return NULL;
				}

				for (int e = 0; e < enabled.Length(); e++) {
					model_event* t = enabled.Item(e);
					DCASSERT(t);DCASSERT(t->actsLikeImmediate() == current_is_vanishing);

					//
					// t is enabled, fire and get new state
					//

					next_st->fillFrom(curr_st);

					if (t->getNextstate()) {
						t->getNextstate()->Compute(x);
					}
					isLessthanNodeUpdate<CG, UID>(slist, next_st);

					if (isRepeated<CG, UID>(slist, next_st)) {
						if (debug.startReport()) {
							debug.report() << "REPEATED!! \n";
							debug.stopIO();
						}
						next_st->set_type(0);
						return NULL;

					}

					if (debug.startReport()) {
						debug.report() << "Got NEW STATE " << "\n";
						rg.show(debug.report(), 0, 0, next_st);
						debug.stopIO();
					}

					if (!xans.isNormal()) {
						if (dsm.StartError(0)) {
							dsm.SendError(
									"Bad next-state expression for event ");
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
							dsm.SendError(
									"Couldn't determine vanishing / tangible");
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
					lchild_rsiblingt* newnode = node->addtothisChild(node,
							next_st);
					newnode->val=new shared_state(&dsm);
					newnode->val->fillFrom(next_st);
					if (debug.startReport()) {
						debug.report() << "\t COV via event " << t->Name()
								<< " to ";
						rg.show(debug.report(), next_is_vanishing, next_id,
								next_st);

						//debug.report() <<next_st->omega(0)<<"\n";
						debug.stopIO();
					}
					//TODO NEEED TO MOVE TO BEST PLACE
					if (next_st->omega(0)) {
						if (debug.startReport()) {
							debug.report() << "\t COV IS OMEGA ";

							debug.report() << "\n";
							debug.stopIO();
						}
						//newnode->val->set_omega(0);
					}
					//
					// check assertions
					//
					if (next_is_new) {
						SWAP(x.current_state, x.next_state);
						dsm.checkAssertions(x);
						SWAP(x.current_state, x.next_state);
						if (0 == xans.getBool()) {
							throw subengine::Assertion_Failure;
						}
					} // if next_is_new

					if (rg.statesOnly() || next_is_vanishing)
						continue;

					//
					// Add edge to RG
					//
					if (valid_from) {

						rg.addEdge(from_id, next_id);
					} else {
						// Must be exploring an initial vanishing state
						rg.addInitial(next_id);
					}
					slist->Append(next_st);
					if (debug.startReport()) {
						debug.report() << "Recursion Call " << "\n";
						debug.stopIO();
					}
					generateCGT<CG, UID>(debug, dsm, rg, slist, newnode, false,
							next_st);

				} // for e

			} // infinite loop

			if (debug.startReport()) {
				debug.report() << "COV Done exploring\n";
				debug.stopIO();
			}
			//
			// Cleanup
			//
			//node->PrintState(node);

			Nullify(x.current_state);
			Nullify(x.next_state);
		}
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

 @param  smp   State sets and semi-Markov process.
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
template<class SMP, typename UID>
void generateSMPt(named_msg &debug, dsde_hlm &dsm, SMP &smp) {
	// allocate temporary states
	shared_state* curr_st = new shared_state(&dsm);
	shared_state* next_st = new shared_state(&dsm);

	// allocate list of enabled events
	List<model_event> enabled;

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
		for (int i = 0; i < dsm.NumInitialStates(); i++) {
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
			if (!newinit)
				continue;
			dsm.checkAssertions(x);
			if (0 == xans.getBool()) {
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
					dsm.SendError(
							"Process construction prematurely terminated");
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
				smp.show(debug.report(), current_is_vanishing, from_id,
						curr_st);
				debug.report() << "\n";
				debug.stopIO();
			}

			//
			// Make enabling list
			//
			if (current_is_vanishing)
				dsm.makeVanishingEnabledList(x, &enabled);
			else
				dsm.makeTangibleEnabledList(x, &enabled);
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
			for (int e = 0; e < enabled.Length(); e++) {
				model_event* t = enabled.Item(e);
				DCASSERT(t);DCASSERT(t->hasFiringType(model_event::Immediate) == current_is_vanishing);
				if (0 == t->getNextstate())
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
						if (enabled.Length() > 1) {
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
						dsm.SendError(
								"Couldn't determine vanishing / tangible");
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
					if (!smp.statesOnly())
						debug.report() << " (" << weight << ")";
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
					if (0 == xans.getBool()) {
						throw subengine::Assertion_Failure;
					}
				} // if next_is_new

				if (smp.statesOnly())
					continue;

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

		if (debug.startReport()) {
			debug.report() << "Done exploring\n";
			debug.stopIO();
		}

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
 Generate a Markov chain from a discrete-event high-level model.
 Differs from the SMP version, in that, we follow vanishing
 edges until we reach tangible states, so there cannot be any
 cycles of vanishing states.
 This is a template function, where parameter MC is a generic
 MC class (see required methods, below) and UID is the type of the
 unique identifier per states.


 @param  debug Debugging channel
 @param  dsm   High-level model

 @param  mc    State sets and Markov chain.
 The following methods are required.

 bool add(bool isVan, const shared_state*, UID &id);
 bool hasUnexploredTangible();
 UID  getUnexploredTangible(shared_state*);
 bool statesOnly();
 void addInitial(bool isVan, const UID id, double wt);
 void addTTEdge(UID from, UID to, double wt);
 void show(OutputStream &s, bool isVan, const UID id, const shared_state* st);
 void show(OutputStream &s, const shared_state* st);
 void show(OutputStream &s, const UID id);
 void makeIllegalID(UID &);

 @throws An appropriate error code

 */
template<class MC, typename UID>
void generateMCt(named_msg &debug, dsde_hlm &dsm, MC &mc) {
	//
	// Build a buffer of states and incoming rates
	//
	const int BUFSIZE = 2048; // TBD - make this an option

	shared_state** statelist = new shared_state*[BUFSIZE];
	double* weightlist = new double[BUFSIZE];
	// bool* isvanlist = new bool[BUFSIZE];
	for (int i = 0; i < BUFSIZE; i++) {
		statelist[i] = new shared_state(&dsm);
	}
	List<model_event> enabled;

	int curr = 0; // index of current state in statelist

	// set up traverse data and such
	traverse_data x(traverse_data::Compute);
	result xans;

	x.answer = &xans;
	x.next_state = 0;

	try {
		//
		// Find and insert the initial states
		//
		for (int i = 0; i < dsm.NumInitialStates(); i++) {
			weightlist[curr] = dsm.GetInitialState(i, statelist[curr]);
			x.current_state = statelist[curr];
			dsm.checkAssertions(x);
			if (0 == xans.getBool()) {
				throw subengine::Assertion_Failure;
			}
			dsm.checkVanishing(x);
			if (!xans.isNormal()) {
				if (dsm.StartError(0)) {
					dsm.SendError("Couldn't determine vanishing / tangible");
					dsm.DoneError();
				}
				throw subengine::Engine_Failed;
			}

			if (xans.getBool()) {
				//
				// Vanishing, "push"
				//
				if (debug.startReport()) {
					debug.report() << "Pushing initial vanishing ";
					mc.show(debug.report(), statelist[curr]);
					debug.report() << " wt " << weightlist[curr] << "\n";
					debug.stopIO();
				}
				// isvanlist[curr] = true;
				curr++;
				if (curr >= BUFSIZE) {
					if (dsm.StartError(0)) {
						dsm.SendError("Vanishing stack overflow");
						dsm.DoneError();
					}
					throw subengine::Engine_Failed;
				}
				continue;
			}
			//
			// Tangible
			//
			UID id;
			mc.add(false, statelist[curr], id);
			mc.addInitial(false, id, weightlist[curr]);
			if (debug.startReport()) {
				debug.report() << "Adding initial ";
				mc.show(debug.report(), false, id, statelist[curr]);
				debug.report() << " wt " << weightlist[curr] << "\n";
				debug.stopIO();
			}
		} // for i

		//
		// Done with initial states, start exploration loop
		//
		bool current_is_vanishing = false;
		UID fromID;
		mc.makeIllegalID(fromID);
		// int from = -1;

		for (;;) {
			//
			// Check for signals
			//
			if (debug.caughtTerm()) {
				if (dsm.StartError(0)) {
					dsm.SendError(
							"Process construction prematurely terminated");
					dsm.DoneError();
				}
				throw subengine::Terminated;
			}

			//
			// Get next state to explore, with priority to vanishing stack
			//
			if (curr) {
				// pop vanishing
				curr--;
				current_is_vanishing = true;
			} else {
				// grab tangible
				if (mc.hasUnexploredTangible()) {
					// from = curr;
					fromID = mc.getUnexploredTangible(statelist[curr]);
					current_is_vanishing = false;
				} else {
					break;  // Done exploring!
				}
			}
			if (debug.startReport()) {
				debug.report() << "Exploring ";
				mc.show(debug.report(), current_is_vanishing, fromID,
						statelist[curr]);
				debug.report() << "\n";
				debug.stopIO();
			}

			//
			// Make enabling list
			//
			x.current_state = statelist[curr];
			if (current_is_vanishing)
				dsm.makeVanishingEnabledList(x, &enabled);
			else
				dsm.makeTangibleEnabledList(x, &enabled);
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
			// Get stack ready for new states
			//
			int next = curr;

			//
			// Traverse enabled events
			//
			for (int e = 0; e < enabled.Length(); e++) {
				model_event* t = enabled.Item(e);
				DCASSERT(t);DCASSERT(t->hasFiringType(model_event::Immediate) == current_is_vanishing);
				if (0 == t->getNextstate())
					continue;  // firing is "no-op", don't bother

				//
				// Next slot on the "stack"
				//
				next++;
				if (next >= BUFSIZE) {
					if (dsm.StartError(0)) {
						dsm.SendError("Vanishing stack overflow");
						dsm.DoneError();
					}
					throw subengine::Engine_Failed;
				}

				//
				// t is enabled, fire and get new state
				//
				x.next_state = statelist[next];
				statelist[next]->fillFrom(statelist[curr]);
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
				if (mc.statesOnly()) {
					weightlist[next] = 1.0;
				} else {
					if (current_is_vanishing) {
						if (enabled.Length() > 1) {
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
						weightlist[next] = xans.getReal();
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
				// Debug info
				//
				if (debug.startReport()) {
					debug.report() << "\t via event " << t->Name();
					if (!mc.statesOnly()) {
						debug.report() << " (" << weightlist[next] << ")";
					}
					debug.report() << " to ";
					mc.show(debug.report(), statelist[next]);
					debug.report() << "\n";
					debug.stopIO();
				}
			} // for enabled event e

			//
			// The stack, between curr and next, are the markings just added
			//

			//
			// For vanishing states, get the total weight and normalize
			//
			if (current_is_vanishing) {
				double wtotal = 0;
				for (int i = next; i > curr; i--) {
					wtotal += weightlist[i];
				}
				double inrate = weightlist[curr] / wtotal;
				for (int i = next; i > curr; i--) {
					weightlist[i] *= inrate;
				}
			}

			//
			// Process the list we just built.
			// Vanishing states are added to the stack.
			// Tangible states are processed as usual.
			//
			for (int i = curr + 1; i <= next; i++) {
				UID toID;
				bool next_is_new;
				bool next_is_vanishing;

				//
				// Determine vanishing/tangible
				//
				x.current_state = statelist[i];
				dsm.checkVanishing(x);
				if (!xans.isNormal()) {
					if (dsm.StartError(0)) {
						dsm.SendError(
								"Couldn't determine vanishing / tangible");
						dsm.DoneError();
					}
					throw subengine::Engine_Failed;
				}
				next_is_vanishing = xans.getBool();

				//
				// Add state, if tangible
				//
				if (next_is_vanishing) {
					mc.makeIllegalID(toID);
					next_is_new = true;
				} else {
					next_is_new = mc.add(false, statelist[i], toID);
				}

				//
				// check assertions
				//
				if (next_is_new) {
					dsm.checkAssertions(x);
					if (0 == xans.getBool()) {
						throw subengine::Assertion_Failure;
					}
				} // if next_is_new

				if (next_is_vanishing) {
					//
					// Add to stack
					//
					SWAP(statelist[curr], statelist[i]);
					weightlist[curr] = weightlist[i];
					curr++;
				} else {
					//
					// Add edge to this tangible states
					//
					if (!mc.statesOnly()) {
						mc.addTTEdge(fromID, toID, weightlist[i]);

						if (debug.startReport()) {
							debug.report() << "Adding MC edge from ";
							mc.show(debug.report(), fromID);
							debug.report() << " rate " << weightlist[i]
									<< " to ";
							mc.show(debug.report(), toID);
							debug.report() << "\n";
							debug.stopIO();
						}
					}
				}

			} // for i

		} // infinite loop

		if (debug.startReport()) {
			debug.report() << "Done exploring\n";
			debug.stopIO();
		}

		//
		// Cleanup
		//
		for (int i = 0; i < BUFSIZE; i++) {
			Delete(statelist[i]);
		}
		delete[] statelist;
		delete[] weightlist;
	} catch (...) {
		//
		// Cleanup
		//
		for (int i = 0; i < BUFSIZE; i++) {
			Delete(statelist[i]);
		}
		delete[] statelist;
		delete[] weightlist;

		throw;
	}

}
