
// $Id: timers.cc 6 2007-03-01 17:59:56Z asminer $

#include "timers.h"  

// #include <math.h>


/** @name timers.cc
    @type File
    @args \ 

   Implementation of the class stopwatch

   This version uses "getrusage"

*/

//@{

double stopwatch::GetElapsed(timeval v1, timeval v2) const
{
  double m1,m2;
  m1 = v1.tv_sec + v1.tv_usec / 1000000.0 ;
  m2 = v2.tv_sec + v2.tv_usec / 1000000.0 ;
  return m2 - m1;
}

void stopwatch::Start()
{
//#ifndef SUN4M
  getrusage(RUSAGE_SELF, &start);
//#endif
}

void stopwatch::Stop()
{
//#ifndef SUN4M
  getrusage(RUSAGE_SELF, &stop);
//#endif
}

/**
 * read current usertime
 */
double stopwatch::getTime() const
{
    int test;
    double duration;
    rusage current;
// #ifndef SUN4M
    test = getrusage(RUSAGE_SELF, &current);
// #endif
    duration = (current.ru_utime.tv_sec * 1000000.0) + current.ru_utime.tv_usec;
    return duration;
}

//@}

