
#ifndef EXPR_H
#define EXPR_H

#include "../include/defines.h"
#include "../include/shared.h"
#include "../include/list.h"
#include "../Utils/location.h"
#include "../Utils/messages.h"
#include "type.h"
#include "result.h"

class debugging_msg;
class result;
class rng_stream;

// class io_environ;    // defined in Streams module

class option;
class option_enum;

class model_def;  // defined in models.h

class sv_encoder;  // defined in dd_front.h

class shared_state; // defined in mod_vars.h

class expr;    // defined below
class symbol;  // defined in symbols.h

// ******************************************************************
// *                                                                *
// *                      traverse_data struct                      *
// *                                                                *
// ******************************************************************

/** Information used for traversing expressions.
    Allows for flexibility and speed.  Not bad.
 */
struct traverse_data {
  /// Types of expression traversals
  enum traversal_type {
    /// Do nothing; useful default.
    None = 0,
    /// Compute the expression.
    Compute,
    /// Like compute, but specialized for expo rates.
    ComputeExpoRate,
    /// Construct a decision diagram encoding of this expression.
    BuildDD,
    /// Construct a decision diagram encoding of the expo rate expression.
    BuildExpoRateDD,
    /// For random values, determine the range of possible values.
    FindRange,
    /// Pre-compute values in const to rand promotions.
    PreCompute,
    /// Clear pre-computed values.
    ClearCache,
    /// Reset guess statements within a converge.
    Guess,
    /// Update values.  Used in converge statements.
    Update,
    /// Finalizes a statement block within another statement.
    Block,
    /// Affix values.  Used primarily in converge statements.
    Affix,
    /// Create a copy with values substituted for certain symbols.
    Substitute,
    /// Get a list of symbols.
    GetSymbols,
    /// Get a list of symbols this expression depends on.
    GetVarDeps,
    /// Get a list of terms in a huge product.
    GetProducts,
    /// Get a list of measures contained in an expression.
    GetMeasures,
    /// Get the current type.  Used by functions.
    GetType,
    /// Perform typechecking.  Used by functions.
    Typecheck,
    /// Promote parameters.  Used by functions.
    Promote,
    /// Signifies that model instantiation is complete.
    ModelDone,
    /// Prepare for generating the set of states satisfying a temporal formula. Used by temporal operations.
    TemporalStateSet,
    /// Prepare for generating a trace verifying a temporal formula. Used by temporal operations.
    TemporalTrace
  };

  /// Input: Traversal type.
  traversal_type which;
  /// Input: aggregate index (normally 0)
  int aggregate;
  /// Random number stream to use, for "rand" expressions.
  rng_stream* stream;
  /// The current state, for "proc" expressions.
  shared_state* current_state;
  /// The current state index, for "proc" expressions.
  long current_state_index;
  /// The next state; written to for "next state" expressions.
  shared_state* next_state;

  /// Interface for building decision diagrams.
  sv_encoder* ddlib;

  /// Parent (calling) expression, if any.
  const expr* parent;

  /// Parent model, if any.
  model_def* model;

  /// The return value, usually.
  result* answer;

  /// Output: type, used by functions.
  const type* the_type;

  /// Output: model type, used by functions.
  const model_def* the_model_type;

  /// Output: callback function, used by CTL witness generation.
  const expr* the_callback;

  /// Used as necessary for lists of exprs.
  List <expr> *elist;

  /// Used as necessary for lists of symbols.
  List <symbol> *slist;

  /// status, for converges.
  bool needs_repeating;

public:
  /// Handy: constructor
  traverse_data(traversal_type w) {
    Clear(w);
  }

  /// Handy for debugging and such
  bool Print(std::ostream &s) const;

  void Clear(traversal_type w) {
    which = w;
    aggregate = 0;
    stream = 0;
    current_state = 0;
    next_state = 0;
    answer = 0;
    ddlib = 0;
    parent = 0;
    model = 0;
    elist = 0;
    slist = 0;
    the_type = 0;
    the_model_type = 0;
    the_callback = 0;
    needs_repeating = 0;
  }

