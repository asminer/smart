
// $Id$

/** \file expl_states.h

    Thin wrapper around explicit state storage library.

    Not so thin wrapper for substate storage.
*/

#ifndef EXPL_STATES_H
#define EXPL_STATES_H

#include "../ExprLib/exprman.h"

namespace StateLib {
  class state_db;
  class state_coll;
}
class hldsm;

/** Abstract base class for substate storage, by "levels".
*/
class substate_colls : public shared_object {
protected:
  int num_levels;
  bool is_static;
public:
  substate_colls(int K);
protected:
  virtual ~substate_colls();
public:
  inline int getNumLevels() const { return num_levels; }

  /** Search for a substate in collection \a k.
        @param  k       Collection number, should be 1 <= k <= num_levels.
        @param  state   Substate to search for.

        @return Index of this substate, or negative on error:
                  -1  if not found
                  -5  if static.
  */
  virtual long findSubstate(int k, const int* state, int size) = 0;

  /** Add a substate to collection \a k.
      If the substate is already present, it will not be added.
        @param  k       Collection number, should be 1 <= k <= num_levels.
        @param  state   Substate to add.

        @return Index of this substate, or negative on error:
                  -2  memory error
                  -3  stack overflow
                  -5  static
  */
  virtual long addSubstate(int k, const int* state, int size) = 0;

  /** Get the substate corresponding to an index.
        @param  k       Collection number, should be 1 <= k <= num_levels.
        @param  i       Index of substate to obtain.
        @param  state   Substate will be written here.
        @param  size    Length of state array.  

        @return Actual size of substate, or negative on error:
                  -1  index out of bounds
  */
  virtual int getSubstate(int k, long i, int* state, int size) const = 0;

  /** Get (one plus) the maximum allowed index for collection \a k.
        @param  k       Collection number, should be 1 <= k <= num_levels.

        @return Smallest I such that any index i for collection k 
                should be i < I.
  */
  virtual long getMaxIndex(int k) const = 0;

  /** Convert to a smaller, static representation.
      Once static, it is impossible to
      add or search for substates.
  */
  virtual void convertToStatic() = 0;

  /** Print a report about memory usage to the given stream.
  */
  virtual void Report(OutputStream &) const = 0;

  // required for shared objects

  virtual bool Print(OutputStream &, int) const;
  virtual bool Equals(const shared_object*) const;
};


/** Thin wrapper around state storage library.
*/
class exp_state_lib : public library {
public:
  exp_state_lib();
  virtual const char* getDBMethod() const = 0;
  virtual StateLib::state_db* 
    createStateDB(bool indexed, bool store_sizes) const = 0;
  virtual substate_colls* createSubstateDBs(int K, bool store_sizes) const = 0;
};

// ******************************************************************
// *                                                                *
// *                           Front  end                           *
// *                                                                *
// ******************************************************************


/** Initialize as necessary, and return the explicit state storage library.
    If the library has already been initialized, then
    the function returns immediately.
    Otherwise, we initialize the library using parameter \a em
    (and return 0 if this is 0).
      @param  em  Expression manager, or 0.
      @return Instance of the state library interface.
*/
const exp_state_lib* InitExplicitStateStorage(exprman* em);


/** Determine proper lexical ordering for a collection of states.

    @param  hm  High-level model that generated the states.
    @param  ss  Collection of states.
    @param  map Mapping is written here.
                On exit, map[i] gives the handle of
                the ith state in lexical order.
*/
void LexicalSort(const hldsm* hm, const StateLib::state_coll* ss, long* map);


/** Determine proper lexical ordering for a collection of states.

    @param  hm  High-level model that generated the states.
    @param  ss  Collection of states.
    @param  sh  State handle array, sh[i] gives handle to
                state i in collection ss.
    @param  map Mapping is written here.
                On exit, map[i] is a permutation so that
                sh[map[i]] is the handle for the ith state 
                in lexical order.
*/
void LexicalSort(const hldsm* hm, const StateLib::state_coll* ss, 
                  const long* sh, long* map);

#endif
