
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


void PrintResult(OutputStream &s, type t, const result &x, int width, int prec)
{
  if (x.isUnknown()) { s.Put('?'); return; }
  if (x.isInfinity()) { 
    if (x.ivalue<0) s.Put('-');
    s << infinity_string->GetString(); 
    return; 
  }
  if (x.isNull()) { s << "null"; return; }
  if (x.isError()) { s << "null"; return; }
  // width not specified
  if (width<0) {
    switch(t) {
      case BOOL: 	s << x.bvalue; 		return;
      case INT:  	s << x.ivalue; 		return;
      case REAL: 	s << x.rvalue; 		return;
      case STRING: 	PrintString(x, s);	return;	
      default: 		DCASSERT(0); 		return;
    }
  }
  // width specified
  if (prec<0) {
    switch(t) {
      case BOOL: 	s.Put(x.bvalue, width);		return;
      case INT:  	s.Put(x.ivalue, width);		return;
      case REAL: 	s.Put(x.rvalue, width);		return;
      case STRING: 	PrintString(x, s, width);	return;	
      default: 		DCASSERT(0); 			return;
    }
  }
  // width and prec are specified
  switch (t) {
    case REAL:		s.Put(x.rvalue, width, prec);	return;
    default: 		DCASSERT(0); 			return;
  }
  // anything fell through the cracks?
  DCASSERT(0);
}

/*  As other complex structures that make use of "other"
    come along, this function will need to be updated
*/
void DeleteResult(type t, result &x)
{
  if (x.isNull()) return;  // nothing to free
  if (!x.isFreeable()) {
    x.setNull();
    return;  // we shouldn't be deleting anything
  }
  switch (t) {
    case STRING:	free(x.other);		break;  // should be ok?
  }
  x.setNull();
}

bool Equals(type t, result &x, result &y)
{
  if (x.isNormal() && y.isNormal()) {
    switch (t) {
      case BOOL:	return x.bvalue == y.bvalue;
      case INT:		return x.ivalue == y.ivalue;
      case REAL:	return x.rvalue == y.rvalue;
      case STRING:	return (strcmp((char *)x.other, (char *)y.other) == 0);
    }
    // still here?
    return false;
  }
  if (x.isInfinity() && y.isInfinity()) return true;
  if (x.isNull() && y.isNull()) return true;
  return false;
}

