
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

void DeleteResult(type t, result &x)
{
  // types that use "other" go here
  switch (t) {
    case STRING:	
		Delete(x.other);	
		x.other = NULL;
	        x.setNull();
		break;  // should be ok?
  }
}

bool Equals(type t, const result &x, const result &y)
{
  // these had better be the same type
  if (x.isNormal() && y.isNormal()) {
    switch (t) {
      case BOOL:	return x.bvalue == y.bvalue;
      case INT:		return x.ivalue == y.ivalue;
      case REAL:	return x.rvalue == y.rvalue;
      case STRING:	return StringEquals(x, y);
    }
    // still here?
    return false;
  }
  if (x.isInfinity() && y.isInfinity()) return x.ivalue == y.ivalue;
  if (x.isNull() && y.isNull()) return true;
  return false;
}

