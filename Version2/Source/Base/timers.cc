
// $Id$

#include "timers.h"  

#include <math.h>


/** @name timers.cc
    @type File
    @args \ 

   Implementation of the class timer

   This version uses "getrusage"

*/

//@{

double timer::GetElapsed(timeval v1, timeval v2) const
{
  double m1,m2;
  m1 = v1.tv_sec + v1.tv_usec / 1000000.0 ;
  m2 = v2.tv_sec + v2.tv_usec / 1000000.0 ;
  return m2 - m1;
}

void timer::ShowElapsed(OutputStream &s, double m) const
{
  s << m << " seconds";
  int days,hours,minutes;
  long newm = int(m+0.5);
  newm = newm / 60;
  minutes = newm % 60;
  newm = newm / 60;
  hours = newm % 24;
  days = newm / 24;
  double seconds = m - days*86400.0 - hours*3600.0 - minutes*60.0;
  if (m>=60.0) {
    s << " (";
    if (days>0) s << days <<"d:";
    if (hours>0) s << hours << "h:";
    if (minutes>0) s << minutes << "m:";
    s << seconds << "s)";
  }
}

void timer::Start()
{
//#ifndef SUN4M
  getrusage(RUSAGE_SELF, &start);
//#endif
}

void timer::Stop()
{
//#ifndef SUN4M
  getrusage(RUSAGE_SELF, &stop);
//#endif
}

/**
 * read current usertime
 */
double timer::getTime() const
{
    int test;
    double duration;
    rusage current;
#ifndef SUN4M
    test = getrusage(RUSAGE_SELF, &current);
#endif
#ifdef SUN4M
    test = 0;
#endif
    if (test != 0) {
      Internal.Start(__FILE__, __LINE__);
      Internal << "Error reading clock\n";
      Internal.Stop();
    }

    duration = (current.ru_utime.tv_sec * 1000000.0) + current.ru_utime.tv_usec;
    return duration;
}

void timer::Show(OutputStream &s) const
{
  s << "User: ";
  ShowElapsed(s, User_Seconds());
  s << " System: ";
  ShowElapsed(s, System_Seconds());
  s << " Page faults: " << MajFlt() << "\n";
}

//@}

