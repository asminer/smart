
// $Id$

#ifndef EXPRS_H
#define EXPRS_H

/** @name exprs.h
    @type File
    @args \ 

  Base class for expressions, and all expressions that
  deal with simple types: bool, int, real, string.
 */

#include "results.h"
#include "../Templates/list.h"

// #define COMPUTE_DEBUG
// #define SHARED_EXPR_DEBUG

//@{

// Defined elsewhere but necessary here.

class Rng;	// Random number generator class, defined elsewhere
class state;    // defined elsewhere, used for proc expressions.


// Defined below but necessary.

class symbol;	
class measure;



enum Engine_type {
  /// Error of some sort
  ENG_Error,
  /// Steady-state instantaneous
  ENG_SS_Inst,
  /// Steady-state accumulated
  ENG_SS_Acc,
  /// Transient instantaneous
  ENG_T_Inst,
  /// Transient accumulated
  ENG_T_Acc,
  /// No engine
  ENG_None,
  /// Model checking (statesets)
  ENG_ModelChecking,
  /// Custom engine 
  ENG_Custom,
  /// Expression containing several of the above
  ENG_Mixed
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
   */
  result starttime;
  /** The stop time.
      Used for numerical solution engines only.
      May be infinite (for steady-state) or equal to the start time (for
      instantaneous)
   */
  result stoptime;

  /// Set to none
  void setNone() {
    engine = ENG_None;
    starttime.setNull();
    stoptime.setNull();
  }

  /// Set to mixed
  void setMixed() {
    engine = ENG_Mixed;
    starttime.setNull();
    stoptime.setNull();
  }
};

// ******************************************************************
// *                                                                *
// *                       compute_data class                       *
// *                                                                *
// ******************************************************************

/** Information used for computing a value from an expression.
    Allows for flexibility and speed.  Not bad.
 */
struct compute_data {
  /// The return value.  
  result* answer;
  /// Aggregate index (normally 0).
  int aggregate;

  /** 
      The random number generation stream.
      Used for rand expressions: if provided, random expressions will 
      be "sampled".  const expressions can use null here.  If null is 
      given for a rand expression, this should return an object that 
      can be sampled.
  */
  Rng *rng_stream;

