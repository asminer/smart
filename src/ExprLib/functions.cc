
// $Id$

#include "../Streams/streams.h"
#include "exprman.h"
#include "../Options/options.h"
#include "functions.h"
#include "mod_def.h"
#include "mod_inst.h"
#include <string.h>
#include <stdlib.h>
#include <limits.h>   // for INT_MIN

// #define DEBUG_FUNC_WRAPPERS

// ******************************************************************

inline const type* randify(const type* ft, bool rand)
{
  if (0==rand) return ft;
  if (0==ft)   return 0;
  if (ft->getModifier() != DETERM) return 0;
  return ft->modifyType(RAND);
}

inline const type* procify(const type* ft, bool proc)
{
  if (0==proc)  return ft;
  if (0==ft)    return 0;
  if (ft->hasProc())  return 0;
  return ft->addProc();
}

// ******************************************************************
// *                                                                *
// *                          fcall  class                          *
// *                                                                *
// ******************************************************************

/**  A function call expression.
 */
class fcall : public expr {
protected:
  function* func;
  expr** pass;
  int numpass;
private:
  inline void construct(function* f, expr** p, int np) {
    func = f;
    pass = p;
    numpass = np;
  }
public:
  fcall(const expr* fnlnt, function* f, expr** p, int np);
  fcall(const char* fn, int line, const type* t, function* f, expr** p, int np);
  virtual ~fcall();
  virtual void Compute(traverse_data &x);
  virtual void Traverse(traverse_data &x);
  virtual bool Print(OutputStream &s, int) const;
};

// ******************************************************************
// *                         fcall  methods                         *
// ******************************************************************

fcall::fcall(const expr* fnlnt, function* f, expr** p, int np)
: expr(fnlnt)
{
  construct(f, p, np);
  DCASSERT(GetModelType() == func->DetermineModelType(p, np));
}

fcall::fcall(const char* fn, int line, const type* t, 
  function* f, expr** p, int np) : expr(fn, line, t)
{
  construct(f, p, np);
  SetModelType(func->DetermineModelType(p, np));
}

fcall::~fcall()
{
  // don't delete func
  int i;
  for (i=0; i<numpass; i++) Delete(pass[i]);
  delete[] pass;
}

void fcall::Compute(traverse_data &x)
{
  DCASSERT(func);
  const expr* oldp = x.parent;
  x.parent = this;
  func->Compute(x, pass, numpass);
  x.parent = oldp;
}

void fcall::Traverse(traverse_data &x)
{
  DCASSERT(func);
  const expr* oldp = x.parent;
  x.parent = this;
  switch (x.which) {
    case traverse_data::Substitute: {
        DCASSERT(x.answer);
        expr** newpass = new expr* [numpass];
        bool notequal = false;
        for (int i=0; i<numpass; i++) {
          if (pass[i]) {
            pass[i]->Traverse(x);
            newpass[i] = smart_cast <expr*> (Share(x.answer->getPtr()));
          } else {
            newpass[i] = 0;
          }
          if (newpass[i] != pass[i])  notequal = true;
         } // for i
        if (func->Traverse(x, newpass, numpass))  break;
        if (notequal) {
          x.answer->setPtr(new fcall(this, func, newpass, numpass));
        } else {
          for (int i=0; i<numpass; i++)  Delete(newpass[i]);
          delete[] newpass;
          x.answer->setPtr(Share(this));
        }
        break;
    } // traverse_data::Substitute
  
    case traverse_data::GetVarDeps:
    case traverse_data::GetSymbols:
    case traverse_data::PreCompute: {
        for (int i=0; i<numpass; i++) if (pass[i]) {
          pass[i]->Traverse(x);
        } // for i
        func->Traverse(x, pass, numpass);
        break;
    } // traverse_data::PreCompute & GetSymbols

    default:
        func->Traverse(x, pass, numpass);
  }
  x.parent = oldp;
}

bool fcall::Print(OutputStream &s, int w) const
{
  if (0==func->Name())  return false;  // hidden?
  if (w>0)  s.Pad(' ', w);
  s << func->Name();
  if (numpass) {
    s << "(";
    bool prev_written = false;
    for (int i=0; i<numpass; i++) {
      if (prev_written)  s << ", ";
      if (func->IsHidden(i))  continue;
      if (pass[i]) {
        prev_written = pass[i]->Print(s, 0);
      } else {
        s.Put("null");  
        prev_written = true;
      }
    }
    s << ")";
  }
  if (w>0)  s.Put(";\n");
  return true;
}

// ******************************************************************
// *                                                                *
// *                        function methods                        *
// *                                                                *
// ******************************************************************

function::function(const function* f)
 : symbol(f)
{
}

function::function(const char* fn, int line, const type* t, char* n)
 : symbol(fn, line, t, n)
{
}

function::~function()
{
}

int function::TypecheckParams(expr** pass, int np)
{
  traverse_data foo(traverse_data::Typecheck);
  return Traverse(foo, pass, np);
}

int function::PromoteParams(expr** pass, int np)
{
  traverse_data foo(traverse_data::Promote);
  return Traverse(foo, pass, np);
}

const type* function::DetermineType(expr** pass, int np)
{
  traverse_data foo(traverse_data::GetType);
  Traverse(foo, pass, np);
  return foo.the_type;
}

const model_def* function::DetermineModelType(expr** pass, int np)
{
  traverse_data foo(traverse_data::GetType);
  Traverse(foo, pass, np);
  return foo.the_model_type;
}

void function::Compute(traverse_data &x)
{
  if (em->startInternal(__FILE__, __LINE__)) {
    em->noCause();
    em->internal() << "Trying to compute a function expression: ";
    Print(em->internal(), 0);
    em->stopIO();
  }
}

void function::Traverse(traverse_data &x)
{
  // reasonable default here
  Traverse(x, 0, 0);
}

int function::Traverse(traverse_data &x, expr** pass, int np)
{
  // reasonable defaults here.
  switch (x.which) {
    case traverse_data::GetType:
        x.the_type = Type();
        return 0;

    case traverse_data::Typecheck:
        return TooManyParams(np);

    default:
        return 0;
  }
}

bool function::DocumentHeader(doc_formatter* df) const
{
  // provided in derived classes
  DCASSERT(0);
  return false;
}

