
// $Id$

#include "exprs.h"
#include "measures.h"

//@Include: exprs.h

/** @name exprs.cc
    @type File
    @args \ 

   Implementation of simple expression classes.

 */

//@{

// ******************************************************************
// *                                                                *
// *                    Output-related functions                    *
// *                                                                *
// ******************************************************************

OutputStream& operator<< (OutputStream &s, expr *e)
{
  if (ERROR==e) {
    s << "error";
    return s;
  }
  if (DEFLT==e) {
    s << "default";
    return s;
  }
  if (e) e->show(s);
  else s << "null";
  return s;
}

void PrintExprType(expr *e, OutputStream &s)
{
  if (NULL==e) {
    s << "null";
    return;
  }
  if (ERROR==e) {
    s << "error";
    return;
  }
  if (DEFLT==e) {
    s << "default";
    return;
  }
  for (int i=0; i<e->NumComponents(); i++) {
    if (i) s << ":";
    s << GetType(e->Type(i));
  }
}


// ******************************************************************
// *                                                                *
// *                           expr class                           *
// *                                                                *
// ******************************************************************

// Defaults for virtual functions here.

int expr::NumComponents() const 
{ 
  return 1; 
}

expr* expr::GetComponent(int i) 
{
  DCASSERT(i==0);
  return this;
}

void expr::Compute(int i, result &x) 
{
  Internal.Start(__FILE__, __LINE__, filename, linenumber);
  Internal << "Illegal expression compuation!";
  Internal.Stop();
}

void expr::Sample(Rng &, int i, result &x) 
{
  Internal.Start(__FILE__, __LINE__, filename, linenumber);
  Internal << "Illegal expression sample!";
  Internal.Stop();
}

void expr::Compute(const state&, int i, result &x) 
{
  Internal.Start(__FILE__, __LINE__, filename, linenumber);
  Internal << "Illegal expression (proc) compuation!";
  Internal.Stop();
}

void expr::Sample(Rng &, const state&, int i, result &x) 
{
  Internal.Start(__FILE__, __LINE__, filename, linenumber);
  Internal << "Illegal expression (proc) sample!";
  Internal.Stop();
}

int expr::GetSums(int i, List <expr> *sums)
{
  DCASSERT(i==0);
  if (sums) sums->Append(this);
  return 1;
}

int expr::GetProducts(int i, List <expr> *prods) 
{
  DCASSERT(i==0);
  if (prods) prods->Append(this);
  return 1;
}

int expr::GetSymbols(int i, List <symbol> *)
{
  DCASSERT(i==0);
  return 0;
}

Engine_type expr::GetEngine(engineinfo *)
{
  Internal.Start(__FILE__, __LINE__, filename, linenumber);
  Internal << "Illegal expression engine request";
  Internal.Stop();
  return ENG_Error;  // Keep compiler quiet
}

expr* expr::SplitEngines(List <measure> *)
{
  Internal.Start(__FILE__, __LINE__, filename, linenumber);
  Internal << "Illegal expression engine split";
  Internal.Stop();
  return NULL;
}

// ******************************************************************
// *                                                                *
// *                         constant class                         *
// *                                                                *
// ******************************************************************

constant::constant(const char* fn, int line, type mt) : expr (fn, line)
{
  mytype = mt;
}

type constant::Type(int i) const
{
  DCASSERT(i==0);
  return mytype;
}

expr* constant::Substitute(int i)
{
  DCASSERT(0==i);
  return Copy(this);
}

Engine_type constant::GetEngine(engineinfo *e)
{
  if (e) e->setNone();
  return ENG_None;
}

expr* constant::SplitEngines(List <measure> *)
{
  return Copy(this);
}

// ******************************************************************
// *                                                                *
// *                          unary  class                          *
// *                                                                *
// ******************************************************************

unary::unary(const char* fn, int line, expr *x) : expr(fn,line) 
{
  DCASSERT(x!=ERROR);
  DCASSERT(x!=DEFLT);
  opnd = x;
  DCASSERT(opnd);
}

unary::~unary()
{
  Delete(opnd);
}

void unary::ClearCache()
{
  if (opnd) opnd->ClearCache();
}

int unary::GetSymbols(int i, List <symbol> *syms)
{
  DCASSERT(i==0);
  if (opnd) return opnd->GetSymbols(0, syms);
  return 0;
}

