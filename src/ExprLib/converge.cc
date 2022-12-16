
#include "converge.h"
#include "../Options/optman.h"
#include "../Options/options.h"
#include "../Utils/init_opts.h"

#include "exprman.h"
#include "symbols.h"
#include "result.h"
#include "arrays.h"

#include <stdlib.h>

// #define DEBUG_CONVERGE

// ******************************************************************
// *                                                                *
// *                      converge_stmt  class                      *
// *                                                                *
// ******************************************************************

/** Class for a converge statement.
    I.e., for statements of the form:

    converge {
      block;
    }
*/
class converge_stmt : public expr {
  static long max_iters;
  expr* block;
  bool topmost;
public:
  converge_stmt(const location &W, expr* b, bool top);
  virtual ~converge_stmt();

  virtual void Compute(traverse_data &x);
  virtual void Traverse(traverse_data &x);
  virtual bool Print(std::ostream &s, int) const;

  inline long GetMaxIters() const { return max_iters; }

  friend class converge_initializer;
};

long converge_stmt::max_iters = 1000;

// ******************************************************************
// *                     converge_stmt  methods                     *
// ******************************************************************

converge_stmt::converge_stmt(const location &W, expr* b, bool top)
 : expr(W, STMT)
{
  block = b;
  topmost = top;
  if (block) {
    traverse_data x(traverse_data::Block);
    block->Traverse(x);
  }
}

converge_stmt::~converge_stmt()
{
  Delete(block);
}

void converge_stmt::Compute(traverse_data &x)
{
  DCASSERT(x.answer);
  if (x.stopExecution())  return;

  x.which = traverse_data::Guess;
  if (block) block->Traverse(x);

  long iters;
  for (iters=GetMaxIters(); iters; iters--) {
    x.clrRepeat();
    x.which = traverse_data::Compute;
    if (block)  block->Compute(x);
    x.which = traverse_data::Update;
    if (block)  block->Traverse(x);
    if (!x.wantsToRepeat()) break;
  }

  if (0==iters) {
    unnamed_warning E(Where());
    E << "converge statement terminating after ";
    E << GetMaxIters() << " iterations";
  }
  if (topmost)  {
    x.which = traverse_data::Affix;
    Traverse(x);
  }
  x.which = traverse_data::Compute;
}

void converge_stmt::Traverse(traverse_data &x)
{
  switch (x.which) {
    case traverse_data::Block:
        // we already did this for our block.
        return;

    default:
        if (block)  block->Traverse(x);
  };
}


bool converge_stmt::Print(std::ostream &s, int w) const
{
    s << padding(w) << "converge {\n";
    block->Print(s, w+2);
    s << padding(w) << "}\n";
    return true;
}

// ******************************************************************
// *                                                                *
// *                       converge_var class                       *
// *                                                                *
// ******************************************************************

/** Real "variables" within a converge statement.
    Members are public because they are manipulated directly
    by their guess and assignment statements.
    Currently, the type is forced to be REAL,
    but values are stored in a result struct in
    case we get null or infinity.
*/
class converge_var : public symbol {
public:
  result current;
  result update;
  bool was_updated;
  bool was_computed;
public:
  converge_var(const location &W, char* n);
  virtual void Compute(traverse_data &x);
};

// ******************************************************************
// *                      converge_var methods                      *
// ******************************************************************

converge_var::converge_var(const location &W, char* n)
 : symbol(W, em->REAL, n)
{
  current.setNull();
  update.setNull();
  was_computed = false;
}

void converge_var::Compute(traverse_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  *x.answer = current;
}

// ******************************************************************
// *                                                                *
// *                      fixpoint_stmt  class                      *
// *                                                                *
// ******************************************************************

/** Abstract base class for statements appearing within a converge block.
*/
class fixpoint_stmt : public expr {
    protected:
        static debugging_msg converge_debug;
    private:
        static double precision;
        static unsigned relative;
        static bool use_current;
        static inline void init(shared_object* o, const char* doc) {
            message_initializer::exec(converge_debug, doc, o);
        }
    public:
        fixpoint_stmt(const location &W);

        inline double GetPrecision() const { return precision; }
        inline bool RelativePrecision() const { return relative; }
        inline bool UseCurrent() const { return use_current; }

