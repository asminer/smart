
// $Id$

#ifndef FUNCTIONS_H
#define FUNCTIONS_H

/** \file functions.h

  Functions with parameters.

  The interface is heavy, and designed for those who need to typecheck
  functions (e.g., a compiler) or need to implement built-in functions.
  Everyone else can probably get away with using exprlib.h or exprbuild.h

  Major changes from earlier versions:
    (1)   We will derive built-in functions from class function
          rather than use pointers to C functions.
          (Different semantics, same effect.)

    (2)   Since a built-in function can override the virtual
          function Traverse(), we can have custom behavior
          for LOTS of different expression operations.

    (3)   We allow the type of the function to depend
          on the passed parameters!

*/

#include "symbols.h"
#include "result.h"

/// Used for user-defined, and "simple" internal functions.
class formal_param;

/// Used by simple_internal.
class model_instance;

// ******************************************************************
// *                                                                *
// *                         function class                         *
// *                                                                *
// ******************************************************************

/**   The base class of functions with parameters.

      This is the third generation design for functions.
      Hopefully it will be clean, efficient, and easy to use.
*/  

class function : public symbol {
public:
  static const int Promote_Success = 0;
  static const int Promote_MTMismatch = 1;
  static const int Promote_Dependent = 2;

public:
  function(const function* f);
  function(const char* fn, int line, const type* t, char* n);

protected:
  virtual ~function();

public:
  /** Typechecking macro.
        @param  np  Number of passed parameters.
        @return The code for "not enough passed parameters".
  */
  inline static int NotEnoughParams(int np) { return -np-2; }

  /** Typechecking macro.
        @param  np  Number of passed parameters.
        @return The code for "too many passed parameters".
  */
  inline static int TooManyParams(int np) { return -np-3; }

  /** Typechecking macro.
        @param  i   Bad passed parameter.
        @param  np  Number of passed parameters.
        @return The code for "parameter i is bad".
  */
  inline static int BadParam(int i, int np) { 
    CHECK_RANGE(0, i, np);
    return -i-1; 
  }
  
  /** Is a given formal parameter hidden?
        @param  fpnum  Formal parameter number.
  */
  virtual bool IsHidden(int fpnum) const = 0;

  /** The type of the function, given the positional parameters.
        @param  pass  Array of positional passed parameters.
        @param  np    Number of passed parameters.
        @return The return type of the function in this case.
  */
  const type* DetermineType(expr** pass, int np);

  /** The model type of the function, given the positional parameters.
        @param  pass  Array of positional passed parameters.
        @param  np    Number of passed parameters.
        @return The model type of the function in this case.
  */
  const model_def* DetermineModelType(expr** pass, int np);

  /** Give a type-checking score for positional parameters.
        @param  pass  Array of passed parameters, in order.
        @param  np    Number of passed parameters.
        @return A "score" for how well the parameters match, as follows.
                0:  Perfect match in type and number.
                +n: Total promotion distance required for
                    passed parameters to match formal parameters.
                -x: An appropriate code.
                    See the "macros"
                    NotEnoughParams(), TooManyParams(), BadParam().
  */
  int TypecheckParams(expr** pass, int np);


  /** Promote passed parameters as necessary.
        @param  pass  Array of passed parameters, in order.
        @param  np    Number of passed parameters.
        @return One of the "Promote" codes, 0 on success.
  */
  int PromoteParams(expr** pass, int np);

  virtual void Compute(traverse_data &x);
  virtual void Traverse(traverse_data &x);

  /** Call the function with the given positional parameters.
      Must be defined in derived classes.
        @param  x     Traversal data.
        @param  pass  Passed (positional) parameters.
        @param  np    Number of passed parameters.
  */
  virtual void Compute(traverse_data &x, expr** pass, int np) = 0;

  /** Traverse the function with the given positional parameters.
      Useful for all kinds of fancy behavior.
      Must be defined in derived classes to be useful.
        @param  x     Traversal data.
        @param  pass  Passed (positional) parameters.
        @param  np    Number of passed parameters.
        @return A value based on the type of traversal:
    
          - Typecheck:
            returns a score of how well the given
            parameters match the formal ones in type.
            See TypecheckParams().

          - Promote:
            returns an appropriate code:
              Promte_Success        on success
              Promote_MTMismatch    if model parameters have different parents
              Promote_Dependent     if RV params are dependent

          - Substitute:
            If 1 is returned, then the function has
            built a special substituted expression,
            stored in x.answer.
            Otherwise, 0 is returned, and a function
            call substitution can be constructed as usual.

          - All others:
            Not used.  The return value is stored
            in the traverse structure \a x.
  */
  virtual int Traverse(traverse_data &x, expr** pass, int np);