  /** 
      The current state.
      Used for proc expressions, which depend on the current "state".  
      Non-proc expressions can use null here.
  */
  const state* current;

public:
  /// Handy: constructor
  compute_data() {
    answer = 0;
    aggregate = 0;
    rng_stream = 0;
    current = 0;
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

class expr : public shared_object {
private:
  /// The name of the file we were declared in.
  const char* filename;  
  /// The line number of the file we were declared on.
  int linenumber;  
public:
  expr(const char* fn, int line) : shared_object() {
    filename = fn;
    linenumber = line;
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

  /** Clear the compute cache.
      We use a compute cache for each function with no parameters.
      This is required to handle dependent random variables correctly!
      So, we need a meachanism to clear the caches.
  */
  virtual void ClearCache() = 0;

  /** Compute the expression.
      For speed and flexibility, uses a struct parameter.
  */
  virtual void Compute(compute_data &x);


  /** Compute the next state from the current state.
      Used by models for deterministic events.
      Used for "proc state" expressions.
      The result is used to catch errors.
  */
  virtual void NextState(const state &current, state &next, result &x);

  /** Create a copy of this expression with values substituted
      for certain symbols. 
      (The symbols themselves determine the substitution.)
      Normally this is used by arrays within for loops.
      We make shallow copies (shared pointers to expressions)
      whenever possible.
      @param	i	The component to substitute.	
   */
  virtual expr* Substitute(int i) = 0;

  /** Split this expression into a list of sums.
      The expressions in the list should be deleted (with Delete)
      when done; we store copies (possibly shared) of parts of expressions. 
      @param	i	The component to split.
      @param	sums	An array-list template of expressions, or NULL.
      			If not null, the list will contain the sums
			on exit;  if NULL, we simply count them.
      @return	The number of sums.  
  */
  virtual int GetSums(int i, List <expr> *sums=NULL);

  
  /** Split this expression into a sequence of products.
      Similar to GetSums.
      @param	i	The component to split.
      @param	prods	An array-list template of pointers, or NULL.
      @return	The number of products.
   */
  virtual int GetProducts(int i, List <expr> *prods=NULL);

  /** Get the symbols contained in this expression.
      @param	i	The component to check.
      @param	syms	Array-list of symbols, or NULL.
      @return	The number of symbols.
   */
  virtual int GetSymbols(int i, List <symbol> *syms=NULL);
  
  /** Get the engine type for this expression.
      @param	e	If not null, fill with all engine information.
      @return	The engine type.
  */
  virtual Engine_type GetEngine(engineinfo *e);

  /** Get the "reward expression".
      E.g., if this expression is "prob_at( instate(A), 6.0 )"
      then this should return expression "instate(A)".
      For ordinary expressions (default behavior) return ERROR.
  */
  virtual expr* GetRewardExpr();

  /** Used for mixed solution engines.
      Make a copy of the expression, except replace single mixed measures 
      with multiple measures of a single engine.
      @param	mlist	Any created measures are added to this list.
      @return	If the engine type is not mixed, a copy of this expression;
      		otherwise, a new expression.
  */
  virtual expr* SplitEngines(List <measure> *mlist);

  /// Required for aggregation.  Used only by aggregates class.
  virtual void TakeAggregates() { ASSERT(0); }

  /// Display the expression to the given output stream.
  virtual void show(OutputStream &s) const = 0;
};

/** Error expression.
    Use this instead of NULL, because NULL means null.
    error should be used for, say, compile-time errors.
*/
static expr* ERROR = ((expr*)0xffffffff);

/** Default expression.
    Used as a placeholder for defaults.
*/
static expr* DEFLT = ((expr*)0xfffffffe);

/**  Create a "copy" of this expression.
     Since we share pointers, this simply
     increases the incoming pointer count.
 */
inline expr* Copy(expr *e)
{
  if (e && e!=ERROR && e!=DEFLT) {
#ifdef SHARED_EXPR_DEBUG
    Output << "Copying expression: ";
    e->show(Output);
    Output << ", already " << e->numRefs() << " copies\n";
    Output.flush();
#endif
    Share(e);
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
  if (e && e!=ERROR && e!=DEFLT) {
#ifdef SHARED_EXPR_DEBUG
    Output << "Deleting expression: ";
    e->show(Output);
    Output << ", currently " << e->numRefs() << " copies\n";
    Output.flush();
#endif
    Delete((shared_object*) e);
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
  ~constant();
  virtual type Type(int i) const;
  virtual void ClearCache() { } // No cache for constants!
  virtual expr* Substitute(int i);
  virtual Engine_type GetEngine(engineinfo *e);
  virtual expr* SplitEngines(List <measure> *);
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
  virtual void ClearCache();
  virtual int GetSymbols(int i, List <symbol> *syms=NULL);
  virtual expr* Substitute(int i);
  virtual Engine_type GetEngine(engineinfo *e);
  virtual expr* SplitEngines(List <measure> *);
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
  virtual void ClearCache();
  virtual int GetSymbols(int i, List <symbol> *syms=NULL);
  virtual expr* Substitute(int i);
  virtual Engine_type GetEngine(engineinfo *e);
  virtual expr* SplitEngines(List <measure> *);
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
  virtual void ClearCache();
  virtual int GetSymbols(int i, List <symbol> *syms=NULL);
  virtual expr* Substitute(int i);
  virtual Engine_type GetEngine(engineinfo *e);
  virtual expr* SplitEngines(List <measure> *);
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
protected:
  /// The symbol type.
  type mytype;
private:
  /// The symbol name.
  char* name;
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
  virtual int GetSymbols(int i, List <symbol> *syms=NULL);
  
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

  /// Returns true if the types and names match perfectly.
  bool Matches(symbol *) const;
};

inline int Compare(symbol *a, symbol *b)
{
  DCASSERT(a);
  DCASSERT(b);
  DCASSERT(a->Name());
  DCASSERT(b->Name());
  return strcmp(a->Name(), b->Name());
}

// ******************************************************************
// *                                                                *
// *                Global functions for expressions                *
// *                                                                *
// ******************************************************************

/** Compute an expression.
    Correctly handles null expressions.
 */
inline void SafeCompute(expr *e, compute_data &x) 
{
#ifdef COMPUTE_DEBUG
  static int depth = 0;
#endif

  DCASSERT(e!=ERROR);
  DCASSERT(e!=DEFLT);
  if (e) {
#ifdef COMPUTE_DEBUG
    Output.Pad(' ', depth*2);
    Output << "Computing expression "; 
    e->show(Output);
    Output << "\n";
    Output.flush();
    depth++;
#endif
    e->Compute(x);
#ifdef COMPUTE_DEBUG
    depth--;
    Output.Pad(' ', depth*2);
    Output << "expression ";
    e->show(Output);
    Output << " evaluated to ";
    DCASSERT(x->answer);
    PrintResult(Output, e->Type(a), *(x.answer));
    Output << "\n";
    Output.flush();
#endif
  } else {
    DCASSERT(x.answer);
    x.answer->Clear();
    x.answer->setNull();
  }
}


/// Safe way to count components
inline int NumComponents(expr *e)
{
  DCASSERT(e!=ERROR);
  DCASSERT(e!=DEFLT);
  if (e) return e->NumComponents();
  return 1;
}

/// Safe way to get expression type
inline type Type(expr *e, int comp)
{
  DCASSERT(e!=ERROR);
  DCASSERT(e!=DEFLT);
  if (NULL==e) return VOID;
  return e->Type(comp);
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

/// Used for immediate events
extern expr* const_0;

/** Build a boolean constant.
    The type can be BOOL, RAND_BOOL, PROC_BOOL...
*/
expr* MakeConstExpr(type t, bool c, const char* file, int line);

/// Build an integer constant.
expr* MakeConstExpr(type t, int c, const char* file, int line);

/// Build a real constant.
expr* MakeConstExpr(type t, double c, const char* file, int line);

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

