
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
#include "ops_proc.h"		// Operators for "rand" nature
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
// **            Global functions  to build expressions            **
// **                                                              **
// **                                                              **
// **                                                              **
// ******************************************************************
// ******************************************************************

expr* IllegalUnaryError(int op, type t, const char *fn, int ln)
{
  // error should have been caught already
  Internal.Start(__FILE__, __LINE__, fn, ln);
  Internal << "Illegal unary operator: ";
  Internal << GetOp(op) << " " << GetType(t);
  Internal.Stop();
  return ERROR;
}


expr* MakeUnaryOp(int op, expr *opnd, const char* file, int line)
{
  if (NULL==opnd) return NULL;
  if (ERROR==opnd) return ERROR;
  DCASSERT(opnd!=DEFLT);

  type optype = opnd->Type(0);
  switch (optype) {
    case BOOL:
      if (op==NOT) return new bool_not(file, line, opnd);
      return IllegalUnaryError(op, optype, file, line);

    case INT:
      if (op==MINUS) return new int_neg(file, line, opnd);
      return IllegalUnaryError(op, optype, file, line);

    case REAL:
      if (op==MINUS) return new real_neg(file, line, opnd);
      return IllegalUnaryError(op, optype, file, line);

    case RAND_BOOL:
      if (op==NOT) return new rand_bool_not(file, line, opnd);
      return IllegalUnaryError(op, optype, file, line);

    case RAND_INT:
      if (op==MINUS) return new rand_int_neg(file, line, opnd);
      return IllegalUnaryError(op, optype, file, line);

    case RAND_REAL:
      if (op==MINUS) return new rand_real_neg(file, line, opnd);
      return IllegalUnaryError(op, optype, file, line);

    case PROC_BOOL:
      if (op==NOT) return new proc_bool_not(file, line, opnd);
      return IllegalUnaryError(op, optype, file, line);

    case PROC_INT:
      if (op==MINUS) return new proc_int_neg(file, line, opnd);
      return IllegalUnaryError(op, optype, file, line);

    case PROC_REAL:
      if (op==MINUS) return new proc_real_neg(file, line, opnd);
      return IllegalUnaryError(op, optype, file, line);

  }
  return IllegalUnaryError(op, optype, file, line);
}