void function::DocumentBehavior(doc_formatter* df) const
{
  DCASSERT(0);
}

void function::PrintDocs(doc_formatter* df, const char*) const
{
  if (!DocumentHeader(df))  return;
  df->begin_indent();
  DocumentBehavior(df);
  df->end_indent();
}

bool function::HeadersMatch(const type* t, symbol** pl, int np) const
{
  return false;
}

bool function::HasNameConflict(symbol** fp, int np, int* tmp) const
{
  return false;
  // Derived classes should override this
}

symbol* function::FindFormal(const char* name) const
{
  return 0;
}

int function::maxNamedParams() const
{
  return -1;  // don't even try with named params
}

int function::named2Positional(symbol**, int, expr**, int) const
{
  return INT_MIN;
}

// ******************************************************************
// *                                                                *
// *                       named_param  class                       *
// *                                                                *
// ******************************************************************

/** Named parameter class.
    Basically, a pair (name, expression).
*/
class named_param : public symbol {
  expr* pass;

public:
  named_param(const char* fn, int line, char* n, expr* p);
  virtual ~named_param();

  virtual bool Print(OutputStream &s, int width) const;

  inline expr* copyPass() const {
    return Share(pass);
  }
};


named_param::named_param(const char* fn, int line, char* n, expr* p)
 : symbol(fn, line, (const type*) 0, n)
{
  SetType(p);
  pass = p;
}

named_param::~named_param()
{
  Delete(pass);
}

bool named_param::Print(OutputStream &s, int width) const
{
  if (symbol::Print(s, 0)) {
    s.Put(":=");
    if (0==pass)  s.Put("null");
    else          pass->Print(s, 0);
    return true;
  }
  return false;
}

// ******************************************************************
// *                                                                *
// *                       formal_param class                       *
// *                                                                *
// ******************************************************************

/** Formal parameters, abstract base class.
    Most functionality here is for user-defined functions,
    but we also use them for models and internal functions.
 
    Note: if the name is NULL, we assume that the parameter is "hidden".

*/  

class formal_param : public symbol {
  /** Do we have a default?  
      This is necessary because the default might be NULL!
   */
  bool hasdefault;
  /// Default values (for user-defined functions)
  expr* deflt;
  /// Are we hidden?  Used for model functions.
  bool hidden;
private:
  inline void Construct() {
    hasdefault = false;
    deflt = 0;
    hidden = false;
  };
public:
  /// Use this constructor for building duplicates
  formal_param(const formal_param* fp);
  /// Use this constructor for builtin functions
  formal_param(const type* t, const char* n);
  /// Use this constructor for builtin functions, aggregate params
  formal_param(typelist* t, const char* n);
  /// Use this constructor for user-defined functions.
  formal_param(const char* fn, int line, const type* t, char* n);
  virtual ~formal_param();

  inline void SetDefault(expr *d) { deflt = d; hasdefault = true; }
  inline bool HasDefault() const { return hasdefault; }
  inline expr* Default() const { return deflt; }

  inline bool IsHidden() const { return hidden; }
  inline void HideMe() { hidden = true; }

  /// Returns true iff we printed something.
  bool PrintHeader(OutputStream &s, bool hide);
};

formal_param::formal_param(const formal_param* fp) : symbol(fp)
{
  DCASSERT(fp);
  hasdefault = fp->hasdefault;
  deflt = Share(fp->deflt);
  hidden = fp->hidden;
}

formal_param::formal_param(const type* t, const char* n)
  : symbol(0, -1, t, n ? strdup(n) : 0)
{
  Construct();
}

formal_param::formal_param(typelist* t, const char* n)
  : symbol(0, -1, t, n ? strdup(n) : 0)
{
  Construct();
}

formal_param::formal_param(const char* fn, int line, const type* t, char* n)
  : symbol(fn, line, t, n)
{
  Construct();
}

formal_param::~formal_param()
{
  Delete(deflt);
}

bool formal_param::PrintHeader(OutputStream &s, bool hide)
{
  if (0==Name())  return false;
  if (hidden && hide)  return false;
  PrintType(s);
  s.Put(' ');
  s.Put(Name());
  if (hasdefault) {
    s.Put(":=");
    if (deflt)  deflt->Print(s, 0);
    else  s.Put("null");
  }
  return true;
}

// ******************************************************************
// *                                                                *
// *                        fp_onstack class                        *
// *                                                                *
// ******************************************************************

/** Formal parameters with a corresponding stack location.
    Used for internal functions and top-level user-defined functions.
*/  
class fp_onstack : public formal_param {
  /** Pointer to the current stack space for the function call.
      Used only for user-defined function calls.
   */
  result** stack;
  /// Position in the stack for the formal parameter.
  int offset;
private:
  inline void Construct() {
    stack = 0;
    offset = 0;
  };
public:
  /// Use this constructor for building duplicates
  fp_onstack(const formal_param* fp);
  /// Use this constructor for builtin functions
  fp_onstack(const type* t, const char* n);
  /// Use this constructor for builtin functions, aggregate params
  fp_onstack(typelist* t, const char* n);
  /// Use this constructor for user-defined functions.
  fp_onstack(const char* fn, int line, const type* t, char* n);
  virtual ~fp_onstack();

  inline void LinkStackLocation(result** s, int off) {
    DCASSERT(0==stack);
    stack = s;
    offset = off;
  }

  virtual void Compute(traverse_data &x);
};

fp_onstack::fp_onstack(const formal_param* fp) : formal_param(fp)
{
  Construct();
}

fp_onstack::fp_onstack(const type* t, const char* n) : formal_param(t, n)
{
  Construct();
}

fp_onstack::fp_onstack(typelist* t, const char* n) : formal_param(t, n)
{
  Construct();
}

fp_onstack::fp_onstack(const char* fn, int line, const type* t, char* n)
  : formal_param(fn, line, t, n)
{
  Construct();
}

fp_onstack::~fp_onstack()
{
  // don't touch stack, it's managed elsewhere!
}

void fp_onstack::Compute(traverse_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(stack);
  DCASSERT(*stack);
  *(x.answer) = (*stack)[offset];
}



