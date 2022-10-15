
/**
  This is a first step towards the ultimate goal of
  a completely general, extensible expression library.
  In such a library we will:
    - Register every type
    - Register every operation
    - Register solution engine types
    - Register solution engines

*/

#ifndef EXPRMAN_H
#define EXPRMAN_H

#include "../Streams/streams.h"
#include "type.h"
#include "expr.h"

class symbol;
class function;
class measure;
class engine;
class engtype;

class unary_op;
class binary_op;
class trinary_op;
class assoc_op;

class option;
class option_const;

class general_conv;
class specific_conv;

/** Abstract base class for external libraries.
    Right now, this is used only for "credits".

    To register an external library, derive a class from this one
    and implement the virtual functions.
*/
class library {
  bool has_copyright;
  bool has_release_date;
public:
  library(bool has_cr, bool has_date);
  virtual ~library();

  /** Can the library be determined from the pointer address?
      If so, we can very quickly check for duplicates;
      otherwise, we have to compare strings.

        @return  true  iff getVersionString() is unique for the library.
  */
  virtual bool hasFixedPointer() const = 0;

  /// Get the version string for a library.
  virtual const char* getVersionString() const = 0;

  /// Does the library have copyright info.
  inline bool hasCopyright() const { return has_copyright; }

  /** Print copyright info for a library.
      Default is to dump core, i.e., assuming there is no copyright info
      then this should not be called.
      Otherwise, if there is copyright info, then this must be overridden
      in the derived class.
  */
  virtual void printCopyright(doc_formatter *df) const;

  /// Does the library have a release date.
  inline bool hasReleaseDate() const { return has_release_date; }

  /** Print release date for a library.
      Default is to dump core.
  */
  virtual void printReleaseDate(doc_formatter *df) const;

};


/** Struct for dealing with groups of named messages.
    Used almost entirely automatically by struct named_msg.
*/
class group_of_named {
  int alloc;
  int curr;
  option_const** items;
public:
  group_of_named(int max);
  ~group_of_named();
   
  void AddItem(option_const* foo);
  
  /** Finish the group, and add an appropriate
      checklist item to the owner.
  */
  void Finish(option* owner, const char* n, const char* docs);
};

/** Struct for named report/warning/debug messages.
*/
class named_msg {
  static io_environ* io;
  bool active;
  const char* name;
  friend exprman* Initialize_Expressions(io_environ* io, option_manager* om);
public:
  /** Initialize.
        @param  owner If nonzero, a new checklist constant for this
                      item will be created and added to owner.
        @param  n     Name of the item.
        @param  d     Documentation (not required if \a owner is 0).
        @param  act   Are we initially active or not.

        @return       Created option constant, if any.
  */
  option_const* Initialize(option* owner, const char* n, const char* docs, bool act);

  inline bool isActive() const { return active; }
  inline bool canWrite() const { return active && io; }

  inline bool startWarning() const {
    if (!active) return false;
    if (!io)     return false;
    io->StartWarning();
    return true;
  }
  inline bool startReport() const {
    if (!active)  return false;
    if (!io)      return false;
    io->StartReport(name);
    return true;
  }
  inline void causedBy(const expr* x) const {
    DCASSERT(io);
    if (x)
      io->CausedBy(x->Filename(), x->Linenumber());
    else
      io->NoCause();
  }
  inline void causedBy(const char* fn, int ln) const {
    DCASSERT(io);
    io->CausedBy(fn, ln);
  }
  inline void noCause() const {
    DCASSERT(io);
    io->NoCause();
  }
  inline void newLine() const {
    DCASSERT(io);
    io->NewLine(name);
  }
  inline void stopIO() const {
    DCASSERT(io);
    io->Stop();
  }
  inline DisplayStream& report() const {
    DCASSERT(io);
    return io->Report;
  }
  inline FILE* Freport() const {
    if (io) return io->Report.getDisplay();
    return stdout;
  }
  inline DisplayStream& warn() const {
    DCASSERT(io);
    return io->Warning;
  }
  inline bool caughtTerm() const {
    DCASSERT(io);
    return io->caughtTerm();
  }
};


/** Expression manager class.
    This is an abstract base class, to provide the interface and hide
    the (possibly vast) implementation details.
    This huge class manages all modules that one can register for
    building and solving expressions.
    In essence, the class encapsulates all of the "state" of the
    expression library.

    Members that require a "registry" are virtual, and are implemented
    in a derived class.  Otherwise, members are not virtual, and
    are provided.

    To save our sanity, there is at most one expression manager
    created during the lifetime of an application.
    This way, we can store the expression manager as a static
    member of every expression without memory overhead.

    Included in this class are centralized collections of 
    options, engines, libraries, and types (including formalisms).
    Before the manager is "finalize()d", new engines, engine types,
    and options may be added, but engines may not be launched.
    After the manager is "finalize()d", new engines, options, libraries
    cannot be added.
*/
class exprman {
  io_environ* io;
protected:
  bool is_finalized;
  option_manager* om;

  // warnings and such
  named_msg promote_arg;

public:
  // "Fundamental" types, these need to be known lots of places.
  simple_type*  VOID;
  simple_type*  NULTYPE;
  simple_type*  BOOL;
  simple_type*  INT;
  simple_type*  REAL;
  simple_type*  MODEL;

  // Internal, for next state computations.
  const type* NEXT_STATE;

  // Optional types, placed in standalone modules.
  simple_type*  EXPO;
  simple_type*  STRING;
  simple_type*  BIGINT;
  simple_type*  STATESET;
  simple_type*  STATEDIST;
  simple_type*  STATEPROBS;
  simple_type*  TEMPORAL;
  simple_type*  TRACE;

  // Indicates "no engine".  This does NOT necessarily mean "easy to compute"
  engtype* NO_ENGINE;

  // Indicates a measure whose classification is waiting for dependencies.
  engtype* BLOCKED_ENGINE;

