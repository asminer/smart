
// $Id$

#ifndef TYPES_H
#define TYPES_H

/** @name types.h
    @type File
    @args \ 

  A bunch of type stuff, including classes for types, functions to 
  convert to / from type names and type codes, and compiler stuff.

  Most of this is for compile-time type checking and for converting
  between character strings and the corresponding type code.

*/

#include "../defines.h"

//@{

typedef unsigned char	type;

typedef unsigned char	modifier;

// Non-model, non-proc types

/// Used to indicate illegal values
const type	NO_SUCH_TYPE	= 0;
/// Void.
const type 	VOID 		= 1;
const type	BOOL 		= 2;
const type	INT 		= 3;
const type	REAL		= 4;
const type	STRING		= 5;
const type	BIGINT		= 6;
const type	STATESET	= 7;

const type	EXPO		= 8;

const type	PH_INT		= 9;
const type	PH_REAL		= 10;

const type	RAND_BOOL	= 11;
const type	RAND_INT	= 12;
const type	RAND_REAL	= 13;

const type	FIRST_SIMPLE	= VOID;
const type	LAST_ONE_WORD	= EXPO;
const type	LAST_SIMPLE	= RAND_REAL;

// Proc types

const type	PROC_BOOL	= 16;
const type	PROC_INT 	= 17;
const type	PROC_REAL	= 18;

const type	PROC_EXPO	= 19;

const type	PROC_PH_INT	= 20;
const type	PROC_PH_REAL	= 21;

const type	PROC_RAND_BOOL	= 22;
const type	PROC_RAND_INT	= 23;
const type	PROC_RAND_REAL	= 24;

const type	PROC_STATE	= 25;

const type	FIRST_PROC	= PROC_BOOL;
const type	LAST_PROC	= PROC_STATE;

//  Model types 

const type  	ANYMODEL 	= 32;

const type	MARKOV		= 33;  // matches both DTMC and CTMC
const type 	DTMC 		= 34;
const type 	CTMC 		= 35;
const type 	PN 		= 36;

const type	FIRST_MODEL	= ANYMODEL;
const type	LAST_MODEL	= PN;

// void types (used by models)

const type 	STATE 		= 64;

const type 	PLACE 		= 65;
const type 	TRANS 		= 66;

const type 	FIRST_VOID	= STATE;
const type 	LAST_VOID	= TRANS;

// Sets.  (not user-definable yet)

const type	SET_INT		= 96;
const type	SET_REAL	= 97;
const type 	SET_STATE	= 98;
const type	SET_PLACE	= 99;
const type	SET_TRANS	= 100;

const type	FIRST_SET	= SET_INT;
const type 	LAST_SET	= SET_TRANS;

// Internal types

const type    ENGINEINFO 	= 120;
const type      EXPR_PTR 	= 121;

const type 	FIRST_INTERNAL 	= ENGINEINFO;
const type 	LAST_INTERNAL 	= EXPR_PTR;


// Used by lexer

const modifier  NO_SUCH_MODIF	= 255;

const modifier 	DETERM = 0;
const modifier 	PHASE = 1;
const modifier	RAND  = 2;
const modifier	FIRST_MODIF = PHASE;
const modifier	LAST_MODIF = RAND;

// Handy functions.

/// Given any type, return the full type name.
const char* GetType(type);

/** Return the type of a set whose elements have type t.
    If sets of these are not allowed, returns NO_SUCH_TYPE.
*/
type SetOf(type t);

/// Returns true if the type is a set of something.
inline bool IsASet(type t) 
{
  return (t >= FIRST_SET) && (t <= LAST_SET);
}

/// Returns true if type t1 can be promoted to type t2.
bool Promotable(type t1, type t2);            

/// Returns true if type t1 can be cast to type t2.
bool Castable(type t1, type t2);              

/// Returns true if the type is a model
inline bool IsModelType(type t)
{
  return (t >= FIRST_MODEL) && (t <= LAST_MODEL);
}

/// Returns true if the given type matches the template
inline bool MatchesTemplate(type templ, type given)
{
  switch (templ) {
    case ANYMODEL:
    	return IsModelType(given);

    case MARKOV:
    	return (given==DTMC) || (given==CTMC);

    default:
        return (templ == given);
  }
}


/** Returns true if the type is a void type.
    This implies that assignments are illegal.
 */
inline bool IsVoidType(type t)
{
  if (VOID==t) return true;
  return (t >= FIRST_VOID) && (t < LAST_VOID);
}

// The following are used by the lexer.

type FindSimpleType(const char *t);     // returns T for a type name, or -1
type FindModel(const char *);	       
type FindVoidType(const char *);

inline type FindType(const char *t)
{
  type n;
  n = FindSimpleType(t);
  if (n!=NO_SUCH_TYPE) return n;
  n = FindModel(t);
  if (n!=NO_SUCH_TYPE) return n;
  n = FindVoidType(t);
  return n;
}

/// Returns M for a modifier name, or -1
modifier FindModif(const char *m);          

/// Returns the type defined by "modif type", or NO_SUCH_TYPE if it is illegal.
inline type ModifyType(modifier modif, type t)
{
  if (PHASE==modif) 
    switch (t) {
      case INT	: return PH_INT;
      case REAL	: return PH_REAL;
      default	: return NO_SUCH_TYPE;
    };
  if (RAND==modif)
    switch (t) {
      case BOOL : return RAND_BOOL;
      case INT	: return RAND_INT;
      case REAL	: return RAND_REAL;
      default	: return NO_SUCH_TYPE;
    };
  return NO_SUCH_TYPE; 
}

inline bool HasProc(type t)
{
  return (t>=FIRST_PROC) && (t<=LAST_PROC);
}

inline modifier GetModifier(type t)
{
  switch (t) {
    case VOID :
    case BOOL :
    case INT  :
    case REAL :
    case STRING :
    case BIGINT :
    case STATESET :
    			return DETERM;

    case EXPO :
    case PH_INT :
    case PH_REAL :
    			return PHASE;

    case RAND_BOOL :
    case RAND_INT :
    case RAND_REAL : 
    			return RAND;
  }
  return NO_SUCH_MODIF;
}

/// Returns the type defined by "proc type", or NO_SUCH_TYPE if it is illegal.
inline type ProcifyType(type t)
{
  switch (t) {
    case BOOL		: return	PROC_BOOL	;
    case INT		: return	PROC_INT 	;
    case REAL		: return	PROC_REAL	;
    case EXPO		: return	PROC_EXPO	;
    case PH_INT		: return	PROC_PH_INT	;
    case PH_REAL	: return	PROC_PH_REAL	;
    case RAND_BOOL	: return	PROC_RAND_BOOL	;
    case RAND_INT	: return	PROC_RAND_INT	;
    case RAND_REAL	: return	PROC_RAND_REAL	;
    default 	: return NO_SUCH_TYPE;
  };
  // should never get here, but the compiler may complain.
  return NO_SUCH_TYPE;
}




//@}

#endif

