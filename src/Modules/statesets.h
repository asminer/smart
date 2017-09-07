
/** \file statesets.h

    Module for statesets.
    Defines a stateset type, and appropriate operators.
*/

#ifndef STATESETS_H
#define STATESETS_H

#include "../include/shared.h"

class expr;
class result;
class exprman;
class symbol_table;
class state_lldsm;
class hldsm;

// ******************************************************************
// *                                                                *
// *                         stateset class                         *
// *                                                                *
// ******************************************************************

/**
    Abstract base class for statesets.
    Derived classes for different implementations.
    Any expression of type stateset will build an object of this type
    (or derived class, of course).
*/
class stateset : public shared_object {
public:
  stateset(const state_lldsm* p);
  stateset(const stateset* clone);
protected:
  virtual ~stateset();
public:
  inline const state_lldsm* getParent() const { return parent; }

  const hldsm* getGrandparent() const;

  /// Build a deep copy of this stateset
  virtual stateset* DeepCopy() const = 0; 

  /** Take the complement of this stateset, in place.
        @return true on success, false on error.
  */
  virtual bool Complement() = 0;

  /** Union us with another stateset, in place.
        @param  c   Expression source, for error reporting.
        @param  op  Operation name, for error reporting.
        @param  x   A set to union with us; should have the same parent.
        @return true on success, false on error.
  */
  virtual bool Union(const expr* c, const char* op, const stateset* x) = 0;

  inline bool Union(const expr* c, const stateset* x) {
    return Union(c, "union", x);
  }

  /** Intersect us with another stateset, in place.
        @param  c   Expression source, for error reporting.
        @param  op  Operation name, for error reporting.
        @param  x   A set to intersect with us; should have the same parent.
        @return true on success, false on error.
  */
  virtual bool Intersect(const expr* c, const char* op, const stateset* x) = 0;

  inline bool Intersect(const expr* c, const stateset* x) {
    return Intersect(c, "intersection", x);
  }

  /** Get the set cardinality, as a long.
        @param  card
            Cardinality is output here.
            On overflow, \a card will be negative.
  */
  virtual void getCardinality(long &card) const = 0;

  /** Get the set cardinality, as a bigint.
        @param  x
            On output, x will be a bigint storing the cardinality of the set.
  */
  virtual void getCardinality(result &x) const = 0;

  /// Is the set empty?
  virtual bool isEmpty() const = 0;

  /** Get a single state from the stateset.
      Mostly used when the stateset contains only one state.
      @return One state in the stateset.
   */
  virtual shared_state* getSingleState() const {
    return 0;
  }

  /** Randomly select one state.
   */
  virtual void Select() {
    return;
  }

  /**
      Helper: check that A and B have the same parents.
      If not, print an appropriate error message.

        @param  c   Expression requiring these to match
        @param  op  Human readable operation name
        @param  A   First stateset 
        @param  B   Second stateset 

        @return true if the parents matched, false otherwise.
  */
  static bool parentsMatch(const expr* c, const char* op, stateset* A, stateset* B);

  /**
      Helper: print error message that storage types mismatch

        @param  c   Expression requiring them to match
        @param  op  Human readable operation name
  */
  static void storageMismatchError(const expr* c, const char* op);

protected:
  static bool printIndexes() {
    return print_indexes;
  }

private:
  static exprman* em;
  const state_lldsm* parent;
  static bool print_indexes;
  friend class init_statesets;
};

#endif