  /// Unary operators.
  enum unary_opcode {
    /// Boolean negation.
    uop_not       = 0,
    /// Arithmetic negation.
    uop_neg       = 1,
    /// Temporal operator "A".
    uop_forall    = 2,
    /// Temporal operator "E".
    uop_exists    = 3,
    /// Temporal operator "F"
    uop_future    = 4,
    /// Temporal operator "G"
    uop_globally  = 5,
    /// Temporal operator "X"
    uop_next      = 6,
    /// no operation (placeholder).  MUST BE THE LARGEST INTEGER.
    uop_none      = 7
  };

  /// Binary operators.
  enum binary_opcode {
    /// Boolean implication
    bop_implies   = 0,
    /// Modulo operator
    bop_mod       = 1,
    /// Set difference
    bop_diff      = 2,
    /// Check for equality
    bop_equals    = 3,
    /// Check for inequality
    bop_nequal    = 4,
    /// Greater than
    bop_gt        = 5,
    /// Greater or equal
    bop_ge        = 6,
    /// Less than
    bop_lt        = 7,
    /// Less or equal
    bop_le        = 8,
    /// Temporal operator "U"
    bop_until     = 9,
    /// Temporal operator "AND"
    bop_and       = 10,
    /// no operation (placeholder).  MUST BE THE LARGEST INTEGER.
    bop_none      = 11
  };

  /// Trinary operators.
  enum trinary_opcode {
    /// Set intervals.
    top_interval  = 0,
    /// If-then-else
    top_ite    = 1,
    /// no operation (placeholder).  MUST BE THE LARGEST INTEGER.
    top_none  = 2
  };

  /// Associative operators.
  enum assoc_opcode {
    /// Boolean AND
    aop_and    = 0,
    /// Boolean OR
    aop_or    = 1,
    /// Addition
    aop_plus  = 2,
    /// Multiplication
    aop_times  = 3,
    /// Aggregation
    aop_colon  = 4,
    /// Aggregation of void expressions
    aop_semi  = 5,
    /// Union of sets
    aop_union  = 6,
    /// no operation (placeholder).
    aop_none  = 7
  };

public:
  exprman(io_environ* io, option_manager* om);
  
  inline bool isFinalized() const { return is_finalized; }

  /// Called when we are done registering objects.
  virtual void finalize() = 0;

  inline const option_manager* OptMan() const { return om; }

  void addOption(option* o);
  option* findOption(const char* name) const;

  // +-----------------------------------------------------------------+
  // |                                                                 |
  // |                         Handy I/O hooks                         |
  // |                                                                 |
  // +-----------------------------------------------------------------+

  inline bool hasIO() const { return io; }

  inline bool isInteractive() const {
    if (io)  return io->IsInteractive();
    else  return false;
  }
  inline void setInteractive() {
    if (io)  io->SetInteractive();
  }
  inline void setBatch() {
    if (io)  io->SetBatch();
  }

  inline bool startInternal(const char* fn, int ln) const {
    if (!io) return false;
    io->StartInternal(fn, ln);
    return true;
  }
  inline bool startError() const {
    if (!io) return false;
    io->StartError();
    return true;
  }
  inline bool startWarning() const {
    if (!io)     return false;
    io->StartWarning();
    return true;
  }
  inline void causedBy(const expr* x) const {
    DCASSERT(io);
    if (x)
      io->CausedBy(x->Filename(), x->Linenumber());
    else
      io->NoCause();
  }
  inline void causedBy(const char* fn, int ln) const {
    DCASSERT(io);
    io->CausedBy(fn, ln);
  }
  inline void noCause() const {
    DCASSERT(io);
    io->NoCause();
  }
  inline void newLine(int delta = 0) const {
    DCASSERT(io);
    io->ChangeIndent(delta);
    io->NewLine();
  }
  inline void changeIndent(int delta) const {
    DCASSERT(io);
    io->ChangeIndent(delta);
  }
  inline void stopIO() const {
    DCASSERT(io);
    io->Stop();
  }
  inline InputStream& cin() const {
    DCASSERT(io);
    return io->Input;
  }
  inline DisplayStream& cout() const {
    DCASSERT(io);
    return io->Output;
  }
  inline FILE* Fstdout() const {
    if (io) return io->Output.getDisplay();
    return stdout;
  }
  inline DisplayStream& cerr() const {
    DCASSERT(io);
    return io->Error;
  }
  inline DisplayStream& warn() const {
    DCASSERT(io);
    return io->Warning;
  }
  inline DisplayStream& report() const {
    DCASSERT(io);
    return io->Report;
  }
  inline FILE* Freport() const {
    if (io) return io->Report.getDisplay();
    return stdout;
  }
  inline DisplayStream& internal() const {
    DCASSERT(io);
    return io->Internal;
  }
  inline bool caughtTerm() const {
    DCASSERT(io);
    return io->caughtTerm();
  }
  inline void waitTerm() {
    DCASSERT(io);
    io->WaitTerm();
  }
  inline void resumeTerm() {
    DCASSERT(io);
    io->ResumeTerm();
  }
  inline void Exit() {
    DCASSERT(io);
    io->Exit();
  }

  // +-----------------------------------------------------------------+
  // |                                                                 |
  // |                       Special expressions                       |
  // |                                                                 |
  // +-----------------------------------------------------------------+

  /** Build a special "error" expression.
      Should be used for, say, compile-time errors.
  */
  virtual expr* makeError() const = 0;

  /// Is the given expression the special "error" expression?
  virtual bool isError(const expr* e) const = 0;

  /** Build a special "default" expression.
      Used as a placeholder for defaults in function declarations.
  */
  virtual expr* makeDefault() const = 0;

  /// Is the given expression the special "default" expression?
  virtual bool isDefault(const expr* e) const = 0;

  /** Is the given expression "ordinary"?
      This is shorthand for: not null, not Error, not Default.
  */
  virtual bool isOrdinary(const expr* e) const = 0;

