
#include "mclib.h"
#include <stdio.h> 

// ******************************************************************
// *                                                                *
// *                                                                *
// *                      Front-end  functions                      *
// *                                                                *
// *                                                                *
// ******************************************************************

const char* MCLib::Version()
{
  const int MAJOR_VERSION = 4;
  const int MINOR_VERSION = 0;

  static char buffer[100];
  snprintf(buffer, sizeof(buffer), "Markov chain Library, version %d.%d",
     MAJOR_VERSION, MINOR_VERSION);
  return buffer;

  // TBD - revision number?
}


