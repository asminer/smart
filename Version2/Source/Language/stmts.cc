
// $Id$

#include "stmts.h"

//@Include: stmts.h

/** @name stmts.cc
    @type File
    @args \ 

   Implementation basic statement classes.

 */

//@{

#define ALLOW_NON_VOID_EXPRSTMT

// ******************************************************************
// *                                                                *
// *                         exprstmt class                         *
// *                                                                *
// ******************************************************************

/** An expression statement.
    Used for function calls, like "print".
 */
class exprstmt : public statement {
  expr *x;
public:
  exprstmt(const char* fn, int line, expr *e);
  virtual ~exprstmt();
  virtual void Execute();
  virtual void show(OutputStream &s) const;
  virtual void showfancy(int depth, OutputStream &s) const;
};

exprstmt::exprstmt(const char* fn, int line, expr *e) : statement(fn,line)
{
  x = e;
}

exprstmt::~exprstmt()
{
  Delete(x);
}

void exprstmt::Execute()
{
  // this should be eliminated at compile time
  DCASSERT(x);

  result dummy;
  x->Compute(0, dummy);

  // hmmm... do we check for errors here?

#ifdef ALLOW_NON_VOID_EXPRSTMT
  if (VOID == x->Type(0)) return;

  Output << "Evaluated statement " << x << ", got: ";
  PrintResult(x->Type(0), dummy, Output);
  Output << "\n";
  Output.flush();
#endif
}

void exprstmt::show(OutputStream &s) const
{
  s << x;
}

void exprstmt::showfancy(int depth, OutputStream &s) const
{
  int i;
  for (i=depth; i; i--) s << " ";
  s << x << ";\n";
}

statement* MakeExprStatement(expr* e, const char* file, int line)
{
  if (NULL==e) return NULL;
#ifndef ALLOW_NON_VOID_EXPRSTMT
  DCASSERT(e->Type(0)==VOID);
#endif
  return new exprstmt(file, line, e);
}
  
// ******************************************************************
// *                                                                *
// *                             options                            *
// *                                                                *
// ******************************************************************

template <class DATA>
class valopt_stmt : public statement {
  option* opt;
  DATA value;
public:
  valopt_stmt(const char* fn, int line, option *o, DATA v)
   : statement(fn, line) 
  { opt = o; value = v; }
  virtual ~valopt_stmt() { }
  virtual void Execute() { opt->SetValue(value, Filename(), Linenumber()); }
  virtual void show(OutputStream &s) const {
     s << opt << " " << value;
  }
  virtual void showfancy(int depth, OutputStream &s) const {
    for (;depth; depth--) s << " "; 
    s << opt << " " << value << ";\n";
  }
};

statement* MakeOptionStatement(option* o, expr* e, const char* file, int line)
{
  DCASSERT(e);
  result x;
  x.Clear();
  e->Compute(0, x);
  // should do error checking here...
  switch (e->Type(0)) {
    case BOOL:
    	return new valopt_stmt<bool> (file, line, o, x.bvalue);

    case INT:
    	return new valopt_stmt<int> (file, line, o, x.ivalue);

    case REAL:
    	return new valopt_stmt<double> (file, line, o, x.rvalue);

    case STRING:
  	return new valopt_stmt<char*> (file, line, o, (char*) x.other);
  }
  Internal.Start(__FILE__, __LINE__, file, line);
  Internal << "Bad type? for option statement " << o << " " << e;
  Internal.Stop();
  return NULL;
}

class enumopt_stmt : public statement {
  option* opt;
  const option_const* value;
public:
  enumopt_stmt(const char* fn, int line, option* o, const option_const* v)
   : statement(fn, line) 
  { opt = o; value = v; }
  virtual ~enumopt_stmt() { }
  virtual void Execute() { opt->SetValue(value, Filename(), Linenumber()); }
  virtual void show(OutputStream &s) const {
     s << opt << " " << value->name;
  }
  virtual void showfancy(int depth, OutputStream &s) const {
    for (;depth; depth--) s << " "; 
    s << opt << " " << value->name << ";\n";
  }
};

statement* MakeOptionStatement(option *o, const option_const *v,
				const char* file, int line)
{
  DCASSERT(o);
  DCASSERT(v);
  return new enumopt_stmt(file, line, o, v);
}

//@}