  // +-----------------------------------------------------------------+
  // |                                                                 |
  // |                              Types                              |
  // |                                                                 |
  // +-----------------------------------------------------------------+

  /** Register a data type.
      This gives a much nicer mechanism for dealing with types,
      and for adding new types.
      In particular, it allows us to add formalisms.
        @param  f       The formalism to register.
        @return true,   on success.
                false,  if the operation could not be completed
                        for any reason.
  */
  virtual bool registerType(type* t) = 0;

  /** Set the fundamental types.
      Should be called after the fundamental types have been registered,
      but before they are used.
        @return  true  iff all the required fundamental types were found.
  */
  virtual bool setFundamentalTypes() = 0;

  /** Find "one-word, definable" types.
      A type is "definable" iff a user can declare a function
      or a variable of that type.
      A type is "one word" if its name consists of a single word
      (i.e., it is "deterministic" and not "proc").
      Not extremely efficient, but since this is
      normally used only at compile time, should be fine.
        @param  name  A one-word name.
        @return A definable type whose name matches \a name, or
                0 if none exists.
  */
  virtual const type* findOWDType(const char* name) const = 0;

  /** Find a "simple" type with the given name.
      Not constant, so we can use this when initializing types.
        @param  name  A type name.
        @return A definable type whose name matches \a name, or
                0 if none exists.
  */
  virtual simple_type* findSimple(const char* name) = 0;

  /** Find a type with the given name.
      May catch types that findOWDType() misses.
      Not extremely efficient, but used at compile or
      initialization time, it should be fine.
        @param  name  A type name.
        @return A definable type whose name matches \a name, or
                0 if none exists.
  */
  virtual const type* findType(const char* name) const = 0;
     
  /// Returns the modifier with given name, or NO_SUCH_MODIF.
  virtual modifier findModifier(const char* name) const = 0;

  /// Get the current number of registered types.
  virtual int getNumTypes() const = 0;
  
  /// Get the ith registered type.
  virtual const type* getTypeNumber(int i) const = 0;

  /// Safe way to get expression type
  inline const type* SafeType(const expr *e, int comp) const {
    if (e)  return e->Type(comp);
    else  return NULTYPE;
  }

  /// Safe way to get (known simple) expression type
  inline const type* SafeType(const expr* e) const {
    if (e)  return e->Type();
    else  return NULTYPE;
  }

  // +-----------------------------------------------------------------+
  // |                                                                 |
  // |                       Building  constants                       |
  // |                                                                 |
  // +-----------------------------------------------------------------+


  /** Make a "literal" value.
        @param  file  Filename of definition.
        @param  line  Line number of definition.
        @param  t     Type of the value.
        @param  c     The value to draw from.
        @return       A new expression.
  */
  expr* makeLiteral(const char* file, int line, 
      const type* t, const result& c) const;


  // +-----------------------------------------------------------------+
  // |                                                                 |
  // |                        Building  symbols                        |
  // |                                                                 |
  // +-----------------------------------------------------------------+


  /** Make an iterator variable.
      I.e., a variable used as an array index, also as for-loop iterators.
      Implemented in forloops.cc.
        @param  fn    Filename of the variable.
        @param  ln    Line number of the variable.
        @param  t     Type of the variable.
        @param  name  Name of the variable.
        @param  vals  Set of values for the variable.
        @return 0, if some error occurred.
                A new expression, otherwise.
  */
  symbol* makeIterator(const char* fn, int ln, const type* t,
      char* name, expr* vals) const;


  /** Make a "constant" (function with no parameters) symbol, 
      not within a converge block.
      Implemented in symbols.cc.
        @param  fn    Filename of the variable.
        @param  ln    Line number of the variable.
        @param  t     Type of the variable.
        @param  name  Name of the variable.
        @param  rhs   Expression to assign to this symbol.
        @param  deps  List of symbols that must be
                      "computed" before this one.
        @return 0, if some error occurred.
                A new expression, otherwise.
  */
  symbol* makeConstant(const char* fn, int ln, const type* t,
      char* name, expr* rhs, List <symbol> *deps) const;


  /** Make a "constant" (function with no parameters) symbol, 
      not within a converge block.
      Implemented in symbols.cc.
        @param  w     Wrapper symbol around this one.
        @param  rhs   Expression to assign to this symbol.
        @param  deps  List of symbols that must be
                      "computed" before this one.
        @return 0, if some error occurred.
                A new expression, otherwise.
  */
  symbol* makeConstant(const symbol* w, expr* rhs, List <symbol> *deps) const;


  // +-----------------------------------------------------------------+
  // |                                                                 |
  // |                       Building statements                       |
  // |                                                                 |
  // +-----------------------------------------------------------------+


  /** Make an expression statement.
      Normally this is allowed only for void expressions,
      and in this case the expression is returned unchanged.
      In the mean time, we also allow statements of the form:
         5+7;
      which will simply cause the expression to be computed and printed.
      Implemented in stmts.cc.

        @param  fn  Filename.
        @param  ln  Line number.
        @param  e   Expression

        @return ERROR, if an error ocurred.
                A new "compute statement", otherwise.
  */
  expr* makeExprStatement(const char* file, int line, expr* e) const;


  /** Make an option-setting statement.
      Implemented in stmts.cc.

        @param  file  Filename.
        @param  line  Line number.
        @param  o     Option to set.
        @param  e     Value to assign to the option.

        @return ERROR, if an error occurred.
                A new statement, otherwise.
  */
  expr* makeOptionStatement(const char* file, int line, 
        option *o, expr *e) const;


  /** Make an option-setting statement.
      Implemented in stmts.cc.

        @param  file  Filename.
        @param  line  Line number.
        @param  o     Option to set.
        @param  v     Constant to assign to the option.

        @return ERROR, if an error occurred.
                A new statement, otherwise.
  */
  expr* makeOptionStatement(const char* file, int line, 
      option *o, option_const *v) const;


