
// $Id$

#ifndef TIMERLIB_H
#define TIMERLIB_H

/**
    Simple timer class, meant to be portable.

    TBD - this kind of relies on 64-bit longs,
    do we want to do something clever for 32-bit systems?

    TBD - I suppose we could split the "start time"
    into int (or unsigned int) seconds and microseconds...
*/

class timer {
  public:
    timer() {
      reset();
    }

    /**
        Reset the timer.
        Start counting up from "now".
    */
    inline void reset() {
      start_time = read_clock();
    }

    /**
        Number of microseconds elapsed.
    */
    inline long elapsed_microseconds() const {
      return read_clock() - start_time;
    }

    /**
        Floating-point number of seconds elapsed.
    */
    inline double elapsed_seconds() const {
      return elapsed_microseconds() / 1000000.0;
    }
    
  protected:
    static long read_clock();

  private:
    long start_time;
};

#endif