        friend class converge_initializer;
};

debugging_msg fixpoint_stmt::converge_debug("converges");
double fixpoint_stmt::precision;
unsigned fixpoint_stmt::relative;
bool fixpoint_stmt::use_current;

// ******************************************************************
// *                     fixpoint_stmt  methods                     *
// ******************************************************************

fixpoint_stmt::fixpoint_stmt(const location &W) : expr(W, STMT)
{
}

// ******************************************************************
// *                                                                *
// *                        guess_stmt class                        *
// *                                                                *
// ******************************************************************

/** Guess statements for non-arrays.
    I.e., statements of the form

      real var guess rhs;
*/
class guess_stmt : public fixpoint_stmt {
  converge_var* var;
  expr* guess;
public:
  guess_stmt(const location &W, converge_var* var, expr* rhs);
  virtual ~guess_stmt();

  virtual void Compute(traverse_data &x);
  virtual void Traverse(traverse_data &x);
  virtual bool Print(std::ostream &s, int) const;

  void Guess(traverse_data &x);
};

// ******************************************************************
// *                       guess_stmt methods                       *
// ******************************************************************

guess_stmt::guess_stmt(const location &W, converge_var* v, expr* g)
 : fixpoint_stmt(W)
{
  var = v;
  DCASSERT(var);
  guess = g;
  DCASSERT(! em->isError(guess) );
}

guess_stmt::~guess_stmt()
{
  Delete(guess);
}

void guess_stmt::Compute(traverse_data &x)
{
}

void guess_stmt::Traverse(traverse_data &x)
{
  switch (x.which) {
    case traverse_data::Guess:
        Guess(x);
        return;

    case traverse_data::Block:
        if (var->isDefined())  return;
        var->setDefined();
        return;

    case traverse_data::Affix:
        if (var->isComputed())  return;
        var->setComputed();
        var->notifyList();
        return;

    case traverse_data::Update:
        return;

    default:
        fixpoint_stmt::Traverse(x);
  }
}

bool guess_stmt::Print(std::ostream &s, int w) const
{
    s << padding(w) << "real " << var->Name() << " guess ";
    if (guess)  guess->Print(s);
    else        s << "null";
    s << ";\n";
    return true;
}

void guess_stmt::Guess(traverse_data &x)
{
  DCASSERT(x.answer);
  result* answer = x.answer;
  x.answer = &(var->current);
  SafeCompute(guess, x);
  x.answer = answer;

  if (converge_debug.start()) {
    converge_debug << "Guessing " << var->Name() << " := ";
    var->Type()->print(converge_debug.stream(), var->current);
    converge_debug.stop();
  }
}


// ******************************************************************
// *                                                                *
// *                       assign_stmt  class                       *
// *                                                                *
// ******************************************************************

class assign_stmt : public fixpoint_stmt {
  converge_var* var;
  expr* rhs;
public:
  assign_stmt(const location &W, converge_var* var, expr* r);
  virtual ~assign_stmt();

  virtual void Compute(traverse_data &x);
  virtual void Traverse(traverse_data &x);
  virtual bool Print(std::ostream &s, int) const;

  void Update();
};

// ******************************************************************
// *                      assign_stmt  methods                      *
// ******************************************************************

assign_stmt::assign_stmt(const location &W, converge_var* v, expr* r)
 : fixpoint_stmt(W)
{
  var = v;
  DCASSERT(var);
  rhs = r;
  DCASSERT(! em->isError(rhs) );
}

assign_stmt::~assign_stmt()
{
  Delete(rhs);
}

void assign_stmt::Compute(traverse_data &x)
{
  DCASSERT(x.answer);
  if (x.stopExecution())  return;
  result* answer = x.answer;
  x.answer = &(var->update);
  if (var->was_computed && var->current.isNull()) {
    x.answer->setNull();
  } else {
    SafeCompute(rhs, x);
  }
  x.answer = answer;
  var->was_updated = false;

  // Do we need to execute again?
  if (var->current.isNormal() && var->update.isNormal()) {
    double delta = var->update.getReal() - var->current.getReal();
    if (RelativePrecision()) if (var->current.getReal())
      delta /= var->current.getReal();
    delta = ABS(delta);
    if (delta > GetPrecision())
      x.setRepeat();
  } else if (! var->update.isNull()) {
    if (var->Type()->compare(var->current, var->update))
      x.setRepeat();
  }

  // Update, if now is the time.
  if (UseCurrent())  Update();
}

