
// $Id$

#ifndef MEASURES_H
#define MEASURES_H

#include "result.h"
#include "functions.h"

/** \file measures.h 

   Implementation of measures, and measure front-end functions.

*/

class model_instance;
class engtype;

// ******************************************************************
// *                                                                *
// *                         measure  class                         *
// *                                                                *
// ******************************************************************


/** Measures, within models.
    Like deterministic constants, except we don't replace the return
    expression with the return value until told.
 */
class measure : public symbol {
  /// Engine required to solve this measure.
  engtype* which_engine;

  /// Value of the measure, if it has been computed.
  result value;

  /** Information about what to compute.
	    e.g., if the measure is "avg_ss(tk(p))",
	    then this expression should be "tk(p)".
  */
  expr* rhs;

  /// Measures we're waiting for, to solve ourselves
  List <symbol> *solve_deps;

protected:
  /// Model that owns us.
  model_instance* owner;

  /// Measures we're waiting for, to classify ourselves
  List <symbol> *class_deps;

public:
  /** Constructor.
	    In this form, we know exactly the engine type of the measure.
	    Normally, invoked by a function that knows its engine type.
	      @param	e	      Expr to grab file, line, etc. from.
	      @param	which	  The engine type required to solve it.
	      @param	parent	Parent model that is building us.
	      @param	rhs	    Right hand side.
  */
  measure(const expr* e, engtype* which, model_def* parent, expr* rhs);
protected:
  virtual ~measure();
public:
  inline engtype* EngineType() const { 
    return which_engine; 
  }
  inline void SetOwner(model_instance* mi) {
    DCASSERT(0==owner);
    owner = mi;
  }
  inline void SetValue(const result &v) {
    value = v;
    Affix();
  }
  inline void SetNull() {
    value.setNull();
    Affix();
  }
  inline void PrecomputeRHS() {
    if (rhs)	rhs->PreCompute();
  }
  inline void ComputeRHS(traverse_data &x) {
    SafeCompute(rhs, x);
  }
  inline void TraverseRHS(traverse_data &x) {
    if (rhs)  rhs->Traverse(x);
  }
  inline const type* RHSType() const { 
    return rhs ? rhs->Type() : 0;
  }
  void Solve(traverse_data &x);
  virtual void Compute(traverse_data &x);
  virtual void Traverse(traverse_data &x);

  /// Update our status if we can...
  virtual void notifyFrom(const symbol* p);

private:
  void SetRHS(expr* r);
  void Affix();	// call after setting the value.

protected:
  /// Returns true if we are blocked, waiting for classification
  bool isBlockedEngine() const;

  /** Classify the measure.
      Default behavior: bail out if the engine type is "BLOCKED_ENGINE",
      otherwise do nothing.
      For measures that allow delayed classification,
      this method must be overridden.
  */
  virtual void classifyNow();

  /// For delayed classification: setting the engine type.
  void setEngType(engtype* w) {
    DCASSERT(isBlockedEngine());
    which_engine = w;
  }

  void waitDepList(List <symbol>* dl);
};


// ******************************************************************
// *                                                                *
// *                       time_measure class                       *
// *                                                                *
// ******************************************************************

/** A measure that has an associated time or two.
*/
class time_measure : public measure {
  double time;
  double stop_time;
public:
  /** Constructor.
	      @param	e	  Expr to grab file, line, etc. from.
	      @param	p	  Parent model that is building us.
	      @param	rhs	Right hand side.
  */
  time_measure(const expr* e, model_def* p, expr* rhs);

  inline double GetTime() const { 
    DCASSERT(!isBlockedEngine());
    return time; 
  }
  inline double GetStopTime() const { 
    DCASSERT(!isBlockedEngine());
    return stop_time; 
  }

protected:
  inline void setClassification(engtype* w) {
    setEngType(w);
  }
  inline void setClassification(engtype* w, double t) {
    setEngType(w);
    time = t;
    stop_time = t;
  }
  inline void setClassification(engtype* w, double t1, double t2) {
    setEngType(w);
    time = t1;
    stop_time = t2;
  }
};