  /** Print the function header.
      For example, print out:
        real sqrt(real x)

        @param  s     Output stream to use.
        @param  hide  If true,  when the first parameter is hidden for 
                                model functions, indicate that this function
                                must appear in a model.
                      If false, shows the complete function header.
  */
  virtual void PrintHeader(OutputStream &s, bool hide) const = 0;
  
  /** Document the header only.
  */
  virtual bool DocumentHeader(doc_formatter* df) const;

  /** Document the behavior only.
  */
  virtual void DocumentBehavior(doc_formatter* df) const;

  virtual void PrintDocs(doc_formatter* df, const char* keyword) const;

  /** Does this header match the passed one?
      Used to detect duplicate or conflicting user-defined functions.
      Parameters must match exactly, including names.
        @param  t   Return type
        @param  fp  Formal parameter list
        @param  np  Number of parameters

        @return  true, iff:
                  this is a user-defined function,
                  it is not yet "defined",
                  and the defaults (if any) match.
  */
  virtual bool HeadersMatch(const type* t, symbol** fp, int np) const;

  /** Is there a name conflict with the given parameters?
      I.e., if we define another function with the same name,
      with the given parameters, can there be ambiguity
      when calling the function with named parameters?
        @param  fp    Formal parameter list to compare against ours.
        @param  np    Number of parameters
        @param  tmp   Scratch space, array with dimension at least np.

        @return   true,   if the new function should not be allowed;
                  false,  if we don't see any problems.
  */
  virtual bool HasNameConflict(symbol** fp, int np, int* tmp) const;

  /** Find a formal parameter with the given name.
      Used by the compiler.

        @param  name  Name of formal parameter to find.

        @return 0, if not found; the formal parameter, otherwise.
  */
  virtual symbol* FindFormal(const char* name) const;


  /** Give the maximum number of named parameters that can be passed.
      Default: returns a negative number.

        @return negative, if this function cannot be called with named parameters;
                max number of named parameters, otherwise.
  */
  virtual int maxNamedParams() const;

  /** Convert named parameters to positional ones.
      Does NO typechecking; only converts based on parameter names.

        @param  np      Input: Array of named parameters 
        @param  nnp     Input: Number of named parameters
        @param  buffer  Output: Positional parameters will be stored here
        @param  bufsize Input: Size of buffer. Must be at least 
                        as large as maxNamedParams().

        @return non-negative:     number of positional parameters
                -1..-nnp:         negated position of first 
                                  named parameter not found 
                -nnp-1..-nnp-mnp: position of required positional parameter,
                                  not provided (negated, and decreased by nnp).

                -a lot            if we cannot do it (default implementation
                                  returns this).
  */
  virtual int named2Positional(symbol** np, int nnp, expr** buffer, int bufsize) const;
};

// ******************************************************************
// *                                                                *
// *                          fplist class                          *
// *                                                                *
// ******************************************************************

/** Class for managing lists of formal parameters.
    Simplifies life for user-defined functions, models, and
    simple internal functions.
*/
class fplist {
protected:
  formal_param** formal;
  int num_formal;
  int repeat_point;
public:
  fplist();
  ~fplist();

  /// Number of parameters.
  inline int getLength() const { return num_formal; }

  /// Grab a parameter
  inline formal_param* getParam(int n) {
    CHECK_RANGE(0, n, num_formal);
    DCASSERT(formal);
    return formal[n];
  }

  /// Allocate formals.
  void allocate(int nf);

  /// Set list of formal parameters.
  void setAll(int nf, formal_param** pl, bool subst);

  /// Set stack locations
  void setStack(result** stack);

  /// Clear formals.
  void clear();

  /** Build a formal parameter.
        @param  n     Formal parameter number to set.
        @param  t     Type of formal parameter.
        @param  name  Name of formal parameter.
  */
  void build(int n, const type* t, const char* name);

