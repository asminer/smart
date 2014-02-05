
// $Id$

#include "dd_front.h"

sv_encoder::sv_encoder()
{
}

sv_encoder::~sv_encoder()
{
}

bool sv_encoder::Print(OutputStream &s, int) const
{
  DCASSERT(0);
  s << "sv_encoder";
  return true;
}

bool sv_encoder::Equals(const shared_object *o) const
{
  return (this == o);
}

const char* sv_encoder::getNameOfError(error e)
{
  switch (e) {
    case Success:             return "Success";
    case Invalid_Edge:        return "Invalid edge";
#ifdef DEVELOPMENT_CODE
    case Shared_Output_Edge:  return "Output edge is shared";
#endif
    case Out_Of_Memory:       return "Not enough memory";
    case Empty_Set:           return "No minterms available";
    case Failed:              return "Operation failed";
  }
  return "Unknown error";
}

