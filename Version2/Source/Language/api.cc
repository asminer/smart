
// $Id$

#include "api.h"

#include "infinity.h"

void InitLanguage()
{
  CreateRuntimeStack(1024); // Large enough?

  // initialize options
  char* inf = strdup("infinity");
  const char* doc1 = "Output string for infinity";
  infinity_string = MakeStringOption("InfinityString", doc1, inf);
  AddOption(infinity_string);
}
