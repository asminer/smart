
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
#include "../heap.h"

//@{
  
// Defined in another module
class state_model;
void Delete(state_model *);

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
      Otherwise, stateindex should be negative (e.g., for a transition).
  */
  int state_index;

  /// Partition info (for structured approaches).
  int part_index;  
public:
  model_var(const char* fn, int line, type t, char* n);

  // stuff for derived classes ?
};

// ******************************************************************
// *                                                                *
// *                          model  class                          *
// *                                                                *
// ******************************************************************

inline int Compare(measure *a, measure *b)
{
  // Required for heap.
  // Sort measures by measure pointer info, for speed.
  if (a < b) return -1;
  if (a > b) return 1;
  return 0;
}

/**   The base class of high-level formalisms (for language support).

      Specific formalisms should be derived from this class.

      Note that models are derived from functions so that we can
      use the same mechanism for parameters.
*/  

class model : public function {

private:
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

  /// Measures, used during model instantiation.
  Heap <measure> *mheap;

  /// Final sorted array of measures.
  measure** mlist;

  /// Size of measure list
  int mlist_size;

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


  /** Fill the body of the function.
      @param	b	Array of statements
      @param	n	Number of statements
  */
  inline void SetStatementBlock(statement **b, int n) {
    DCASSERT(NULL==stmt_block);
    stmt_block = b; num_stmts = n;
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
  inline void AcceptMeasure(measure *m) { mheap->Insert(m); }

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

  /** Find the index of s within the sorted list of measures.
      Returns NOT_FOUND if s is not in the list.
  */
  int FindMeasure(symbol* s);

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

// expr* MakeArrayMeasureCall(model *m, expr **p, int np, expr **i, int ni);

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