// ******************************************************************
// *                                                                *
// *                        fp_wrapper class                        *
// *                                                                *
// ******************************************************************

/** Formal parameters as placeholders.
    Used for user-defined functions within models.
    (Each model instance will create its own user-defined function).
*/  
class fp_wrapper : public formal_param {
  formal_param* link;
public:
  fp_wrapper(const char* fn, int line, const type* t, char* n);

  inline formal_param* newCopy() {
#ifdef DEBUG_FUNC_WRAPPERS
    em->cout() << "creating link for fp wrapper " << Name() << "\n";
#endif
    link = new fp_onstack(this);
    return link;
  };

  virtual void Compute(traverse_data &x);
  virtual void Traverse(traverse_data &x);
};

fp_wrapper::fp_wrapper(const char* fn, int line, const type* t, char* n)
  : formal_param(fn, line, t, n)
{
  link = 0;
}

void fp_wrapper::Compute(traverse_data &x)
{
  SafeCompute(link, x);
}

void fp_wrapper::Traverse(traverse_data &x)
{
  if (traverse_data::ModelDone == x.which) {
    link = 0;
    return;
  }
  if (link) link->Traverse(x);
  else      formal_param::Traverse(x);
}

// ******************************************************************
// *                                                                *
// *                         fplist methods                         *
// *                                                                *
// ******************************************************************

fplist::fplist()
{
  formal = 0;
  num_formal = 0;
  repeat_point = -1; 
}

fplist::~fplist()
{
  clear();
}

void fplist::allocate(int nf)
{
  formal = new formal_param* [nf];
#ifdef DEVELOPMENT_CODE
  for (int i=0; i<nf; i++)  formal[i] = 0;
#endif
  num_formal = nf;
}

void fplist::setAll(int nf, formal_param** pl, bool subst)
{
  if (pl == formal) return;
  if (formal) clear();
  num_formal = nf;
  formal = pl;
  if (pl) for (int i=0; i<num_formal; i++) {
    DCASSERT(formal);
    DCASSERT(formal[i]);
    formal[i]->SetSubstitution(subst);
  }
}

void fplist::setStack(result** stack)
{
  for (int i=0; i<num_formal; i++) {
    DCASSERT(formal);
    DCASSERT(formal[i]);
    fp_onstack* fpi = smart_cast <fp_onstack*>(formal[i]);
    DCASSERT(fpi);
    fpi->LinkStackLocation(stack, i);
  }
}

void fplist::clear()
{
  if (formal) for (int i=0; i<num_formal; i++) Delete(formal[i]);
  delete[] formal;
  formal = 0;
}

void fplist::build(int n, const type* t, const char* name)
{
  CHECK_RANGE(0, n, num_formal);
  DCASSERT(t);
  DCASSERT(0==formal[n]);
  formal[n] = new formal_param(t, name);
}

void fplist::build(int n, const type* t, const char* name, expr* deflt)
{
  build(n, t, name);
  DCASSERT(formal[n]);
  formal[n]->SetDefault(deflt);
}


void fplist::build(int n, typelist* t, const char* name)
{
  CHECK_RANGE(0, n, num_formal);
  DCASSERT(t);
#ifdef DEVELOPMENT_CODE
  for (int a=0; a<t->Length(); a++) DCASSERT(t->GetItem(a));
#endif
  formal[n] = new formal_param(t, name);
}

void fplist::hide(int n)
{
  CHECK_RANGE(0, n, num_formal);
  DCASSERT(formal[n]);
  formal[n]->HideMe();
}

const type* fplist::getType(int n) const
{
  CHECK_RANGE(0, n, num_formal);
  DCASSERT(formal[n]);
  return formal[n]->Type();
}

bool fplist::isHidden(int fpnum) const
{
  if (repeat_point >= 0) {
    while (fpnum >= num_formal) {
      fpnum -= num_formal;
      fpnum += repeat_point;
    }
  }
  CHECK_RANGE(0, fpnum, num_formal);
  if (formal[fpnum])  return formal[fpnum]->IsHidden();
  return false;
}

void fplist::PrintHeader(OutputStream &s, bool hide) const
{
  s << "(";
  bool printed = false;
  for (int i=0; i<num_formal; i++) {
    if (printed)
        s << ", ";
    if (repeat_point == i)
        s << "..., ";
    if (formal[i]->PrintHeader(s, hide))
        printed = true;
  }
  if (repeat_point>=0)
  s << ", ...";
  s << ")";
}

int fplist::findIndex(const char* name) const
{
  if (0==name) return -1;
  for (int i=0; i<num_formal; i++) {
    DCASSERT(formal);
    DCASSERT(formal[i]);
    DCASSERT(formal[i]->Name());
    if (0==strcmp(name, formal[i]->Name())) return i;
  }
  return -1;
}

bool fplist::matches(symbol** pl, int np) const
{
  if (np != num_formal)  return false;
  for (int i=0; i<num_formal; i++) {
    DCASSERT(formal[i]);
    const formal_param* fp = dynamic_cast <const formal_param*> (pl[i]);
    if (0==fp)                                        return false;
    if (formal[i]->Type() != fp->Type())              return false;
    if (formal[i]->HasDefault() != fp->HasDefault())  return false;
    if (strcmp(formal[i]->Name(), fp->Name()))        return false;
    if (!formal[i]->HasDefault())  continue;

    // Aha!
    expr* midef = formal[i]->Default();
    expr* tudef = fp->Default();
    if ((0==midef) != (0==tudef))   return false;
    if (0==midef)      continue;
    if (! midef->Equals(tudef) )    return false;
  }
  return true;
}