  /** Make a checkbox-setting statement.
      Used for checkbox-style options.  Note that the action is performed
      directly on the option constant.  The option is provided only
      for display purposes.
      Implemented in stmts.cc.

        @param  file  Filename.
        @param  line  Line number.
        @param  o     Option.
        @param  check If true, we will "check the box";
                      otherwise, we will "uncheck the box".
        @param  vlist List of items to check or uncheck.
        @param  nv    Number of list items.

        @return ERROR, if an error occurred.
                A new statement, otherwise.
  */
  expr* makeOptionStatement(const char* file, int line, option* o, 
    bool check, option_const **vlist, int nv) const;


  /** Make a for loop statement.
      This is an expression with type "void" which,
      when computed, will set all possible assignments to
      a set of iterator variables, and for each assignment,
      will execute another statement (void-type expression).
      Implemented in forloops.cc.
    
        @param  fn    Filename.
        @param  ln    Line number.
        @param  iters Array of iterators.
        @param  dim   Dimension of the loop (number of iterators).
        @param  stmt  The statement to execute within the loop.
                      (can be a statement block).

        @return ERROR,  if some error occurred.
                A new void-type expression, otherwise.
  */
  expr* makeForLoop(const char* fn, int ln, 
      symbol** iters, int dim, expr* stmt) const;


  // +-----------------------------------------------------------------+
  // |                                                                 |
  // |                         Arrays 'n' such                         |
  // |                                                                 |
  // +-----------------------------------------------------------------+

  /** Make a new array.
      Implemented in arrays.cc.
  
        @param  fn      Filename of the array.
        @param  ln      Line number of the array.
        @param  t       Type of the array.
        @param  name    Name of the array.
        @param  indexes List of iterators, which define the "shape"
                        of the array.  Each iterator must have been
                        created by calling MakeIterator().
        @param  dim     Length of the list of indexes.
                        Can be considered the dimension of the array.
        @return 0, if some error occurred (will make noise).
                A new array, otherwise.
  */
  symbol* makeArray(const char* fn, int ln, const type* t,
      char* n, symbol** indexes, int dim) const;

  /** Make an array assignment statement, not within a converge block.
      These handle statements of the form
        int a[i][j][k] := rhs;
      Implemented in arrays.cc.

        @param  fn    Filename of the statement.
        @param  ln    Line number of the statement.
        @param  array Array to use for the assignment.
                      Must have been created by a call to MakeArray().
        @param  rhs   The right-hand side of the assignment.
        @return ERROR, if some error occurred (will make noise).
                A new statement, otherwise.
  */
  expr* makeArrayAssign(const char* fn, int ln, 
      symbol* array, expr* rhs) const;

  // +-----------------------------------------------------------------+
  // |                                                                 |
  // |                   Array and function  "calls"                   |
  // |                                                                 |
  // +-----------------------------------------------------------------+


  /** Make an array dereferencing expression.
      These handle expressions of the form
        a[3][5+n][4-3*foobar(7, x)]
      Implemented in arrays.cc.

        @param  fn      Filename of the expression.
        @param  ln      Line number of the expression.
        @param  array   Array to use.
                        Must have been created by a call to MakeArray().
        @param  indexes List of indices to be "passed" to the array.
        @param  dim     Number of indices in the list \a indexes.
        @return ERROR, if some error occurred (will make noise).
                A new expression, otherwise.
  */
  expr* makeArrayCall(const char* fn, int ln, 
      symbol* array, expr** indexes, int dim) const;

  /** Make an expression to call a function.
      Passed parameters must match exactly in type.

        @param  fn  Filename we are declared in.
        @param  ln  line number we are declared on.
        @param  f   The function to call.  Can be user-defined, 
                    internal, or pretty much anything.
        @param  p   The parameters to pass, as an array of expressions.
        @param  np  Number of passed parameters.
  */
  expr* makeFunctionCall(const char* fn, int ln, 
      symbol *f, expr **p, int np) const;

  /** Find a (list of) function callable within a model.
        @param  mt  Type of model we are calling within.
        @param  n   Name of the function.
        @return A list of functions with name \a n that can be called
                inside a model of type \a mt.
                This will either be a list of "generic" functions,
                callable within any type of model, or a list of functions
                specific to models of type \a mt.
                0 signifies an empty list.
  */
  symbol* findFunction(const type* mt, const char* n);


  // +-----------------------------------------------------------------+
  // |                                                                 |
  // |                       Converge statements                       |
  // |                                                                 |
  // +-----------------------------------------------------------------+


  /** Make a variable within a converge block.
      Implemented in converge.cc.

        @param  fn  Filename of the variable.
        @param  ln  Line number of the variable.
        @param  t   Type of the variable.
        @param  n   Name of the variable.
        @return 0, if some error occurred (will make noise).
                A new variable, otherwise.
  */
  symbol* makeCvgVar(const char* fn, int ln, const type* t, char* name) const;

  /** Make a converge statement.
      Implemented in converge.cc.

        @param  fn    Filename.
        @param  ln    Line number.
        @param  stmt  The statement to execute within the converge.
                      (can be a statement block).
        @param  top   True iff this is the topmost converge statement.
                      (If so we will "affix" variables after executing.)
      
        @return ERROR,  if some error occurred.
                A new void-type expression, otherwise.
  */
  expr* makeConverge(const char* fn, int ln, expr* stmt, bool top) const;


  /** Make a guess statement for inside a converge block.
      These handle statements of the form
        real foo guess rhs;
      Implemented in converge.cc.

        @param  fn      Filename of the statement.
        @param  ln      Line number of the statement.
        @param  cvgvar  Converge variable to use.
                        Must have been created by a call to MakeCvgVar().
        @param  rhs     The right-hand side of the guess.
        @return 0, if some error occurred (will make noise).
                A new statement, otherwise.
  */
  expr* makeCvgGuess(const char* fn, int ln, symbol* cvgvar, expr* rhs) const;