expr* IllegalBinaryError(type t1, int op, type t2, const char *fn, int ln)
{
  // error should have been caught already
  Internal.Start(__FILE__, __LINE__, fn, ln);
  Internal << "Illegal binary operator: ";
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
      return IllegalBinaryError(ltype, op, rtype, file, line);

      
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
      return IllegalBinaryError(ltype, op, rtype, file, line);
      

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
      return IllegalBinaryError(ltype, op, rtype, file, line);
      

    //===============================================================
    case RAND_BOOL:

      DCASSERT(rtype==RAND_BOOL);

      switch (op) {
          case PLUS:
          case OR:	return new rand_bool_or(file, line, left, right);

	  case TIMES:
	  case AND:   	return new rand_bool_and(file, line, left, right);

	  case EQUALS:	return new rand_bool_equal(file, line, left, right);
	  case NEQUAL:	return new rand_bool_neq(file, line, left, right);
      }
      return IllegalBinaryError(ltype, op, rtype, file, line);

      
    //===============================================================
    case RAND_INT:

      DCASSERT(rtype==RAND_INT);

      switch (op) {
          case PLUS:	return new rand_int_add(file, line, left, right);
          case TIMES:	return new rand_int_mult(file, line, left, right);
          case MINUS:	return new rand_int_sub(file, line, left, right);
          case DIVIDE:	return new rand_int_div(file, line, left, right);
	  case EQUALS:	return new rand_int_equal(file, line, left, right);
	  case NEQUAL:	return new rand_int_neq(file, line, left, right);
	  case GT:	return new rand_int_gt(file, line, left, right);
	  case GE:	return new rand_int_ge(file, line, left, right);
	  case LT:	return new rand_int_lt(file, line, left, right);
	  case LE:	return new rand_int_le(file, line, left, right);
      }
      return IllegalBinaryError(ltype, op, rtype, file, line);
      

    //===============================================================
    case RAND_REAL:

      DCASSERT(rtype==RAND_REAL);

      switch (op) {
          case PLUS:	return new rand_real_add(file, line, left, right);
          case TIMES:	return new rand_real_mult(file, line, left, right);
          case MINUS:	return new rand_real_sub(file, line, left, right);
          case DIVIDE:	return new rand_real_div(file, line, left, right);
	  case EQUALS:	return new rand_real_equal(file, line, left, right);
	  case NEQUAL:	return new rand_real_neq(file, line, left, right);
	  case GT:	return new rand_real_gt(file, line, left, right);
	  case GE:	return new rand_real_ge(file, line, left, right);
	  case LT:	return new rand_real_lt(file, line, left, right);
	  case LE:	return new rand_real_le(file, line, left, right);
      }
      return IllegalBinaryError(ltype, op, rtype, file, line);
      

    //===============================================================
    case PROC_BOOL:

      DCASSERT(rtype==PROC_BOOL);

      switch (op) {
          case PLUS:
          case OR:	return new proc_bool_or(file, line, left, right);

	  case TIMES:
	  case AND:   	return new proc_bool_and(file, line, left, right);

	  case EQUALS:	return new proc_bool_equal(file, line, left, right);
	  case NEQUAL:	return new proc_bool_neq(file, line, left, right);
      }
      return IllegalBinaryError(ltype, op, rtype, file, line);

      
    //===============================================================
    case PROC_INT:

      DCASSERT(rtype==PROC_INT);

      switch (op) {
          case PLUS:	return new proc_int_add(file, line, left, right);
          case TIMES:	return new proc_int_mult(file, line, left, right);
          case MINUS:	return new proc_int_sub(file, line, left, right);
          case DIVIDE:	return new proc_int_div(file, line, left, right);
	  case EQUALS:	return new proc_int_equal(file, line, left, right);
	  case NEQUAL:	return new proc_int_neq(file, line, left, right);
	  case GT:	return new proc_int_gt(file, line, left, right);
	  case GE:	return new proc_int_ge(file, line, left, right);
	  case LT:	return new proc_int_lt(file, line, left, right);
	  case LE:	return new proc_int_le(file, line, left, right);
      }
      return IllegalBinaryError(ltype, op, rtype, file, line);
      

    //===============================================================
    case PROC_REAL:

      DCASSERT(rtype==PROC_REAL);

      switch (op) {
          case PLUS:	return new proc_real_add(file, line, left, right);
          case TIMES:	return new proc_real_mult(file, line, left, right);
          case MINUS:	return new proc_real_sub(file, line, left, right);
          case DIVIDE:	return new proc_real_div(file, line, left, right);
	  case EQUALS:	return new proc_real_equal(file, line, left, right);
	  case NEQUAL:	return new proc_real_neq(file, line, left, right);
	  case GT:	return new proc_real_gt(file, line, left, right);
	  case GE:	return new proc_real_ge(file, line, left, right);
	  case LT:	return new proc_real_lt(file, line, left, right);
	  case LE:	return new proc_real_le(file, line, left, right);
      }
      return IllegalBinaryError(ltype, op, rtype, file, line);
      

    //===============================================================
    case STRING:
      
      DCASSERT(rtype==STRING);
      // Defined in strings.cc
      return MakeStringBinary(left, op, right, file, line);
    
  }

  return IllegalBinaryError(ltype, op, rtype, file, line);
}

expr* IllegalAssocError(int op, type alltype, const char *fn, int ln)
{
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
  switch (alltypes) {
    //===============================================================
    case VOID:

      switch (op) {
	case SEMI:	return new void_seq(file, line, opnds, n);

 	default:	return IllegalAssocError(op, alltypes, file, line);
      }
      return NULL;

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
    case RAND_BOOL:

      switch (op) {
          case PLUS:
          case OR:	return new rand_bool_or(file, line, opnds, n);

	  case TIMES:
	  case AND:   	return new rand_bool_and(file, line, opnds, n);

	  default:	return IllegalAssocError(op, alltypes, file, line);
      }
      return NULL;

      
    //===============================================================
    case RAND_INT:

      switch (op) {
          case PLUS:	return new rand_int_add(file, line, opnds, n);
          case TIMES:	return new rand_int_mult(file, line, opnds, n);

	  default:	return IllegalAssocError(op, alltypes, file, line);
      }
      return NULL;


    //===============================================================
    case RAND_REAL:

      switch (op) {
          case PLUS:	return new rand_real_add(file, line, opnds, n);
          case TIMES:	return new rand_real_mult(file, line, opnds, n);

	  default:	return IllegalAssocError(op, alltypes, file, line);
      }
      return NULL;


    //===============================================================
    case PROC_BOOL:

      switch (op) {
          case PLUS:
          case OR:	return new proc_bool_or(file, line, opnds, n);

	  case TIMES:
	  case AND:   	return new proc_bool_and(file, line, opnds, n);

	  default:	return IllegalAssocError(op, alltypes, file, line);
      }
      return NULL;

      
    //===============================================================
    case PROC_INT:

      switch (op) {
          case PLUS:	return new proc_int_add(file, line, opnds, n);
          case TIMES:	return new proc_int_mult(file, line, opnds, n);

	  default:	return IllegalAssocError(op, alltypes, file, line);
      }
      return NULL;


    //===============================================================
    case PROC_REAL:

      switch (op) {
          case PLUS:	return new proc_real_add(file, line, opnds, n);
          case TIMES:	return new proc_real_mult(file, line, opnds, n);

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