void assign_stmt::Traverse(traverse_data &x)
{
  switch (x.which) {
    case traverse_data::Update:
        Update();
        return;

    case traverse_data::Block:
    case traverse_data::Guess:
        return;

    case traverse_data::Affix:
        if (var->isComputed())  return;
        var->setComputed();
        var->notifyList();
        return;

    default:
        fixpoint_stmt::Traverse(x);
  }
}

bool assign_stmt::Print(std::ostream &s, int w) const
{
    s << padding(w) << "real " << var->Name() << " := ";
    if (rhs)  rhs->Print(s);
    else      s << "null";
    s << ";\n";
    return true;
}

void assign_stmt::Update()
{
  DCASSERT(var);
  if (var->was_updated)  return;
  var->current = var->update;
  // var->update.Clear();
  var->was_updated = true;
  var->was_computed = true;

  if (converge_debug.start()) {
    converge_debug << "Updating " << var->Name() << " := ";
    var->Type()->print(converge_debug.stream(), var->current);
    converge_debug.stop();
  }
}


// ******************************************************************
// *                                                                *
// *                     array_guess_stmt class                     *
// *                                                                *
// ******************************************************************

class array_guess_stmt : public fixpoint_stmt {
  array* var;
  expr* guess;
public:
  array_guess_stmt(const location &W, array* var, expr* rhs);
  virtual ~array_guess_stmt();

  virtual void Compute(traverse_data &x);
  virtual void Traverse(traverse_data &x);
  virtual bool Print(std::ostream &s, int) const;

  void Guess(traverse_data &x);
};

// ******************************************************************
// *                    array_guess_stmt methods                    *
// ******************************************************************

array_guess_stmt::array_guess_stmt(const location &W, array* v, expr* g)
 : fixpoint_stmt(W)
{
  var = v;
  DCASSERT(var);
  guess = g;
  DCASSERT(! em->isError(guess) );
}

array_guess_stmt::~array_guess_stmt()
{
  Delete(guess);
}

void array_guess_stmt::Compute(traverse_data &x)
{
}

void array_guess_stmt::Traverse(traverse_data &x)
{
  switch (x.which) {
    case traverse_data::Guess:
        Guess(x);
        return;

    case traverse_data::Block:
        if (var->isDefined())  return;
        var->setDefined();
        return;

    case traverse_data::Affix:
        if (var->isComputed())  return;
        var->setComputed();
        var->notifyList();
        return;

    case traverse_data::Update:
        return;

    default:
        fixpoint_stmt::Traverse(x);
  }
}

bool array_guess_stmt::Print(std::ostream &s, int w) const
{
    s << padding(w);
    var->PrintHeader(s);
    s << " guess ";
    if (guess)  guess->Print(s);
    else        s << "null";
    s << ";\n";
    return true;
}

void array_guess_stmt::Guess(traverse_data &x)
{
  converge_var* ccv;
  array_item* curr = var->GetCurrentReturn();
  if (curr) {
    ccv = smart_cast <converge_var*> (curr->e);
    DCASSERT(ccv);
  } else {
    ccv = new converge_var(Where(), 0);
    var->SetCurrentReturn(ccv, true);
  }

  result* answer = x.answer;
  x.answer = &(ccv->current);
  SafeCompute(guess, x);
  x.answer = answer;

  if (converge_debug.start()) {
    converge_debug << "Guessing " << ccv->Name() << " := ";
    var->Type()->print(converge_debug.stream(), ccv->current);
    converge_debug.stop();
  }
}


// ******************************************************************
// *                                                                *
// *                    array_assign_stmt  class                    *
// *                                                                *
// ******************************************************************

class array_assign_stmt : public fixpoint_stmt {
  array* var;
  expr* rhs;
public:
  array_assign_stmt(const location &W, array* var, expr* r);
  virtual ~array_assign_stmt();