  /** Make an assignment statement for inside a converge block.
      These handle statements of the form
        real foo := rhs;
      that appear within a converge block.
      Implemented in converge.cc.

        @param  fn      Filename of the statement.
        @param  ln      Line number of the statement.
        @param  cvgvar  Converge variable to use.
                        Must have been created by a call to MakeCvgVar().
        @param  rhs     The right-hand side of the assignment.
        @return 0, if some error occurred (will make noise).
                A new statement, otherwise.
  */
  expr* makeCvgAssign(const char* fn, int ln, symbol* cvgvar, expr* rhs) const;


  /** Make an array guess statement for inside a converge block.
      These handle statements of the form
        real foo[i][j] guess rhs;
      Implemented in converge.cc.

        @param  fn      Filename of the statement.
        @param  ln      Line number of the statement.
        @param  cvgvar  Converge variable to use.
                        Must have been created by a call to MakeCvgVar().
        @param  rhs     The right-hand side of the guess.
        @return 0, if some error occurred (will make noise).
                A new statement, otherwise.
  */
  expr* makeArrayCvgGuess(const char* fn, int ln, symbol* array, expr* guess) const;


  /** Make an array assignment statement, within a converge block.
      These handle statements of the form
        int a[i][j][k] := rhs;
      that appear within a converge block.
      Implemented in converge.cc.

        @param  fn    Filename of the statement.
        @param  ln    Line number of the statement.
        @param  array Array to use for the assignment.
                      Must have been created by a call to MakeArray().
        @param  rhs   The right-hand side of the assignment.
        @return ERROR, if some error occurred (will make noise).
                A new statement, otherwise.
  */
  expr* makeArrayCvgAssign(const char* fn, int ln, 
        symbol* array, expr* rhs) const;


  // +-----------------------------------------------------------------+
  // |                                                                 |
  // |                         Changing  types                         |
  // |                                                                 |
  // +-----------------------------------------------------------------+

  /// Register a general type conversion rule.
  virtual void registerConversion(general_conv *) = 0;

  /// Register a specific type conversion rule.
  virtual void registerConversion(specific_conv *) = 0;

  /** Returns the "promotion distance" from type t1 to t2.
      For example:
        -  getPromoteDistance(INT, BOOL) = -1
        -  getPromoteDistance(INT, INT) = 0
        -  getPromoteDistance(INT, REAL) = 1
        -  getPromoteDistance(INT, PH_INT) = 2
        -  getPromoteDistance(INT, PROC_INT) = 4
        -  getPromoteDistance(INT, PROC_RAND_INT) = 6
        -  getPromoteDistance(INT, PROC_RAND_REAL) = 7
        -  getPromoteDistance(INT, SET_REAL) = 9
    
        @param  t1  Original type
        @param  t2  Target type
    
        @return -1, if the promotion is impossible
                0,  if the types are equal
                n,  a binary encoding of which level(s)
                    needs changing: 
                    1 for base type,
                    2 for rand / phase / determ,
                    4 for proc / non-proc,
                    8 for set / non-set.
  */
  virtual int getPromoteDistance(const type* t1, const type* t2) const = 0;


  /// Returns true if type t1 can be promoted to type t2.
  inline bool isPromotable(const type* t1, const type* t2) const { 
    return getPromoteDistance(t1, t2) >= 0; 
  }

  /** Least common type that types a and b can be promoted to.
      Useful for, say, determining the type of
        rand int + proc real  
      (which is proc rand real).

        @param  a  First type.
        @param  b  Second type.

        @return If such a type \a c exists, return the \a c that
                minimizes the (non-negative) promotion distance from a to c,
                and the non-negative promotion distance from b to c.
                Otherwise, return 0.
  */
  const type* getLeastCommonType(const type* a, const type* b) const;

  /** Returns true if type t1 can be cast to type t2.
      This includes both promotions and explicit casts.

        @param  t1  Original type
        @param  t2  Target type

        @return true  iff it is possible to change from
                      type \a t1 to type \a t2.
  */
  virtual bool isCastable(const type* t1, const type* t2) const = 0;

  /** Make a type conversion expression.

        @param  file    Filename of the expression.
        @param  line    Line number of the expression.
        @param  newtype The desired type.
        @param  e       The original expression.
        @return 0,      if e is 0.
                ERROR,  if e is ERROR, or if the change of type 
                        is impossible.
                e,      if e already has type \a newtype.
                a new expression, otherwise.
  */
  virtual expr* makeTypecast(const char* file, int line, 
      const type* newtype, expr* e) const = 0;

  /** Make a type conversion expression.
      If a component of e is not promotable as requested, we return ERROR.
      If all components of e are already of the specified type, we return it.

        @param  file  Filename of the expression.
        @param  line  Line number of the expression.
        @param  proc  Should components of fp be promoted to proc.
        @param  rand  Should components of fp be promoted to rand.
        @param  fp    An expression whose type we want to match.
        @param  e     The original expression.
                      both \a e and \a fp must have the same number
                      of components.
        @return 0,      if e is 0.
                ERROR,  if e is ERROR, or if the change of type 
                        is impossible.
                e,      if e already has type \a newtype.
                a new expression, otherwise.
  */
  expr* makeTypecast(const char* file, int line, 
      bool proc, bool rand, const expr *fp, expr* e) const;

  /** Make a type conversion expression.
      Like MakeTypecast(), but rules out type casts (e.g., real to int).
      It is assumed that this is for parameter passing, so the new
      expression will have the same filename and linenumber as the original.

        @param  e       The original expression.
        @param  newtype The desired type.
        @return 0,      if e is 0.
                ERROR,  if e is ERROR, or if no promotion is possible.
                e,      if e already has type \a newtype.
                a new expression, otherwise.
  */
  expr* promote(expr* e, const type* newtype) const;

