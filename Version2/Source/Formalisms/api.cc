
// $Id$

#include "api.h"

model* MakeNewModel(const char* fn, int line, type t, char* name, formal_param
**pl, int np)
{
  switch(t) {
    case DTMC:
    	Output << "DTMC not implemented yet\n";
	Output.flush();
	return NULL;

    case CTMC:
    	Output << "CTMC not implemented yet\n";
	Output.flush();
	return NULL;

    case SPN:
    	Output << "SPN not implemented yet\n";
	Output.flush();
	return NULL;

  }
  
  // still here?  Bad input file?
  Internal.Start(__FILE__, __LINE__, fn, line);
  Internal << "Cannot build model of type " << GetType(t) << "\n";
  Internal.Stop();

  // Won't get here
  return NULL;
}

bool CanDeclareType(type modeltype, type vartype)
{
  switch (modeltype) {
    case DTMC:
    case CTMC:
    		return (vartype == STATE);

    case SPN:
    		return (vartype == PLACE) || (vartype == TRANS);
  }

  // slipped through the cracks
  return false;
}