expr* unary::Substitute(int i)
{
  DCASSERT(i==0);
  // Slick...
  expr* newopnd = opnd->Substitute(0);
  if (newopnd==opnd) {
    // we created a copy... so delete the copy and copy ourself
    Delete(newopnd);
    return Copy(this);
  }
  // The substitution is different... make a new one of us
  return MakeAnother(newopnd);
}

void unary::unary_show(OutputStream &s, const char* op) const
{
  s << op << opnd;
}

Engine_type unary::GetEngine(engineinfo *e)
{
  if (opnd) return opnd->GetEngine(e);
  if (e) e->setNone();
  return ENG_None;
}

expr* unary::SplitEngines(List <measure> *mlist)
{
  if (NULL==opnd) return NULL;  // sanity
  // like substitute...
  expr* newopnd = opnd->SplitEngines(mlist);
  if (newopnd==opnd) {
    // we created a copy... so delete the copy and copy ourself
    Delete(newopnd);
    return Copy(this);
  }
  // The split is different... make a new one of us
  return MakeAnother(newopnd);
}

// ******************************************************************
// *                                                                *
// *                          binary class                          *
// *                                                                *
// ******************************************************************

binary::binary(const char* fn, int line, expr *l, expr *r) : expr(fn,line) 
{
  left = l;
  right = r;
  DCASSERT(left);
  DCASSERT(right);
  DCASSERT(left!=ERROR);
  DCASSERT(left!=DEFLT);
  DCASSERT(right!=ERROR);
  DCASSERT(right!=DEFLT);
}

binary::~binary()
{
  Delete(left);
  Delete(right);
}

void binary::ClearCache()
{
  if (left) left->ClearCache();
  if (right) right->ClearCache();
}

int binary::GetSymbols(int i, List <symbol> *syms)
{
  DCASSERT(i==0);
  int answer = 0;
  if (left) {
    answer = left->GetSymbols(0, syms);
  } 
  if (right) {
    answer += right->GetSymbols(0, syms);
  }
  return answer;
}

expr* binary::Substitute(int i)
{
  DCASSERT(i==0);
  // Slick...
  expr* newleft = left->Substitute(0);
  expr* newright = right->Substitute(0);
  if ((newleft==left) && (newright==right)) {
    // We can safely just copy ourself
    Delete(newleft);
    Delete(newright);
    return Copy(this);
  }
  // The substitution is different... make a new one of us
  return MakeAnother(newleft, newright);
}

void binary::binary_show(OutputStream &s, const char* op) const
{
  s << "(" << left << op << right << ")";
}

Engine_type binary::GetEngine(engineinfo *e)
{
  Engine_type l,r;
  l = left ? left->GetEngine(NULL) : ENG_None;
  r = right ? right->GetEngine(NULL) : ENG_None;
  if ((l == ENG_None) && (r == ENG_None)) {
    if (e) e->setNone();
    return ENG_None;
  }
  if ((l == ENG_Error) || (r == ENG_Error)) {
    if (e) e->engine = ENG_Error;
    return ENG_Error;
  }
  // It is best to merge model-checking engines
  if ((l == ENG_ModelChecking) && (r == ENG_ModelChecking)) {
    if (e) e->engine = ENG_ModelChecking;
    return ENG_ModelChecking;
  }
  // any other cases to merge?

  // Mixed, even if they are the same (because it is better to split)
  if (e) e->setMixed();
  return ENG_Mixed; 
}

expr* binary::SplitEngines(List <measure> *mlist)
{
  measure* m;
  Engine_type mine = GetEngine(NULL);
  if (ENG_Error == mine) return NULL;
  if (ENG_Mixed != mine) return Copy(this);

  // Still here?  We have a mixed engine, must split.

  Engine_type l = left ? left->GetEngine(NULL) : ENG_None;
  expr* newleft;
  switch (l) {
    case ENG_None:
    	newleft = Copy(left);
	break;

    case ENG_Mixed: 
    	newleft = left->SplitEngines(mlist);
	break;

    default:
     	m = new measure(left->Filename(), left->Linenumber(), 
			left->Type(0), NULL);
	m->SetReturn(Copy(left));
	mlist->Append(m);
	newleft = m;
  }

  Engine_type r = right ? right->GetEngine(NULL) : ENG_None;
  expr* newright;
  switch (r) {
    case ENG_None:
    	newright = Copy(right);
	break;

    case ENG_Mixed: 
    	newright = right->SplitEngines(mlist);
	break;

    default:
     	m = new measure(right->Filename(), right->Linenumber(), 
			right->Type(0), NULL);
	m->SetReturn(Copy(right));
	mlist->Append(m);
	newright = m;
  }

  return MakeAnother(newleft, newright);
}