  /** Build a formal parameter.
        @param  n     Formal parameter number to set.
        @param  t     Type of formal parameter.
        @param  name  Name of formal parameter.
        @param  deflt Default value for the parameter
  */
  void build(int n, const type* t, const char* name, expr* deflt);

  /** Build a formal parameter.
        @param  n     Formal parameter number to set.
        @param  t     Type of formal parameter.
        @param  name  Name of formal parameter.
  */
  void build(int n, typelist* t, const char* name);

  /** Hide a formal parameter.
        @param  n  Formal parameter number to set.
  */
  void hide(int n);

  /// Type of parameter n, assuming it is simple!
  const type* getType(int n) const;

  /// Is parameter n hidden?
  bool isHidden(int n) const;

  /** Set the "repeat point".
      If the function does not have repeating parameters, this
      should not be called.
      Otherwise, if the repeat point is set to r, then
      the formal parameter list specifies parameters:
      0, 1, 2, ..., r, r+1, ..., num_formal-1, r, r+1, ...
      where the number of parameters passed must be
        num_formal + k*(num_formal - r)
      for some non-negative integer k.

      @param  r  Repeat point.
  */
  inline void setRepeat(int r) { repeat_point = r; }

  /// Prints the formal params, surrounded by parens. 
  void PrintHeader(OutputStream &s, bool hide) const;

  /// Compute formals, assuming no repeats!
  inline void compute(traverse_data &x, expr** pass, result* stack) const {
    result* save = x.answer;
    for (int i=0; i<num_formal; i++) {
      x.answer = stack;
      SafeCompute(pass[i], x);
      stack++;
    }
    x.answer = save;
  }

  /// Find formal param with the given name.
  int findIndex(const char* name) const;

  /// Is there a formal param with given name?
  inline symbol* find(const char* name) const {
    int i = findIndex(name);
    if (i<0) return 0;
    return (symbol*) formal[i];
  }

  /// Do the formal params match
  bool matches(symbol** pl, int np) const;

  /// Is there a name conflict between these formal param lists
  bool hasNameConflict(symbol** pl, int np, int* tmp) const;

private:
  /** Give type-checking scores for positional parameters.
      Each score indicuates how well the parameters match, as follows.
         0: Perfect match in type and number.
        +n: Total promotion distance required for
            passed parameters to match formal parameters.
        -x: An appropriate code for failure to match.

        @param  em      Expression manager.
        @param  pass    Array of passed parameters, in order.
        @param  np      Number of passed parameters.
        @param  scores  Dimension is at least 4.
                On output:
                  scores[0] is the score with no formal promotions.
                  scores[1] is the score if formals are made "rand".
                  scores[2] is the score if formals are made "proc".
                  scores[3] is the score if formals are made "proc rand".
  */
  void check(const exprman* em, expr** pass, int np, int* scores) const;



public:
  /** Give a type-checking score for positional parameters, and return type.
      The return type is required so we can determine if
      "formal parameter promotion" is possible or not.

      Basically, calls the other version of check and then makes sure
      the return type can be promoted as required.

      @param  em    Expression manager.
      @param  pass  Array of passed parameters, in order.
      @param  np    Number of passed parameters.
      @param  rt    Unmodified return type.

      @return The score, taking everything into account.
  */
  int check(const exprman* em, expr** pass, int np, const type* rt) const;


  /** Return type, based on passed parameters.
      Assumes the passed parameters will fit with some kind of 
      formal parameter promotion.

      @param  em    Expression manager.
      @param  pass  Array of passed parameters, in order.
      @param  np    Number of passed parameters.
      @param  rt    Unmodified return type.

      @return Modified return type, according to promotions
              necessary to formal parameters.
  */
  const type* getType(const exprman* em, expr** pass, 
        int np, const type* rt) const;

  /** Promote passed parameters as necessary.
      @param  em    Expression manager.
      @param  pass  Array of passed parameters, in order.
      @param  np    Number of passed parameters.
      @param  rt    Return type, as a sanity check.
      @return true on success.
  */
  bool promote(const exprman* em, expr** pass, int np, const type* rt) const;

  /// Traverse all formals.
  void traverse(traverse_data &x);