  /** Statements: should we stop execution?
      for later expansion as necessary.
  */
  inline bool stopExecution() const { return false; }

  /** For converge statements:
      would (some) statement like to execute again?
  */
  inline bool wantsToRepeat() const { return needs_repeating; }

  inline void setRepeat() { needs_repeating = true; }
  inline void clrRepeat() { needs_repeating = false; }
};


// ******************************************************************
// *                                                                *
// *                           expr class                           *
// *                                                                *
// ******************************************************************


/**   The base class of all expressions and statements.

      Possible states of an expression.
      Some of these states only make sense for symbols, a derived class.
        -2  = error:      Construction error, set to special "error" expr.
        -1  = null:       Construction error, set to null.
        0   = declared:   Symbol is declared but not defined.
        1   = guessed:    Symbol is declared, and has a guess value.
        2   = defined:    Symbol is declared and defined.
        3   = blocked:    Symbol cannot be computed until some dependencies
                          are resolved.
        4   = ready:      Dependencies are resolved, but symbol has
                          not been computed yet.
        5   = computed:   Symbol has been computed.

      New: expressions contain a unique identifier (an integer)
      for comparisons; this is to be preferred (for portability)
      over comparing pointers, but the effect is exactly the same.
*/

class expr : public shared_object {
    public:
        /** Constructor for simple types.
                @param  W     Where declared.
                @param  t     simple type of expression.
        */
        expr(const location& W, const type* t);

        /** Constructor for aggregates.
                @param  W     Where declared.
                @param  t     Aggregate type, will be "owned" by expression.
        */
        expr(const location& W, typelist* t);

        /** NOT a copy constructor.
            Sets the filename, linenumber, and type
            from the given expression.
                @param  x  Expr to grab from.
        */
        expr(const expr* x);

    protected:
        virtual ~expr();

        /// Helper for constructors
        void Init(const location &W, const type* st,
            typelist* at, const model_def* mt);

    public:
        inline int getID() const { return IDnum; }

        inline bool OK() const { return state >= 0; }
        inline bool hadNull() const { return -1 == state; }
        inline bool hadError() const { return -2 == state; }
        inline bool isComputed() const { return state >= 5; }
        inline bool isReady() const { return state >= 4; }
        inline bool isBlocked() const { return 3 == state; }
        inline bool isDefined() const { return state >= 2; }
        inline bool isGuessed() const { return state >= 1; }

        inline void setGuessed() {
            CHECK_RANGE(0, state, 1);
            state = 1;
        }
        inline void setDefined() {
            CHECK_RANGE(0, state, 2);
            state = 2;
        }
        inline void setBlocked() {
            CHECK_RANGE(0, state, 3);   // or 4?
            state = 3;
        }
        inline void setReady() {
            CHECK_RANGE(0, state, 4);
            state = 4;
        }
        inline void setComputed() {
            CHECK_RANGE(0, state, 5);
            state = 5;
        }

    protected:
        inline void setOK() { state = 0; }
        inline void setNull() { state = -1; }
        inline void setError() { state = -2; }

    public:

        /** Useful for rare cases when we do not know the expression
            type a priori.  If this is to be used, then the expression
            should be constructed either with type 0, or a typelist of 0
            (the two are equivalent).
                @param  t  The type of the expression.
        */
        void SetType(const type* t);

        /** Useful for rare cases when we do not know the expression
            type a priori.  If this is to be used, then the expression
            should be constructed either with type 0, or a typelist of 0
            (the two are equivalent).
                @param  t  The type of the expression.
        */
        void SetType(typelist* t);

        /** Useful for rare cases when we do not know the expression
            type a priori.  If this is to be used, then the expression
            should be constructed either with type 0, or a typelist of 0
            (the two are equivalent).
                @param  t  Expression to take the type from.
        */
        void SetType(const expr* t);

        /** Set the model that constructed us.
            Will also set the type if necessary.

                @param  mt  Model definition that the expression
                            is based on.  Makes sense only for
                            expressions of type MODEL.
        */
        void SetModelType(const model_def* mt);

