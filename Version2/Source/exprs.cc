
// $Id$

#include "exprs.h"
//@Include: exprs.h

/** @name exprs.cc
    @type File
    @args \ 

   Implementation of simple expression classes.

 */

//@{

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
  cerr << "Internal error: Illegal expression compuation!\n";
  DCASSERT(0);
}

void expr::Sample(long &, int i, result &x) 
{
  cerr << "Internal error: Illegal expression sample!\n";
  DCASSERT(0);
}

int expr::GetSums(int i, expr **sums, int N, int offset) 
{
  DCASSERT(i==0);
  if (offset<N) sums[offset] = this;
  return 1;
}

int expr::GetProducts(int i, expr **prods, int N, int offset) 
{
  DCASSERT(i==0);
  if (offset<N) prods[offset] = this;
  return 1;
}

int expr::GetSymbols(int i, symbol **syms, int N, int offset)
{
  DCASSERT(i==0);
  return 0;
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

// ******************************************************************
// *                                                                *
// *                          unary  class                          *
// *                                                                *
// ******************************************************************

unary::unary(const char* fn, int line, expr *x) : expr(fn,line) 
{
  opnd = x;
}

unary::~unary()
{
  Delete(opnd);
}

int unary::GetSymbols(int i, symbol **syms, int N, int offset)
{
  DCASSERT(i==0);
  if (opnd) return opnd->GetSymbols(0, syms, N, offset);
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

// ******************************************************************
// *                                                                *
// *                          binary class                          *
// *                                                                *
// ******************************************************************

binary::binary(const char* fn, int line, expr *l, expr *r) : expr(fn,line) 
{
  left = l;
  right = r;
}

binary::~binary()
{
  Delete(left);
  Delete(right);
}

int binary::GetSymbols(int i, symbol **syms, int N, int offset)
{
  DCASSERT(i==0);
  int answer = 0;
  if (left) {
    answer = left->GetSymbols(0, syms, N, offset);
  } 
  if (right) {
    answer += right->GetSymbols(0, syms, N, offset+answer);
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

// ******************************************************************
// *                                                                *
// *                          symbol class                          *
// *                                                                *
// ******************************************************************

symbol::symbol(const char* fn, int line, type t, char* n) : expr (fn, line)
{
  mytype = t;
  name = n;
  substitute_value = true;
}

symbol::~symbol()
{
  delete[] name;
}

type symbol::Type(int i) const
{
  DCASSERT(i==0);
  return mytype;
}

int symbol::GetSymbols(int i, symbol **syms, int N, int offset) 
{
  DCASSERT(i==0);
  if (offset<N) syms[offset] = this;
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

void symbol::show(ostream &s) const
{
  if (NULL==name) return;  // Hidden symbol?
  s << name;
}

// ******************************************************************
// *                                                                *
// *                        aggregates class                        *
// *                                                                *
// ******************************************************************

/**   The class used to aggregate expressions.
 
      Note: instead of using several binary operations, like we used to,
      we group all the aggregates together in one place.
      Why?  Speed, of course.
*/  

class aggregates : public expr {
  /// The individual expressions.
  expr** items;
  /// The number of individual expressions.
  int numitems;
public:
  /// Empty constructor.
  aggregates();

  /** Constructor.
      Note: if left or right is already an aggregate, its entries
            will be copied, and it will be destroyed.
   */
  aggregates(expr* left, expr* right);

  virtual ~aggregates();

  virtual int NumComponents() const;
  virtual expr* GetComponent(int i);
  virtual type Type(int i) const;
  virtual void Compute(int i, result &x) const;
  virtual void Sample(long &seed, int i, result &x) const;

  virtual expr* Substitute(int i);

  virtual int GetSums(int i, expr **sums=NULL, int N=0, int offset=0);
  virtual int GetProducts(int i, expr **prods=NULL, int N=0, int offset=0);
  virtual int GetSymbols(int i, symbol **syms=NULL, int N=0, int offset=0);

  virtual void show(ostream &s) const;

protected:
  virtual void TakeAggregates();
};

aggregates::aggregates() : expr(NULL,0)
{
  items = NULL;
  numitems = 0;
}

aggregates::aggregates(expr* left, expr* right) : expr(NULL,0)
{
  int leftcount = 1;
  if (left) leftcount = left->NumComponents();
  int rightcount = 1;
  if (rightcount) rightcount = right->NumComponents();
  // make space for us
  numitems = leftcount+rightcount;
  items = new expr* [numitems];
  int i,j;
  // Copy left items
  if (left) {
    if (1==leftcount) {
      items[0] = left;
      i = 1;
    } else {
      // we have aggregates
      for (i=0; i<leftcount; i++) 
	items[i] = left->GetComponent(i);
      left->TakeAggregates();
    }
  } else {
    items[0] = NULL;
    i = 1;
  }
  // Copy right items
  if (right) {
    if (1==rightcount) {
      items[i] = right;
    } else {
      // we have aggregates
      for (j=0; j<leftcount; j++) 
	items[j+i] = right->GetComponent(j);
      right->TakeAggregates();
    }
  } else {
    items[i] = NULL;
  }
  // Cleanup
  delete left;
  delete right;
}

aggregates::~aggregates()
{
  int i;
  for (i=0; i<numitems; i++) Delete(items[i]);
  delete[] items;
}

int aggregates::NumComponents() const 
{ 
  return numitems; 
}

expr* aggregates::GetComponent(int i) 
{ 
  CHECK_RANGE(0, i, numitems);
  return items[i]; 
}

type aggregates::Type(int i) const 
{
  CHECK_RANGE(0, i, numitems);
  if (items[i]) return items[i]->Type(0);
  return VOID;
}

void aggregates::Compute(int i, result &x) const 
{
  CHECK_RANGE(0, i, numitems);
  if (items[i]) items[i]->Compute(0, x);
  else x.null = true;
}

void aggregates::Sample(long &seed, int i, result &x) const 
{
  CHECK_RANGE(0, i, numitems);
  if (items[i]) items[i]->Sample(seed, 0, x);
  else x.null = true;
}

expr* aggregates::Substitute(int i) 
{
  CHECK_RANGE(0, i, numitems);
  if (items[i]) return items[i]->Substitute(0);
  return NULL;
}

int aggregates::GetSums(int i, expr **sums, int N, int offset) 
{
  CHECK_RANGE(0, i, numitems);
  if (items[i]) return items[i]->GetSums(0, sums, N, offset);
  return 0;  // null expression
}

int aggregates::GetProducts(int i, expr **prods, int N, int offset) 
{
    CHECK_RANGE(0, i, numitems);
    if (items[i]) return items[i]->GetProducts(0, prods, N, offset);
    return 0;  // null expression
}

int aggregates::GetSymbols(int i, symbol **syms, int N, int offset) 
{
    CHECK_RANGE(0, i, numitems);
    if (items[i]) return items[i]->GetSymbols(0, syms, N, offset);
    return 0;  // null expression
}

void aggregates::show(ostream &s) const
{
  int i;
  s << items[0];
  for (i=1; i<numitems; i++) s << ":" << items[i];
}

void aggregates::TakeAggregates()
{
  // Our items have been copied, so set them to NULL so we don't
  // delete them in the destructor
  int i;
  for (i=0; i<numitems; i++) items[i] = NULL;
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

  virtual void Sample(long &, int i, result &x) {
    DCASSERT(0==i);
    x.Clear();
    x.bvalue = value;
  }

  virtual void show(ostream &s) const {
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

  virtual void Sample(long &, int i, result &x) {
    DCASSERT(0==i);
    x.Clear();
    x.ivalue = value;
  }

  virtual void show(ostream &s) const {
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

  virtual void Sample(long &, int i, result &x) {
    DCASSERT(0==i);
    x.Clear();
    x.rvalue = value;
  }

  virtual void show(ostream &s) const {
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
    x.other = strdup(value);
  }

  virtual void Sample(long &, int i, result &x) {
    DCASSERT(0==i);
    x.Clear();
    x.other = strdup(value);
  }

  virtual void show(ostream &s) const {
    if (value) s << '"' << value << '"';
    else s << "null string";
  }
};

// ******************************************************************
// *                                                                *
// *                  Global functions for results                  *
// *                                                                *
// ******************************************************************

void PrintResult(type t, const result &x, ostream &s)
{
  if (x.infinity) { s << "infinity"; return; }
  if (x.null) { s << "null"; return; }
  switch(t) {
    case VOID: 	DCASSERT(0); 				return;
    case BOOL: 	s << (x.bvalue?"true":"false"); 	return;
    case INT:  	s << x.ivalue;  			return;
    case REAL: 	s << x.rvalue;  			return;
  }
  DCASSERT(0);
}

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

//@}