// ******************************************************************
// *                                                                *
// *                          assoc  class                          *
// *                                                                *
// ******************************************************************

assoc::assoc(const char* fn, int line, expr **x, int n) : expr(fn,line) 
{
  opnd_count = n;
  operands = x;
#ifdef DEVELOPMENT_CODE
  int i;
  for (i=0; i<n; i++)
    DCASSERT(operands[i]);
#endif
}

assoc::assoc(const char* fn, int line, expr *l, expr *r) : expr(fn,line)
{
  opnd_count = 2;
  operands = new expr*[2];
  operands[0] = l;
  operands[1] = r;
#ifdef DEVELOPMENT_CODE
  DCASSERT(l);
  DCASSERT(r);
#endif
}

assoc::~assoc()
{
  int i;
  for (i=0; i<opnd_count; i++) Delete(operands[i]);
  delete[] operands;
}

void assoc::ClearCache()
{
  int i;
  for (i=0; i<opnd_count; i++) {
    DCASSERT(operands[i]);
    operands[i]->ClearCache();
  } 
}

int assoc::GetSymbols(int a, List <symbol> *syms)
{
  DCASSERT(a==0);
  int answer = 0;
  int i;
  for (i=0; i<opnd_count; i++) {
    DCASSERT(operands[i]);
    answer += operands[i]->GetSymbols(0, syms);
  }
  return answer;
}

expr* assoc::Substitute(int a)
{
  DCASSERT(a==0);
  // Slick...
  expr** newops = new expr* [opnd_count];
  bool notequal = false;
  int i;
  for (i=0; i<opnd_count; i++) {
    DCASSERT(operands[i]);
    newops[i] = operands[i]->Substitute(0);
    if (newops[i] != operands[i]) notequal = true;
  }
  if (notequal)  // The substitution is different... make a new one of us
    return MakeAnother(newops, opnd_count);

  // Substitution doesn't change anything, copy ourself

  // But delete the copies first...
  for (i=0; i<opnd_count; i++)
    Delete(newops[i]);
  delete[] newops;

  return Copy(this);
}

void assoc::assoc_show(OutputStream &s, const char* op) const
{
  s << "(" << operands[0];
  int i;
  for (i=1; i<opnd_count; i++) 
    s << op << operands[i];
  s << ")";
}

Engine_type assoc::GetEngine(engineinfo *e)
{
  DCASSERT(operands);
  DCASSERT(operands[0]);
  if (opnd_count < 2) return operands[0]->GetEngine(e);

  Engine_type foo = operands[0]->GetEngine(NULL);
  if (ENG_Error == foo) {
    if (e) e->engine = ENG_Error;
    return foo;
  }
  int i;
  for (i=1; i<opnd_count; i++) {
    DCASSERT(operands[i]);
    Engine_type bar = operands[i]->GetEngine(NULL);
    if (ENG_Error == bar) {
      if (e) e->engine = ENG_Error;
      return ENG_Error;
    }
    if ((ENG_ModelChecking == foo) && (ENG_ModelChecking == bar)) {
      // merge model checking engines
      continue;
    }
    if ((ENG_None == foo) && (ENG_None == bar)) {
      // merge when no engine
      continue;
    }
    // still here? Must be mixed.
    foo = ENG_Mixed; 
  }
  if (e) e->engine = foo;
  return foo;
}

