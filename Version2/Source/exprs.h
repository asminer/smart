
// $Id$

#ifndef EXPRS_H
#define EXPRS_H

/** @name exprs.h
    @type File
    @args \ 

  New, very fast, and simple way to represent results.
  Virtual-function free!

  Base class for expressions, and all expressions that
  deal with simple types: bool, int, real, string.
 */

#include "types.h"

//@{
  

/// Things that can go wrong when computing a result.
enum compute_error {
  /** No problems.  
      By forcing the value to zero we should be able to do things like
      "if (error) ..."
   */
  CE_Ok = 0,
  /// We encountered a "don't know" state
  CE_Dont_Know,
  /// We encountered a "don't care" state
  CE_Dont_Care,
  /// We encountered overflow when casting to another result type
  CE_Overflow,
  /// Divide by zero
  CE_ZeroDivide,
  /// Undefined quantity such as infinity-infinity
  CE_Undefined
};


/// Possible solution engines
enum Engine_type {
  /// We don't know yet
  ENG_UNKNOWN,
  /// No engine 
  ENG_NONE,
  /// Custom solution engine
  ENG_CUSTOM,
  /// Numerical solution engine
  ENG_NUMERICAL,
  /// Model checking engine
  ENG_MODEL_CHECKING,
  /// A mix of things.  Currently this is illegal.
  ENG_MIXED
};


/// Possible ways to classify measures
enum Group_type {
  /// Don't group
  GT_UNGROUPED,
  /// Steady state instantaneous
  GT_SSINST,
  /// Steady state cumulative
  GT_SSCUMUL,
  /// Transient instantaneous
  GT_TINST,
  /// Transient cumulative
  GT_TCUMUL
};


// ******************************************************************
// *                                                                *
// *                          result class                          *
// *                                                                *
// ******************************************************************

/**   The structure used by expressions to represent values.
 
      Completely redesigned for version 2:
      No more derived classes or virtual functions.

      Conventions:

      Whenever an error occurs, the value should be set to
      null by setting null to true.
      (A value can be null when no error occurs.)

      For +- infinity, set infinity to true.
      Then, set the sign appropriately for the proper type.
      For example, if the type is integer, and we want -infinity,
      set infinity to true, and ivalue to -1 (or any other - int).
*/  

struct result {
  // everything is public... we know what we are doing.

  // No type!  can be determined from expression that computes us, if needed

  /// The first thing that went wrong while computing us.
  compute_error error;

  /// Are we infinite.  Sign is determined from the value.
  bool infinity;

  /// Are we a null value?  
  bool null;

  union {
    /// If we are a boolean type
    bool   bvalue;
    /// If we are an integer type
    int    ivalue;
    /// If we are a real type
    double rvalue;
    /// Everything else
    void*  other; 
  };

  inline void Clear() {
    error = CE_Ok;
    infinity = false;
    null = false;
  }
};


// ******************************************************************
// *                                                                *
// *                        engineinfo class                        *
// *                                                                *
// ******************************************************************

/** Information returned by an engine query.
    If the engine type is numerical, then
    stoptime is used to determine the solution time.
    For instantaneous measures, the starttime pointer
    should equal the stoptime pointer.
    A stop time of infinity is used for steady-state.
 */
struct engineinfo {
  /// The engine to use.
  Engine_type engine;
  /** The starting time.
      Used for numerical solution engines only.
      For instantaneous, set to the same time as the stop time
      (or, to be safe, set to a value larger than the stop time).
   */
  result starttime;
  /** The stop time.
      Used for numerical solution engines only.
      May be infinite.
   */
  result stoptime;
  /// Does this require numerical solution?
  inline bool IsNumerical() const {
    return engine==ENG_NUMERICAL;
  }
  /// Assuming we are numerical, is it a steady-state measure?
  inline bool IsSteadyState() const {
    DCASSERT(stoptime.null == false);
    return stoptime.infinity;
  }
  /// Assuming we are numerical, is it a transient measure?
  inline bool IsTransient() const {
    DCASSERT(stoptime.null==false);
    return !(stoptime.infinity);
  }
  /// Assuming we are numerical, is it a cumulative measure?
  inline bool IsCumulative() const {
    return starttime.rvalue < stoptime.rvalue;
  }
  /// Assuming we are numerical, is it an instantaneous measure?
  inline bool IsInstantaneous() const {
    return starttime.rvalue >= stoptime.rvalue;
  }
  /// Handy for non-numerical engines and such.
  inline void SetEngine(Engine_type e) {
    engine = e;
    starttime.null = true;
    stoptime.null = true;
  }
  /// Basically, an explicit destructor.
  inline void Clear() {
    SetEngine(ENG_UNKNOWN);
  }
  /// Returs true if there was an error
  inline bool Error() const {
    return IsNumerical() && (starttime.null || stoptime.null);
  }
  /// Tells how to group a measure requiring this engine
  inline Group_type GetGroup() const {
    if (!IsNumerical()) return GT_UNGROUPED;
    if (Error()) return GT_UNGROUPED; 
    if (IsSteadyState())
      if (IsInstantaneous())
	return GT_SSINST;
      else
	return GT_SSCUMUL;
    else
      if (IsInstantaneous())
	return GT_TINST;
      else
	return GT_TCUMUL;
  }

};


