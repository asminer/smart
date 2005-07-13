
// $Id$

#include "ops_state.h"

#include "../Language/operators.h"


/** @name ops_state.cc
    @type File
    @args \ 

   Special, internally-used operator classes for states.

   The necessary backend of ops_state.h
   Classes are defined here.

 */

//@{


// ******************************************************************
// *                   statevar_comp (base) class                   *
// ******************************************************************

class statevar_comp : public constant {
protected:
  model_var* var;
  int op;
  int rightside;
public:
  statevar_comp(const char* f, int ln, model_var* v, int o, int rhs)
  : constant(f, ln, PROC_BOOL) {
    var = v;
    op = o;
    rightside = rhs;
  }
  virtual void show(OutputStream &s) const {
    s << var->Name() << GetOp(op) << rightside;
  }
  virtual int GetSymbols(int i, List <symbol> *syms=NULL) {
    DCASSERT(i==0);
    if (syms) syms->Append(var);
    return 1;
  }
};

// ******************************************************************
// *                       sv_eq_const  class                       *
// ******************************************************************

class sv_eq_const : public statevar_comp {
public:
  sv_eq_const(const char* f, int ln, model_var* v, int rhs)
  : statevar_comp(f, ln, v, EQUALS, rhs) { }
  virtual void Compute(Rng *r, const state *s, int i, result &x) {
    DCASSERT(0==i);
    DCASSERT(s);
    x.bvalue = (s->Read(var->state_index).ivalue == rightside); 
  }
};

// ******************************************************************
// *                       sv_le_const  class                       *
// ******************************************************************

class sv_lt_const : public statevar_comp {
public:
  sv_lt_const(const char* f, int ln, model_var* v, int rhs)
  : statevar_comp(f, ln, v, LT, rhs) { }
  virtual void Compute(Rng *r, const state *s, int i, result &x) {
    DCASSERT(0==i);
    DCASSERT(s);
    x.bvalue = (s->Read(var->state_index).ivalue < rightside); 
  }
};

// ******************************************************************
// *                       sv_ge_const  class                       *
// ******************************************************************

class sv_ge_const : public statevar_comp {
public:
  sv_ge_const(const char* f, int ln, model_var* v, int rhs)
  : statevar_comp(f, ln, v, GE, rhs) { }
  virtual void Compute(Rng *r, const state *s, int i, result &x) {
    DCASSERT(0==i);
    DCASSERT(s);
    x.bvalue = (s->Read(var->state_index).ivalue >= rightside); 
  }
};

// ******************************************************************
// *                statevar_comp_expr  (base) class                *
// ******************************************************************

class statevar_comp_expr : public unary {
protected:
  model_var* var;
  int op;
public:
  statevar_comp_expr(const char* f, int ln, model_var* v, int o, expr* rhs) : unary(f, ln, rhs) {
    var = v;
    op = o;
  }
  virtual void show(OutputStream &s) const {
    s << var->Name() << GetOp(op) << opnd;
  }
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return PROC_BOOL;
  }
  virtual int GetSymbols(int i, List <symbol> *syms=NULL) {
    DCASSERT(i==0);
    int answer = 1;
    if (syms) syms->Append(var);
    answer += opnd->GetSymbols(i, syms);
    return answer;
  }
protected:
  virtual expr* MakeAnother(expr* newopnd) {
    return new statevar_comp_expr(Filename(), Linenumber(), var, op, opnd);
  }
};

// ******************************************************************
// *                        sv_lt_expr class                        *
// ******************************************************************

class sv_lt_expr : public statevar_comp_expr {
public:
  sv_lt_expr(const char* f, int ln, model_var* v, expr* rhs)
  : statevar_comp_expr(f, ln, v, LT, rhs) { }
  virtual void Compute(Rng *r, const state *s, int i, result &x) {
    DCASSERT(0==i);
    DCASSERT(s);
    SafeCompute(opnd, r, s, 0, x);
    if (x.isNormal()) {
      x.bvalue = (s->Read(var->state_index).ivalue < x.ivalue); 
      return;
    }
    if (x.isInfinity()) {
      x.bvalue = (x.ivalue > 0);
      x.Clear(); // clear the infinity setting
      return;
    }
    // anything else, propogate
  }
};

// ******************************************************************
// *                        sv_ge_expr class                        *
// ******************************************************************