expr* assoc::SplitEngines(List <measure> *mlist)
{
  DCASSERT(operands);
  DCASSERT(operands[0]);
  if (opnd_count < 2) return operands[0]->SplitEngines(mlist);

  expr** newops = new expr* [opnd_count];
  int i;
  for (i=0; i<opnd_count; i++) {
    DCASSERT(operands[i]);
    Engine_type et = operands[i]->GetEngine(NULL);
    measure *m;
    switch (et) {
      case ENG_Error:
      	delete[] newops;
	return NULL;
	break;

      case ENG_None:
    	newops[i] = Copy(operands[i]);
	break;

      case ENG_Mixed: 
    	newops[i] = operands[i]->SplitEngines(mlist);
	break;

      default:  // A "leaf"
     	m = new measure(operands[i]->Filename(), operands[i]->Linenumber(), 
			operands[i]->Type(0), NULL);
	m->SetReturn(Copy(operands[i]));
	mlist->Append(m);
	newops[i] = m;
    } // switch
  } // for i

  return MakeAnother(newops, opnd_count);
}

// ******************************************************************
// *                                                                *
// *                          symbol class                          *
// *                                                                *
// ******************************************************************

symbol::symbol(const char* fn, int line, type t, char* n) : expr (fn, line)
{
  mytype = t;
  aggtype = NULL;
  agglength = 1;
  name = n;
  substitute_value = true;
}

symbol::symbol(const char* fn, int line, type *t, int tlen, char* n) 
 : expr (fn, line)
{
  mytype = VOID;
  aggtype = t;
  agglength = tlen;
  DCASSERT(tlen>1);
  DCASSERT(t);
  name = n;
  substitute_value = true;
}



symbol::~symbol()
{
  delete[] name;
}

type symbol::Type(int i) const
{
  DCASSERT(i<agglength);
  if (NULL==aggtype) return mytype;
  return aggtype[i];
}

int symbol::NumComponents() const
{
  return agglength;
}

int symbol::GetSymbols(int i, List <symbol> *syms) 
{
  DCASSERT(i==0);
  if (syms) syms->Append(this);
  return 1;
}

expr* symbol::Substitute(int i)
{
  DCASSERT(i==0);
  if (substitute_value) {
    result x;
    Compute(0, x);
    return MakeConstExpr(Type(0), x, Filename(), Linenumber());
  }
  // Don't substitute
  return Copy(this);
}

void symbol::show(OutputStream &s) const
{
  if (NULL==name) return;  // Hidden symbol?
  s << name;
}

bool symbol::Matches(symbol *x) const
{
  if (NULL==x) return false;
  // check type
  if (agglength != x->agglength) return false;
  if (agglength>1) {
    // check aggregate type
    DCASSERT(aggtype);
    DCASSERT(x->aggtype);
    for (int i=0; i<agglength; i++) {
      if (aggtype[i] != x->aggtype[i]) return false;
    }
  } else {
    // check simple type
    if (mytype != x->mytype) return false;
  }
  // types match, check names
  if (NULL==name && NULL==x->name) return true;
  if (NULL==name || NULL==x->name) return false;
  return (strcmp(name, x->name)==0);
}

// ******************************************************************
// *                                                                *
// *                        aggregates class                        *
// *                                                                *
// ******************************************************************

/**   The class used to aggregate expressions.
 
      We are derived from assoc, but most of the
      provided functions of assoc must be overloaded
      (because of the special nature of aggregates).
*/  

class aggregates : public assoc {
public:
  /// Constructor.
  aggregates(const char* fn, int line, expr **x, int n) 
    : assoc (fn, line, x, n) { }

  virtual int NumComponents() const;
  virtual expr* GetComponent(int i);
  virtual type Type(int i) const;
  virtual void Compute(int i, result &x);
  virtual void Sample(Rng &seed, int i, result &x);

  virtual expr* Substitute(int i);

  virtual int GetSums(int i, List <expr> *sums=NULL);
  virtual int GetProducts(int i, List <expr> *prods=NULL);
  virtual int GetSymbols(int i, List <symbol> *syms=NULL);

  virtual void show(OutputStream &s) const { assoc_show(s, ":"); }
protected:
  virtual assoc* MakeAnother(expr **, int) {
    Internal.Start(__FILE__, __LINE__, Filename(), Linenumber());
    Internal << "call to aggregates::MakeAnother";
    Internal.Stop();
    return NULL;  // shouldn't get here
  }
};


int aggregates::NumComponents() const 
{ 
  return opnd_count;
}

expr* aggregates::GetComponent(int i) 
{ 
  CHECK_RANGE(0, i, opnd_count);
  return operands[i]; 
}

type aggregates::Type(int i) const 
{
  CHECK_RANGE(0, i, opnd_count);
  if (operands[i]) return operands[i]->Type(0);
  return VOID;
}

