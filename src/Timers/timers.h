
// $Id$

#ifndef TIMERS_H
#define TIMERS_H

/** 
    A centralized class for handling time measurement.
    Implementation details are completely hidden to
    help with portability across platforms.
*/

class timer {
public:
  /// Constructor, will reset the clock.
  timer();
protected:
  virtual ~timer();
  friend void doneTimer(timer* &t);
public:
  /// Reset the clock to "now".
  virtual void reset() = 0;

  /// Get the number of seconds elapsed since the last reset.
  virtual double elapsed() const = 0;
};

/** Front end: build a timer object.
    TBD: do we want options,
    e.g., user time, system time, wall time, etc?
*/
timer* makeTimer();

/// Recycle a timer object.
void doneTimer(timer* &t);


#endif

