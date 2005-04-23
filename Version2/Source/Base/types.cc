
// $Id$

#include "types.h"  
//@Include: types.h

/** @name types.cc
    @type File
    @args \ 

  Implementation of type stuff.

 */

//@{

// ********************************************************
// *                    Global  stuff                     *
// ********************************************************

const char* OneWordSimple[] = {
  "void",
  "bool",
  "int",
  "real",
  "string",
  "bigint",
  "stateset",
  "expo"
};

const char* Modifs[] = {
  "ph",
  "rand"
};

const char* NonProcNames[] = {
  "void",
  "bool",
  "int",
  "real",
  "string",
  "bigint",
  "stateset",
  "expo",
  "ph int",
  "ph real",
  "rand bool",
  "rand int",
  "rand real"
};

const char* ProcNames[] = {
  "proc bool",
  "proc int",
  "proc real",
  "proc expo",
  "proc ph int",
  "proc ph real",
  "proc rand bool",
  "proc rand int",
  "proc rand real",
  "proc state"
};

const char* ModelNames[] = {
  "any model",
  "Markov chain",
  "dtmc",
  "ctmc",
  "pn"
};

const char* VoidNames[] = {
  "state",
  "place",
  "trans"
};

const char* SetNames[] = {
  "{int}",
  "{real}",
  "{state}",
  "{place}",
  "{trans}"
};

const char* InternalNames[] = {
  "(internal) engine info",
  "(internal) expr*"
};

//
//  Code --> Type name conversion
//

const char* GetType(type t)
{
  if (NO_SUCH_TYPE == t)
    return "Unknown type";
  
  if (t<=LAST_SIMPLE)
    return NonProcNames[t-FIRST_SIMPLE];

  if (t<=LAST_PROC)
    return ProcNames[t-FIRST_PROC];

  if (t<=LAST_MODEL)
    return ModelNames[t-FIRST_MODEL];

  if (t<=LAST_VOID)
    return VoidNames[t-FIRST_VOID];

  if (t<=LAST_SET)
    return SetNames[t-FIRST_SET];

  if (t<=LAST_INTERNAL)
    return InternalNames[t-FIRST_INTERNAL];

  // Still here?
  return "Unknown type";
}

type SetOf(type t)
{
  switch (t) {
    case INT:		return SET_INT;
    case REAL:		return SET_REAL;
    case STATE:		return SET_STATE;
    case PLACE:		return SET_PLACE;
    case TRANS:		return SET_TRANS;
  }
  return NO_SUCH_TYPE;
}

//
//  Type name --> integer code conversion
//

// Which simple types can be defined by a user
inline bool isUserDefinable(type t)
{
  return (t!=EXPO);
}

type FindSimpleType(const char *t)
{
  type i;
  for (i=FIRST_SIMPLE; i<=LAST_ONE_WORD; i++)
    if (isUserDefinable(i))
      if (0==strcmp(t,OneWordSimple[i-FIRST_SIMPLE])) 
	return i;
  return NO_SUCH_TYPE;
} 

type FindModel(const char *t)
{
  type i;
  for (i=FIRST_MODEL; i<=LAST_MODEL; i++)
    if (0==strcmp(t,ModelNames[i-FIRST_MODEL])) return i;
  return NO_SUCH_TYPE;
} 

type FindVoidType(const char *t)
{
  type i;
  for (i=FIRST_VOID; i<=LAST_VOID; i++)
    if (0==strcmp(t,VoidNames[i-FIRST_VOID])) return i;
  return NO_SUCH_TYPE;
}

modifier FindModif(const char *m)
{
  modifier i;
  for (i=FIRST_MODIF; i<=LAST_MODIF; i++)
    if (0==strcmp(m,Modifs[i-FIRST_MODIF])) return i;
  return NO_SUCH_MODIF;
}


// *******************************************************************
// *                                                                 *
// *                  Compile-time  type promotions                  *
// *                                                                 *
// *******************************************************************

