
// $Id$

#include "streams.h"

void OutOfMemoryError(const char* cause)
{
  Internal.Start(__FILE__, __LINE__);
  Internal << "Memory overflow";
  if (cause) Internal << " caused by " << cause;
  Internal << "\n";
  Internal.Stop();
}