        inline const model_def* GetModelType() const { return model_type; }

        inline const location& Where() const { return where; }

        /// The number of aggregate components in this expression.
        inline int NumComponents() const {
            return (aggtype) ? aggtype->Length() : 1;
        }

        /// Return a pointer to component i.
        virtual expr* GetComponent(int i);

        /// The type of component i.
        inline const type* Type(int i) const {
            if (aggtype) {
                return aggtype->GetItem(i);
            } else {
                DCASSERT(0==i);
                return simple;
            }
        }

        /// The type, assuming we are simple.
        inline const type* Type() const {
            DCASSERT(0==aggtype);
            return simple;
        }

        /** Show the type.  Super handy!
            virtualness: super duper handy for error expressions!
        */
        virtual void PrintType(std::ostream &s) const;

        /** Compute the expression.
            A special case of Traversal, because it needs to be fast.
        */
        virtual void Compute(traverse_data &x);

        /** Get the expression name.
            Will be NULL unless this is a symbol, in which case
            the symbol name is returned.
        */
        virtual const char* Name() const;

        /** Get the expression name, as a shared_object.
            Will be NULL unless this is a symbol.
        */
        virtual shared_object* SharedName() const;

        /** Rename the expression.
            @param  newname The new name to use, as a shared_object.
                            (Should be a shared_string.)
        */
        virtual void Rename(shared_object* newname);

        /// Returns true if the types and names match perfectly.
        bool Matches(const expr* sym) const;


        /** Clear the compute cache.
            We use a compute cache for each function with no parameters.
            This is required to handle dependent random variables correctly!
            So, we need a meachanism to clear the caches.
            This method is a front-end for Traverse().
        */
        void ClearCache();

        /** Compute the const parts of an expression.
            Used to prevent repeated sampling of consts, e.g.:
                Avg(uniform(0.0, 1000.0) + Fib(20));
                Avg(uniform(Fib(1), Fib(20));
                real m := avg_ss(tk(p) + Fib(20));

            Also, causes execution of converge "guess" statements.

            This method is a front-end for Traverse().
        */
        void PreCompute();

        /** Fix values of converge variables.
            This method is a front-end for Traverse().
        */
        void Affix();

        /** Create a copy of this expression with values substituted
            for certain symbols.
            (The symbols themselves determine the substitution.)
            Normally this is used by arrays within for loops.
            We make shallow copies (shared pointers to expressions)
            whenever possible.
                @param  i  The component to substitute.
        */
        expr* Substitute(int i);

        /** Like substitute, but for measures.
        */
        expr* Measurify(model_def* parent);


        /** Build a list of expressions for a particular type of traversal.
                @param  w   Type of list to build.
                @param  i   The component to check.
                @param  L   List in which we try to store items;
                            can be 0.
                @return     The number of items that should appear in the
                            list. The actual list might be smaller (e.g.,
                            if list is 0, or we ran out of memory).
        */
        int BuildExprList(traverse_data::traversal_type w, int i,
                List <expr> *L);

        /** Build a list of symbols for a particular type of traversal.
                @param  w   Type of list to build.
                @param  i   The component to check.
                @param  L   List in which we try to store items;
                            can be 0.
                @return     The number of items that should appear in the
                            list. The actual list might be smaller (e.g.,
                            if list is 0, or we ran out of memory).
        */
        int BuildSymbolList(traverse_data::traversal_type w, int i,
                List <symbol> *L);

