
// $Id$

#include "arrays.h"

#include <strstream>

//@Include: arrays.h

/** @name arrays.cc
    @type File
    @args \ 

   Implementation of user-defined arrays.

 */

//@{

//#define ARRAY_TRACE

//#define LONG_INTERNAL_ARRAY_NAME

// ******************************************************************
// *                                                                *
// *                      array_index  methods                      *
// *                                                                *
// ******************************************************************

array_index::array_index(const char *fn, int line, type t, char *n, expr *v) 
  : symbol(fn, line, t, n)
{
  values = v;
  current = NULL;
}

array_index::~array_index()
{
  Delete(values);
  Delete(current);
}

void array_index::Compute(int i, result &x)
{
  DCASSERT(i==0);
  x.Clear();
  if (NULL==current) {
    x.null = true;
    // set error condition?
    return;
  }
  current->GetElement(index, x);
}

void array_index::showfancy(ostream &s) const
{
  s << Name() << " in {" << values << "}";
}


// ******************************************************************
// *                                                                *
// *                         array  methods                         *
// *                                                                *
// ******************************************************************

array::array(const char* fn, int line, type t, char* n, array_index **il, int dim)
  : symbol(fn, line, t, n)
{
  index_list = il;
  dimension = dim;
  descriptor = NULL;
}
 
array::~array()
{
  // Does this *ever* get called?
  if (index_list) {
    int i;
    for (i=0; i<dimension; i++) Delete(index_list[i]);
    delete[] index_list;
  }
  // delete descriptor here...
}

void array::SetCurrentReturn(constfunc *retvalue)
{
  int i;
  array_desc *prev = NULL;
  array_desc *curr = descriptor;
  int lastindex = 0;
  for (i=0; i<dimension; i++) {
    if (NULL==curr) {
      curr = new array_desc(index_list[i]->CopyCurrent());
      if (prev) prev->down[lastindex] = curr;
      else descriptor = curr;
    }
    lastindex = index_list[i]->Index();
    prev = curr;
    curr = (array_desc*) curr->down[lastindex];
  }
  if (prev->down[lastindex]) {
    // we already have a value...
    cerr << "Internal error: array reassignment?\n";
    exit(0);
  }
  prev->down[lastindex] = retvalue;
}

void array::GetName(ostream &s) const
{
  s << Name() << "[";
  int i;
  for (i=0; i<dimension; i++) {
    if (i) s << ", ";
    result ind;
    index_list[i]->Compute(0, ind);
    PrintResult(index_list[i]->Type(0), ind, s);
  }
  s << "]";
}

void array::Compute(expr **il, result &x)
{
  x.Clear();
  int i;
  array_desc *ptr = descriptor;
  for (i=0; i<dimension; i++) {
    result y;
    il[i]->Compute(0, y);
    if (y.null) {
      x.null = true;
      return;
    }
    if (y.error) {
      // Trace the error here?
      x.error = y.error;
      return;
    }
    if (NULL==ptr) {
      // error?  
      x.null = true;
      return;
    }
    int ndx = ptr->values->IndexOf(y);
    if (ndx<0) {
      // range error
      x.error = CE_OutOfRange;
      // print something?
      return;
    }
    ptr = (array_desc*) ptr->down[ndx];
  }
  x.other = ptr;
}

void array::Sample(long &seed, expr **il, result &x)
{
  x.Clear();
  int i;
  array_desc *ptr = descriptor;
  for (i=0; i<dimension; i++) {
    result y;
    il[i]->Sample(seed, 0, y);
    if (y.null) {
      x.null = true;
      return;
    }
    if (y.error) {
      // Trace the error here?
      x.error = y.error;
      return;
    }
    if (NULL==ptr) {
      // error?  
      x.null = true;
      return;
    }
    int ndx = ptr->values->IndexOf(y);
    if (ndx<0) {
      // range error
      x.error = CE_OutOfRange;
      // print something?
      return;
    }
    ptr = (array_desc*) ptr->down[ndx];
  }
  x.other = ptr;
}

void array::show(ostream &s) const
{
  DCASSERT(Name());
  s << GetType(Type(0)) << " " << Name() << "[" << index_list[0];
  int i;
  for (i=1; i<dimension; i++) {
    s << ", " << index_list[i];
  }
  s << "]";
}


// ******************************************************************
// *                                                                *
// *                          acall  class                          *
// *                                                                *
// ******************************************************************

/**  An expression used to obtain an array element.
 */

class acall : public expr {
protected:
  array *func;
  expr **pass;
  int numpass;
public:
  acall(const char *fn, int line, array *f, expr **p, int np);
  virtual ~acall();
  virtual type Type(int i) const;
  virtual void Compute(int i, result &x);
  virtual void Sample(long &, int i, result &x);
  virtual expr* Substitute(int i);
  virtual int GetSymbols(int i, symbol **syms=NULL, int N=0, int offset=0);
  virtual void show(ostream &s) const;
};

// acall methods

acall::acall(const char *fn, int line, array *f, expr **p, int np)
  : expr(fn, line)
{
  func = f;
  pass = p;
  numpass = np;
}

acall::~acall()
{
  // does this ever get called?
  // don't delete func
  int i;
  for (i=0; i<numpass; i++) Delete(pass[i]);
  delete[] pass;
}

type acall::Type(int i) const
{
  DCASSERT(0==i);
  return func->Type(0);
}

