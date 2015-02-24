
// $Id$

#ifndef MOD_DEF_H
#define MOD_DEF_H

/** \file mod_def.h

    User-defined models and such.

    Used heavily by specific formalisms in the "Formalism module".

 */

#include "functions.h"
#include "mod_inst.h"

class measure;
class model_var;
  
// ******************************************************************
// *                                                                *
// *                        model_def  class                        *
// *                                                                *
// ******************************************************************

/**   The base class of high-level formalisms (for language support).

      Specific formalisms should be derived from this class.

      Instances of this class (well, of derived classes) 
      are used to store the language part of a user-defined model.
      It contains a list of statements and lots of wrappers and any
      other useful (usually compile-time) information about a user-defined
      model.  Note that this class is derived from functions so that we can
      use the same mechanism for parameters.

      To better handle the hidden model parameter for model functions,
      "Substitute" will return the current model_instance.
*/  
class model_def : public function {
  /// Warning option.
  static named_msg not_our_var;

  friend void InitModelDefs(exprman* em);

  /// Formal parameters of the model
  fplist formals;

  /// Statements to execute when instantiating the model
  expr* stmt_block;

  /// List of externally visible measures and measure arrays
  symbol** mysymbols;
  /// Size of the symbol "table"
  int num_symbols;

  /// Stack space for computing parameters.
  result* current_params;

  /// Parameters computed last time we were called directly.
  result* last_params;

  /// Previously constructed model.
  model_instance* last_build;

  /// Name to use for dotfile, if any
  shared_string* dotfile;

protected:
  /// The model instance currently being constructed
  model_instance* current;

public:
  model_def(const char* fn, int line, const type* t, char* n,
    formal_param **pl, int np);
protected:
  virtual ~model_def();
public:
  inline int NumSlots() const { return num_symbols; }
  inline const symbol* GetSymbol(int slot) const {
    DCASSERT(mysymbols);
    CHECK_RANGE(0, slot, num_symbols);
    return mysymbols[slot];
  }

  /** Fill the body of the function.
        @param  b  Expression = block of statements.
  */
  inline void SetStatementBlock(expr* b) {
    DCASSERT(0==stmt_block);
    stmt_block = b; 
  }

  /** Set the symbol table (done by compiler).
      These are the externally-visible symbols (usually measures).
      Still necessary for old-style of model calls.
  */
  inline void SetSymbolTable(symbol **st, int n) {
    DCASSERT(0==mysymbols);
    mysymbols = st;
    num_symbols = n;
  }

  /** Find an externally-visible symbol with specified name.
      Performs a binary search over mtable.
        @param  name  The symbol to look for.
        @return The slot number of the matching name (there can be only one)
                or -1 if there is none.
  */
  int FindVisible(const char* name) const;

  /** Set the dotfile name.
      Allows models to dump a dot file when they are instantiated.
  */
  void SetDotFile(result& x);

protected:
  /** Builds current instantiation.
      Model parameters have already been computed.
      Don't call this directly; it is used by Compute and Instantiate.
  */
  void BuildModel(traverse_data &x);

public:

  /** Start a warning message.
      Returns true on success.
  */
  bool StartWarning(const named_msg &who, const expr* cause) const;

  /** Finish a warning message.
  */
  void DoneWarning() const;
  
  /** Start an error message.
      Returns true on success.
  */
  bool StartError(const expr* cause) const;
  
  /** Finish an error message.
  */
  void DoneError() const;

  /** Check that a state variable belongs to this model!
      If not, a warning message is displayed.
        @param  mv    Model variable to check.
        @param  cause Cause, for error message.
        @param  what  String, (what happens now) for error message.
        @return true  If the model variable belongs to us;
                false otherwise.
  */
  bool isVariableOurs(const model_var* mv, const expr* cause, 
            const char* what) const;

  /** Build an actual instance of the model.
      This is the usual function calling interface.
  */
  virtual void Compute(traverse_data &x, expr** pass, int np);

  void PrintHeader(OutputStream &s, bool hide) const;
  virtual symbol* FindFormal(const char* name) const;
  virtual bool IsHidden(int fpnum) const;
  virtual bool HasNameConflict(symbol** fp, int np, int* tmp) const;
  virtual void Traverse(traverse_data &x);
  virtual int Traverse(traverse_data &x, expr** pass, int np);
  virtual int maxNamedParams() const;
  virtual int named2Positional(symbol** np, int nnp, expr** buffer, int bufsize) const;

  /// Like Compute(), but rebuild only if parameters have not changed.
  model_instance* Instantiate(traverse_data &x, expr** pass, int np);

  inline void AcceptSymbolOwnership(symbol* a) {
    DCASSERT(current);
    current->AcceptSymbolOwnership(a);
  }

  inline void AcceptExternalSymbol(int slot, symbol* s) {
    DCASSERT(current);
    current->AcceptExternalSymbol(slot, s);
  }

  inline void AcceptMeasure(measure* m) {
    DCASSERT(current);
    current->AcceptMeasure(m);
  }

  inline void GroupMeasure(measure* m) {
    DCASSERT(current);
    current->GroupMeasure(m);
  }

  // Methods that must be written in derived classes.

  /** Create a model variable of the specified type.
      Must be provided in derived classes.
        @param  wrap  Name and type of the variable.
        @param  bnds  Bounds, or 0 for none.
  */
  virtual model_var* MakeModelVar(const symbol* wrap, shared_object* s) = 0;

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

      Upon completion of this method, the variable "current" will
      be completely built, and any temporary data used for instantiation
      should be deleted.

      This function should call either
        ConstructionSuccess() or
        ConstructionError()

      @param  s  Stream to write the model to, as a dot file.
  */
  virtual void FinalizeModel(OutputStream &s) = 0;

  /// Call this when Finalization is successful for a high-level model.
  void ConstructionSuccess(hldsm* cm) {
    DCASSERT(current);
    current->SetConstructionSuccess(cm);
  }

  /// Call this when Finalization fails for any reason.
  void ConstructionError() {
    DCASSERT(current);
    current->SetConstructionError();
  }

private:
  bool SameParams() const;
  void SaveParams();
};

// ******************************************************************
// *                                                                *
// *                           Front  end                           *
// *                                                                *
// ******************************************************************

/// Initialize model def options.
void InitModelDefs(exprman* em);

#endif

