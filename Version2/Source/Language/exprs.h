
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

#include "results.h"

//#define SHARE_DEBUG

//@{



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

class symbol; // defined below

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
  /// The number of incoming pointers to this expression.
  int incoming;
public:
  expr(const char* fn, int line) {
    filename = fn;
    linenumber = line;
    incoming = 1;
  }

  virtual ~expr() { }

  inline const char* Filename() const { return filename; } 
  inline int Linenumber() const { return linenumber; }

  /// The number of aggregate components in this expression.
  virtual int NumComponents() const;

  /// Return a pointer to component i.
  virtual expr* GetComponent(int i);

  /// The type of component i.
  virtual type Type(int i) const = 0;

  /// Compute the value of component i.
  virtual void Compute(int i, result &x);

  /// Sample a value of component i (with given rng seed).
  virtual void Sample(long &, int i, result &x);

  /** Create a copy of this expression with values substituted
      for certain symbols. 
      (The symbols themselves determine the substitution.)
      Normally this is used by arrays within for loops.
      We make shallow copies (shared pointers to expressions)
      whenever possible.
      @param	i	The component to substitute.	
   */
  virtual expr* Substitute(int i) = 0;

  /** Split this expression into a sequence of sums.
      We store pointers to parts of the expressions, not copies;
      so don't delete them.
      @param	i	The component to split.
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
  virtual int GetSums(int i, expr **sums=NULL, int N=0, int offset=0);

  /** Split this expression into a sequence of products.
      Similar to GetSums.
      @param	i	The component to split.
      @param	prods	An array of pointers, or NULL.
      @param    N	The size of the array (or 0).
      @param	offset	The first slot to use in the array.
      @return	The number of products.
   */
  virtual int GetProducts(int i, expr **prods=NULL, int N=0, int offset=0);

  /** Get the symbols contained in this expression.
      @param	i	The component to check.
      @param	syms	Array of symbols, or NULL.
      @param	N	The size of the array (or 0).
      @param	offset	The first slot to use in the array.
      @return	The number of symbols.
   */
  virtual int GetSymbols(int i, symbol **syms=NULL, int N=0, int offset=0);

  /// Required for aggregation.  Used only by aggregates class.
  virtual void TakeAggregates() { ASSERT(0); }

  /// Display the expression to the given output stream.
  virtual void show(OutputStream &s) const = 0;

  friend expr* Copy(expr *e);
  friend void Delete(expr *e);
};

/**  Create a "copy" of this expression.
     Since we share pointers, this simply
     increases the incoming pointer count.
 */
inline expr* Copy(expr *e)
{
  if (e) {
    e->incoming++;
#ifdef SHARE_DEBUG
    cerr << "increased incoming count for " << e << " to " << e->incoming << endl;
#endif
  }
  return e;
}

/**  Delete an expression.
     This should *always* be called instead
     of doing it "by hand", because the expression
     might be shared.  This version takes sharing into account.
 */
inline void Delete(expr *e)
{
  if (e) {
    DCASSERT(e->incoming>0);
    e->incoming--;
#ifdef SHARE_DEBUG
    cerr << "decreased incoming count for " << e << " to " << e->incoming << endl;
#endif
    if (0==e->incoming) {
#ifdef SHARE_DEBUG
      cerr << "Deleting " << e << "\n";
#endif
      delete e;
    }
  }
}

// ******************************************************************
// *                                                                *
// *                         constant class                         *
// *                                                                *
// ******************************************************************

/**  The base class for "constants".
     That means things like "3.2", "infinity".
 */

class constant : public expr {
protected:
  type mytype;  // Simplify life
public:
  constant(const char* fn, int line, type mt);
  virtual type Type(int i) const;
  virtual expr* Substitute(int i);
};

// ******************************************************************
// *                                                                *
// *                          unary  class                          *
// *                                                                *
// ******************************************************************

/**  The base class for unary operations.
     Deriving from this class will save you from having
     to implement a few things.
 */

class unary : public expr {
protected:
  expr* opnd;
public:
  unary(const char* fn, int line, expr* x);
  virtual ~unary();
  virtual int GetSymbols(int i, symbol **syms=NULL, int N=0, int offset=0);
  virtual expr* Substitute(int i);
protected:
  /** Used by Substitute.
      Whatever kind of unary operation we are, make another one.
      The filename and line number should be copied.
   */
  virtual expr* MakeAnother(expr* newopnd) = 0;
  void unary_show(OutputStream &s, const char *op) const;
};

// ******************************************************************
// *                                                                *
// *                          binary class                          *
// *                                                                *
// ******************************************************************

/**  The base class for binary operations.
     Deriving from this class will save you from having
     to implement a few things.
 */