  /** Make a type conversion expression.
      Like MakeTypecast(), but rules out type casts (e.g., real to int).
      It is assumed that this is for parameter passing, so the new
      expression will have the same filename and linenumber as the original.

        @param  e     The original expression.
        @param  proc  Should fp components be promoted to proc.
        @param  rand  Should fp components be promoted to rand.
        @param  fp    An expression (e.g., formal parameter)
                      whose type we want to match.
        @return 0,      if e is 0.
                ERROR,  if e is ERROR, or if no promotion is possible.
                e,      if e already has type \a newtype.
                a new expression, otherwise.
  */
  expr* promote(expr* e, bool proc, bool rand, const expr* fp) const;

  // +-----------------------------------------------------------------+
  // |                                                                 |
  // |                Registration of operator handlers                |
  // |                                                                 |
  // +-----------------------------------------------------------------+

  /** Register a unary operation.
        @param  op  Unary operation to register.
        @return true,   on success.
                false,  if any error occurred.
  */
  virtual bool registerOperation(unary_op* op) = 0;

  /** Register a binary operation.
        @param  op  Binary operation to register.
        @return true,   on success.
                false,  if any error occurred.
  */
  virtual bool registerOperation(binary_op* op) = 0;

  /** Register a trinary operation.
        @param  op  Trinary operation to register.
        @return true,   on success.
                false,  if any error occurred.
  */
  virtual bool registerOperation(trinary_op* op) = 0;

  /** Register an associative operation.
        @param  op  Associative operation to register.
        @return true,   on success.
                false,  if any error occurred.
  */
  virtual bool registerOperation(assoc_op* op) = 0;

  // +-----------------------------------------------------------------+
  // |                                                                 |
  // |               Building expressions with operators               |
  // |                                                                 |
  // +-----------------------------------------------------------------+

  static const char* getOp(unary_opcode op);
  static const char* getOp(binary_opcode op);
  static const char* getFirst(trinary_opcode op);
  static const char* getSecond(trinary_opcode op);
  static const char* getOp(bool flip, assoc_opcode op);

  static const char* documentOp(unary_opcode op);
  static const char* documentOp(binary_opcode op);
  static const char* documentOp(trinary_opcode op);
  static const char* documentOp(bool flip, assoc_opcode op);

  /** Determine the type of a unary operation expression.

        @param  op    The operation to perform.
        @param  opnd  The operand type.
        @return 0,  on any kind of error;
                the type of the operation, otherwise.
  */
  virtual const type* getTypeOf(unary_opcode op, const type* x) const = 0;

  /** Determine the type of a binary operation expression.

        @param  left  The left operand type.
        @param  op    The operation to perform.
        @param  right The right operand type.
        @return 0,  on any kind of error;
                the type of the operation, otherwise.
  */
  virtual const type* getTypeOf(const type* left, binary_opcode op, 
          const type* right) const = 0;

  /** Determine the type of a trinary operation expression.

        @param  op      The operation to perform.
        @param  left    The left operand type.
        @param  middle  The middle operand type.
        @param  right   The right operand type.
        @return 0,  on any kind of error;
                the type of the operation, otherwise.
  */
  virtual const type* getTypeOf(trinary_opcode op, const type* left,
      const type* middle, const type* right) const = 0;

  /** Determine the type of an associative operation (sub)expression.

        @param  left  The left operand type.
        @param  flip  Do we "flip" the operation.
        @param  op    The operation to perform.
        @param  right The right operand type.
        @return 0,  on any kind of error;
                the type of the operation, otherwise.
  */
  virtual const type* getTypeOf(const type* left, bool flip, assoc_opcode op, 
          const type* right) const = 0;

  /** Make a unary operation expression.

        @param  file  Filename of the expression.
        @param  line  Line number of the expression.
        @param  op    The operation to perform.
        @param  opnd  The operand.
        @return NULL,   if opnd is NULL.
                ERROR,  if opnd is ERROR, or if there is a type mismatch.
                a new expression, otherwise.
  */
  virtual expr* makeUnaryOp(const char* file, int line, 
      unary_opcode op, expr* opnd) const = 0;


  /** Make a binary operation expression.

        @param  fn    Filename of the expression.
        @param  ln    Line number of the expression.
        @param  left  The left operand.
        @param  op    The operation to perform.
        @param  rt    The right operand.
        @return NULL,   if either opnd is NULL.
                ERROR,  if either opnd is ERROR, 
                        or if the operand cannot be applied
                        (i.e., type mismatch).
                a new expression, otherwise.
  */
  virtual expr* makeBinaryOp(const char* fn, int ln, 
      expr* left, binary_opcode op, expr* rt) const = 0;


  /** Make a trinary operation expression.

        @param  fn  Filename of the expression.
        @param  ln  Line number of the expression.
        @param  op  The operation to perform.
        @param  l   The left operand.
        @param  m   The middle operand.
        @param  r   The right operand.
        @return ERROR,  if any opnd is ERROR, 
                        or if the operand cannot be applied
                        (i.e., type mismatch).
                a new expression, otherwise.
  */
  virtual expr* makeTrinaryOp(const char* fn, int ln, trinary_opcode op, 
        expr* l, expr* m, expr* r) const = 0;


  /** Make an associative operation expression.

        @param  fn    Filename of the expression.
        @param  ln    Line number of the expression.
        @param  op    The operation to perform.
        @param  opnds Array of operands.
        @param  f     For each operand, should it be "flipped"?
                      If missing (null), we assume that no
                      operands should be flipped.
        @param  nops  Number of operands.
        @return NULL,   if any opnd is NULL.
                ERROR,  if any opnd is ERROR, 
                        or if the operand cannot be applied
                        (i.e., type mismatch).
                a new expression, otherwise.
  */
  virtual expr* makeAssocOp(const char* fn, int ln, assoc_opcode op, 
        expr** opnds, bool* f, int nops) const = 0;