  /** Convert named parameters to positional ones.
      Does NO typechecking; only converts based on parameter names.

        @param  em      Expression manager; needed to build defaults.
        @param  np      Input: Array of named parameters 
        @param  nnp     Input: Number of named parameters
        @param  buffer  Output: Positional parameters will be stored here
        @param  bufsize Input: Size of buffer. Must be at least 
                        as large as maxNamedParams().

        @return non-negative:     number of positional parameters
                -1..-nnp:         negated position of first 
                                  named parameter not found 
                -nnp-1..-nnp-mnp: position of required positional parameter,
                                  not provided (negated, and decreased by nnp).

                -a lot            if we cannot do it (default implementation
                                  returns this).
  */
  int named2Positional(exprman* em, symbol** np, int nnp, 
          expr** buffer, int bufsize) const;

private:
  
  // Helper for hasNameConflict
  int findParamWithName(symbol** pl, int np, int* tmp, const char* name) const;
};

// ******************************************************************
// *                                                                *
// *                      internal_func  class                      *
// *                                                                *
// ******************************************************************

/**   An abstract base class of internal functions.
      This class handles documentation, which is common
      for all derived classes.
*/  
class internal_func : public function {
  const char* docs;
  bool hidden;  // not documented in release version
public:
  /** Constructor.
        @param  t     Function return type.
        @param  name  Function name.
  */
  internal_func(const type* t, const char* name);

  /// Hide the function from documentation.
  inline void Hide() { hidden = true; }

  /// Set documentation for this function.
  inline void SetDocumentation(const char* d) { docs = d; }

  virtual bool DocumentHeader(doc_formatter* df) const;
  virtual void DocumentBehavior(doc_formatter* df) const;
};

// ******************************************************************
// *                                                                *
// *                     simple_internal  class                     *
// *                                                                *
// ******************************************************************

/**   The base class of internal functions with formal parameters.

      Use for built-in functions without customized type checking.
*/  
class simple_internal : public internal_func {
protected:
  fplist formals;
public:
  /** Constructor.
        @param  t     Function return type (fixed).
        @param  name  Function name.
        @param  nf    Number of formal parameters.
  */
  simple_internal(const type* t, const char* name, int nf);
  virtual ~simple_internal();

  inline void SetFormal(int n, const type* t, const char* name) {
    formals.build(n, t, name);
  }

  inline void SetFormal(int n, const type* t, const char* name, expr* deflt) {
    formals.build(n, t, name, deflt);
  }

  inline void SetFormal(int n, typelist* t, const char* name) {
    formals.build(n, t, name);
  }

  virtual bool IsHidden(int fpnum) const;

  virtual bool HasNameConflict(symbol** fp, int np, int* tmp) const;

  /** Set the "repeat point".
      If the function does not have repeating parameters, this
      should not be called.
      Otherwise, if the repeat point is set to r, then
      the formal parameter list specifies parameters:
      0, 1, 2, ..., r, r+1, ..., num_formal-1, r, r+1, ...
      where the number of parameters passed must be
        num_formal + k*(num_formal - r)
      for some non-negative integer k.

      @param  r  Repeat point.
  */
  inline void SetRepeat(int r) { formals.setRepeat(r); }

  virtual int Traverse(traverse_data &x, expr** pass, int np);
  virtual void PrintHeader(OutputStream &s, bool hide) const;
  virtual symbol* FindFormal(const char* name) const;

  virtual int maxNamedParams() const;
  virtual int named2Positional(symbol** np, int nnp, expr** buffer, int bufsize) const;

  /** For model functions and "measures".
      Obtain the passed "model instance" from the first parameter.
      An error message is displayed if an error occurs.
  */
  model_instance* grabModelInstance(traverse_data &x, expr* first) const;
};

// ******************************************************************
// *                                                                *
// *                      model_internal class                      *
// *                                                                *
// ******************************************************************

/**   The base class of internal model functions with formal parameters.

      Basically, a thin wrapper around simple_internal,
      automatically adding the hidden model parameter.
*/  
class model_internal : public simple_internal {
public:
  /** Constructor.
        @param  t     Function return type (fixed).
        @param  name  Function name.
        @param  nf    Number of visible formal parameters.
  */
  model_internal(const type* t, const char* name, int nf);
  virtual ~model_internal();
};

// ******************************************************************
// *                                                                *
// *                     custom_internal  class                     *
// *                                                                *
// ******************************************************************