bool fplist::hasNameConflict(symbol** pl, int np, int* tmp) const
{
  //
  // Initialize scratch space - holds index of matching param
  //
  for (int i=0; i<np; i++) tmp[i] = -1; // no match yet

  //
  // Go through all of our parameters...
  //
  int ourExtra = 0;

  for (int i=0; i<num_formal; i++) {
    //
    // First, check for a matching name
    //
    int j = findParamWithName(pl, np, tmp, formal[i]->Name());

    if (j>=0) tmp[j] = i;   // remember where we found this

    if (formal[i]->IsHidden())    continue; // parameter won't be listed

    //
    // Still here?  Our FP must appear in a name list.
    //


    if (j<0) {
      // 
      // Same FP name cannot appear in other name list.
      //

      // If this is a required parameter, then there is never a conflict.
      if (!formal[i]->HasDefault()) return false;

      // Keep going because there might be a conflict.
      ourExtra++;
      continue;
    }

    //
    // Same FP name CAN appear in other name list.
    // If it is missing in the other list, and the other list has
    // a default, then we know to select the other function.
    // So we can now consider only the cases where this FP name
    // appears in both name lists.
    //
    if (formal[i]->Type() != pl[j]->Type()) {
      // 
      // Types can distinguish.  No conflict.
      return false;
    }

    //
    // Still here?  This FP cannot distinguish the function calls.  
    // Keep looking.
  }

  //
  // Check if something in the other list can
  // force a difference in a name list.
  //
  int theirExtra = 0;
  for (int i=0; i<np; i++) {

    if (tmp[i]>=0) continue;    // this matched one of ours

    // We have an unmatched parameter.

    formal_param* fpli = smart_cast <formal_param*> (pl[i]);
    if (0==fpli) continue;  // strange things afoot.
    if (fpli->IsHidden()) continue;     // won't be listed

    // If it is required to appear in a name list,
    // then there cannot be a conflict.
    if (!fpli->HasDefault()) return false;

    // Keep going
    theirExtra++;
  }

  //
  // Still here?  All parameters with the same names match.
  // Any other parameters are optional (with defaults).
  //
  // If some function has no extras, then that one
  // will always have ambiguous function calls.
  // Don't allow that.
  //

  return (0==ourExtra || 0==theirExtra);
}

void addToScores(const exprman* em, int errcode, const type* pass,
  const type* formal, int* scores)
{
  DCASSERT(formal);
  // scores[0]: no promotion of formal parameter
  if (scores[0] >= 0) {
    int d = em->getPromoteDistance(pass, formal);
    if (d < 0)  scores[0] = errcode;
    else  scores[0] += d;
  }
  // scores[1]: can we promote formal to "rand"
  if (scores[1] >= 0) {
    const type* rf = randify(formal, true);
    int d = em->getPromoteDistance(pass, rf);
    if (d < 0)  scores[1] = errcode;
    else  scores[1] += d;
  }
  // scores[2]: can we promote formal to "proc"
  if (scores[2] >= 0) {
    const type* pf = procify(formal, true);
    int d = em->getPromoteDistance(pass, pf);
    if (d < 0)  scores[2] = errcode;
    else  scores[2] += d;
  }
  // scores[3]: what about "proc rand"?
  if (scores[3] >= 0) {
    const type* prf = procify(randify(formal, true), true);
    int d = em->getPromoteDistance(pass, prf);
    if (d < 0)  scores[3] = errcode;
    else  scores[3] += d;
  }
}

inline void SetAllScores(int* scores, int val)
{
  scores[0] = scores[1] = scores[2] = scores[3] = val;
}

void fplist::check(const exprman* em, expr** pass, int np, int* scores) const
{
  DCASSERT(scores);
  scores[0] = 0;  // score with no formal param promotion
  scores[1] = 0;  // score if we promote all formals to "rand"
  scores[2] = 0;  // score if we promote all formals to "proc"
  scores[3] = 0;  // score if we promote all formals to "proc rand"
  int fp = 0;
  for (int i=0; i<np; i++, fp++) {
    int err = function::BadParam(i, np);
    if (fp >= num_formal) {
      if (repeat_point < 0) {
        SetAllScores(scores, function::TooManyParams(np));
        return;
      }
      fp = repeat_point;
    } // if fp

    if (em->isDefault(pass[i])) {
      // Parameter is "default".
      if (!formal[i]->HasDefault()) {
        SetAllScores(scores, err);
        return;
      }
      continue;  
    }

    if (0==pass[i]) { // null parameter, may be ok
      addToScores(em, err, em->NULTYPE, formal[fp]->Type(), scores);
      continue;
    } 

    if (pass[i]->NumComponents() != formal[fp]->NumComponents()) {
      SetAllScores(scores, err);
      return;
    }

    for (int a=pass[i]->NumComponents()-1; a>=0; a--) 
      addToScores(em, err, pass[i]->Type(a), formal[fp]->Type(a), scores);

  } // for i
  if (fp < num_formal)  SetAllScores(scores, function::NotEnoughParams(np));
}

int fplist::check(const exprman* em, expr** pass, int np, const type* ret) const
{
  int scores[4];
  check(em, pass, np, scores);

  if (scores[0] >= 0)  return scores[0];
  if (scores[1] >= 0) {
    const type* rmod = randify(ret, true);
    if (rmod)  return scores[1] + em->getPromoteDistance(ret, rmod);
    return scores[0];
  }
  if (scores[2] >= 0) {
    const type* rmod = procify(ret, true);
    if (rmod)  return scores[2] + em->getPromoteDistance(ret, rmod);
    return scores[0];
  }
  if (scores[3] >= 0) {
    const type* rmod = procify(randify(ret, true), true);
    if (rmod)  return scores[3] + em->getPromoteDistance(ret, rmod);
    return scores[0];
  }

  return scores[0];
}

const type* fplist
::getType(const exprman* em, expr** pass, int np, const type* rt) const
{
  int scores[4];
  check(em, pass, np, scores);

  if (scores[0] >= 0)  return rt;
  if (scores[1] >= 0)  return randify(rt, true);
  if (scores[2] >= 0)  return procify(rt, true);
  if (scores[3] >= 0)  return procify(randify(rt, true), true);

  DCASSERT(0);
  return 0;
}

bool fplist
::promote(const exprman* em, expr** pass, int np, const type* rt) const
{
  bool rand = false;
  bool proc = false;
  int scores[4];
  check(em, pass, np, scores);
  if (scores[0] >= 0) {
    // nothing!
  } else if (scores[1] >= 0) {
    rand = true;
  } else if (scores[2] >= 0) {
    proc = true;
  } else if (scores[3] >= 0) {
    rand = proc = true;
  } else {
    DCASSERT(0);
  }
  DCASSERT(rt);
  DCASSERT(procify(randify(rt, rand), proc));

  int fp = 0;
  DCASSERT(np >= num_formal);
  for (int i=0; i<np; i++, fp++) {
    if (fp >= num_formal) {
      DCASSERT(repeat_point >= 0);
      fp = repeat_point;
    } // if fp
    if (em->isDefault(pass[i])) {
      pass[i] = Share(formal[i]->Default());
    } else if (pass[i]) {
      pass[i] = em->promote(pass[i], proc, rand, formal[fp]);
    }
    DCASSERT(! em->isError(pass[i]) );
  } // for i
  return true;
}