  // +-----------------------------------------------------------------+
  // |                                                                 |
  // |                         Building models                         |
  // |                                                                 |
  // +-----------------------------------------------------------------+


  /** Start construction of a model definition, for a given model type.
        @param  fn      Filename of the model definition.
        @param  ln      Line number of the model definition.
        @param  t       Type of formalism.
        @param  name    Name of the model.
        @param  formals List of formal parameters.
        @param  np      Number of formal parameters.

        @return NULL,  if some error occurred.
                A new model definition, otherwise.
  */
  model_def* makeModel(const char* fn, int ln, const type* t,
        char* name, symbol** formals, int np) const;

  /** Make a symbol within a model definition.
      The symbol is simply a "placeholder" that will be replaced
      each time the model is instantiated.
      Implemented in mod_vars.cc.

        @param  fn    Filename of the symbol.
        @param  ln    Line number of the symbol.
        @param  t     Type of the symbol.
        @param  name  Name of the symbol.

        @return  a new placeholder symbol.
  */
  symbol* makeModelSymbol(const char* fn, int ln, 
        const type* t, char* name) const;


  /** Make an array within a model definition.
      The array is simply a "placeholder" that will be replaced
      each time the model is instantiated.
      Implemented in mod_vars.cc.
  
        @param  fn      Filename of the array.
        @param  ln      Line number of the array.
        @param  t       Type of the array.
        @param  name    Name of the array.
        @param  indexes List of iterators, which define the "shape"
                        of the array.  Each iterator must have been
                        created by calling MakeIterator().
        @param  dim     Length of the list of indexes.
                        Can be considered the dimension of the array.
        @return  0, if some error occurred (will make noise).
                A new array, otherwise.
  */
  symbol* makeModelArray(const char* fn, int ln, const type* t, char* n, 
      symbol** indexes, int dim) const;


  /** Make a statement for constructing model variables.
      This handles declarations of the form
        place p, q, r, s;
      within a model.
      Implemented in mod_vars.cc.

        @param  fn      Filename of the declaration.
        @param  ln      Line number of the declaration.
        @param  p       Model definition block containing the declaration.
        @param  t       Type of the variables.
        @param  bounds  Bounds for the variables (a set of something), or 0.
        @param  names   Names of the variables
                        (they already exist as placeholder symbols).
        @param  N       Number of names.

        @return NULL,   if any name is NULL,
                ERROR,  if any error occurs,
                a new statement (void expression), otherwise.
  */
  expr* makeModelVarDecs(const char* fn, int ln, model_def* p, 
    const type* t, expr* bounds, symbol** names, int N) const;


  /** Make a statement for constructing model array variables.
      This handles declarations of the form
        place p[i], q[i], r[i], s[i];
      within a model.
      Implemented in mod_vars.cc.

        @param  fn      Filename of the declaration.
        @param  ln      Line number of the declaration.
        @param  p       Model definition block containing the declaration.
        @param  t       Type of the arrays.
        @param  arrays  Names of the arrays
                        (they already exist as placeholder symbols).
        @param  N       Number of arrays.
      
        @return NULL,   if any name is NULL,
                ERROR,  if any error occurs,
                a new statement (void expression), otherwise.
  */
  expr* makeModelArrayDecs(const char* fn, int ln, model_def* p, 
      const type* t, symbol** arrays, int N) const;


  /** Make a statement to build a measure in a model.
      Implemented in mod_vars.cc.

        @param  fn  Filename of the statement.
        @param  ln  Line number of the statement.
        @param  p   Model definition block containing the statement.
        @param  m   The measure.
        @param  rhs Right-hand side of the measure assignment.

        @return NULL,   if p, m, or rhs is NULL.
                ERROR,  if some error occurs (will make noise).
                A new statement, otherwise.
  */
  expr* makeModelMeasureAssign(const char* fn, int ln, 
      model_def* p, symbol* m, expr* rhs) const;
      

  /** Make a statement to build an array of measures in a model.
      Implemented in mod_vars.cc.

        @param  fn  Filename of the statement.
        @param  ln  Line number of the statement.
        @param  p   Model definition block containing the statement.
        @param  am  The array of measures.
        @param  rhs Right-hand side of the measure assignment.

        @return NULL,   if p, m, or rhs is NULL.
                ERROR,  if some error occurs (will make noise).
                A new statement, otherwise.
  */
  expr* makeModelMeasureArray(const char* fn, int ln, 
      model_def* p, symbol* am, expr* rhs) const;


  /** Finish a model definition.
      Implemented in mod_def.cc.

        @param  p       Model "header", everything is ready except for
                        the statements and symbols within the model.
        @param  stmts   Block of statements to execute when
                        instantiating the model.
        @param  st      Array of externally-visible symbols
                        (usually measures, or arrays of measures) 
                        of the model.
        @param  ns      Number of externally-visible symbols.
  */
  void finishModelDef(model_def* p, expr* stmts, 
      symbol** st, int ns) const;


  // +-----------------------------------------------------------------+
  // |                                                                 |
  // |                Building  model and measure calls                |
  // |                                                                 |
  // +-----------------------------------------------------------------+
  
  /** Determine if a function is actually a model definition.
      (The compiler is unable to determine this by itself.)
  */
  static model_def* isAModelDef(symbol* f);

  /** Make a measure call expression.
      For expressions of the form
        foo(3.4, 7).msr;
      I.e., to be used when we call the model definition directly.
      Implemented in mod_def.cc.

        @param  fn    Filename of the expression.
        @param  ln    Line number of the expression.
        @param  p     Model definition block containing the measure.
        @param  pass  Parameters to pass to the model definition.
        @param  np    Number of passed parameters.
        @param  name  Name of the measure.

        @return ERROR,  if some error occurs (e.g., no measure with
                        the given name), with error messages relayed
                        as appropriate on the error channel.
                A new expression p(pass).name, otherwise.
  */
  expr* makeMeasureCall(const char* fn, int ln, model_def* p, 
      expr** pass, int np, const char* name) const;