class binary : public expr {
protected:
  expr* left;
  expr* right;
public:
  binary(const char* fn, int line, expr* l, expr* r);
  virtual ~binary();
  virtual int GetSymbols(int i, symbol **syms=NULL, int N=0, int offset=0);
  virtual expr* Substitute(int i);
protected:
  /** Used by Substitute.
      Whatever kind of binary operation we are, make another one.
      The filename and line number should be copied.
   */
  virtual expr* MakeAnother(expr* newleft, expr* newright) = 0;
  void binary_show(OutputStream &s, const char *op) const;
};

// ******************************************************************
// *                                                                *
// *                          assoc  class                          *
// *                                                                *
// ******************************************************************

/**  The base class for associative operations.
     This allows us to string together things like sums
     into a single operator (for efficiency).
     Deriving from this class will save you from having
     to implement a few things.
 */

class assoc : public expr {
protected:
  int opnd_count;
  expr** operands;
public:
  assoc(const char* fn, int line, expr **x, int n);
  assoc(const char* fn, int line, expr *l, expr *r);
  virtual ~assoc();
  virtual int GetSymbols(int i, symbol **syms=NULL, int N=0, int offset=0);
  virtual expr* Substitute(int i);
protected:
  /** Used by Substitute.
      Whatever kind of associative operation we are, make another one.
      The filename and line number should be copied.
   */
  virtual expr* MakeAnother(expr **newx, int newn) = 0;
  void assoc_show(OutputStream &s, const char *op) const;
};

// ******************************************************************
// *                                                                *
// *                          symbol class                          *
// *                                                                *
// ******************************************************************

/**   The base class of all symbols.
      That includes formal parameters, for loop iterators,
      functions, and model objects like states, places, and transitions.

      Note: we derive symbols from expressions because it 
      greatly simplifies for-loop iterators
      (otherwise you have an expression wrapped around
      an iterator, and that is unnecessary overhead.)
*/  

class symbol : public expr {
private:
  /// The symbol name.
  char* name;
  /// The symbol type.
  type mytype;
  /// If the symbol is an aggregate, then the type is an array.
  type* aggtype;
  /// Length of aggregate, or 1 for "normal" types.
  int agglength;
  /// Should we substitute our value?  Default: true.
  bool substitute_value;
public:

  symbol(const char* fn, int line, type t, char* n);
  symbol(const char* fn, int line, type *t, int tlen, char* n);
  virtual ~symbol();

  virtual type Type(int i) const;
  virtual int NumComponents() const;
  virtual int GetSymbols(int i, symbol **syms=NULL, int N=0, int offset=0);
  
  inline const char* Name() const { return name; }

  /** Change our substitution rule.
      @param sv		If sv is true, whenever an expression calls
                        "Substitute", we will replace ourself with
			a constant equal to our current value.
			If sv is false, whenever an expression calls
			"Substitute", we will copy ourself.
   */ 
  inline void SetSubstitution(bool sv) { substitute_value = sv;}

  virtual expr* Substitute(int i);
  virtual void show(OutputStream &) const;
};


// ******************************************************************
// *                                                                *
// *                Global functions for expressions                *
// *                                                                *
// ******************************************************************

/** Compute an expression.
    This deals with error traces and such.
 */
inline void Compute(expr *e, int a, result &x) 
{
  if (e) {
    e->Compute(a, x);
  } else {
    x.Clear();
    x.null = true;
  }
}

/** Sample an expression.
    This deals with error traces and such.
 */
inline void Sample(expr *e, int a, long &seed, result &x) 
{
  if (e) {
    e->Sample(seed, a, x);
  } else {
    x.Clear();
    x.null = true;
  }
}


// ******************************************************************
// *                                                                *
// *                  Global functions for output                   *
// *                                                                *
// ******************************************************************

OutputStream& operator<< (OutputStream &s, expr *e);

/**
    Print the type of an expression.
    Works correctly for aggregates, also.
*/
void PrintExprType(expr *e, OutputStream &s);

// ******************************************************************
// *                                                                *
// *             Global functions  to build expressions             *
// *                                                                *
// ******************************************************************

/// Build a boolean constant.
expr* MakeConstExpr(bool c, const char* file, int line);

/// Build an integer constant.
expr* MakeConstExpr(int c, const char* file, int line);

/// Build a real constant.
expr* MakeConstExpr(double c, const char* file, int line);

/// Build a string constant.
expr* MakeConstExpr(char *c, const char* file, int line);

/** Build a generic constant.
    Defined in infinity.cc.
 */
expr* MakeConstExpr(type t, const result &x, const char* file, int line);

/// Build an aggregate
assoc* MakeAggregate(expr **list, int size, const char* file, int line);

//@}

#endif

