
// $Id$

#include "results.h"
#include "infinity.h"
#include "strings.h"

/** @name results.cc
    @type File
    @args \ 

   Implementation of result stuff.

 */

//@{


void PrintResult(type t, const result &x, OutputStream &s)
{
  if (x.infinity) { s << infinity_string->GetString(); return; }
  if (x.null) { s << "null"; return; }
  if (x.error) { s << "error"; return; }
  switch(t) {
    case VOID: 		DCASSERT(0); 		return;
    case BOOL: 		s << x.bvalue; 		return;
    case INT:  		s << x.ivalue; 		return;
    case REAL: 		s << x.rvalue; 		return;
    case STRING: 	PrintString(x, s);	return;	
  }
  DCASSERT(0);
}

/*  As other complex structures that make use of "other"
    come along, this function will need to be updated
*/
void DeleteResult(type t, result &x)
{
  if (!x.canfree) return;  // we shouldn't be deleting anything
  switch (t) {
    case STRING:	free(x.other);		break;  // should be ok?
  }
}

