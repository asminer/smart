
// $Id$

#ifndef MOD_VARS_H
#define MOD_VARS_H

/** \file mod_vars.h

    Classes that define, and construct, variables inside models.

    Used heavily by specific formalisms in the "Formalism module".

 */

#include "symbols.h"

class model_instance;
class shared_set;
class hldsm;
  
// ******************************************************************
// *                                                                *
// *                        model_var  class                        *
// *                                                                *
// ******************************************************************

/** Base class of variables defined within a model.
    These exist only in a model instance.
*/
class model_var : public symbol {
public:
  /// Method by which this variable is stored in the global state.
  enum storage {
    /// Don't know yet
    Unknown,
    /// Some kind of error occurred
    Error,
    /// The variable does not contain any state information.
    No_State,
    /// The variable contains state information, but it is redundant.
    Implicit,
    /// The variable requires a single integer of state.
    Integer,
    /// The variable requires a list of integers (of varying length).
    Int_List,
  };
private:
  storage state_type;
protected:
  const model_instance* parent;
public:
  model_var(const symbol* wrapper, const model_instance* p);
  model_var(const char* fn, int line, const type* t, char* n, 
            const model_instance* p);
protected:
  virtual ~model_var();
  inline void setStateType(storage t) {
    DCASSERT(Unknown == state_type);
    state_type = t;
  }
public:
  inline const model_instance* getParent() const { return parent; }

  inline storage getStateType() const { return state_type; }

  virtual void Compute(traverse_data &x);

  /** Compute the value of this state variable, in the given state.
        @param  x   Traverse structure.
                    Sets "answer" to the value of this state
                    variable according to "current_state".
  */
  virtual void ComputeInState(traverse_data &x) const;

  /** Add delta to this state variable.
        @param  x   Traverse structure.
                    "answer" used to catch any errors.
                    "next_state" is set to value according to
                    "current_state", plus delta.

        @param  delta   Amount to change the state variable.
  */
  virtual void AddToState(traverse_data &x, long delta) const;

  /** Set the state variable.
        @param  x   Traverse structure.
                    "answer" is used to catch any errors.

        @param  ns  Next state structure to use for result.

        @param  rhs Value to set in \a ns.
  */
  virtual void SetNextState(traverse_data &x, shared_state* ns, long rhs) const;
  
  /** Set the next state as "unknown".
        @param  x   Traverse structure.
                    "answer" is used to catch any errors.

        @param  ns  Next state structure to use for result.
  */
  virtual void SetNextUnknown(traverse_data &x, shared_state* ns) const;

};


// ******************************************************************
// *                                                                *
// *                      model_statevar class                      *
// *                                                                *
// ******************************************************************

/** Base class of model state variables.
    These are model variables that are part of the model's state.
*/
class model_statevar : public model_var {
private:
  static result tempresult; // used for checking bounds

protected:
  /** Linkage between state and variables.
      If this variable requires state, then this is the index
      of the variable within a state array.
  */
  int state_index;

  /// Partition info (for structured approaches).
  int part_index;  

  /** Bounds for the variable, if any.
      NULL indicates no known bounds.
      Otherwise the variable must assume some
      value within the set.
  */
  shared_set* bounds;
  
public:
  model_statevar(const symbol* wrapper, const model_instance* p, shared_object* bnds);
  model_statevar(const char* fn, int line, const type* t, char* n,
                  const model_instance* p, shared_object* bnds);
protected:
  void Init(shared_object* bnds);
  virtual ~model_statevar();
public:
  inline int GetIndex() const { return state_index; }
  inline void SetIndex(int si) {
    DCASSERT(si >= 0);
    state_index = si;
  }
  inline int GetPart() const { return part_index; }
  inline void SetPart(int p) { part_index = p; }
  virtual bool HasBounds() const;
  inline void SetBounds(shared_set* b) {
    DCASSERT(0==bounds);
    bounds = b;
  }
  virtual long NumPossibleValues() const;
  virtual void SetToValueNumber(long i);
  virtual void GetValueNumber(long i, result& foo) const;
  virtual long GetValueIndex(result& val) const;
  virtual void ComputeInState(traverse_data &x) const;
  virtual void AddToState(traverse_data &x, long delta) const;
  virtual void SetNextState(traverse_data &x, shared_state* ns, long rhs) const;
  virtual void SetNextUnknown(traverse_data &x, shared_state* ns) const;
  void printBoundsError(const result &x) const;
protected:
  void ownerError(traverse_data &x) const;
  inline void boundsError(traverse_data &x, long badval) const {
    DCASSERT(x.answer);
    x.answer->setOutOfBounds(this, badval);
  }
};

// ******************************************************************
// *                                                                *
// *                     model_enum_value class                     *
// *                                                                *
// ******************************************************************

