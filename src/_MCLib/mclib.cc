
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


#ifndef DISABLE_OLD_INTERFACE

#include <string.h>

// ******************************************************************
// *                       Macros and such                          *
// ******************************************************************

inline void ShowVector(double* p, long size)
{
  printf("[%f", p[0]);
  for (long s = 1; s<size; s++)
    printf(", %f", p[s]);
  printf("]");
}

#ifdef DEBUG_UNIF
void ShowUnifStep(int steps, double poiss, double* p, double* a, long size)
{
  printf("After %d steps, dtmc distribution is:\n\t", steps);
  ShowVector(p, size);
  printf("\n\tPoisson: %g\n", poiss);
  printf("accumulator so far:\n\t");
  ShowVector(a, size);
  printf("\n");
}
#endif


// ******************************************************************
// *                                                                *
// *                         error  methods                         *
// *                                                                *
// ******************************************************************

const char* Old_MCLib::error::getString() const
{
  switch (errcode) {
    case Not_Implemented:   return "Not implemented";
    case Null_Vector:       return "Null vector";
    case Wrong_Type:        return "Wrong type of chain";
    case Bad_Index:         return "Bad index";
    case Bad_Class:         return "Bad class";
    case Bad_Rate:          return "Bad Rate";
    case Out_Of_Memory:     return "Out of memory";
    case Bad_Time:          return "Bad Time";
    case Finished_Mismatch: return "Finished/unfinished chain expected";
    case Wrong_Format:      return "Wrong format for linear solver";
    case Bad_Linear:        return "Error in  linear solver";
    case Loop_Of_Vanishing: return "Absorbing vanishing loop";
    case Internal:          return "Internal error";
    case Miscellaneous:     return "Misc. error";
  };
  return "Unknown error";
}


// ******************************************************************
// *                                                                *
// *                      Markov_chain methods                      *
// *                                                                *
// ******************************************************************

Old_MCLib::Markov_chain::Markov_chain(bool disc)
{
  discrete = disc;
  finished = false;
  our_type = Unknown;
  num_states = 0;
  num_classes = -1;
}

Old_MCLib::Markov_chain::~Markov_chain()
{
}

// ******************************************************************
// *                                                                *
// *                 Markov_chain subclass  methods                 *
// *                                                                *
// ******************************************************************

Old_MCLib::Markov_chain::renumbering::renumbering()
{
  type = 'N';
  newhandle = 0;
}

Old_MCLib::Markov_chain::renumbering::~renumbering()
{
  free(newhandle);
}

void Old_MCLib::Markov_chain::renumbering::clear()
{
  free(newhandle);
  type = 'N';
  newhandle = 0;
}

// ******************************************************************
// *                                                                *
// *                    vanishing_chain  methods                    *
// *                                                                *
// ******************************************************************

Old_MCLib::vanishing_chain::vanishing_chain(bool disc, long nt, long nv)
{
  discrete = disc;
  num_tangible = nt;
  num_vanishing = nv;
}

Old_MCLib::vanishing_chain::~vanishing_chain()
{
}

// ******************************************************************
// *                                                                *
// *                                                                *
// *                      Front-end  functions                      *
// *                                                                *
// *                                                                *
// ******************************************************************


const char* Old_MCLib::Version()
{
  const int MAJOR_VERSION = 3;
  const int MINOR_VERSION = 1;

  static char buffer[100];
  snprintf(buffer, sizeof(buffer), "Markov chain Library, version %d.%d",
     MAJOR_VERSION, MINOR_VERSION);
  return buffer;

  // TBD - revision number?
}


#endif // #ifndef DISABLE_OLD_INTERFACE