  virtual void Compute(traverse_data &x);
  virtual void Traverse(traverse_data &x);
  virtual bool Print(std::ostream &s, int) const;

  void Update(converge_var* var);
};

// ******************************************************************
// *                   array_assign_stmt  methods                   *
// ******************************************************************

array_assign_stmt::array_assign_stmt(const location &W, array* v, expr* r)
 : fixpoint_stmt(W)
{
  var = v;
  DCASSERT(var);
  rhs = r;
  DCASSERT(! em->isError(rhs) );
}

array_assign_stmt::~array_assign_stmt()
{
  Delete(rhs);
}

void array_assign_stmt::Compute(traverse_data &x)
{
  DCASSERT(x.answer);
  if (x.stopExecution())  return;

  converge_var* ccv;
  array_item* curr = var->GetCurrentReturn();
  if (curr) {
    ccv = smart_cast <converge_var*> (curr->e);
    DCASSERT(ccv);
  } else {
    ccv = new converge_var(Where(), 0);
    var->SetCurrentReturn(ccv, true);
  }

  result* answer = x.answer;
  x.answer = &(ccv->update);
  if (ccv->was_computed && ccv->current.isNull()) {
    x.answer->setNull();
  } else {
    SafeCompute(rhs, x);
  }
  x.answer = answer;
  ccv->was_updated = false;

#ifdef DEBUG_CONVERGE
  io->Output << "Current: ";
  ccv->current.Print(io->Output, REAL);
  io->Output << "  Update: ";
  ccv->update.Print(io->Output, REAL);
  io->Output << "\n";
#endif

  // Do we need to execute again?
  if (ccv->current.isNormal() && ccv->update.isNormal()) {
    double delta = ccv->update.getReal() - ccv->current.getReal();
    if (RelativePrecision()) if (ccv->current.getReal())
      delta /= ccv->current.getReal();
    delta = ABS(delta);
    if (delta > GetPrecision())
      x.setRepeat();
  } else if (! ccv->update.isNull()) {
    if (var->Type()->compare(ccv->current, ccv->update))
      x.setRepeat();
  }

  // Update, if now is the time.
  if (UseCurrent())  Update(ccv);
}

void array_assign_stmt::Traverse(traverse_data &x)
{
  switch (x.which) {
    case traverse_data::Update: {
        array_item* curr = var->GetCurrentReturn();
        DCASSERT(curr);
        converge_var* ccv = smart_cast <converge_var*> (curr->e);
        DCASSERT(ccv);
        Update(ccv);
        return;
    }

    case traverse_data::Block:
    case traverse_data::Guess:
        return;

    case traverse_data::Affix:
        if (var->isComputed())  return;
        var->setComputed();
        var->notifyList();
        return;

    default:
        fixpoint_stmt::Traverse(x);
  }
}

bool array_assign_stmt::Print(std::ostream &s, int w) const
{
    s << padding(w);
    var->PrintHeader(s);
    s << " := ";
    if (rhs)  rhs->Print(s);
    else      s << "null";
    s << ";\n";
    return true;
}

void array_assign_stmt::Update(converge_var* var)
{
  DCASSERT(var);
  if (var->was_updated)  return;
  var->current = var->update;
  // var->update.Clear();
  var->was_updated = true;
  var->was_computed = true;

  if (converge_debug.start()) {
    converge_debug << "Updating " << var->Name() << " := ";
    var->Type()->print(converge_debug.stream(), var->current);
    converge_debug.stop();
  }
}


// ******************************************************************
// *                                                                *
// *                        exprman  methods                        *
// *                                                                *
// ******************************************************************

symbol* exprman::makeCvgVar(const location &W, const type* t, char* name) const
{
    if (0==t || !t->matches("real")) {
        typechecking_error E(W);
        E << "Converge variable " << name << " must have type real";
        free(name);
        return nullptr;
    }
    return new converge_var(W, name);
}


expr* exprman::makeConverge(const location &W, expr* stmt, bool top) const
{
  if (isOrdinary(stmt))   return new converge_stmt(W, stmt, top);

  return Share(stmt);
}