        /** Traverse the expression.
            All non-critical expression traversals are
            handled by this function, by specifying the
            appropriate members of the passed parameter.

            @param  x  Struct of data for traversals.

            Based on the value of \a which, we do the following.
            For all of them, \a aggregate specifies
            the component to use of an aggregate expression.

            PreCompute:
                pre-compute and remember
                any "const" parts of the expression
                (because we are about to sample it,
                perhaps several times).

            ClearCache:
                clear any memory used by PreCompute.

            Substitute:
                Make a copy of this expression, with values
                substituted for the appropriate symbols,
                and return an expression pointer in \a answer.

            GetSymbols:
            GetVarDeps:
            GetProducts:
            GetMeasures:
                Input: \a answer is a list of expressions
                (see objlist.h)
                Output: we have added the requested items to the list.
                        We do not check for duplicates, so
                        the same item may appear several times.

            GetVarDeps:
                Like GetSymbols, except it also includes any
                symbols that a symbol depends on.

            BuildDD:
                Builds a decision diagram for this expression,
                using the specified Decision Diagram library
                (member \a ddlib); the result is stored as a
                shared_object in member \a answer.
        */
        virtual void Traverse(traverse_data &x);
        virtual long getDelta() const;
        virtual long getLower() const;
        virtual long getUpper() const;

    //
    // static things and their manipulation.
    //

    public:
        static inline const type* SafeType(const expr* x)
        {
            return x ? x->Type() : type::null;
        }

        /** Make an expression statement.
            Normally this is allowed only for void expressions,
            and in this case the expression is returned unchanged.
            In the mean time, we also allow statements of the form:
                5+7;
            which will simply cause the expression to be computed and printed.
            Implemented in stmts.cc.

                @param  W     Where defined.
                @param  e   Expression

                @return ERROR, if an error ocurred.
                        A new "compute statement", otherwise.
        */
        static expr* makeExprStatement(const location& W, expr* e);


        /** Make an option-setting statement.
            Implemented in stmts.cc.

                @param  W     Where defined.
                @param  o     Option to set.
                @param  e     Value to assign to the option.

                @return ERROR, if an error occurred.
                        A new statement, otherwise.
        */
        static expr* makeOptionStatement(const location& W,
                option *o, expr *e);


        /** Make an option-setting statement.
            Implemented in stmts.cc.

                @param  W     Where defined.
                @param  o     Option to set.
                @param  v     Constant to assign to the option.

                @return ERROR, if an error occurred.
                        A new statement, otherwise.
        */
        static expr* makeOptionStatement(const location& W,
            option *o, option_enum *v);


        /** Make a checkbox-setting statement.
            Used for checkbox-style options.  Note that the action is
            performed directly on the option constant.  The option is
            provided only for display purposes.
            Implemented in stmts.cc.

                @param  W       Where defined.
                @param  o       Option.
                @param  check   If true, we will "check the box";
                                otherwise, we will "uncheck the box".
                @param  vlist   List of items to check or uncheck.
                @param  nv      Number of list items.

                @return ERROR, if an error occurred.
                        A new statement, otherwise.
        */
        static expr* makeOptionStatement(const location& W, option* o,
            bool check, option_enum **vlist, int nv);


        /** Make a for loop statement.
            This is an expression with type "void" which,
            when computed, will set all possible assignments to
            a set of iterator variables, and for each assignment,
            will execute another statement (void-type expression).
            Implemented in forloops.cc.

                @param  W       Where defined.
                @param  iters   Array of iterators.
                @param  dim     Dimension of the loop (number of iterators).
                @param  stmt    The statement to execute within the loop.
                                (can be a statement block).

                @return ERROR,  if some error occurred.
                        A new void-type expression, otherwise.
        */
        static expr* makeForLoop(const location& W,
            symbol** iters, int dim, expr* stmt);

        /** Make an array assignment statement, not within a converge block.
            These handle statements of the form
                int a[i][j][k] := rhs;

            @param  W       Where defined.
            @param  array   Array to use for the assignment.
                            Must have been created by a call to MakeArray().
            @param  rhs     The right-hand side of the assignment.
            @return ERROR,  if some error occurred (will make noise).
                            A new statement, otherwise.
        */
        static expr* makeArrayAssign(const location& W, symbol* array,
            expr* rhs);

