
// $Id$

#include "operators.h"

//@Include: operators.h

/** @name operators.cc
    @type File
    @args \ 

   Implementation of operator classes.
   Actually, there are so many that they have been split into several files.

   Also, some other expression things are here,
   since this is essentially the "top" of the
   expression stuff.

 */

//@{

//#define DEBUG_DEEP

#include "baseops.h"		// Base operators (remove eventually)
#include "ops_const.h"		// Operators for "const" nature
#include "ops_rand.h"		// Operators for "rand" nature
#include "strings.h"		// operators for strings


const char* GetOp(int op)
{
  switch (op) {
    case NOT: 		return "!";
    case PLUS: 		return "+";
    case MINUS:		return "-";
    case TIMES:		return "*";
    case DIVIDE:	return "/";
    case EQUALS:	return "==";
    case NEQUAL:	return "!=";
    case GT:		return ">";
    case GE:		return ">=";
    case LT:		return "<";
    case LE:		return "<=";
    default:
			return "unknown_op";
  }
  return "error";  // keep compilers happy
}






// ******************************************************************
// ******************************************************************
// **                                                              **
// **                                                              **
// **                                                              **
// **                     Operators for  procs                     **
// **                                                              **
// **                                                              **
// **                                                              **
// ******************************************************************
// ******************************************************************

// ******************************************************************
// *                                                                *
// *                        proc_unary class                        *
// *                                                                *
// ******************************************************************

/** Unary operators for procs.
 */
class proc_unary : public unary {
protected:
  type returntype;
  int oper;
public:
  proc_unary(const char* fn, int line, type rt, int op, expr *x)
    : unary(fn, line, x) 
  { 
    returntype = rt;
    oper = op;
  }
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return returntype;
  }
  virtual void Compute(int i, result &x);
  virtual void show(OutputStream &s) const { unary_show(s, GetOp(oper)); }
protected:
  virtual expr* MakeAnother(expr *x) {
    return new proc_unary(Filename(), Linenumber(), returntype, oper, x);
  }
};

void proc_unary::Compute(int i, result &x)
{
  DCASSERT(0==i);
  DCASSERT(opnd);
  x.Clear();
  opnd->Compute(0, x); 

  if (x.isNormal()) {
    x.other = MakeUnaryOp(oper, (expr*)x.other, Filename(), Linenumber());
    x.setFreeable();
    return;
  }
  x.other = NULL;
}

// ******************************************************************
// *                                                                *
// *                       proc_binary  class                       *
// *                                                                *
// ******************************************************************

/** Binary operations for procs.
 */
class proc_binary : public binary {
protected:
  type returntype;
  int oper;
public:
  proc_binary(const char* fn, int line, type rt, int op, expr *l, expr *r)
    : binary(fn, line, l, r) 
  { 
    returntype = rt;
    oper = op;
  }
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return returntype;
  }
  virtual void Compute(int i, result &x);
  virtual void show(OutputStream &s) const { binary_show(s, GetOp(oper)); }
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new proc_binary(Filename(), Linenumber(), returntype, oper, l, r);
  }
};

void proc_binary::Compute(int i, result &x)
{
  DCASSERT(0==i);
  DCASSERT(left);
  DCASSERT(right);
  result r;
  x.Clear();
  left->Compute(0, x); 
  x.other = NULL;
  if (!x.isNormal()) return;
  right->Compute(0, r); 
  if (!r.isNormal()) {
    Delete((expr *)x.other);
    x.setNull();
    x.special = r.special;
    return;
  }

  x.other = MakeBinaryOp((expr*)x.other, oper, (expr*)r.other,
	                 Filename(), Linenumber());
  x.setFreeable();
}


// ******************************************************************
// *                                                                *
// *                        proc_assoc class                        *
// *                                                                *
// ******************************************************************

/** Associative operations for procs.
 */
class proc_assoc : public assoc {
protected:
  type returntype;
  int oper;
public:
  proc_assoc(const char* fn, int line, type rt, int op, expr **x, int n)
    : assoc(fn, line, x, n) 
  { 
    returntype = rt;
    oper = op;
  }

  proc_assoc(const char* fn, int line, type rt, int op, expr *l, expr *r)
    : assoc(fn, line, l, r) 
  { 
    returntype = rt;
    oper = op;
  }
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return returntype;
  }
  virtual void Compute(int i, result &x);
  virtual void show(OutputStream &s) const { assoc_show(s, GetOp(oper)); }
protected:
  virtual expr* MakeAnother(expr **x, int n) {
    return new proc_assoc(Filename(), Linenumber(), returntype, oper, x, n);
  }
};