void fplist::traverse(traverse_data &x)
{
  for (int i=0; i<num_formal; i++) {
    DCASSERT(formal);
    DCASSERT(formal[i]);
    formal[i]->Traverse(x);
  }
}

int fplist
::named2Positional(exprman* em, symbol** np, int nnp, 
    expr** buffer, int bufsize) const
{
  // if buffer is not large enough, don't bother!
  if (bufsize < num_formal) {
    buffer = 0;
    bufsize = 0;
  }

  //
  // First, initialize everything to "default"
  if (buffer) {
    for (int i=0; i<num_formal; i++) {
      buffer[i] = em->makeDefault();
    }
  }

  //
  // Now, copy over named stuff
  for (int i=0; i<nnp; i++) {
    named_param* npi = dynamic_cast <named_param*> (np[i]);
    if (0==npi) return -1-i;

    int j = findIndex(npi->Name()); 
    if (j < 0) return -1-i; 
    
    if (buffer) {
      // Replace j with our passed expression
      Delete(buffer[j]);  // should be default
      buffer[j] = npi->copyPass();
    }
  }

  return num_formal;
}

//
// private
//

int fplist
::findParamWithName(symbol** pl, int np, int* tmp, const char* name) const
{
  for (int i=0; i<np; i++) {
    if (tmp[i]>=0) continue;
    if (strcmp(pl[i]->Name(), name)) continue;
    return i;
  }
  return -1;
}

// ******************************************************************
// *                                                                *
// *                     internal_func  methods                     *
// *                                                                *
// ******************************************************************

internal_func::internal_func(const type* t, const char* name)
 : function(" internally", -1, t, strdup(name))
{
  docs = 0;
  hidden = false;
}

bool internal_func::DocumentHeader(doc_formatter* df) const
{
  if (0==df)      return false;
  if (0==Name())  return false;
#ifndef DEVELOPMENT_CODE
  if (hidden)     return false;
#endif
  df->begin_heading();
  PrintHeader(df->Out(), true);
  if (hidden)  df->Out() << " (undocumented)";
  df->end_heading();
  return true;
}

void internal_func::DocumentBehavior(doc_formatter* df) const
{
  if (docs) df->Out() << docs;
  else      df->Out() << "no documentation";
}

// ******************************************************************
// *                                                                *
// *                    simple_internal  methods                    *
// *                                                                *
// ******************************************************************

simple_internal::simple_internal(const type* t, const char* name, int nf)
 : internal_func(t, name)
{
  formals.allocate(nf);
}

simple_internal::~simple_internal()
{
}

bool simple_internal::IsHidden(int fpnum) const
{
  return formals.isHidden(fpnum);
}

bool simple_internal::HasNameConflict(symbol** fp, int np, int* tmp) const
{
  return formals.hasNameConflict(fp, np, tmp);
}

int simple_internal::Traverse(traverse_data &x, expr** pass, int np)
{
  switch (x.which) {
    case traverse_data::Typecheck: 
        return formals.check(em, pass, np, Type());

    case traverse_data::GetType:
        x.the_type = formals.getType(em, pass, np, Type());
        return 0;
    
    case traverse_data::Promote: 
        formals.promote(em, pass, np, Type());
        return Promote_Success;

    default:
        return function::Traverse(x, pass, np);
  };
}

void simple_internal::PrintHeader(OutputStream &s, bool hide) const
{
  if (Type())  s << Type()->getName();
  s << " " << Name();
  formals.PrintHeader(s, hide);
}

symbol* simple_internal::FindFormal(const char* name) const
{
  return formals.find(name);
}

int simple_internal::maxNamedParams() const
{
  return formals.getLength();
}

int simple_internal
::named2Positional(symbol** np, int nnp, expr** buffer, int bufsize) const
{
  return formals.named2Positional(em, np, nnp, buffer, bufsize);
}

model_instance* simple_internal
 ::grabModelInstance(traverse_data &x, expr* first) const
{
  if (0==first)  return 0;
  model_instance* mi = dynamic_cast <model_instance*> (first);
  if (mi) {
    if (mi->NotProperInstance(x.parent, Name()))  return 0;
    return mi;
  }
  // still here?  someone plugged this function into something NOT a measure.
  if (!em->startError())  return 0;
  em->causedBy(x.parent);
  em->cerr() << "Function " << Name() << " is allowed only in measures";
  em->stopIO();
  return 0;
}

// ******************************************************************
// *                                                                *
// *                     model_internal methods                     *
// *                                                                *
// ******************************************************************

model_internal::model_internal(const type* t, const char* name, int nf)
 : simple_internal(t, name, nf)
{
  SetFormal(0, em->MODEL, "-m");  // Impossible name!
  formals.hide(0);
}

model_internal::~model_internal()
{
}

// ******************************************************************
// *                                                                *
// *                    custom_internal  methods                    *
// *                                                                *
// ******************************************************************

custom_internal::custom_internal(const char* name, const char* h)
 : internal_func(0, name)
{
  header = h;
}

void custom_internal::PrintHeader(OutputStream &s, bool hide) const
{
  DCASSERT(header);
  s.Put(header);
}

bool custom_internal::IsHidden(int fpnum) const
{
  return false;
}

// ******************************************************************
// *                                                                *
// *                        user_func  class                        *
// *                                                                *
// ******************************************************************

/**   Base class for user-defined functions with parameters.
*/
class user_func : public function {
protected:
  fplist formals;
  expr* return_expr;
public:
  user_func(function* f, formal_param** pl, int np);
  user_func(const char* fn, int line, const type* t, char* n, 
           formal_param **pl, int np);
  virtual ~user_func();

  inline void Invalidate() {
    DCASSERT(!isDefined());
    setError();
  };