        /** Make a converge statement.
            Implemented in converge.cc.

                @param  W       Where defined.
                @param  stmt    The statement to execute within the converge.
                                (can be a statement block).
                @param  top     True iff this is the topmost converge
                                statement. (If so we will "affix" variables
                                after executing.)

                @return ERROR,  if some error occurred.
                                A new void-type expression, otherwise.
        */
        static expr* makeConverge(const location& W, expr* stmt, bool top);

        /** Make a guess statement for inside a converge block.
            These handle statements of the form
                real foo guess rhs;
            Implemented in converge.cc.

                @param  W       Where defined.
                @param  cvgvar  Converge variable to use.  Must have
                                been created by a call to MakeCvgVar().
                @param  rhs     The right-hand side of the guess.
                @return 0,      if some error occurred (will make noise).
                                A new statement, otherwise.
        */
        static expr* makeCvgGuess(const location& W, symbol* cvgvar, expr* rhs);


        /** Make an assignment statement for inside a converge block.
            These handle statements of the form
                real foo := rhs;
            that appear within a converge block.
            Implemented in converge.cc.

                @param  W       Where defined.
                @param  cvgvar  Converge variable to use.  Must have
                                been created by a call to MakeCvgVar().
                @param  rhs     The right-hand side of the assignment.
                @return 0,      if some error occurred (will make noise).
                                A new statement, otherwise.
        */
        static expr* makeCvgAssign(const location& W, symbol* cvgvar,
                expr* rhs);


        /** Make an array guess statement for inside a converge block.
            These handle statements of the form
                real foo[i][j] guess rhs;
            Implemented in converge.cc.

                @param  W       Where defined.
                @param  cvgvar  Converge variable to use.  Must have
                                been created by a call to MakeCvgVar().
                @param  rhs     The right-hand side of the guess.
                @return 0,      if some error occurred (will make noise).
                                A new statement, otherwise.
        */
        static expr* makeArrayCvgGuess(const location& W, symbol* array,
                expr* guess);


        /** Make an array assignment statement, within a converge block.
            These handle statements of the form
                int a[i][j][k] := rhs;
            that appear within a converge block.
            Implemented in converge.cc.

                @param  W       Where defined.
                @param  array   Array to use for the assignment.  Must have
                                been created by a call to MakeArray().
                @param  rhs     The right-hand side of the assignment.
                @return ERROR,  if some error occurred (will make noise).
                                A new statement, otherwise.
        */
        static expr* makeArrayCvgAssign(const location& W, symbol* array,
                expr* rhs);


        /** Make a statement for constructing model variables.
            This handles declarations of the form
                place p, q, r, s;
            within a model.
            Implemented in mod_vars.cc.

                @param  W       Where defined.
                @param  p       Model definition containing the declaration.
                @param  t       Type of the variables.
                @param  bounds  Bounds for the variables (a set of something),
                                or null for unknown or no bounds.
                @param  names   Names of the variables
                                (they already exist as placeholder symbols).
                @param  N       Number of names.

                @return NULL,   if any name is NULL,
                        ERROR,  if any error occurs,
                        a new statement (void expression), otherwise.
        */
        static expr* makeModelVarDecs(const location& W, model_def* p,
            const type* t, expr* bounds, symbol** names, int N);

        /** Make a statement for constructing model array variables.
            This handles declarations of the form
                place p[i], q[i], r[i], s[i];
            within a model.
            Implemented in mod_vars.cc.

                @param  W       Where defined.
                @param  p       Model definition containing the declaration.
                @param  t       Type of the arrays.
                @param  arrays  Names of the arrays
                                (they already exist as placeholder symbols).
                @param  N       Number of arrays.

                @return NULL,   if any name is NULL,
                        ERROR,  if any error occurs,
                        a new statement (void expression), otherwise.
        */
        static expr* makeModelArrayDecs(const location& W, model_def* p,
            const type* t, symbol** arrays, int N);

        /** Make a statement to build a measure in a model.
            Implemented in mod_vars.cc.

                @param  W       Where defined.
                @param  p       Model definition containing the statement.
                @param  m       The measure.
                @param  rhs     Right-hand side of the measure assignment.

                @return NULL,   if p, m, or rhs is NULL.
                        ERROR,  if some error occurs (will make noise).
                        A new statement, otherwise.
        */
        static expr* makeModelMeasureAssign(const location& W,
            model_def* p, symbol* m, expr* rhs);

