
// $Id$

#include "operators.h"

#include "sets.h"

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
#include "ops_bool.h"		// Operators for bools
#include "ops_int.h"		// Operators for ints
#include "ops_real.h"		// Operators for reals
#include "ops_misc.h"		// Operators for misc. types
#include "strings.h"		// operators for strings


const char* GetOp(int op)
{
  switch (op) {
    case AND:		return "&";
    case OR:		return "|";
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
    case COMMA:		return ",";
    case SEMI:		return ";";
    case COLON:		return ":";
    case DOT:		return ".";
    case DOTDOT:	return "..";
    case GETS:		return ":=";
    case ARROW:		return "->";
    
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
// **            Global functions  to build expressions            **
// **                                                              **
// **                                                              **
// **                                                              **
// ******************************************************************
// ******************************************************************

expr* IllegalUnaryError(int op, type t, const char *fn, int ln)
{
  // For the unary case, fine to have compile checking here.
  Error.Start(fn, ln);
  Error << "Illegal unary operation: ";
  Error << GetOp(op) << " " << GetType(t);
  Error.Stop();
  return ERROR;
}


expr* MakeUnaryOp(int op, expr *opnd, const char* file, int line)
{
  if (NULL==opnd) return NULL;
  if (ERROR==opnd) return ERROR;
  DCASSERT(opnd!=DEFLT);

  type optype = opnd->Type(0);
  switch (op) {
    case NOT:
	if (GetBase(optype)==BOOL) 
		return new bool_not(file, line, opnd);

        return IllegalUnaryError(op, optype, file, line);

    case MINUS:
	// Below works for everything but phase.
	if (GetModifier(optype)==PHASE) 
		return IllegalUnaryError(op, optype, file, line);

	if (GetBase(optype)==INT)
		return new int_neg(file, line, opnd);

	if (GetBase(optype)==REAL)
		return new real_neg(file, line, opnd);

  }
  return IllegalUnaryError(op, optype, file, line);
}


expr* IllegalBinaryError(type t1, int op, type t2, const char *fn, int ln)
{
  // error should have been caught already
  Internal.Start(__FILE__, __LINE__, fn, ln);
  Internal << "Illegal binary operation: ";
  Internal << GetType(t1) << " " << GetOp(op) << " " << GetType(t2);
  Internal.Stop();
  return ERROR;
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

  if (ltype == STRING) {
    DCASSERT(rtype == STRING);
    return MakeStringBinary(left, op, right, file, line);
  }

  // New and simpler, switch on operator first
  switch (op) {
    //---------------------------------------------------------------
    case OR:
	DCASSERT(ltype == rtype);  // must be true for OR
	if (GetBase(ltype)==BOOL) 
		return new bool_or(file, line, left, right);

        return IllegalBinaryError(ltype, op, rtype, file, line);

    //---------------------------------------------------------------
    case AND:
	DCASSERT(ltype == rtype);  // must be true for AND
	if (GetBase(ltype)==BOOL) 
		return new bool_and(file, line, left, right);

        return IllegalBinaryError(ltype, op, rtype, file, line);

    //---------------------------------------------------------------
    case PLUS:
	DCASSERT(ltype == rtype);
	if (GetModifier(ltype) == PHASE) {
 	  // PHASE + PHASE here, for both int and real
	  return NULL;
        }
	switch (GetBase(ltype)) {
	  case BOOL:
		return new bool_or(file, line, left, right);

	  case INT:
		return new int_add(file, line, left, right);

	  case REAL:
		return new real_add(file, line, left, right);
	}
        return IllegalBinaryError(ltype, op, rtype, file, line);

    //---------------------------------------------------------------
    case MINUS:
	DCASSERT(ltype == rtype);
	if (GetModifier(ltype) == PHASE)
        	return IllegalBinaryError(ltype, op, rtype, file, line);

	switch (GetBase(ltype)) {
	  case INT:
		return new int_sub(file, line, left, right);

	  case REAL:
		return new real_sub(file, line, left, right);
	}
        return IllegalBinaryError(ltype, op, rtype, file, line);

    //---------------------------------------------------------------
    case TIMES:
	if (GetModifier(ltype) == PHASE) {
 	  // PHASE * DETERM here, for both int and real
	  return NULL;
        }
	if (GetModifier(rtype) == PHASE) {
 	  // DETERM * PHASE here, for both int and real
	  return NULL;
        }
	DCASSERT(ltype == rtype);
	switch (GetBase(ltype)) {
	  case BOOL:
		return new bool_and(file, line, left, right);

	  case INT:
		return new int_mult(file, line, left, right);

	  case REAL:
		return new real_mult(file, line, left, right);
	}
        return IllegalBinaryError(ltype, op, rtype, file, line);

    //---------------------------------------------------------------
    case DIVIDE:
	DCASSERT(ltype == rtype);
	if (GetModifier(ltype) == PHASE)
        	return IllegalBinaryError(ltype, op, rtype, file, line);

	switch (GetBase(ltype)) {
	  case INT:
		return new int_div(file, line, left, right);

	  case REAL:
		return new real_div(file, line, left, right);
	}
        return IllegalBinaryError(ltype, op, rtype, file, line);

    //---------------------------------------------------------------
    case EQUALS:
	DCASSERT(ltype == rtype);
	if (GetModifier(ltype) == PHASE) 
        	return IllegalBinaryError(ltype, op, rtype, file, line);

	switch (GetBase(ltype)) {
	  case BOOL:
		return new bool_equal(file, line, left, right);

	  case INT:
		return new int_equal(file, line, left, right);

	  case REAL:
		return new real_equal(file, line, left, right);
	}
        return IllegalBinaryError(ltype, op, rtype, file, line);

    //---------------------------------------------------------------
    case NEQUAL:
	DCASSERT(ltype == rtype);
	if (GetModifier(ltype) == PHASE) 
        	return IllegalBinaryError(ltype, op, rtype, file, line);

	switch (GetBase(ltype)) {
	  case BOOL:
		return new bool_neq(file, line, left, right);

	  case INT:
		return new int_neq(file, line, left, right);

	  case REAL:
		return new real_neq(file, line, left, right);
	}
        return IllegalBinaryError(ltype, op, rtype, file, line);

    //---------------------------------------------------------------
    case GT:
	DCASSERT(ltype == rtype);
	if (GetModifier(ltype) == PHASE) 
        	return IllegalBinaryError(ltype, op, rtype, file, line);

	switch (GetBase(ltype)) {
	  case INT:
		return new int_gt(file, line, left, right);

	  case REAL:
		return new real_gt(file, line, left, right);
	}
        return IllegalBinaryError(ltype, op, rtype, file, line);

    //---------------------------------------------------------------
    case GE:
	DCASSERT(ltype == rtype);
	if (GetModifier(ltype) == PHASE) 
        	return IllegalBinaryError(ltype, op, rtype, file, line);

	switch (GetBase(ltype)) {
	  case INT:
		return new int_ge(file, line, left, right);

	  case REAL:
		return new real_ge(file, line, left, right);
	}
        return IllegalBinaryError(ltype, op, rtype, file, line);

    //---------------------------------------------------------------
    case LT:
	DCASSERT(ltype == rtype);
	if (GetModifier(ltype) == PHASE) 
        	return IllegalBinaryError(ltype, op, rtype, file, line);

	switch (GetBase(ltype)) {
	  case INT:
		return new int_lt(file, line, left, right);

	  case REAL:
		return new real_lt(file, line, left, right);
	}
        return IllegalBinaryError(ltype, op, rtype, file, line);

    //---------------------------------------------------------------
    case LE:
	DCASSERT(ltype == rtype);
	if (GetModifier(ltype) == PHASE) 
        	return IllegalBinaryError(ltype, op, rtype, file, line);

	switch (GetBase(ltype)) {
	  case INT:
		return new int_le(file, line, left, right);

	  case REAL:
		return new real_le(file, line, left, right);
	}
        return IllegalBinaryError(ltype, op, rtype, file, line);

  }
  return IllegalBinaryError(ltype, op, rtype, file, line);
}

expr* IllegalAssocError(int op, type alltype, const char *fn, int ln)
{
  // shouldn't ever get here
  Internal.Start(__FILE__, __LINE__, fn, ln);
  Internal << "Illegal associative operator " << GetOp(op);
  Internal << " for type " << GetType(alltype);
  Internal.Stop();
  return ERROR;
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

  switch (op) {
    //---------------------------------------------------------------
    case SEMI:
	switch (alltypes) {
	  case VOID:
		return new void_seq(file, line, opnds, n);

	  case PROC_STATE:
		return new proc_state_seq(file, line, opnds, n);
 	}
 	return IllegalAssocError(op, alltypes, file, line);

    //---------------------------------------------------------------
    case OR:
	if (GetBase(alltypes) == BOOL)
		return new bool_or(file, line, opnds, n);

        return IllegalAssocError(op, alltypes, file, line);

    //---------------------------------------------------------------
    case AND:
	if (GetBase(alltypes) == BOOL)
		return new bool_and(file, line, opnds, n);

        return IllegalAssocError(op, alltypes, file, line);

    //---------------------------------------------------------------
    case PLUS:
	if (GetModifier(alltypes) == PHASE) {
 	  // sum of PHASE here, for both int and real
	  return NULL;
        }
	switch (GetBase(alltypes)) {
	  case BOOL:
		return new bool_or(file, line, opnds, n);

	  case INT:
		return new int_add(file, line, opnds, n);

	  case REAL:
		return new real_add(file, line, opnds, n);

	  case STRING:
		return MakeStringAdd(opnds, n, file, line);
	}
        return IllegalAssocError(op, alltypes, file, line);

    //---------------------------------------------------------------
    case TIMES:
	if (GetModifier(alltypes) == PHASE) 
        	return IllegalAssocError(op, alltypes, file, line);

	switch (GetBase(alltypes)) {
	  case BOOL:
		return new bool_and(file, line, opnds, n);

	  case INT:
		return new int_mult(file, line, opnds, n);

	  case REAL:
		return new real_mult(file, line, opnds, n);
	}
        return IllegalAssocError(op, alltypes, file, line);

  }

  return IllegalAssocError(op, alltypes, file, line);
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

  // Handle sets differently
  if (IsASet(e->Type(a))) {
    e = OptimizeUnion(e);
    return;
  }

  int i;
  // First... try to split us into sums
  optbuffer.Clear();
  int sumcount = e->GetSums(a, &optbuffer);
  if (sumcount>1) {
    // There is a sum below e
    expr **opnds = optbuffer.Copy();
    for (i=0; i<sumcount; i++)  Optimize(a, opnds[i]); 
    // replace with associative sum
    expr *ne = MakeAssocOp(PLUS, opnds, sumcount, 
    				e->Filename(), e->Linenumber());
    Delete(e);
    e = ne;
    return;  // done
  } else {
    // the list holds a copy to the expression itself
    DCASSERT(sumcount);
    Delete(optbuffer.Item(0));
  }
  // Still here?  try to split into products
  optbuffer.Clear();
  int prodcount = e->GetProducts(a, &optbuffer);
  if (prodcount>1) {
    // There is a product below us
    expr **opnds = optbuffer.Copy();
    for (i=0; i<prodcount; i++)  Optimize(a, opnds[i]); 
    // replace with associative product
    expr *ne = MakeAssocOp(TIMES, opnds, prodcount, 
    				e->Filename(), e->Linenumber());
    Delete(e);
    e = ne;
  } else {
    // the list holds a copy to the expression itself
    DCASSERT(prodcount);
    Delete(optbuffer.Item(0));
  }
}


//@}