void aggregates::Compute(int i, result &x)
{
  CHECK_RANGE(0, i, opnd_count);
  if (operands[i]) operands[i]->Compute(0, x);
  else x.setNull();
}

void aggregates::Sample(Rng &seed, int i, result &x)
{
  CHECK_RANGE(0, i, opnd_count);
  if (operands[i]) operands[i]->Sample(seed, 0, x);
  else x.setNull();
}

expr* aggregates::Substitute(int i) 
{
  CHECK_RANGE(0, i, opnd_count);
  if (operands[i]) return operands[i]->Substitute(0);
  return NULL;
}

int aggregates::GetSums(int i, List <expr> *sums) 
{
  CHECK_RANGE(0, i, opnd_count);
  if (operands[i]) return operands[i]->GetSums(0, sums);
  return 0;  // null expression
}

int aggregates::GetProducts(int i, List <expr> *prods) 
{
    CHECK_RANGE(0, i, opnd_count);
    if (operands[i]) return operands[i]->GetProducts(0, prods);
    return 0;  // null expression
}

int aggregates::GetSymbols(int i, List <symbol> *syms) 
{
    CHECK_RANGE(0, i, opnd_count);
    if (operands[i]) return operands[i]->GetSymbols(0, syms);
    return 0;  // null expression
}


// ******************************************************************
// *                                                                *
// *                        boolconst  class                        *
// *                                                                *
// ******************************************************************

/** A boolean constant expression.
 */
class boolconst : public constant {
  bool value;
  public:
  boolconst(const char* fn, int line, bool v) : constant (fn, line, BOOL) {
    value = v;
  }

  virtual void Compute(int i, result &x) {
    DCASSERT(0==i);
    x.Clear();
    x.bvalue = value;
  }

  virtual void show(OutputStream &s) const {
    if (value) s << "true"; else s << "false";
  }
};

// ******************************************************************
// *                                                                *
// *                         intconst class                         *
// *                                                                *
// ******************************************************************

/** An integer constant expression.
 */
class intconst : public constant {
  int value;
  public:
  intconst(const char* fn, int line, int v) : constant (fn, line, INT) {
    value = v;
  }

  virtual void Compute(int i, result &x) {
    DCASSERT(0==i);
    x.Clear();
    x.ivalue = value;
  }

  virtual void show(OutputStream &s) const {
    s << value;
  }
};

// ******************************************************************
// *                                                                *
// *                        realconst  class                        *
// *                                                                *
// ******************************************************************

/** A real constant expression.
 */
class realconst : public constant {
  double value;
  public:
  realconst(const char* fn, int line, double v) : constant(fn, line, REAL) {
    value = v;
  }

  virtual void Compute(int i, result &x) {
    DCASSERT(0==i);
    x.Clear();
    x.rvalue = value;
  }

  virtual void show(OutputStream &s) const {
    s << value;
  }
};

// ******************************************************************
// *                                                                *
// *                       stringconst  class                       *
// *                                                                *
// ******************************************************************

/** A string constant expression.
 */
class stringconst : public constant {
  char *value;
  public:
  stringconst(const char* fn, int line, char *v) : constant(fn, line, STRING) {
    value = v;
  }

  virtual ~stringconst() {
    delete[] value;
  }

  virtual void Compute(int i, result &x) {
    DCASSERT(0==i);
    x.Clear();
    x.other = value;
    x.notFreeable();
  }

  virtual void show(OutputStream &s) const {
    if (value) s << '"' << value << '"';
    else s << "null string";
  }
};

// ******************************************************************
// *                                                                *
// *             Global functions  to build expressions             *
// *                                                                *
// ******************************************************************

expr* MakeConstExpr(bool c, const char* file, int line) 
{
  return new boolconst(file, line, c);
}

expr* MakeConstExpr(int c, const char* file, int line) 
{
  return new intconst(file, line, c);
}

expr* MakeConstExpr(double c, const char* file, int line) 
{
  return new realconst(file, line, c);
}

expr* MakeConstExpr(char *c, const char* file, int line) 
{
  return new stringconst(file, line, c);
}

assoc* MakeAggregate(expr **list, int size, const char* file, int line)
{
  return new aggregates(file, line, list, size);
}

//@}

