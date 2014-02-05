
// $Id$

#include "timers.h"  

#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>


// ==================================================================
// |                                                                |
// |                         timer  methods                         |
// |                                                                |
// ==================================================================

timer::timer()
{
}

timer::~timer()
{
}

// ==================================================================
// |                                                                |
// |                       rusage_timer class                       |
// |                                                                |
// ==================================================================

class rusage_timer : public timer {
  double start_time;
public:
  rusage_timer();
  virtual void reset();
  virtual double elapsed() const;
protected:
  inline double Time() const {
    rusage r;
    getrusage(RUSAGE_SELF, &r);
    return r.ru_utime.tv_sec + r.ru_utime.tv_usec / 1000000.0;
  }
};

rusage_timer::rusage_timer() : timer()
{
  reset();
}

void rusage_timer::reset()
{
  start_time = Time();
}

double rusage_timer::elapsed() const
{
  double stop_time = Time();
  return stop_time - start_time;
}

// ==================================================================
// |                                                                |
// |                           Front  end                           |
// |                                                                |
// ==================================================================

timer* makeTimer()
{
  return new rusage_timer;
}

void doneTimer(timer* &x)
{
  delete x;
  x = 0;
}