void acall::Compute(int i, result &x)
{
  DCASSERT(0==i);
  func->Compute(pass, x);
  if (x.null) return;
  if (x.error) return;  // print message?
  constfunc* foo = (constfunc*) x.other;
#ifdef ARRAY_TRACE
  cout << "Computing " << foo << "\n";
#endif
  foo->Compute(0, x);
#ifdef ARRAY_TRACE
  cout << "Now we have " << foo << "\n";
#endif
}

void acall::Sample(long &seed, int i, result &x)
{
  DCASSERT(0==i);
  func->Sample(seed, pass, x);
  if (x.null) return;
  if (x.error) return;  // print message?
  constfunc* foo = (constfunc*) x.other;
  foo->Sample(seed, 0, x);
}

expr* acall::Substitute(int i)
{
  DCASSERT(0==i);
  DCASSERT(numpass);

  // substitute each index
  expr** pass2 = new expr*[numpass];
  int n;
  for (n=0; n<numpass; n++) {
    pass2[n] = pass[n]->Substitute(0);
  }
  return new acall(Filename(), Linenumber(), func, pass2, numpass);
}

int acall::GetSymbols(int i, symbol **syms, int N, int offset)
{
  // implement this, too
  return 0;
}

void acall::show(ostream &s) const
{
  if (func->Name()==NULL) return; // hidden?
  s << func->Name();
  DCASSERT(numpass>0);
  s << "[" << pass[0];
  int i;
  for (i=1; i<numpass; i++) s << ", " << pass[i];
  s << "]";
}


// ******************************************************************
// *                                                                *
// *                         forstmt  class                         *
// *                                                                *
// ******************************************************************

/**  A statement used for for-loops.
 */

class forstmt : public statement {
  array_index *index;
  statement **block;
  int blocksize;
public:
  forstmt(const char *fn, int l, array_index *i, statement **b, int n);
  virtual ~forstmt(); 

  virtual void Execute();
  virtual void show(ostream &s) const;
  virtual void showfancy(int depth, ostream &s) const;
};

forstmt::forstmt(const char *fn, int l, array_index *i, statement **b, int n)
  : statement(fn, l)
{
  index = i;
  block = b;
  blocksize = n;

  // set us as parent of the block
  int j;
  for (j=0; j<blocksize; j++) {
    DCASSERT(block[j]);
    block[j]->SetParent(this);
  }
}

forstmt::~forstmt()
{
  // Is this ever called?
  int j;
  for (j=0; j<blocksize; j++) delete block[j];
  delete[] block;
}

void forstmt::Execute()
{
  index->ComputeCurrent();
  if (!index->FirstIndex()) return;  // error?

  do {
    // execute block
    int j;
    for (j=0; j<blocksize; j++) block[j]->Execute();

  } while (index->NextValue());
}

void forstmt::show(ostream &s) const
{
  s << "for (";
  index->showfancy(s);
  s << ")";
}

void forstmt::showfancy(int depth, ostream &s) const
{
  int j;
  for (j=depth; j; j--) s << " ";
  show(s);
  s << " {\n";
  for (j=0; j<blocksize; j++) {
    block[j]->showfancy(depth+2, s);
  }
  for (j=depth; j; j--) s << " ";
  s << "}\n";
}

// ******************************************************************
// *                                                                *
// *                       arrayassign  class                       *
// *                                                                *
// ******************************************************************

/**  A statement used for array assignments.
 */

class arrayassign : public statement {
  array *f;
  expr *retval;
public:
  arrayassign(const char *fn, int l, array *a, expr *e);
  virtual ~arrayassign(); 

  virtual void Execute();
  virtual void show(ostream &s) const;
  virtual void showfancy(int depth, ostream &s) const;
};

arrayassign::arrayassign(const char *fn, int l, array *a, expr *e)
  : statement(fn, l)
{
  f = a;
  retval = e;
}

arrayassign::~arrayassign()
{
  Delete(retval);
}

void arrayassign::Execute()
{
  // De-iterate the return value
  expr* rv = retval->Substitute(0);

  DCASSERT(rv); // hmmmm....  can this ever happen?
  
#ifdef LONG_INTERNAL_ARRAY_NAME
  // Build a long name.  Useful for debugging, otherwise 
  // I think it is unnecessary work.
  char buffer[1024];
  strstream s(buffer, 1024);
  f->GetName(s);
  s.put(0);
  char* name = strdup(buffer);
#else
  char* name = NULL;
#endif

  determfunc *frv = new determfunc(Filename(), Linenumber(), rv->Type(0), name);
  frv->SetReturn(rv);

  f->SetCurrentReturn(frv);

#ifdef ARRAY_TRACE
  cout << "Array assign: " << frv << "\n";
#endif
}

void arrayassign::show(ostream &s) const
{
  s << f << " := " << retval;
}

void arrayassign::showfancy(int depth, ostream &s) const
{
  int j;
  for (j=depth; j; j--) s << " ";
  show(s);
  s << ";\n";
}

// ******************************************************************
// *                                                                *
// *                                                                *
// *                          Global stuff                          *
// *                                                                *
// *                                                                *
// ******************************************************************

expr* MakeArrayCall(array *f, expr **p, int np, const char *fn=NULL, int l=0)
{
  return new acall(fn, l, f, p, np);
}

statement* MakeForLoop(array_index *i, statement** block, int blocksize,
                       const char *fn, int line)
{
  return new forstmt(fn, line, i, block, blocksize);
}

statement* MakeArrayAssign(array *f, expr* retval,
    		    	   const char *fn, int line)
{
  return new arrayassign(fn, line, f, retval);
}

//@}