  inline void SetReturn(expr *e) {
    return_expr = e;
    setDefined();
  }

  virtual void ResetFormals(formal_param** newformal, int nfp) = 0;

  virtual int Traverse(traverse_data &x, expr** pass, int np);

  virtual void PrintHeader(OutputStream &s, bool hide) const;
  virtual symbol* FindFormal(const char* name) const;
  virtual bool IsHidden(int fpnum) const;

  virtual bool HeadersMatch(const type* t, symbol** pl, int np) const;
  virtual bool HasNameConflict(symbol** fp, int np, int* tmp) const;
 
  virtual bool DocumentHeader(doc_formatter* df) const;
  virtual void DocumentBehavior(doc_formatter* df) const;
  inline void ShowWhereDefined(OutputStream &s) const {
    if (return_expr) {
      s.PutFile(return_expr->Filename(), return_expr->Linenumber());
    }
  };
  virtual int maxNamedParams() const;
  virtual int named2Positional(symbol** np, int nnp, expr** buffer, int bufsize) const;
};

user_func::user_func(function* f, formal_param** pl, int np) : function(f)
{
  formals.setAll(np, pl, false);
  return_expr = 0;
}

user_func::user_func(const char* fn, int line, const type* t, char* n, 
  formal_param **pl, int np) : function(fn, line, t, n)
{
  formals.setAll(np, pl, false);
  return_expr = 0;
}

user_func::~user_func()
{
  Delete(return_expr);
}

int user_func::Traverse(traverse_data &x, expr** pass, int np)
{
  switch (x.which) {
    case traverse_data::Typecheck: 
        return formals.check(em, pass, np, Type());
    
    case traverse_data::GetType:
        x.the_type = formals.getType(em, pass, np, Type());
        return 0;

    case traverse_data::Promote:
        formals.promote(em, pass, np, Type());
        return Promote_Success;

    default:
        return function::Traverse(x, pass, np);
  };
}

void user_func::PrintHeader(OutputStream &s, bool hide) const
{
  if (Type())  s << Type()->getName();
  s << " " << Name();
  formals.PrintHeader(s, hide);
}

symbol* user_func::FindFormal(const char* name) const
{
  return formals.find(name);
}

bool user_func::IsHidden(int fpnum) const
{
  return formals.isHidden(fpnum);
}

bool user_func::HeadersMatch(const type* t, symbol** pl, int np) const
{
  if (isDefined())  return false;
  if (Type() != t)  return false;
  return formals.matches(pl, np);
}

bool user_func::HasNameConflict(symbol** fp, int np, int* tmp) const
{
  return formals.hasNameConflict(fp, np, tmp);
}


bool user_func::DocumentHeader(doc_formatter* df) const
{
  if (0==df)      return false;
  if (0==Name())  return false;
  df->begin_heading();
  PrintHeader(df->Out(), 0);
  df->end_heading();
  return true;
}

void user_func::DocumentBehavior(doc_formatter* df) const
{
  df->Out() << "Defined ";
  df->Out().PutFile(Filename(), Linenumber());
}

int user_func::maxNamedParams() const
{
  return formals.getLength();
}

int user_func
::named2Positional(symbol** np, int nnp, expr** buffer, int bufsize) const
{
  return formals.named2Positional(em, np, nnp, buffer, bufsize);
}



// ******************************************************************
// *                                                                *
// *                      top_user_func  class                      *
// *                                                                *
// ******************************************************************

/**   Class for top-level user-defined functions.
      Note the new, static stack used for function calls.
*/  
class top_user_func : public user_func {
  static result* stack;
  static long stack_size;
  static long stack_top;
  static result* stackptr;
public:
  top_user_func(function* f, formal_param** pl, int np);
  top_user_func(const char* fn, int line, const type* t, char* n, 
           formal_param **pl, int np);

  virtual void ResetFormals(formal_param** newformal, int nfp);
  virtual void Compute(traverse_data &x, expr** pass, int np);
  virtual int Traverse(traverse_data &x, expr** pass, int np);

  // friends, because of stack manipulation
  friend void InitFunctions(exprman* om);
  friend class stack_size_option;
};

result* top_user_func::stack;
result* top_user_func::stackptr;
long top_user_func::stack_size;
long top_user_func::stack_top;


top_user_func::top_user_func(function* f, formal_param** pl, int np)
 : user_func(f, pl, np)
{
  formals.setStack(&stackptr);
}

top_user_func::top_user_func(const char* fn, int line, const type* t, char* n, 
  formal_param **pl, int np) : user_func(fn, line, t, n, pl, np)
{
  formals.setStack(&stackptr);
}

void top_user_func::ResetFormals(formal_param** newformal, int nfp) 
{
  DCASSERT(!isDefined());
  DCASSERT(nfp == formals.getLength());
  formals.setAll(nfp, newformal, false);
  formals.setStack(&stackptr);
}

void top_user_func::Compute(traverse_data &x, expr** pass, int np)
{
  result* answer = x.answer;
  DCASSERT(answer);
  DCASSERT(x.parent);
  // answer->Clear();
  if (0==return_expr) {
    answer->setNull();
    return;
  }

  // first... make sure there is enough room on the stack to save params
  if (stack_top+np > stack_size) {
    if (em->startError()) {
      em->cerr() << " in function " << Name() << " called ";
      em->causedBy(x.parent);
      em->cerr() << "Stack overflow";
      em->stopIO();
    }
    answer->setNull();
    return;
  }

  // Compute the passed parameters.
  result* startpos = stack + stack_top;
  stack_top += np;
  formals.compute(x, pass, startpos);

  // Re-align the formal parameters.
  result* old_stackptr = stackptr;
  stackptr = startpos;

  // "call" function
  DCASSERT(return_expr);
  x.answer = answer;
  return_expr->Compute(x);
  
  // Clear and pop the parameters
  for (int i=0; i<np; i++) {
    stack_top--;
    stack[stack_top].deletePtr(); 
  }
  stackptr = old_stackptr;
}

int top_user_func::Traverse(traverse_data &x, expr** pass, int np)
{
  switch (x.which) {
    case traverse_data::Substitute:
        if (0==formals.getLength()) {
          x.answer->setPtr(Share(return_expr));
          return 1;
        }
        return user_func::Traverse(x, pass, np);

    default:
        return user_func::Traverse(x, pass, np);
  }
}