bool Promotable(type t1, type t2)
// Can type t1 be promoted to type t2?
{
  if (t1==t2) return true;
  if (t1==VOID) return true;  // Allows null -> int/real/etc promotion
  if (t2==VOID) return false; // I think...

  // Handle "promotion" of specific models to generic models.
  if (IsModelType(t1) && (ANYMODEL == t2)) return true;

  if (t1>=FIRST_MODEL) return false;
  if (t2>=FIRST_MODEL) return false;

  switch (t1) {
    case BOOL		: return (RAND_BOOL==t2) 	|| 
			         (PROC_BOOL==t2) 	|| 
				 (PROC_RAND_BOOL==t2);

    case INT		: return (BIGINT==t2) 		||
			  	 (REAL==t2)		||
				 (PH_INT==t2)		||
				 (PH_REAL==t2)		|| // needed for 0
				 (RAND_REAL==t2)	||
				 (RAND_INT==t2)		||
				 (PROC_INT==t2)		||
				 (PROC_REAL==t2)	||
				 (PROC_PH_INT==t2)	||
				 (PROC_PH_REAL==t2)	||
				 (PROC_RAND_INT==t2)	||
				 (PROC_RAND_REAL==t2);
			  	 // whew!

    case REAL 		: return (RAND_REAL==t2)	||
			  	 (PROC_REAL==t2)	||
				 (PROC_RAND_REAL==t2);

    case EXPO		: return (PH_REAL==t2)		||
			  	 (RAND_REAL==t2)	||
				 (PROC_EXPO==t2)	||
				 (PROC_PH_REAL==t2)	||
				 (PROC_RAND_REAL==t2);

    case PH_INT		: return (RAND_INT==t2)		||
                             	 (RAND_REAL==t2)	||
			  	 (PROC_PH_INT==t2)	||
				 (PROC_RAND_INT==t2) 	||
				 (PROC_RAND_REAL==t2);

    case PH_REAL	: return (RAND_REAL==t2)	||
			  	 (PROC_PH_REAL==t2)	||
				 (PROC_RAND_REAL==t2);

    case RAND_BOOL	: return (PROC_RAND_BOOL==t2);

    case RAND_INT	: return (RAND_REAL==t2)	||
			  	 (PROC_RAND_INT==t2)    ||
				 (PROC_RAND_REAL==t2);

    case RAND_REAL	: return (PROC_RAND_REAL==t2);

    case PROC_BOOL	: return (PROC_RAND_BOOL==t2);

    case PROC_INT	: return (PROC_REAL==t2)	||
				 (PROC_PH_INT==t2)	||
				 (PROC_PH_REAL==t2)	||
				 (PROC_RAND_INT==t2)	||
				 (PROC_RAND_REAL==t2);

    case PROC_REAL	: return (PROC_RAND_REAL==t2);

    case PROC_EXPO	: return (PROC_PH_REAL==t2)	||
			  	 (PROC_RAND_REAL==t2);

    case PROC_PH_INT	: return (PROC_RAND_INT==t2) 	||
			  	 (PROC_RAND_REAL==t2);

    case PROC_PH_REAL	: return (PROC_RAND_REAL==t2);

    case PROC_RAND_INT	: return (PROC_RAND_REAL==t2);

    // internal stuff

    case SET_INT	: return (SET_REAL==t2);

    default: return false;  // no promotions
  };

  // shouldn't get here
  return false;
}


// *******************************************************************
// *                                                                 *
// *                    Compile-time type casting                    *
// *                                                                 *
// *******************************************************************

bool Castable(type t1, type t2)
// Can type t1 be cast to type t2?
{
  if (Promotable(t1,t2)) return true;
  switch (t1) {
    case INT:
      return (t2 == EXPO);

    case REAL:
      return (t2 == INT) || (t2 == BIGINT) || (t2 == EXPO);

    case BIGINT:
      return (t2 == INT);

    case RAND_REAL:
      return (t2 == RAND_INT);

    case PROC_REAL:
      return (t2 == PROC_INT);

    case PROC_RAND_REAL:
      return (t2 == PROC_RAND_INT);

  }
  return false;
}


//@}