class sv_ge_expr : public statevar_comp_expr {
public:
  sv_ge_expr(const char* f, int ln, model_var* v, expr* rhs)
  : statevar_comp_expr(f, ln, v, GE, rhs) { }
  virtual void Compute(Rng *r, const state *s, int i, result &x) {
    DCASSERT(0==i);
    DCASSERT(s);
    SafeCompute(opnd, r, s, 0, x);
    if (x.isNormal()) {
      x.bvalue = (s->Read(var->state_index).ivalue >= x.ivalue); 
      return;
    }
    if (x.isInfinity()) {
      x.bvalue = (x.ivalue < 0);
      x.Clear(); // clear the infinity setting
      return;
    }
    // anything else, propogate
  }
};

// ******************************************************************
// *                        sv_between class                        *
// ******************************************************************

/** INT <= state variable <= INT
*/
class sv_between : public constant {
protected:
  model_var* var;
  int lower;
  int upper;
public:
  sv_between(const char* fn, int ln, int L, model_var* v, int U)
  : constant(fn, ln, PROC_BOOL) {
    var = v;
    lower = L;
    upper = U;
  }
  virtual void show(OutputStream &s) const {
    s << lower << "<=" << var->Name() << "<=" << upper;
  }
  virtual void Compute(Rng *r, const state *s, int i, result &x) {
    DCASSERT(0==i);
    DCASSERT(s);
    x.bvalue =	(lower <= s->Read(var->state_index).ivalue) 
		&& 
		(s->Read(var->state_index).ivalue <= upper); 
  }
  virtual int GetSymbols(int i, List <symbol> *syms=NULL) {
    DCASSERT(i==0);
    if (syms) syms->Append(var);
    return 1;
  }
};

// ******************************************************************
// *                     add_const_to_sv  class                     *
// ******************************************************************

class add_const_to_sv : public constant {
private:
  const char* modelname;
  model_var* var;
  int delta;
public:
  add_const_to_sv(const char* f, int ln, const char* mn, model_var* v, int d) : constant(f, ln, PROC_STATE) {
    modelname = mn;
    var = v;
    delta = d;
    DCASSERT(delta);
  }
  virtual ~add_const_to_sv() {
    // DO NOT DELETE any names
  }
  virtual void NextState(const state &, state &next, result &x);
  virtual void show(OutputStream &s) const {
    s << var->Name();
    if (delta>0) s << "+=" << delta; 
    else s << "-=" << -delta;
  }
  virtual int GetSymbols(int i, List <symbol> *syms=NULL) {
    DCASSERT(i==0);
    if (syms) syms->Append(var);
    return 1;
  }
};

void add_const_to_sv::NextState(const state &, state &next, result &x) 
{
  next[var->state_index].ivalue += delta;
  if (	( // lower bound violation
	  var->hasLowerBound() && 
          next[var->state_index].ivalue < var->getLowerBound()
        ) 
  	||
   	( // upper bound violation
 	  var->hasUpperBound() &&
    	  next[var->state_index].ivalue > var->getUpperBound() 
   	)  ) 
    {
      Error.Start(Filename(), Linenumber());
      if (delta>0) Error << "Overflow of ";
      else Error << "Underflow of ";
      Error << GetType(var->Type(0)) << " " << var->Name();
      Error << " in model " << modelname;
      x.setError();
      Error.Stop();
      return;
    } // if
}

// ******************************************************************
// *                     change_sv (base) class                     *
// ******************************************************************

class change_sv : public unary {
protected:
  const char* modelname;
  model_var* var;
  int op;  // for display
public:
  change_sv(const char* f, int ln, const char* mn, model_var* v, int o, expr* rhs) : unary(f, ln, rhs) {
    modelname = mn;
    var = v;
    op = o;
  }
  virtual ~change_sv() {
    // DO NOT DELETE any names
  }
  virtual void show(OutputStream &s) const {
    s << var->Name() << GetOp(op) << '=' << opnd;
  }
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return PROC_STATE;
  }
  virtual int GetSymbols(int i, List <symbol> *syms=NULL) {
    DCASSERT(i==0);
    int answer = 1;
    if (syms) syms->Append(var);
    answer += opnd->GetSymbols(i, syms);
    return answer;
  }