expr* MakeCvgThing(const exprman* em, const location &W,
      symbol* cvgvar, expr* rhs, bool guess)
{
    if (nullptr==em || em->isError(rhs))  return nullptr;
    converge_var* var = dynamic_cast <converge_var*> (cvgvar);
    if (nullptr==var) {
        Delete(rhs);
        return nullptr;
    }
    const type* gt = em->SafeType(rhs);
    if (!em->isPromotable(gt, em->REAL)) {
        typechecking_error E(W);
        E << "Return type for identifier " << var->Name() << " should be ";
        var->PrintType(E.stream());
        Delete(rhs);
        return nullptr;
    }
    rhs = em->promote(rhs, em->REAL);
    DCASSERT(! em->isError(rhs) );
    if (guess) {
        var->setGuessed();
        return new guess_stmt(W, var, rhs);
    } else {
        var->setDefined();
        return new assign_stmt(W, var, rhs);
    }
}


expr* exprman::makeCvgGuess(const location &W, symbol* cvgvar, expr* rhs) const
{
  return MakeCvgThing(this, W, cvgvar, rhs, true);
}


expr* exprman::makeCvgAssign(const location &W, symbol* cvgvar, expr* rhs) const
{
  return MakeCvgThing(this, W, cvgvar, rhs, false);
}


expr* MakeArrayThing(const exprman* em, const location &W,
      symbol* a, expr* rhs, bool guess)
{
    if (nullptr==em || em->isError(rhs))  return nullptr;
    array* var = dynamic_cast <array*> (a);
    if (nullptr==var) {
        Delete(rhs);
        return nullptr;
    }
    const type* gt = em->SafeType(rhs);
    if (!em->isPromotable(gt, em->REAL)) {
        typechecking_error E(W);
        E << "Return type for array " << var->Name() << " should be ";
        var->PrintType(E.stream());
        Delete(rhs);
        return nullptr;
    }
    rhs = em->promote(rhs, em->REAL);
    DCASSERT(! em->isError(rhs) );
    if (guess) {
        var->setGuessed();
        return new array_guess_stmt(W, var, rhs);
    } else {
        var->setDefined();
        return new array_assign_stmt(W, var, rhs);
    }
}


expr* exprman::makeArrayCvgGuess(const location &W, symbol* arr, expr* gss) const
{
  return MakeArrayThing(this, W, arr, gss, true);
}


expr* exprman::makeArrayCvgAssign(const location &W, symbol* arr, expr* rhs) const
{
  return MakeArrayThing(this, W, arr, rhs, false);
}


// ******************************************************************
// *                                                                *
// *                          Initializers                          *
// *                                                                *
// ******************************************************************

class converge_initializer : public initializer {
    public:
        converge_initializer();
    protected:
        virtual void execute();
};

converge_initializer::converge_initializer()
    : initializer("converge.cc", 1, 2)
{
    builds_resource(0, "converge.cc");
    needs_resource(1, "OM");
    needs_resource(2, fixpoint_stmt::converge_debug.optName());
}

void converge_initializer::execute()
{
    option_manager* OM = dynamic_cast <option_manager*> (get_object(1));
    if (!OM) return;

    fixpoint_stmt::init(
        get_object(2),
        "Use to view the sequence of assignments during the execution of a converge statement."
    );

    converge_stmt::max_iters = 1000;
    OM->addIntOption("MaxConvergeIters",
        "Maximum number of iterations of a converge statement.",
        converge_stmt::max_iters, 1, 2000000000
    );

    fixpoint_stmt::precision = 1e-5;
    OM->addRealOption("ConvergePrecision",
        "Desired precision for values within a converge statement.",
        fixpoint_stmt::precision, true, false, 0, true, false, 1
    );

    option* prec_test = OM->addRadioOption("ConvergePrecisionTest",
        "Comparison to use for convergence test of values within a converge statement.",
        2, fixpoint_stmt::relative
    );
    DCASSERT(prec_test);
    prec_test->addRadioButton("ABSOLUTE", "Use absolute precision", 0);
    prec_test->addRadioButton("RELATIVE", "Use relative precision", 1);
    fixpoint_stmt::relative = 1;

    fixpoint_stmt::use_current = true;
    OM->addBoolOption("UseCurrent",
        "Should variables within a converge statement be updated immediately.",
        fixpoint_stmt::use_current
    );
}


static converge_initializer _the_converge_init;