// ******************************************************************
// *                                                                *
// *                    wrapped_user_func  class                    *
// *                                                                *
// ******************************************************************

/**   Class for user-defined functions inside models.
      This is basically a wrapper and we will construct
      the "actual" function when the model is instantiated.
*/  
class wrapped_user_func : public user_func {
protected:
  top_user_func* link;
public:
  wrapped_user_func(const char* fn, int line, const type* t, char* n, 
           formal_param **pl, int np);
  virtual void ResetFormals(formal_param** newformal, int nfp);
  virtual int Traverse(traverse_data &x, expr** pass, int np);
  virtual void Compute(traverse_data &x, expr** pass, int np);

  inline void showAll(OutputStream &s) const {
    PrintHeader(s, false);
    s << " := ";
    if (return_expr) return_expr->Print(s, 0);
    else s << "null";
  };

  symbol* instantiate();
};

wrapped_user_func::wrapped_user_func(const char* fn, int line, const type* t, 
  char* n, formal_param **pl, int np) : user_func(fn, line, t, n, pl, np)
{
  link = 0;
}

void wrapped_user_func::ResetFormals(formal_param** newformal, int nfp) 
{
  DCASSERT(!isDefined());
  DCASSERT(nfp == formals.getLength());
  formals.setAll(nfp, newformal, false);
}

void wrapped_user_func::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  if (link) link->Compute(x, pass, np);
  else      x.answer->setNull();
}

int wrapped_user_func::Traverse(traverse_data &x, expr** pass, int np)
{
  switch (x.which) {
    case traverse_data::Substitute:
        if (0==link) return 0;
        if (link->Traverse(x, pass, np)) return 1;
        x.answer->setPtr(new fcall(x.parent, link, pass, np));
        return 1;

    case traverse_data::ModelDone:
        link = 0;
        formals.traverse(x);
        return 0;

    default:
        if (link) return link->Traverse(x, pass, np);
        return user_func::Traverse(x, pass, np);
  }
}

symbol* wrapped_user_func::instantiate()
{
  int nfp = formals.getLength();
  formal_param** fp = nfp ? new formal_param*[nfp] : 0;
  for (int i=0; i<nfp; i++) {
    DCASSERT(formals.getParam(i));
    fp_wrapper* fpw = smart_cast <fp_wrapper*> (formals.getParam(i));
    DCASSERT(fpw);
    fp[i] = fpw->newCopy();
  }
  link = new top_user_func(this, fp, nfp);
  expr* rhs = return_expr ? return_expr->Substitute(0) : 0;
  link->SetReturn(rhs);
#ifdef DEBUG_FUNC_WRAPPERS
  em->cout() << "instantiated function: ";
  link->Print(em->cout(), 0);
  em->cout() << " := ";
  if (rhs) rhs->Print(em->cout(), 0); else em->cout() << "null";
  em->cout().Put('\n');
#endif
  return link;
}

// ******************************************************************
// *                                                                *
// *                        func_stmt  class                        *
// *                                                                *
// ******************************************************************

class func_stmt : public expr {
  wrapped_user_func* wuf;
  model_def* parent;
public:
  func_stmt(const char* fn, int ln, model_def* p, wrapped_user_func* f);
  virtual ~func_stmt();

  virtual bool Print(OutputStream &s, int) const;
  virtual void Compute(traverse_data &x);
  virtual void Traverse(traverse_data &x);
};

func_stmt
::func_stmt(const char* fn, int ln, model_def* p, wrapped_user_func* f)
 : expr(fn, ln, STMT)
{
  parent = p;
  wuf = f;
}

func_stmt::~func_stmt()
{
  Delete(wuf);
}

bool func_stmt::Print(OutputStream &s, int w) const
{
  s.Pad(' ', w);
  DCASSERT(wuf);
  wuf->showAll(s);
  s << ";\n";
  return true;
}

void func_stmt::Compute(traverse_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  if (x.stopExecution()) return;
  
  DCASSERT(parent);
  symbol* f = wuf->instantiate();
  parent->AcceptSymbolOwnership(f);
}

void func_stmt::Traverse(traverse_data &x)
{
  switch (x.which) {
    case traverse_data::ModelDone:
      wuf->Traverse(x, 0, 0);
      return;

    default:
      expr::Traverse(x);
  };
}

// ******************************************************************
// *                                                                *
// *                    stack_size_option  class                    *
// *                                                                *
// ******************************************************************

class stack_size_option : public custom_option {
public:
  stack_size_option(const char* name, const char* doc);
  virtual error SetValue(long n);
  virtual error GetValue(long &v) const;
};

stack_size_option::stack_size_option(const char* name, const char* doc)
 : custom_option(option::Integer, name, doc, "non-negative integers")
{
}

option::error stack_size_option::SetValue(long s)
{
  // this is slow and sure, but since this is RARELY called, should be ok.
  if (s<0) return RangeError;
  if (s == top_user_func::stack_size) {
    return Success;
  }
  result* newstack = new result[s];
  for (long i=0; i<top_user_func::stack_top; i++) {
    newstack[i] = top_user_func::stack[i];
  }
  delete[] top_user_func::stack;
  top_user_func::stack = newstack;
  top_user_func::stack_size = s;
  return Success;
}

option::error stack_size_option::GetValue(long &v) const
{
  v = top_user_func::stack_size;
  return Success;
}

// ******************************************************************
// *                                                                *
// *                        exprman  methods                        *
// *                                                                *
// ******************************************************************

