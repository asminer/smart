
// $Id$

#ifndef MODELS_H
#define MODELS_H

/** @name models.h
    @type File
    @args \ 

  Language support for models.
  Actual discrete-state models are defined in another module.

 */

#include "functions.h"
#include "arrays.h"
#include "stmts.h"

//@{
  
// Defined in another module
class state_model;
void Delete(state_model *);

#define DEBUG_MODEL

// ******************************************************************
// *                                                                *
// *                        model_var  class                        *
// *                                                                *
// ******************************************************************

/**
     Base class of variables (without "data") defined within a model.
*/
class model_var : public symbol {
public:
  /** Linkage between state and variables.
      If this variable requires state, then this is the index
      of the variable within a state array.
  */
  int state_index;

  /// Partition info (for structured approaches).
  int part_index;  

  /** Lower bound, if any.
      Used during state changes to generate errors.
  */
  int lower;

  /// do we have a lower bound
  bool has_lower_bound;

  /** Upper bound, if any.
      Used during state changes to generate errors.
  */
  int upper;

  /// do we have an upper bound
  bool has_upper_bound;
  
public:
  model_var(const char* fn, int line, type t, char* n);
  virtual ~model_var();

  inline void SetIndex(int si) {
    DCASSERT(state_index<0);
    DCASSERT(si>=0);
    state_index = si;
  }

  inline void ResetIndex(int si) {
    DCASSERT(si>=0);
    state_index = si;
  }

  inline int GetIndex() const { return state_index; }

  inline bool hasLowerBound() const { return has_lower_bound; }
  inline bool hasUpperBound() const { return has_upper_bound; }
  inline int getLowerBound() const {
    DCASSERT(has_lower_bound);
    return lower;
  }
  inline int getUpperBound() const {
    DCASSERT(has_upper_bound);
    return upper; 
  }
  inline void setLowerBound(int L) {
    has_upper_bound = true;
    lower = L;
  }
  inline void setUpperBound(int U) {
    has_upper_bound = true;
    upper = U;
  }

  /// Returns the state_index
  virtual void Compute(int i, result &x);

  // other required virtual functions here

  virtual void ClearCache();
};

// ******************************************************************
// *                                                                *
// *                          model  class                          *
// *                                                                *
// ******************************************************************

/**   The base class of high-level formalisms (for language support).

      Specific formalisms should be derived from this class.

      Note that models are derived from functions so that we can
      use the same mechanism for parameters.
*/  

class model : public function {

private:
  bool never_built;
  result* current_params;
  result* last_params;
  inline void FreeLast() {
    for (int i=0; i<num_params; i++)
      DeleteResult(parameters[i]->Type(0), last_params[i]);
  }
  inline void FreeCurrent() {
    for (int i=0; i<num_params; i++)
      DeleteResult(parameters[i]->Type(0), current_params[i]);
  }
  bool SameParams();

  /// All measures for this instantiation
  List <measure> *mlist;

  /// All transient measures
  List <measure> *mtrans;

  /// All steady-state measures
  List <measure> *msteady;

  /// All transient, accumulated measures
  List <measure> *macc_trans;

  /// All steady-state, accumulated measures
  List <measure> *macc_steady;

  // Add more info for each measure here...

protected:
  statement **stmt_block;
  int num_stmts;
  result last_build;
  /// Discrete-state "meta" model used by engines.
  state_model *dsm;

  /** Symbols that can be exported (usually measures or arrays of measures).
      Stored as an array of symbols, sorted by name.
  */
  symbol **mtable;  
  int size_mtable;  

public:
  model(const char* fn, int line, type t, char* n,
  	formal_param **pl, int np);

  virtual ~model();

  // Required as a function...
  virtual void Compute(expr **, int np, result &x);
  virtual void Sample(Rng &, expr **, int np, result &x);

  // Required because models will be passed as parameters...
  virtual Engine_type GetEngine(engineinfo *);

