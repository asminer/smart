
// $Id$

#include "timerlib.h"

/*
  TBD - do we want to use system-dependent methods
  for determining time, in case some mechanisms are
    more accurate than others?
*/

#include <sys/time.h>
#include <time.h>

long timer::read_clock()
{
  static struct timeval curr_time;
  static struct timezone time_zone;
  gettimeofday(&curr_time, &time_zone);

  return (curr_time.tv_sec * 1000000) + curr_time.tv_usec;
}

