
// $Id$

#ifndef STRINGS_H
#define STRINGS_H

/** @name strings.h
    @type File
    @args \ 

    String operators.
 */

#include "exprs.h"

//@{

class shared_string : public shared_object {
public:
  char* string;
  shared_string() { string = NULL; }
  shared_string(char* s) { string = s; }
  virtual~ shared_string() { delete[] string; }
  inline int length() const { return strlen(string); }
  inline void show(OutputStream &s) { s.Put(string); }
};

inline int compare(shared_string *a, shared_string *b)
{
  if (a==b) return 0;  // shared or null
  if (NULL==a) return -1;
  if (b==NULL) return 1;
  return strcmp(a->string, b->string);
}

inline char* stringconvert(shared_object *x)
{
  if (NULL==x) return NULL;
  shared_string *ss = dynamic_cast<shared_string*>(x);
  DCASSERT(ss);
  return ss->string;
}

// ******************************************************************
// *                                                                *
// *             Global functions  to build expressions             *
// *                                                                *
// ******************************************************************

/**
    Build an associative sum of strings expression.

    @param	opnds	The operands as expressions,
    			all with type STRING already.
    @param	n	Number of operands.
    			Must be at least 1.
    @param	file	Source file where this is defined
    @param	line	Line number where this is defined

    @return 	The appropriate expression, or NULL if we couldn't build it.
*/
expr* MakeStringAdd(expr** opnds, int n, const char* file, int line);

/**
     Build a binary expression for strings.

     @param	left	The left-hand expression (type STRING).
     @param	op	The operator (as defined in smart.tab.h)
     			Must be one of:
			PLUS
			EQUALS
			NEQUAL
			GT
			GE
			LT
			LE
			
     @param	right	The right-hand expression (type STRING).
     @param	file	Source file where this is defined
     @param	line	Line number where this is defined

     @return	The appropriate new expression, or NULL if
     		we could not build the expression.

*/
expr* MakeStringBinary(expr* left, int op, expr* right, const char* file, int
line);

/** Print the specified string.
    This is necessary to handle fancy string things, line \n, \t, etc.
*/
void PrintString(const result &x, OutputStream &out, int width=-1);

/** Check equality of two string results.
*/
bool StringEquals(const result &x, const result &y);


//@}

#endif

