
// $Id$

#ifndef TIMERS_H
#define TIMERS_H

#include "streams.h"

#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>

//@{

/** A class for measuring usage, not unlike a stopwatch.

    Various measurements are possible between the designated
    "Start" and "Stop" points of the computation.
*/
class timer {
  rusage start, stop;
  double GetElapsed(timeval v1, timeval v2) const;
  void ShowElapsed(OutputStream &s, double m) const;
public:
  /// Set "now" as the start time.
  void Start();
  /// Set "now" as the stop time.
  void Stop();
  /// Show the elapsed user and system CPU times, in long format.
  void Show(OutputStream &) const;
  /// The number of seconds of user CPU elapsed. 
  inline double User_Seconds() const { 
    return GetElapsed(start.ru_utime, stop.ru_utime); 
  }
  /// The number of seconds of system CPU elapsed.
  inline double System_Seconds() const {
    return GetElapsed(start.ru_stime, stop.ru_stime);
  }

  /// Number of major page faults (forced to read from disk)
  inline long MajFlt() const {
    return stop.ru_majflt - start.ru_majflt;
  }

  /// Number of minor page faults
  inline long MinFlt() const {
    return stop.ru_minflt - start.ru_minflt;
  }

  // get current time value
  double getTime() const;
};

inline OutputStream& operator << (OutputStream &s, const timer &t)
{
  t.Show(s);
  return s;
}

//@}

#endif