void proc_assoc::Compute(int a, result &x)
{
  DCASSERT(0==a);
  expr** r = new expr* [opnd_count];
  int i;
  for (i=0; i<opnd_count; i++) {
    DCASSERT(operands[i]);
    operands[i]->Compute(0, x);
    if (!x.isNormal()) {
      // bail out
      int j;
      for (j=0; j<i; j++) Delete(r[j]);
      delete[] r;
      return;
    }
    r[i] = (expr *) x.other;
  }
  // all opnds computed successfully, build expression
  x.other = MakeAssocOp(oper, r, opnd_count, Filename(), Linenumber());
  x.setFreeable();
}


// ******************************************************************
// ******************************************************************
// **                                                              **
// **                                                              **
// **                                                              **
// **            Global functions  to build expressions            **
// **                                                              **
// **                                                              **
// **                                                              **
// ******************************************************************
// ******************************************************************


expr* MakeUnaryOp(int op, expr *opnd, const char* file, int line)
{
  if (NULL==opnd) return NULL;
  if (ERROR==opnd) return ERROR;
  DCASSERT(opnd!=DEFLT);

  type optype = opnd->Type(0);
  switch (optype) {
    case BOOL:
      if (op==NOT) return new bool_not(file, line, opnd);
      return NULL;

    case INT:
      if (op==MINUS) return new int_neg(file, line, opnd);
      return NULL;

    case REAL:
      if (op==MINUS) return new real_neg(file, line, opnd);
      return NULL;

    case PROC_BOOL:
    case PROC_INT:
    case PROC_REAL:
    case PROC_PH_INT:
    case PROC_PH_REAL:
    case PROC_RAND_BOOL:
    case PROC_RAND_INT:
    case PROC_RAND_REAL:
      return new proc_unary(file, line, optype, op, opnd);
  }
  return NULL;
}



// Note: the types left and right must match properly already
expr* MakeBinaryOp(expr *left, int op, expr *right, const char* file, int line)
{
  if (NULL==left || NULL==right) {
    Delete(left);
    Delete(right);
    return NULL;
  }
  if (ERROR==left || ERROR==right) {
    Delete(left);
    Delete(right);
    return ERROR;
  }
  DCASSERT(left!=DEFLT);
  DCASSERT(right!=DEFLT);

  type ltype = left->Type(0);
  type rtype = right->Type(0);

  switch (ltype) {

    //===============================================================
    case BOOL:

      DCASSERT(rtype==BOOL);

      switch (op) {
          case PLUS:
          case OR:	return new bool_or(file, line, left, right);

	  case TIMES:
	  case AND:   	return new bool_and(file, line, left, right);

	  case EQUALS:	return new bool_equal(file, line, left, right);
	  case NEQUAL:	return new bool_neq(file, line, left, right);
      }
      return NULL;

      
    //===============================================================
    case INT:

      if (rtype==PH_INT) {
	DCASSERT(op==TIMES);
	// do ph int times int here
	return NULL;
      }

      DCASSERT(rtype==INT);

      switch (op) {
          case PLUS:	return new int_add(file, line, left, right);
          case TIMES:	return new int_mult(file, line, left, right);
          case MINUS:	return new int_sub(file, line, left, right);
          case DIVIDE:	return new int_div(file, line, left, right);
	  case EQUALS:	return new int_equal(file, line, left, right);
	  case NEQUAL:	return new int_neq(file, line, left, right);
	  case GT:	return new int_gt(file, line, left, right);
	  case GE:	return new int_ge(file, line, left, right);
	  case LT:	return new int_lt(file, line, left, right);
	  case LE:	return new int_le(file, line, left, right);
      }
      return NULL;
      

    //===============================================================
    case REAL:

      if (rtype==PH_REAL) {
	DCASSERT(op==TIMES);
	// do ph real times real here
	return NULL;
      }

      DCASSERT(rtype==REAL);

      switch (op) {
          case PLUS:	return new real_add(file, line, left, right);
          case TIMES:	return new real_mult(file, line, left, right);
          case MINUS:	return new real_sub(file, line, left, right);
          case DIVIDE:	return new real_div(file, line, left, right);
	  case EQUALS:	return new real_equal(file, line, left, right);
	  case NEQUAL:	return new real_neq(file, line, left, right);
	  case GT:	return new real_gt(file, line, left, right);
	  case GE:	return new real_ge(file, line, left, right);
	  case LT:	return new real_lt(file, line, left, right);
	  case LE:	return new real_le(file, line, left, right);
      }
      return NULL;
      

    //===============================================================
    case STRING:
      
      DCASSERT(rtype==STRING);
      // Defined in strings.cc
      return MakeStringBinary(left, op, right, file, line);
    
    //===============================================================
    case PROC_INT:
      if (rtype==PROC_PH_INT) {
	DCASSERT(op==TIMES);
	return new proc_binary(file, line, rtype, op, left, right);
      }
      return new proc_binary(file, line, ltype, op, left, right);

      
    //===============================================================
    case PROC_REAL:
      if (rtype==PROC_PH_REAL) {
	DCASSERT(op==TIMES);
	return new proc_binary(file, line, rtype, op, left, right);
      }
      return new proc_binary(file, line, ltype, op, left, right);


    //===============================================================
    case PROC_BOOL:
    case PROC_PH_INT:
    case PROC_PH_REAL:
    case PROC_RAND_INT:
    case PROC_RAND_REAL:
      return new proc_binary(file, line, ltype, op, left, right); 


  }

  return NULL;
}

