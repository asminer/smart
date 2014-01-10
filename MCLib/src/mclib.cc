
// $Id$

#include "mclib.h"

#include <stdio.h> 
#include <string.h>

#include "revision.h"

const int MAJOR_VERSION = 3;
const int MINOR_VERSION = 1;

// ******************************************************************
// *                       Macros and such                          *
// ******************************************************************

/// SWAP "macro".
template <class T> inline void SWAP(T &x, T &y) { T tmp=x; x=y; y=tmp; }

/// Standard MAX "macro".
template <class T> inline T MAX(T X,T Y) { return ((X>Y)?X:Y); }

/// Standard MIN "macro".
template <class T> inline T MIN(T X,T Y) { return ((X<Y)?X:Y); }

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

const char* MCLib::error::getString() const
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
    case Miscellaneous:     return "Misc. error";
  };
  return "Unknown error";
}


// ******************************************************************
// *                                                                *
// *                      Markov_chain methods                      *
// *                                                                *
// ******************************************************************

MCLib::Markov_chain::Markov_chain(bool disc)
{
  discrete = disc;
  finished = false;
  our_type = Unknown;
  num_states = 0;
  num_classes = -1;
}

MCLib::Markov_chain::~Markov_chain()
{
}

// ******************************************************************
// *                                                                *
// *                 Markov_chain subclass  methods                 *
// *                                                                *
// ******************************************************************

MCLib::Markov_chain::renumbering::renumbering()
{
  type = 'N';
  newhandle = 0;
}

MCLib::Markov_chain::renumbering::~renumbering()
{
  free(newhandle);
}

void MCLib::Markov_chain::renumbering::clear()
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

MCLib::vanishing_chain::vanishing_chain(bool disc, long nt, long nv)
{
  discrete = disc;
  num_tangible = nt;
  num_vanishing = nv;
}

MCLib::vanishing_chain::~vanishing_chain()
{
}

// ******************************************************************
// *                                                                *
// *                                                                *
// *                      Front-end  functions                      *
// *                                                                *
// *                                                                *
// ******************************************************************


const char* MCLib::Version()
{
  static char buffer[100];
  snprintf(buffer, sizeof(buffer), "Markov chain Library, version %d.%d.%d",
     MAJOR_VERSION, MINOR_VERSION, REVISION_NUMBER);
  return buffer;
}