  /** Fill the body of the function.
      @param	b	Array of statements
      @param	n	Number of statements
  */
  inline void SetStatementBlock(statement **b, int n) {
    DCASSERT(NULL==stmt_block);
    stmt_block = b; num_stmts = n;
#ifdef DEBUG_MODEL
    Output << "Set statement block for model " << Name() << ":\n";
    int i;
    for (i=0; i<n; i++) {
      Output << "\t" << stmt_block[i] << ";\n";
    }
    Output.flush();
#endif
  }

  /** Set the symbol table (done by compiler).
      These are the externally-visible symbols (usually measures).
  */
  inline void SetSymbolTable(symbol **st, int n) {
    DCASSERT(NULL==mtable);
    mtable = st; size_mtable = n;
  }

  inline state_model* GetModel() const { return dsm; }
    
  /** Create a model variable of the specified type.
      Must be provided in derived classes.
  */
  virtual model_var* MakeModelVar(const char *fn, int l, type t, char* n) = 0;

  /** Instantiate a measure.
      This must be called whenever a measure is "created" for a model
      (handled by an appropriate statement)
      so that we can group the measures by solution engine.
  */
  void AcceptMeasure(measure *m);

  /** Find an externally-visible symbol with specified name.
      @param	name	The symbol to look for.
      @return	The symbol with matching name (there can be only one)
      		or NULL if there is none.
  */
  symbol* FindVisible(char* name) const;

  /** Solve a given measure.
      If the measure is part of a group, all measures in the group
      are solved.
  */
  void SolveMeasure(measure *m);

private:
  /** Process the list of measures.
      Called at the end of model instantiation.
  */
  void GroupMeasures();

  /** Clear out all old structs.
      Used before re-creating a model (with new parameters).
  */
  void Clear();

protected:
  /** Prepare for instantiation.
      Provided in derived classes.  Called immediately before
      the model is constructed for specific parameters.
      Allows formalisms to initialize any structures needed.
  */
  virtual void InitModel() = 0;

  /** Complete instantiation.
      Provided in derived classes.  Called immediately after
      the model is constructed for specific parameters.
      Allows formalisms to perform any finalization necessary.

      @param	x	Result.  Should have error set if there was an error,
      			otherwise should simply contain a pointer to the model
			(i.e., x.other = this).
  */
  virtual void FinalizeModel(result &x) = 0;

  /** Construct the underlying state_model.
      Called after "FinalizeModel" if it was successful.
  */
  virtual state_model* BuildStateModel() = 0;
};

// ******************************************************************
// *                                                                *
// *                                                                *
// *                          Global stuff                          *
// *                                                                *
// *                                                                *
// ******************************************************************

/** Make an expression to call a model measure.
    Passed parameters must match exactly in type.
    Expression for m(p).s
    @param	m	The model to instantiate (if necessary)

    @param	p	The parameters to pass to the model.
    @param	np	Number of passed parameters.

    @param	s	The measure within the model to call.

    @param	fn	Filename we are declared in.
    @param	line	line number we are declared on.
 */
expr* MakeMeasureCall(model *m, expr **p, int np, measure *s, const char *fn, int line);

expr* MakeMeasureArrayCall(model *m, expr **p, int np, array *s, expr **i, int ni, const char *fn, int line);

/** New for version 2.
    When a model variable is declared within a model and used in an
    expression, we use an empty wrapper to represent the variable
    because the variable does not exist until the model is instantiated.
    The wrapper is filled by a "ModelVarStmt" statement (see below).
    A call to "Substitite" will eliminate the wrapper.
*/
expr* MakeEmptyWrapper(const char *fn, int line);

/** Statement to construct model variables (no arrays).
    @param	p	The parent model
    @param	t	The type of variables
    @param	names	Array of names to create
    @param	wraps	Array of wrappers to use
    @param	N	size of names and wraps arrays
    @param	fn	filename
    @param	l	linenumber
*/
statement* MakeModelVarStmt(model *p, type t, char** names, expr** wraps, int N,
			const char* fn, int l);

statement* MakeModelArrayStmt(model *p, array** alist, int N, 
			const char* fn, int l);

/// Measure assignments.
statement* MakeMeasureAssign(model *p, measure *m, const char *fn, int line);

/// Measure array assignments.
statement* MakeMeasureArrayAssign(model *p, array *f, expr* retval, 
					const char *fn, int line);

//@}

#endif