protected:
  inline void CheckBounds(const state &s, int d, result &x) const {
    if (( // lower bound violation
	  var->hasLowerBound() && 
          s.Read(var->state_index).ivalue < var->getLowerBound()
        ) 
  	||
   	( // upper bound violation
 	  var->hasUpperBound() &&
    	  s.Read(var->state_index).ivalue > var->getUpperBound() 
   	)) 
    {
      Error.Start(Filename(), Linenumber());
      if (d>0) Error << "Overflow of ";
      else Error << "Underflow of ";
      Error << GetType(var->Type(0)) << " " << var->Name();
      Error << " in model " << modelname;
      x.setError();
      Error.Stop();
    } // if
  }
  inline void InfinityError(int sign) {
      Error.Start(Filename(), Linenumber());
      Error << "Infinity ";
      if (sign>0) Error << "added to ";
      else Error << "subtracted from ";
      Error << GetType(var->Type(0));
      Error << " " << var->Name() << " in model " << modelname;
      Error.Stop();
  }
};

// ******************************************************************
// *                        add_to_sv  class                        *
// ******************************************************************

class add_to_sv : public change_sv {
public:
  add_to_sv(const char* f, int ln, const char* mn, model_var* v, expr* rhs)
   : change_sv(f, ln, mn, v, PLUS, rhs) { }
  virtual void NextState(const state &curr, state &next, result &x);
protected:
  virtual expr* MakeAnother(expr* newopnd) {
    return new add_to_sv(Filename(), Linenumber(), modelname, var, newopnd);
  }
};

void add_to_sv::NextState(const state &curr, state& next, result &x)
{
  SafeCompute(opnd, NULL, &curr, 0, x);
  if (x.isNormal()) {
    next[var->state_index].ivalue += x.ivalue;
    CheckBounds(next, x.ivalue, x);
    return;
  } // normal x
  if (x.isUnknown()) {
    next[var->state_index].setUnknown();
    return;
  }
  if (x.isInfinity()) {
      InfinityError(x.ivalue);
      x.setError();
      return;
  }
  // some other error, propogate it
}

// ******************************************************************
// *                       sub_from_sv  class                       *
// ******************************************************************

class sub_from_sv : public change_sv {
public:
  sub_from_sv(const char* f, int ln, const char* mn, model_var* v, expr* rhs)
   : change_sv(f, ln, mn, v, MINUS, rhs) { }
  virtual void NextState(const state &curr, state &next, result &x);
protected:
  virtual expr* MakeAnother(expr* newopnd) {
    return new sub_from_sv(Filename(), Linenumber(), modelname, var, newopnd);
  }
};

void sub_from_sv::NextState(const state &curr, state& next, result &x)
{
  SafeCompute(opnd, NULL, &curr, 0, x);
  if (x.isNormal()) {
    next[var->state_index].ivalue -= x.ivalue;
    CheckBounds(next, -x.ivalue, x);
    return;
  } // normal x
  if (x.isUnknown()) {
    next[var->state_index].setUnknown();
    return;
  }
  if (x.isInfinity()) {
      InfinityError(-x.ivalue);
      x.setError();
      return;
  }
  // some other error, propogate it
}

// ******************************************************************
// *                                                                *
// *                        Global frontends                        *
// *                                                                *
// ******************************************************************

expr* MakeConstCompare(model_var* var, int op, int right, const char* f, int ln)
{
  switch (op) {
    case EQUALS:	return new sv_eq_const(f, ln, var, right);
    case LT:		return new sv_lt_const(f, ln, var, right);
    case GE:		return new sv_ge_const(f, ln, var, right);
    // Not sure if the others are necessary
    default:
	DCASSERT(0);
  }
  return NULL;
}

expr* MakeExprCompare(model_var* var, int op, expr* right, const char* f, int ln)
{
  switch (op) {
    case LT:	return new sv_lt_expr(f, ln, var, right);
    case GE:	return new sv_ge_expr(f, ln, var, right);
    // Not sure if the others are necessary
    default:
	DCASSERT(0);
  }
  return NULL;
}

expr* MakeConstBounds(int lower, model_var* var, int upper, const char* f, int ln)
{
  DCASSERT(lower <= upper);
  if (lower==upper) return new sv_eq_const(f, ln, var, lower);

  return new sv_between(f, ln, lower, var, upper);
}

expr* ChangeStateVar(const char* mdl, model_var* var, int delta, const char* f, int ln)
{
  DCASSERT(delta);
  return new add_const_to_sv(f, ln, mdl, var, delta);
}

expr* ChangeStateVar(const char* mdl, model_var* var, int OP, expr* rhs, const char* f, int ln)
{
  DCASSERT(rhs);
  switch (OP) {
    case PLUS:	return new add_to_sv(f, ln, mdl, var, rhs);
    case MINUS:	return new sub_from_sv(f, ln, mdl, var, rhs);
    // Not sure if the others are necessary
    default:
	DCASSERT(0);
  }
  return NULL;
}

//@}