expr* exprman::makeFunctionCall(const char* fn, int ln, 
      symbol *f, expr **p, int np) const
{
  function* func = dynamic_cast <function*> (f);
  bool bail_out = (0==func);

  int code = bail_out ? function::Promote_Success : func->PromoteParams(p, np);
  switch (code) {
    case function::Promote_Success:
        break;

    case function::Promote_MTMismatch:
        bail_out = true;
        if (startError()) {
          causedBy(fn, ln);
          cerr() << "Model parameters in call to function " << f->Name();
          cerr() << " must have the same parent";
          stopIO();
        }
        break;

    case function::Promote_Dependent:
        if (startWarning()) {
          causedBy(fn, ln);
          warn() << "Function " << f->Name();
          warn() << " requires independent parameters.";
          newLine();
          warn() << "Phase-type model will be incorrect.";
          stopIO();
        }
        break;

    default:
        bail_out = true;
  } // code

  const type* t = bail_out ? 0 : func->DetermineType(p, np);

  if (0 == t) {
    for (int i=0; i<np; i++)  Delete(p[i]);
    delete[] p;
    return makeError();
  }

  return new fcall(fn, ln, t, func, p, np);
}



// ******************************************************************
// *                                                                *
// *                           front  end                           *
// *                                                                *
// ******************************************************************

const int init_stack_size = 1024;

symbol* MakeFormalParam(const char* fn, int ln, 
      const type* t, char* name, bool in_model)
{
  if (in_model) {
    return new fp_wrapper(fn, ln, t, name);
  } else {
    return new fp_onstack(fn, ln, t, name); 
  }
}

symbol* MakeFormalParam(typelist* t, char* name)
{
  return new fp_onstack(t, name); 
}

symbol* MakeFormalParam(const exprman* em, const char* fn, int ln, 
      const type* t, char* name, expr* def, bool in_model)
{
  // check return type for default
  const type* dt = em->SafeType(def);
  if (!em->isPromotable(dt, t)) {
    if (em->startError()) {
      em->causedBy(fn, ln);
      em->cerr() << "default type does not match parameter " << name;  
      em->stopIO();
    }
    free(name);
    Delete(def);
    return 0;
  }

  formal_param* fp;
  if (in_model) {
    fp = new fp_wrapper(fn, ln, t, name);
  } else {
    fp = new fp_onstack(fn, ln, t, name); 
  }
  fp->SetDefault(def);
  return fp;
}

symbol* MakeNamedParam(const char* fn, int ln, char* name, expr* pass)
{
  return new named_param(fn, ln, name, pass);
}

function* MakeUserFunction(const exprman* em, const char* fn, int ln, 
      const type* t, char* name, symbol** formals, int np, bool in_model)
{
  if (0==formals) {
    free(name);
    return 0;
  }
  // check formals
  for (int i=0; i<np; i++) {
    formal_param* fp = dynamic_cast <formal_param*> (formals[i]);
    if (fp)  continue;
    // bad formal parameter, bail out
    for (int j=0; j<np; j++)  Delete(formals[j]);
    delete[] formals;
    free(name);
    return 0;
  }

  if (in_model) {
    return new wrapped_user_func(fn, ln, t, name, (formal_param**) formals, np);
  } else {
    return new top_user_func(fn, ln, t, name, (formal_param**) formals, np);
  }
}

function* MakeUserConstFunc(const exprman* em, const char* fn, int ln, 
      const type* t, char* name, bool in_model)
{
  if (in_model) {
    return new wrapped_user_func(fn, ln, t, name, 0, 0);
  } else {
    return new top_user_func(fn, ln, t, name, 0, 0);
  }
}

void ResetUserFunctionParams(const exprman* em, const char* fn, int ln, 
      symbol* userfunc, symbol** formals, int nfp)
{
  function* f = dynamic_cast <function*> (userfunc);
  if (0==f) {
    for (int i=0; i<nfp; i++) Delete(formals[i]);
    delete[] formals;
    return;
  }
  // check if f is already an internal function
  user_func* uf = dynamic_cast <user_func*> (f);
  if (0==uf) {
    if (em->startError()) {
      em->causedBy(fn, ln);
      em->cerr() << "Function declaration conflicts with existing function:";
      em->newLine();
      f->PrintHeader(em->cerr(), true);
      em->cerr() << " declared internally";
      em->stopIO();
    }
    for (int i=0; i<nfp; i++) Delete(formals[i]);
    delete[] formals;
    return;
  }
  // check if f is defined already
  if (uf->isDefined()) {
    if (em->startError()) {
      em->causedBy(fn, ln);
      em->cerr() << "Function ";
      f->PrintHeader(em->cerr(), true);
      em->cerr() << " was already defined";
      em->newLine();
      uf->ShowWhereDefined(em->cerr());
      em->stopIO();
    }
    for (int i=0; i<nfp; i++) Delete(formals[i]);
    delete[] formals;
    return;
  }

  // We can set these now.
  uf->ResetFormals((formal_param**) formals, nfp);

  // check formals
  for (int i=0; i<nfp; i++) {
    formal_param* fp = dynamic_cast <formal_param*> (formals[i]);
    if (fp)  continue;
    // bad formal parameter, bail out
    uf->Invalidate();
  }
}

expr* DefineUserFunction(const exprman* em, const char* fn, int ln,
      symbol* userfunc, expr* rhs, model_def* mdl)
{
  user_func* uf = dynamic_cast <user_func*> (userfunc);

  if (0==uf) {
    Delete(rhs);
    return 0;
  }

  if (0==rhs || em->isError(rhs)) {
    uf->SetReturn(0);
    return 0;
  }

  // check if return expression matches type of f
  const type* target = uf->Type();
  if (!em->isPromotable(rhs->Type(), target)) {
    if (em->startError()) {
      em->causedBy(fn, ln);
      em->cerr() << "Return type for function " << uf->Name();
      em->cerr() << " should be ";
      uf->PrintType(em->cerr());
      em->stopIO();
    }
    Delete(rhs);
    uf->Invalidate();
    return 0;
  }
  rhs = em->promote(rhs, target);
  DCASSERT(! em->isError(rhs) );
  uf->SetReturn(rhs);

  if (0==mdl) return 0;
  wrapped_user_func* wuf = smart_cast <wrapped_user_func*>(uf);
  DCASSERT(wuf);

  return new func_stmt(fn, ln, mdl, wuf);
}



void InitFunctions(exprman* em)
{
  top_user_func::stack = new result[init_stack_size];
  top_user_func::stackptr = top_user_func::stack;
  top_user_func::stack_size = init_stack_size;
  top_user_func::stack_top = 0;

  DCASSERT(em);
  em->addOption(
    new stack_size_option("StackSize", "Size of run-time stack to use for function calls.")
  );
}