inline int Compare(time_measure* a, time_measure* b) {
  DCASSERT(a);
  DCASSERT(b);
  // Want SMALLER times first
  int c = SIGN(b->GetTime() - a->GetTime());
  if (c!=0) return c;
  return SIGN(b->GetStopTime() - a->GetStopTime());
}

// ******************************************************************
// *                                                                *
// *                     set_of_measures  class                     *
// *                                                                *
// ******************************************************************


/** A collection of measures with a common solution engine.
    Measures in this collection have not yet been solved.
 */
class set_of_measures {
protected:
  struct msrnode {
    measure* msr;
    msrnode* next;
  };
  static msrnode* free_list;
  inline static msrnode* NewNode(measure* m) {
    msrnode* tmp;
    if (free_list) {
      tmp = free_list;
      free_list = free_list->next;
    } else {
      tmp = new msrnode;
    }
    tmp->msr = m;
    return tmp;
  }
  inline static void RecycleNode(msrnode* n) {
    n->next = free_list;
    free_list = n;
  }
public:
  set_of_measures();
  virtual ~set_of_measures();
  
  /** Add a new measure to the collection, to solve.
      The measure may be blocked or ready.
  */
  virtual void addMeasure(measure* m) = 0;

  /** Get and remove the next (unblocked) measure to solve.
      Returns 0 if this solution "batch" is done.
      There may still be unsolved measures in the collection,
      for instance, if all of them are blocked, or if all of
      them have a solution time less than the "current" time.
  */
  virtual measure* popMeasure() = 0;

  /** Are there any measures in the collection?
      Useful for determining if we may need to solve more measures
      (once they become unblocked) of this type.
	      @return true iff there are any measures, blocked or unblocked.
  */
  virtual bool hasAnyMeasures() = 0;
};


// ******************************************************************
// *                                                                *
// *                         msr_func class                         *
// *                                                                *
// ******************************************************************

/** Measure generating function.
    Use this type of function for functions that should not
    be computed directly, but instead indicate that an engine
    must be called, such as "avg_ss".

    These functions work by creating the appropriate measure
    during substitution, as provided in derived classes by 
    the virtual method buildMeasure().  No other methods must
    be provided in derived classes.
*/
class msr_func : public simple_internal {
public:
  enum eng_class {
    Nothing = 0,
    CTL,
    Stochastic,
    CSL,  // CTL + Stochastic
    DCP
  };
private:
  eng_class ect;
public:
  msr_func(eng_class ec, const type* t, const char* name, int nf);
  virtual ~msr_func();
  virtual void Compute(traverse_data &x, expr** pass, int np);
  virtual int Traverse(traverse_data &x, expr** pass, int np);
  virtual measure* buildMeasure(traverse_data &x, expr** pass, int np) = 0;

  inline eng_class getEngClass() const { return ect; }
};

// ******************************************************************
// *                                                                *
// *                       msr_noengine class                       *
// *                                                                *
// ******************************************************************

/** For measures that do not require engines.
    In this case we build a measure that will be computed directly,
    such as "num_states".
    Desired behavior should be provided in derived classes
    by virtual method Compute().  No other methods must be 
    provided in derived classes.
    Note that we still build an appropriate measure during 
    substitution to handle complex measure dependencies.
*/
class msr_noengine : public msr_func {
public:
  msr_noengine(eng_class ec, const type* t, const char* name, int nf);
  virtual ~msr_noengine();
  virtual void Compute(traverse_data &x, expr** pass, int np);
  virtual measure* buildMeasure(traverse_data &x, expr** pass, int np);
};

// ******************************************************************
// *                                                                *
// *                           Front  end                           *
// *                                                                *
// ******************************************************************

/// Build an "unsorted" collection of measures.
set_of_measures* MakeUnsortedMeasures();

/// Build a collection of measures, sorted by "time".
set_of_measures* MakeTimeSortedMeasures();

#endif