        /** Make a statement to build an array of measures in a model.
            Implemented in mod_vars.cc.

                @param  W       Where defined.
                @param  p       Model definition containing the statement.
                @param  am      The array of measures.
                @param  rhs     Right-hand side of the measure assignment.

                @return NULL,   if p, m, or rhs is NULL.
                        ERROR,  if some error occurs (will make noise).
                        A new statement, otherwise.
        */
        static expr* makeModelMeasureArray(const location& W,
            model_def* p, symbol* am, expr* rhs);



        //
        //
        //

        /** Make an expression to call a function.
            Passed parameters must match exactly in type.

                @param  W   Where defined.
                @param  f   The function to call.  Can be user-defined,
                            internal, or pretty much anything.
                @param  p   The parameters to pass, as an array of expressions.
                @param  np  Number of passed parameters.
        */
        static expr* makeFunctionCall(const location& W, symbol *f,
                expr **p, int np);

        /** Make an array dereferencing expression.
            These handle expressions of the form
                a[3][5+n][4-3*foobar(7, x)]

            @param  W     Where defined.
            @param  array   Array to use.
                            Must have been created by a call to MakeArray().
            @param  indexes List of indices to be "passed" to the array.
            @param  dim     Number of indices in the list \a indexes.
            @return ERROR,  if some error occurred (will make noise).
                            A new expression, otherwise.
        */
        static expr* makeArrayCall(const location& W, symbol* array,
            expr** indexes, int dim);


    protected:
        /// Expression debugging.
        static debugging_msg expr_debug;
        /// Symbol waiting list debugging.
        static debugging_msg waitlist_debug;
        /// Model debugging.
        static debugging_msg model_debug;
    protected:
        /// Type to use for statements.
        static const type* STMT;
    private:
        /// Where the expression was declared.
        location where;
        /// The expression type, if it is not aggregate.
        const type* simple;
        /// The expression type, if it is aggregate.
        typelist* aggtype;
        /// Model definition that built us, if any (more detailed type info).
        const model_def* model_type;
        /// Helpful for compiling, and for returning construction errors.
        char state;
        /// Unique identifier, for quick comparison of expression equality.
        int IDnum;
        /// Static member used to set the identifiers
        static int global_IDnum;

        friend class expr_initializer;
};


// ******************************************************************
// *                                                                *
// *                 Errors caused by an expression                 *
// *                                                                *
// ******************************************************************

class expr_error : public error_msg {
        result* ans;
    public:
        /*
         * Start an error message caused by an expression.
         *      @param  cause   Cause; will print its location
         *      @param  ans     If not a null pointer, sets to Null()
         *                      on destruction.
         */
        expr_error(const expr* cause, result* ans = nullptr);
        ~expr_error();
};

// ******************************************************************
// *                                                                *
// *                      Typechecking  errors                      *
// *                                                                *
// ******************************************************************

class typechecking_error : public error_msg {
    public:
        typechecking_error(const location& W);
        typechecking_error(const expr* x);
};

// ******************************************************************
// *                                                                *
// *                Global functions for expressions                *
// *                                                                *
// ******************************************************************

/// Safe way to compute expressions
inline void SafeCompute(expr* e, traverse_data &x)
{
  DCASSERT(x.answer);
  if (e)  e->Compute(x);
  else    x.answer->setNull();
}

/// Safe way to get expo rates
inline void SafeComputeExpoRate(expr* e, traverse_data &x)
{
  DCASSERT(x.answer);
  if (e)  e->Traverse(x);
  else    x.answer->setNull();
}

/// Safe way to count components
inline int NumComponents(const expr *e)
{
  if (e) return e->NumComponents();
  return 1;
}

/// Safe way to get an expression ID
inline int SafeID(const expr* e)
{
  return e ? e->getID() : 0;
}

#endif

