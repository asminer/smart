
/**

  Implementation of error class

*/

#include "mclib.h"

// ======================================================================
// |                                                                    |
// |                           error  methods                           |
// |                                                                    |
// ======================================================================

MCLib::error::error(code c)
{
  errcode = c;
}

const char* MCLib::error::getString() const
{
  switch (errcode) {
    case Not_Implemented:   return "Not implemented";
    case Null_Vector:       return "Null vector";
    case Wrong_Type:        return "Wrong type of chain";
    case Out_Of_Memory:     return "Out of memory";
    case Bad_Time:          return "Bad Time";
    case Bad_Linear:        return "Error in  linear solver";
    case Bad_Class:         return "Invalid class index";
    case Loop_Of_Vanishing: return "Absorbing vanishing loop";
    case Internal:          return "Internal error";
    case Miscellaneous:     return "Misc. error";
  };
  return "Unknown error";
}

