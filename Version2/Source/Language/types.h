
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

// In case we change to short or something...

typedef unsigned char	type;

typedef unsigned char	modifier;

// Non-model, non-proc types

/// Void.
const type 	VOID 		= 0;
const type	BOOL 		= 1;
const type	INT 		= 2;
const type	REAL		= 3;
const type	STRING		= 4;
const type	BIGINT		= 5;
const type	STATESET	= 6;

const type	EXPO		= 7;

const type	PH_INT		= 8;
const type	PH_REAL		= 9;

const type	RAND_BOOL	= 10;
const type	RAND_INT	= 11;
const type	RAND_REAL	= 12;

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

const type	FIRST_PROC	= PROC_BOOL;
const type	LAST_PROC	= PROC_RAND_REAL;

//  Model types 

const type  	ANYMODEL 	= 64;

const type 	DTMC 		= 65;
const type 	CTMC 		= 66;
const type 	SPN 		= 67;

const type	FIRST_MODEL	= ANYMODEL;
const type	LAST_MODEL	= SPN;

// void types (used by models)

const type 	STATE 		= 96;

const type 	PLACE 		= 97;
const type 	TRANS 		= 98;

const type 	FIRST_VOID	= STATE;
const type 	LAST_VOID	= TRANS;

// Sets.  (not user-definable yet)

const type	SET_INT		= 110;
const type	SET_REAL	= 111;

const type	FIRST_SET	= SET_INT;
const type 	LAST_SET	= SET_REAL;

// Internal types

const type    ENGINEINFO 	= 128;
const type      EXPR_PTR 	= 129;

const type 	FIRST_INTERNAL 	= ENGINEINFO;
const type 	LAST_INTERNAL 	= EXPR_PTR;

// Used by lexer

const modifier 	PHASE = 0;
const modifier	RAND  = 1;
const modifier	LAST_MODIF = RAND;

// Handy functions.

/// Given any type, return the full type name.
const char* GetType(type);

/// Returns true if type t1 can be promoted to type t2.
bool Promotable(type t1, type t2);            

/// Returns true if type t1 can be cast to type t2.
bool Castable(type t1, type t2);              

/// Returns true if the type is a model
inline bool IsModelType(type t)
{
  return (t >= FIRST_MODEL) && (t < LAST_MODEL);
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

int FindSimpleType(const char *t);     // returns T for a type name, or -1
int FindModel(const char *);	       
int FindVoidType(const char *);

inline int FindType(const char *t)
{
  int n;
  n = FindSimpleType(t);
  if (n>=0) return n;
  n = FindModel(t);
  if (n>=0) return n;
  n = FindVoidType(t);
  return n;
}

int FindModif(const char *m);          // returns M for a modifier name, or -1

/// Returns the type defined by "modif type", or 0 if it is illegal.
inline type ModifyType(modifier modif, type t)
{
  if (PHASE==modif) 
    switch (t) {
      case INT	: return PH_INT;
      case REAL	: return PH_REAL;
      default	: return 0;
    };
  if (RAND==modif)
    switch (t) {
      case BOOL : return RAND_BOOL;
      case INT	: return RAND_INT;
      case REAL	: return RAND_REAL;
      default	: return 0;
    };
  return 0; 
}

/// Returns the type defined by "proc type", or 0 if it is illegal.
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
    default 	: return 0;
  };
  // should never get here, but the compiler may complain.
  return 0;
}




//@}

#endif

