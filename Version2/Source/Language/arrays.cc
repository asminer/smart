
// $Id$

#include "arrays.h"

#include <sstream>

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
    x.setNull();
    // set error condition?
    return;
  }
  current->GetElement(index, x);
}

void array_index::showfancy(OutputStream &s) const
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
    Internal.Start(__FILE__, __LINE__);
    Internal << "array reassignment?";
    Internal.Stop();
  }
  prev->down[lastindex] = retvalue;
}

void array::GetName(OutputStream &s) const
{
  s << Name() << "[";
  int i;
  for (i=0; i<dimension; i++) {
    if (i) s << ", ";
    result ind;
    index_list[i]->Compute(0, ind);
    PrintResult(s, index_list[i]->Type(0), ind);
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
    if (y.isNull()) {
      x.setNull();
      return;
    }
    if (y.error) {
      // Trace the error here?
      x.error = y.error;
      return;
    }
    if (NULL==ptr) {
      // error?  
      x.setNull();
      return;
    }
    int ndx = ptr->values->IndexOf(y);
    if (ndx<0) {
      // range error
      x.error = CE_OutOfRange;
      Error.Start(il[i]->Filename(), il[i]->Linenumber());
      Error << "Bad value: ";
      PrintResult(Error, il[i]->Type(0), y);
      Error << " for index " << index_list[i];
      Error << " in array " << Name();
      Error.Stop();
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
    if (y.isNull()) {
      x.setNull();
      return;
    }
    if (y.error) {
      // Trace the error here?
      x.error = y.error;
      return;
    }
    if (NULL==ptr) {
      // error?  
      x.setNull();
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

void array::show(OutputStream &s) const
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
  virtual void show(OutputStream &s) const;
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
  if (x.isNull()) return;
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
  if (x.isNull()) return;
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

void acall::show(OutputStream &s) const
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
     Note we allow multiple dimensions now!
 */

class forstmt : public statement {
  array_index **index;
  int dimension;
  statement **block;
  int blocksize;
public:
  forstmt(const char *fn, int l, array_index **i, int d, statement **b, int n);
  virtual ~forstmt(); 

  virtual void Execute();
  virtual void InitialGuess();
  virtual bool HasConverged();
  virtual void Affix(); 
  virtual void show(OutputStream &s) const;
  virtual void showfancy(int depth, OutputStream &s) const;
protected:
  void Execute(int d);
  void InitialGuess(int d);
  bool HasConverged(int d);
  void Affix(int d);
};

forstmt::forstmt(const char *fn, int l, array_index **i, int d, statement **b, int n)
  : statement(fn, l)
{
  index = i;
  dimension = d;
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
  for (j=0; j<dimension; j++) Delete(index[j]);
  delete[] index;
}

void forstmt::Execute()
{
  Execute(0);
}

void forstmt::InitialGuess()
{
  InitialGuess(0);
}

bool forstmt::HasConverged()
{
  return HasConverged(0);
}

void forstmt::Affix()
{
  Affix(0);
}

void forstmt::Execute(int d)
{
  if (d>=dimension) {
    // execute block
    int j;
    for (j=0; j<blocksize; j++) block[j]->Execute();
  } else {
    // Loop this dimension
    index[d]->ComputeCurrent();
    if (!index[d]->FirstIndex()) return;  // empty loop
    do {
      Execute(d+1);
    } while (index[d]->NextValue());
  }
}

void forstmt::InitialGuess(int d)
{
  if (d>=dimension) {
    // execute block
    int j;
    for (j=0; j<blocksize; j++) block[j]->InitialGuess();
  } else {
    // Loop this dimension
    index[d]->ComputeCurrent();
    if (!index[d]->FirstIndex()) return;  // empty loop
    do {
      InitialGuess(d+1);
    } while (index[d]->NextValue());
  }
}

bool forstmt::HasConverged(int d)
{
  if (d>=dimension) {
    // execute block
    int j;
    for (j=0; j<blocksize; j++) 
      if (!block[j]->HasConverged()) return false;
  } else {
    // Loop this dimension
    index[d]->ComputeCurrent();
    if (!index[d]->FirstIndex()) return true;  // empty loop
    do {
      if (!HasConverged(d+1)) return false;
    } while (index[d]->NextValue());
  }
  return true;
}

void forstmt::Affix(int d)
{
  if (d>=dimension) {
    // execute block
    int j;
    for (j=0; j<blocksize; j++) block[j]->Affix();
  } else {
    // Loop this dimension
    index[d]->ComputeCurrent();
    if (!index[d]->FirstIndex()) return;  // empty loop
    do {
      Affix(d+1);
    } while (index[d]->NextValue());
  }
}


void forstmt::show(OutputStream &s) const
{
  s << "for (";
  index[0]->showfancy(s);
  int d;
  for (d=1; d<dimension; d++) {
    s << ", ";
    index[d]->showfancy(s);
  }
  s << ")";
}

void forstmt::showfancy(int depth, OutputStream &s) const
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
  virtual void show(OutputStream &s) const;
  virtual void showfancy(int depth, OutputStream &s) const;
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
  expr* rv = (retval) ? (retval->Substitute(0)) : NULL;

  if (NULL==rv) {
    f->SetCurrentReturn(NULL);
#ifdef ARRAY_TRACE
    cout << "Array assign: null\n";
#endif
    return;
  }
  
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

  constfunc *frv = MakeConstant(rv->Type(0), name, Filename(), Linenumber());
  frv->SetReturn(rv);

  f->SetCurrentReturn(frv);

#ifdef ARRAY_TRACE
  cout << "Array assign: " << frv << "\n";
#endif
}

void arrayassign::show(OutputStream &s) const
{
  s << f << " := " << retval;
}

void arrayassign::showfancy(int depth, OutputStream &s) const
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

expr* MakeArrayCall(array *f, expr **p, int np, const char *fn, int l)
{
  return new acall(fn, l, f, p, np);
}

statement* MakeForLoop(array_index **i, int dim, 
		       statement** block, int blocksize,
                       const char *fn, int line)
{
  return new forstmt(fn, line, i, dim, block, blocksize);
}

statement* MakeArrayAssign(array *f, expr* retval,
    		    	   const char *fn, int line)
{
  return new arrayassign(fn, line, f, retval);
}

//@}