/** Model variables that are really enumerated values for a state variable.
    For instance, declared "states" in a Markov chain.
*/
class model_enum_value : public model_var {
protected:
  int index;
public:
  model_enum_value(const symbol* wrapper, const model_instance* p, int ndx);
protected:
  virtual ~model_enum_value();
public:

  inline int GetIndex() const { return index; }
  inline void SetIndex(int ndx) { 
    DCASSERT(ndx >= 0);
    index = ndx; 
  }

  inline int Compare(const model_enum_value* s) {
    DCASSERT(s);
    return index - s->index;
  }
};


// ******************************************************************
// *                                                                *
// *                        model_enum class                        *
// *                                                                *
// ******************************************************************

/** Model state variables that are one of an enumerated set.
    For instance, the state of a Markov chain.
*/
class model_enum : public model_statevar {
protected:
  long* indexes;
  model_enum_value** values;
  int num_values;
public:
  /** Constructor.
        @param  wrapper Name and type.
        @param  p       Parent model.
        @param  List    A list of the possible enumerated values
                        (of type model_enum_value), in reverse order.
  */
  model_enum(const symbol* wrapper, const model_instance* p, symbol* list);
protected:
  virtual ~model_enum();
public:
  inline model_enum_value* GetValue(int n) {
    CHECK_RANGE(0, n, num_values);
    DCASSERT(values);
    return values[n];
  }
  inline const model_enum_value* ReadValue(int n) const {
    CHECK_RANGE(0, n, num_values);
    DCASSERT(values);
    return values[n];
  }
  inline int NumValues() const { return num_values; }

  /** Sort the values, but not really.
        @param  indexes An array of dimension NumValues() that will
                        be written to.  On return, indexes[i] will give the
                        index of the \a i th "smallest" value (in alpha order).
  */
  void MakeSortedMap(long* indexes);


  // Required for sorting...
  inline int Compare(long i, long j) {
    DCASSERT(indexes);
    CHECK_RANGE(0, i, num_values);
    CHECK_RANGE(0, j, num_values);
    return strcmp(values[indexes[i]]->Name(), values[indexes[j]]->Name());
  }
  inline void Swap(long i, long j) {
    DCASSERT(indexes);
    CHECK_RANGE(0, i, num_values);
    CHECK_RANGE(0, j, num_values);
    SWAP(indexes[i], indexes[j]);
  }
};


// ******************************************************************
// *                                                                *
// *                       int_statevar class                       *
// *                                                                *
// ******************************************************************

/**
     Model state variables that have type "integer".
*/
class int_statevar : public model_statevar {
public:
  long value;
public:
  int_statevar(const symbol* wrapper, const model_instance* p, shared_object* bnds);
  virtual void Compute(traverse_data &x);

  virtual void SetToValueNumber(long i);
};


// ******************************************************************
// *                                                                *
// *                      bool_statevar  class                      *
// *                                                                *
// ******************************************************************

/**
     Model state variables that have type "boolean".
*/
class bool_statevar : public model_statevar {
public:
  bool value;
public:
  bool_statevar(const symbol* wrapper, const model_instance* p);
  virtual void Compute(traverse_data &x);
  virtual bool HasBounds() const;
  virtual long NumPossibleValues() const;
  virtual void SetToValueNumber(long i);
  virtual void GetValueNumber(long i, result& foo) const;
  virtual long GetValueIndex(result& val) const;
};


// ******************************************************************
// *                                                                *
// *                       shared_state class                       *
// *                                                                *
// ******************************************************************

/** Generic state class; this is the state of a model (regardless).

    Conceptually, a state consists of N "buckets",
    where each bucket holds either a single integer,
    or a variable-length list of integers (useful for colored Nets).

    Variable-length lists are not yet fully implemented.
    Ideas:
      * keep the integers first, lists last.
      * For packing / unpacking, no need to touch the integers.
*/
class shared_state : public shared_object {
  /// Owner of this state.
  const hldsm* parent;

  /// Fixed number of "state variables".
  int num_buckets;

  /** For each state variable, is its value unknown?
      Allows us to express partial knowledge of the state.
      If this is a null pointer, then every state variable is known.
  */
  bool* is_unknown;

  /** For each state variable, is it a list?  
      If not, it is an integer.
      If this is a null pointer, then every state variable is an integer.
  */
  bool* is_list;

  /** Pointer to data for each state variable.
      Only necessary if there are list state variables.
  */
  int* bucket_ptr;

  /// State data.
  int* data;

  /// Substate offsets.
  int* substate_offset;

  /// Number of substates.
  int num_substates;

  /** Next pointers.
      Only necessary if there are list state variables.
  */
  int* next;

  /** Current dimension of arrays \a data and \a next.
      If all state variables are integers, then this
      will equal \a num_buckets.
  */
  int data_alloc;