expr* IllegalAssocError(int op, type alltype, const char *fn, int ln)
{
  Internal.Start(__FILE__, __LINE__, fn, ln);
  Internal << "Illegal associative operator " << GetOp(op);
  Internal << " for type " << GetType(alltype);
  Internal.Stop();
  return NULL;
}


// Note: operand types must match properly already
expr* MakeAssocOp(int op, expr **opnds, int n, const char* file, int line)
{
  int i,j;
  for (i=0; i<n; i++) {
    if (NULL==opnds[i]) {
      for (j=0; j<n; j++) Delete(opnds[j]);
      delete[] opnds;
      return NULL;
    }
    if (ERROR==opnds[i]) {
      for (j=0; j<n; j++) Delete(opnds[j]);
      delete[] opnds;
      return ERROR;
    }
    DCASSERT(opnds[i]!=DEFLT);
  }
  type alltypes = opnds[0]->Type(0);
#ifdef DEVELOPMENT_CODE
  for (i=1; i<n; i++) {
    type foo = opnds[1]->Type(0);
    DCASSERT(foo==alltypes);
  }
#endif
  switch (alltypes) {

    //===============================================================
    case BOOL:

      switch (op) {
          case PLUS:
          case OR:	return new bool_or(file, line, opnds, n);

	  case TIMES:
	  case AND:   	return new bool_and(file, line, opnds, n);

	  default:	return IllegalAssocError(op, alltypes, file, line);
      }
      return NULL;

      
    //===============================================================
    case INT:

      switch (op) {
          case PLUS:	return new int_add(file, line, opnds, n);
          case TIMES:	return new int_mult(file, line, opnds, n);

	  default:	return IllegalAssocError(op, alltypes, file, line);
      }
      return NULL;


    //===============================================================
    case REAL:

      switch (op) {
          case PLUS:	return new real_add(file, line, opnds, n);
          case TIMES:	return new real_mult(file, line, opnds, n);

	  default:	return IllegalAssocError(op, alltypes, file, line);
      }
      return NULL;


    //===============================================================
    case STRING:
      
      switch (op) {
	case PLUS:	return MakeStringAdd(opnds, n, file, line);
			// Defined in strings.cc

	default:	return IllegalAssocError(op, alltypes, file, line);
      }
      return NULL;
      
  }

  return NULL;
}

// ******************************************************************
// *                    Expression  optimization                    *
// ******************************************************************

void Optimize(int a, expr* &e)
{
  static List <expr> optbuffer(128);
  if (NULL==e) return;
  if (ERROR==e) return;
  DCASSERT(e!=DEFLT);
  int i;
  // First... try to split us into sums
  optbuffer.Clear();
  int sumcount = e->GetSums(a, &optbuffer);
  if (sumcount>1) {
    // There is a sum below e
    expr **opnds = optbuffer.Copy();
    for (i=0; i<sumcount; i++) {
      opnds[i] = Copy(opnds[i]);  // make this our copy
      Optimize(a, opnds[i]);
    }
    // replace with associative sum
    expr *ne = MakeAssocOp(PLUS, opnds, sumcount, 
    				e->Filename(), e->Linenumber());
    Delete(e);
    e = ne;
    return;  // done
  } 
  // Still here?  try to split into products
  optbuffer.Clear();
  int prodcount = e->GetProducts(a, &optbuffer);
  if (prodcount>1) {
    // There is a product below us
    expr **opnds = optbuffer.Copy();
    for (i=0; i<prodcount; i++) {
      opnds[i] =Copy(opnds[i]);
      Optimize(a, opnds[i]);
    }
    // replace with associative product
    expr *ne = MakeAssocOp(TIMES, opnds, prodcount, 
    				e->Filename(), e->Linenumber());
    Delete(e);
    e = ne;
  } 
}


//@}