/**   The base class of internal functions with crazy type checking.
*/  
class custom_internal : public internal_func {
  const char* header;
public:
  custom_internal(const char* name, const char* header);
  virtual void PrintHeader(OutputStream &s, bool hide) const;
  // default: nothing is hidden.
  virtual bool IsHidden(int fpnum) const;
};

// ******************************************************************
// *                                                                *
// *                           front  end                           *
// *                                                                *
// ******************************************************************

symbol* MakeFormalParam(typelist* t, char* name);

/** Used primarily by compiler.
    Build a formal parameter for a function or model.
    
      @param  fn        Filename of declaration
      @param  ln        Line number of declaration
      @param  t         Data type of parameter
      @param  name      Parameter name
      @param  in_model  Is this for a function defined within a model?

      @return An appropriate symbol.
*/
symbol* MakeFormalParam(const char* fn, int ln, 
                        const type* t, char* name, bool in_model);


/** Used primarily by compiler.
    Build a formal parameter with a default value for a function or model.
    
      @param  em        Expression manager for error reporting.
      @param  fn        Filename of declaration
      @param  ln        Line number of declaration
      @param  t         Data type of parameter
      @param  name      Parameter name
      @param  def       The default value (an expression)
      @param  in_model  Is this for a function defined within a model?

      @return An appropriate symbol, or 0 on error (will make noise).
*/
symbol* MakeFormalParam(const exprman* em, const char* fn, int ln, 
                        const type* t, char* name, expr* def, bool in_model);


/** Used primarily by compiler.
    Build a named parameter.

      @param  fn        Filename of declaration
      @param  ln        Line number of declaration
      @param  name      Parameter name
      @param  pass      Expression passed to the parameter

      @return An appropriate symbol.
*/
symbol* MakeNamedParam(const char* fn, int ln, char* name, expr* pass);


/** Used primarily by compiler.
    Build a user-function "header".

      @param  em        Expression manager for error reporting.
      @param  fn        Filename of declaration
      @param  ln        Line number of declaration
      @param  t         Return type of function
      @param  name      Name of function
      @param  formals   Array of formal parameters
      @param  np        Number of formal parameters
      @param  in_model  Is this function defined within a model?

      @return An appropriate function, or 0 on error (will make noise).
*/
function* MakeUserFunction(const exprman* em, const char* fn, int ln, 
            const type* t, char* name, symbol** formals, int np, bool in_model);

/** Used primarily by compiler.
    Build a user-defined "header" with no parameters.

      @param  em        Expression manager for error reporting.
      @param  fn        Filename of declaration
      @param  ln        Line number of declaration
      @param  t         Return type of function
      @param  name      Name of function
      @param  in_model  Is this function defined within a model?

      @return An appropriate function, or 0 on error (will make noise).
*/
function* MakeUserConstFunc(const exprman* em, const char* fn, int ln, 
            const type* t, char* name, bool in_model);

/** Used primarily by compiler, for forward-defined functions.
    Reset the formal parameters for an existing user function.
    This must be done for forward-defined functions, e.g.:
      int foo(int a, int b);
      int foo(int c, int d) := c+d;
    The old formal parameters are destroyed.
      @param  em        Expression manager for error reporting.
      @param  fn        Filename of declaration
      @param  ln        Line number of declaration
      @param  userfunc  User function to modify.
      @param  formals   Array of formal parameters
      @param  nfp       Number of formal parameters
*/
void ResetUserFunctionParams(const exprman* em, const char* fn, int ln, 
                              symbol* userfunc, symbol** formals, int nfp);


/** Build a "user function definition" statement.
    E.g., a statement of the form
      int foo(int c, int d) := c+d;
    If we are not inside a model function, we simply set the return
    value for the function (and return 0); otherwise an actual
    statement is required.
      @param  em        Expression manager for error reporting.
      @param  fn        Filename of declaration
      @param  ln        Line number of declaration
      @param  userfunc  User function to modify.
                        Should have been created using
                        MakeUserFunction().
      @param  rhs       Return expression for the function.
      @param  mdl       Model that contains this function, or 0 for none.

      @return 0, if mdl is 0 or an error occurred.
              An appropriate void type expression (statement) otherwise.
*/
expr* DefineUserFunction(const exprman* em, const char* fn, int ln, 
                          symbol* userfunc, expr* rhs, model_def* mdl);

void InitFunctions(exprman* om);

#endif