  /** Current usage of array \a data.
      If all state variables are integers, then this
      will equal \a num_buckets.
  */
  int data_size;

public:
  shared_state(const hldsm* p);
protected:
  virtual ~shared_state();
public:
  inline const hldsm* Parent() const { return parent; }
  inline const int* readState() const { return data; }
  inline int* writeState() { return data; }
  inline int readSubstateSize(int i) const { 
    DCASSERT(substate_offset);
    CHECK_RANGE(1, i, 1+num_substates);
    return substate_offset[i-1] - substate_offset[i];
  }
  inline const int* readSubstate(int i) const {
    DCASSERT(data);
    DCASSERT(substate_offset);
    CHECK_RANGE(1, i, 1+num_substates);
    return data + substate_offset[i];
  }
  inline int* writeSubstate(int i) {
    DCASSERT(data);
    DCASSERT(substate_offset);
    CHECK_RANGE(1, i, 1+num_substates);
    return data + substate_offset[i];
  }

  /// Are there any buckets with lists?
  inline bool isFixedSize() const { return 0==is_list; }

  /// Get number of buckets
  inline int getNumStateVars() const { return num_buckets; }

  /// Get storage space required for state
  inline int getStateSize() const { return data_size; }

  /// Copy from another state
  void fillFrom(const shared_state &s);

  inline void fillFrom(const shared_state *s) {
    DCASSERT(s);
    fillFrom(*s);
  }
  
  // required for shared object
  virtual bool Print(OutputStream &, int) const;
  virtual bool Equals(const shared_object*) const;

  /// Is the value for state variable i unknown?
  inline bool unknown(int i) const {
    if (0==is_unknown) return false;
    CHECK_RANGE(0, i, num_buckets);
    return is_unknown[i];
  }

  /// Get value for state variable i; must not be a list.
  inline int get(int i) const {
    DCASSERT(data);
    CHECK_RANGE(0, i, num_buckets);
    DCASSERT((0==is_unknown) || (false==is_unknown[i]));
    if (0==is_list) return data[i];
    DCASSERT(false==is_list[i]);

    // Not implemented yet
    DCASSERT(0);
    return 0;
  }

protected:
  inline void clear_unknown() {
    DCASSERT(is_unknown);
    for (int b=num_buckets; b; is_unknown[--b]=false);
  }

public:

  /// Set state variable i to be unknown
  inline void set_unknown(int i) {
    CHECK_RANGE(0, i, num_buckets);
    if (0==is_unknown) {
      is_unknown = new bool[num_buckets];
      clear_unknown();
    }
    is_unknown[i] = true;
  }

  /// Set a substate to be unknown
  inline void set_substate_unknown(int k) {
    if (0==is_unknown) {
      is_unknown = new bool[num_buckets];
      clear_unknown();
    }
    CHECK_RANGE(1, k, 1+num_substates);
    for (int i=substate_offset[k]; i<substate_offset[k-1]; i++) {
      CHECK_RANGE(0, i, num_buckets);
      is_unknown[i] = true;
    }
  }

  /// Set a substate to be known
  inline void set_substate_known(int k) {
    if (0==is_unknown) return;
    CHECK_RANGE(1, k, 1+num_substates);
    for (int i=substate_offset[k]; i<substate_offset[k-1]; i++) {
      CHECK_RANGE(0, i, num_buckets);
      is_unknown[i] = false;
    }
  }

  /// Set value for state variable i; must not be a list.
  inline void set(int i, int sv) {
    DCASSERT(data);
    CHECK_RANGE(0, i, num_buckets);
    if (is_unknown) is_unknown[i] = false;
    if (0==is_list) {
      data[i] = sv;
      return;
    }
    DCASSERT(false==is_list[i]);

    // Not implemented yet
    DCASSERT(0);
  }
};


// ******************************************************************
// *                                                                *
// *                           Front  end                           *
// *                                                                *
// ******************************************************************

/** Make a comparison expression:
      lb <= sv < ub
    If lb is null, then make the expression sv < ub,
    and if ub us null, then make the expression lb <= sv.
*/
expr* MakeBleVltB(const exprman* em, expr* lb, model_var* sv, expr* ub);

/** Make a model var update expression.
    Used for building "next-state" expressions.
      @param  sv  State var to update.
      @param  dec Proc int expression for decrementing, or 0.
      @param  inc Proc int expression for incrementing, or 0.
      @return A new expression, or 0 if both inc, dec are 0.
*/
expr* MakeVarUpdate(const exprman* em, model_var* sv, expr* dec, expr* inc);

/** Make a model var assignment expression.
    Used for building "next-state" expressions.
      @param  sv    State var to update.
      @param  rhs   New value to be assigned to sv.
      @return A new expression, or 0 if rhs or sv are 0.
*/
expr* MakeVarAssign(const exprman* em, model_var* sv, expr* rhs);

/** Make a model var assignment expression.
    Used for building "next-state" expressions.
      @param  sv    State var to update.
      @param  rhs   New value to be assigned to sv.
      @return A new expression, or 0 if sv is 0.
*/
expr* MakeVarAssign(const exprman* em, model_var* sv, long rhs);

#endif