  /** Make a measure array call expression.
      For expressions of the form
        foo(3.4, 7).msr[i, j, k];
      I.e., to be used when we call the model definition directly.
      Implemented in mod_def.cc.

        @param  fn    Filename of the expression.
        @param  ln    Line number of the expression.
        @param  p     Model definition block containing the measure.
        @param  pass  Parameters to pass to the model definition.
        @param  np    Number of passed parameters.
        @param  name  Name of the measure array.
        @param  i     Passed array indexes.
        @param  ni    Number of passed indexes.

        @return ERROR,  if some error occurs (e.g., no measure with
                        the given name, wrong dimension), with error 
                        messages relayed as appropriate on the error channel.
                A new expression p(pass).name[i], otherwise.
  */
  expr* makeMeasureCall(const char* fn, int ln, model_def* p, 
      expr** pass, int np, const char* name,
      expr** i, int ni) const;

  /** Make a measure call expression.
      For expressions of the form
        m.msr;
      where m is a constant with type "model".
      Implemented in mod_inst.cc.

        @param  fn    Filename of the expression.
        @param  ln    Line number of the expression.
        @param  mi    A model instance.
        @param  name  Name of the measure.
  
        @return ERROR,  if some error occurs (will make noise).
                A new measure call expression, otherwise.
  */
  expr* makeMeasureCall(const char* fn, int ln,
      symbol* mi, const char* name) const;

  /** Make a measure array call expression.
      For expressions of the form
        m.msr[i, j, k];
      where m is a constant with type "model".
      Implemented in mod_inst.cc.

        @param  fn    Filename of the expression.
        @param  ln    Line number of the expression.
        @param  mi    A model instance.
        @param  name  Name of the measure.
        @param  i     Passed array indexes.
        @param  ni    Number of passed indexes.
  
        @return ERROR,  if some error occurs (will make noise).
                A new measure call expression, otherwise.
  */
  expr* makeMeasureCall(const char* fn, int ln,
      symbol* mi, const char* name,
      expr** i, int ni) const;

  /** Make a measure array call expression.
      For expressions of the form
        m[i, j, k].msr;
      where m is a constant with type "model".
      Implemented in mod_inst.cc.

        @param  fn    Filename of the expression.
        @param  ln    Line number of the expression.
        @param  mi    A model instance array.
        @param  i     Passed array indexes.
        @param  ni    Number of passed indexes.
        @param  name  Name of the measure.
  
        @return ERROR,  if some error occurs (will make noise).
                A new measure call expression, otherwise.
  */
  expr* makeMeasureCall(const char* fn, int ln,
      symbol* mi, expr** i, int ni,
      const char* name) const;

  
  /** Make a measure array call expression.
      For expressions of the form
        m[i].msr[j];
      where m is a constant with type "model".
      Implemented in mod_inst.cc.

        @param  fn    Filename of the expression.
        @param  ln    Line number of the expression.
        @param  mi    A model instance array.
        @param  i     Passed array indexes, for mi.
        @param  ni    Number of passed indexes for mi.
        @param  name  Name of the measure.
        @param  j     Passed array indexes, for name.
        @param  nj    Number of passed indexes for name.
  
        @return ERROR,  if some error occurs (will make noise).
                A new measure call expression, otherwise.
  */
  expr* makeMeasureCall(const char* fn, int ln,
      symbol* mi, expr** i, int ni,
      const char* name, expr** j, int nj) const;

  

  // +-----------------------------------------------------------------+
  // |                                                                 |
  // |                        Solution  engines                        |
  // |                                                                 |
  // +-----------------------------------------------------------------+
  
  /** Register a new type of solution engine.
        @param  et  Engine type to register (see engine.h)
        @return true on success
  */
  virtual bool registerEngineType(engtype* et) = 0;

  /** Find the engine type with the given name, if there is one.
        @param  name  Name of engine type to look for.
        @return The desired engine type, or 0 if none found.
  */
  virtual engtype* findEngineType(const char* name) const = 0;

  /// Get the current number of registered engine types.
  virtual int getNumEngineTypes() const = 0;
  
  /// Get the ith registered engine type.
  virtual const engtype* getEngineTypeNumber(int i) const = 0;


  // +-----------------------------------------------------------------+
  // |                                                                 |
  // |                      Supporting  libraries                      |
  // |                                                                 |
  // +-----------------------------------------------------------------+
  
  /** Register an external supporting library.
        @param  lib  The library to register.
        @return  0,   on success.
                1,  if \a lib is 0.
                2,  if the manager is already finalized.
                3,  if there is no version string for this library.
                4,  if this library is a duplicate
                    of another one registered.
  */
  virtual char registerLibrary(const library* lib) = 0;

  /// Print version info for all registered supporting libraries.
  virtual void printLibraryVersions(OutputStream &s) const = 0;

  /// Print copyright info for all registered supporting libraries.
  virtual void printLibraryCopyrights(doc_formatter* df) const = 0;

protected:
  virtual ~exprman();  // don't want user to delete one...
  friend void destroyExpressionManager(exprman* em);  // ... except this way
};


// ******************************************************************
// *                                                                *
// *                           Front  end                           *
// *                                                                *
// ******************************************************************

/** Initialize the expression manager and return it.

      @param  io  Streams to use for errors and such.

      @param  om  Collection to add any expression-related
                  options into.  If 0, the options will
                  still be created, but it will be impossible
                  to change them from their default values.

      @return The existing expression manager (and ignoring the parameters)
              if one already exists.
              Otherwise, creates and returns an expression manager.
*/
exprman* Initialize_Expressions(io_environ* io, option_manager* om);

/// Return the expression manager, or 0 if none exists.
exprman* getExpressionManager();

/// Destroy the expression manager.
void destroyExpressionManager(exprman* em);


#endif
