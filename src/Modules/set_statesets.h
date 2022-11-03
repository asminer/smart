/** \file set_set_statesets.h

    Module for set of set_statesets.
    Defines a set of set_stateset type, and appropriate operators.
*/

#ifndef SET_STATESETS_H
#define SET_STATESETS_H

#include "../include/shared.h"
#include "../Modules/statesets.h"


class expr;
class result;
class exprman;
class symbol_table;
class state_lldsm;
class hldsm;
class stateset;

// ******************************************************************
// *                                                                *
// *                         set_set_stateset class                     *
// *                                                                *
// ******************************************************************

/**
    Abstract base class for set of set_statesets.
    Derived classes for different implementations.
    Any expression of type set_stateset will build an object of this type
    (or derived class, of course).
*/

class set_stateset: public shared_object {
public:
	set_stateset(const state_lldsm* p);
protected:
	virtual ~set_stateset();

public:
	// what methods do we need for set of set_statesets?
	// intersection: Tset_stateset and initial -> property true
	//union
	
  inline const state_lldsm* getParent() const { return parent; }

  const hldsm* getGrandparent() const;

  /// Build a deep copy of this set_stateset
  virtual set_stateset* DeepCopy() const = 0; 

  /** Take the complement of this set_stateset, in place.
        @return true on success, false on error.
  */
  virtual bool Complement() = 0;

  /** Union us with another set_stateset, in place.
        @param  c   Expression source, for error reporting.
        @param  op  Operation name, for error reporting.
        @param  x   A set to union with us; should have the same parent.
        @return true on success, false on error.
  */
  virtual bool Union(const expr* c, const char* op, const set_stateset* x) = 0;

  inline bool Union(const expr* c, const set_stateset* x) {
    return Union(c, "union", x);
  }

  /** Intersect us with another set_stateset, in place.
        @param  c   Expression source, for error reporting.
        @param  op  Operation name, for error reporting.
        @param  x   A set to intersect with us; should have the same parent.
        @return true on success, false on error.
  */
  virtual bool Intersect(const expr* c, const char* op, const set_stateset* x) = 0;

  inline bool Intersect(const expr* c, const set_stateset* x) {
    return Intersect(c, "intersection", x);
  }

  /** Plus us with another set_stateset, in place.
      If states are attached with weights, apply an element-wise plus operation.
      Otherwise, apply an intersection operation.
          @param  c   Expression source, for error reporting.
          @param  op  Operation name, for error reporting.
          @param  x   A set to plus with us; should have the same parent.
          @return true on success, false on error.
    */
  virtual bool Plus(const expr* c, const char* op, const set_stateset* x) = 0;

  inline bool Plus(const expr* c, const set_stateset* x) {
    return Plus(c, "plus", x);
  }

  /** Get the set cardinality, as a long.
        @param  card
            Cardinality is output here.
            On overflow, \a card will be negative.
        @param i
        	Index of stateset for cardinality to compute
  */
  // virtual void getCardinality(long &card, int i) const = 0;

  /** Get the set cardinality, as a bigint.
        @param  x
            On output, x will be a bigint storing the cardinality of the set.
  */
  // virtual void getCardinality(result &x, int i) const = 0;

  /// Is the set empty?
  // virtual bool isEmpty() const = 0;

  /**
      Helper: check that A and B have the same parents.
      If not, print an appropriate error message.

        @param  c   Expression requiring these to match
        @param  op  Human readable operation name
        @param  A   First set_stateset 
        @param  B   Second set_stateset 

        @return true if the parents matched, false otherwise.
  */
  static bool parentsMatch(const expr* c, const char* op, set_stateset* A, set_stateset* B);

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
  friend class init_set_statesets;

};

#endif