// ******************************************************************
// *                                                                *
// *                           expr class                           *
// *                                                                *
// ******************************************************************

/**   The base class of all expressions, and the
      heart of all expression trees.
 
      Conventions:

      New for version 2, we rely a lot more on compile-time
      type checking to simplify our lives (and improve speed) 
      once an expression has been built.  That means we'll write 
      another derived class just to avoid an if statement if necessary.
*/  

class expr {
  private:
    /// The name of the file we were declared in.
    const char* filename;  
    /// The line number of the file we were declared on.
    int linenumber;  

  public:

  expr() {
    // for expressions built by smart
    filename = NULL;
    linenumber = 0;
  }

  expr(const char* fn, int line) {
    filename = fn;
    linenumber = line;
  }

  virtual ~expr() { }

  inline const char* Filename() const { return filename; } 
  inline int Linenumber() const { return linenumber; }

  /// Make a new copy of this expression tree.
  virtual expr* Copy() const = 0;

  /// The number of aggregate components in this expression.
  virtual int NumComponents() const { return 1; }

  /// Return a pointer to component i.
  virtual expr* GetComponent(int i) {
    DCASSERT(i==0);
    return this;
  }

  /// The type of component i.
  virtual type Type(int i) const = 0;

  /// Compute the value of component i.
  virtual void Compute(int i, result &x) const = 0;

  /// Sample a value of component i (with given rng seed).
  virtual void Sample(long &, int i, result &x) const {
    Compute(i, x);
  }

  /** Split this expression into a sequence of sums.
      We store pointers to parts of the expressions, not copies.
      @param	sums	An array of pointers, already allocated, to store
      			each piece of the expression.
			If NULL, we only count the sums.
      @param    N	The size of the array.
      			We'll avoid writing past the end of the array.
			This had better be 0 if the array is NULL.
      @param	offset	The first slot to use in the array.
      			(Used by recursion, mainly.)

      @return	The number of sums.
      		Obviously, if this exceeds N, then not all sums
		are stored in the array.
   */
  virtual int GetSums(expr **sums=NULL, int N=0, int offset=0) {
    if (offset<N) sums[offset] = this;
    return 1;
  }

  /** Split this expression into a sequence of products.
      Similar to GetSums.
      @param	prods	An array of pointers, or NULL.
      @param    N	The size of the array (or 0).
      @param	offset	The first slot to use in the array.
      @return	The number of products.
   */
  virtual int GetProducts(expr **prods=NULL, int N=0, int offset=0) {
    if (offset<N) prods[offset] = this;
    return 1;
  }

  /// Required for aggregation.  Used only by aggregates class.
  virtual void TakeAggregates() { ASSERT(0); }

  /// Display the expression to the given output stream.
  virtual void show(ostream &s) const = 0;
};

inline ostream& operator<< (ostream &s, expr *e)
{
  if (NULL==e) s << "null";
  else e->show(s);
  return s;
}

inline expr* CopyExpr(expr *e)
{
  if (NULL==e) return NULL;
  return e->Copy();
}

// ******************************************************************
// *                                                                *
// *             Global functions  to build expressions             *
// *                                                                *
// ******************************************************************

/// Build a boolean constant.
expr* MakeConstExpr(bool c, const char* file=NULL, int line=0);

/// Build an integer constant.
expr* MakeConstExpr(int c, const char* file=NULL, int line=0);

/// Build a real constant.
expr* MakeConstExpr(double c, const char* file=NULL, int line=0);

/// Build a string constant.
expr* MakeConstExpr(char *c, const char* file=NULL, int line=0);



//@}

#endif

