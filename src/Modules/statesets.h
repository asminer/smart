
// $Id$

/** \file statesets.h

    Module for statesets.
    Defines a stateset type, and appropriate operators.
*/

#ifndef STATESETS_H
#define STATESETS_H

#ifndef INITIALIZERS_ONLY

#include "../include/shared.h"

class expr;
class result;
class exprman;
class symbol_table;
class state_lldsm;
class hldsm;

// #define NEW_STATESETS

#ifdef NEW_STATESETS

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
  friend void InitStatesets(exprman* em, symbol_table* st);
};

#else

class sv_encoder;
class intset;   // external, explicit library

// ******************************************************************
// *                                                                *
// *                         stateset class                         *
// *                                                                *
// ******************************************************************

/// Both explicit and implicit together.  For now.
/*
    TBD - this should become an abstract base class,
    containing a reachset object and a reachgraph object.
    Derived classes for explicit, meddly-based implementations.
    Virtual methods in this class for union, intersection, etc.
    Virtual methods in the reachgraph class for preimage, postimage, etc.
*/
class stateset : public shared_object {
  const state_lldsm* parent;
  bool is_explicit;
  intset* expl_data;
  sv_encoder* state_forest;
  shared_object* state_dd;
  sv_encoder* relation_forest;
  shared_object* relation_dd;
public:
  stateset(const state_lldsm* p, intset* e);
  stateset(const state_lldsm* p, sv_encoder* sf, shared_object* s,
                           sv_encoder* rf, shared_object* r);
  virtual ~stateset();
public:
  inline const state_lldsm* getParent() const { return parent; }
  inline bool isExplicit() const { return is_explicit; }
  inline bool isSymbolic() const { return !is_explicit; }
  inline const intset& getExplicit() const {
    DCASSERT(is_explicit);
    DCASSERT(expl_data);
    return *expl_data;
  }
  inline intset& changeExplicit() {
    DCASSERT(is_explicit);
    DCASSERT(1==numRefs());
    DCASSERT(expl_data);
    return *expl_data;
  }
  inline const shared_object* getStateDD() const {
    DCASSERT(!is_explicit);
    DCASSERT(state_dd);
    return state_dd;
  }
  inline shared_object* changeStateDD() {
    DCASSERT(!is_explicit);
    DCASSERT(1==numRefs());
    DCASSERT(state_dd);
    return state_dd;
  }
  inline sv_encoder* getStateForest() const {
    DCASSERT(!is_explicit);
    DCASSERT(state_forest);
    return state_forest;
  }
  inline shared_object* getRelationDD() {
    DCASSERT(!is_explicit);
    DCASSERT(relation_dd);
    return relation_dd;
  }
  inline sv_encoder* getRelationForest() {
    DCASSERT(!is_explicit);
    DCASSERT(relation_forest);
    return relation_forest;
  }

  // Handy stuff

  /** Get the set cardinality.
        @param  card
            Cardinality is output here.
            On overflow, \a card will be negative.
  */
  void getCardinality(long &card) const;

  /// Get the set cardinality, as a bigint.
  void getCardinality(result &x) const;

  /// Is the set empty?
  bool isEmpty() const;

  // required for shared_object
  virtual bool Print(OutputStream &s, int width) const;
  virtual bool Equals(const shared_object *o) const;
private:
  static bool print_indexes;
  friend void InitStatesets(exprman* em, symbol_table* st);
  friend stateset* Complement(exprman* em, const expr* c, stateset* x);
  bool print_explicit(OutputStream &s) const;
  bool print_symbolic(OutputStream &s) const;
};

/** Build the complement of the given stateset.
    Done "in place" if possible.
      @param  em  Expression manager, for error reporting.
      @param  c   Expression source, for error reporting.
      @param  x   Set to complement.  Will be deleted.
      @return A new set that is the complement of x.
*/
stateset* Complement(exprman* em, const expr* c, stateset* x);

/** Build the union of the given statesets.
      @param  em  Expression manager, for error reporting.
      @param  c   Expression source, for error reporting.
      @param  x   A set, will not be modified or deleted.
      @param  y   A set, will not be modified or deleted.
      @return A new set that is the union of x and y.
*/
stateset* Union(exprman* em, const expr* c, stateset* x, stateset* y);

/** Build the intersection of the given statesets.
      @param  em  Expression manager, for error reporting.
      @param  c   Expression source, for error reporting.
      @param  x   A set, will not be modified or deleted.
      @param  y   A set, will not be modified or deleted.
      @return A new set that is the intersection of x and y.
*/
stateset* Intersection(exprman* em, const expr* c, stateset* x, stateset* y);

#endif

#endif  // INITIALIZERS_ONLY

// **************************************************************************
// *                                                                        *
// *                               Front  end                               *
// *                                                                        *
// **************************************************************************

/** Initialize stateset module.
    Nice, minimalist front-end.
      @param  em  The expression manager to use.
      @param  st  Symbol table to add any stateset functions.
                  If 0, functions will not be added.
*/
void InitStatesets(exprman* em, symbol_table* st);

#endif
