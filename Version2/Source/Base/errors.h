
// $Id$

/*
    A centralized set of classes to handle the various error streams in Smart
*/

#ifndef ERRORS_H
#define ERRORS_H

#include "output.h"

/**
    Used for centralized error reporting.
*/
class ErrorStream : public OutputStream {
protected:
  bool active;
  const char* errortype;
public:
  ErrorStream(const char* et, std::ostream *deflt);

  void Activate() { active = true; }
  void Deactivate() { active = ready = false; }

  /**
      Used to start reporting an error.
      @param filename	Offending input file to smart
      @param lineno	Approximate line number of offending input file
  */
  void Start(const char* filename=NULL, int lineno=-1);

  /**
      We're done reporting an error.
  */
  void Stop();
};

/**
    Used for centralized error reporting of internal errors.
*/
class InternalStream : public ErrorStream {
public:
  InternalStream(const char* et, std::ostream *d) : ErrorStream(et, d) { }
  /**
      Used to start reporting an internal error.

      @param srcfile		Which source (i.e., c++ file) is unhappy
      @param srcline		Where in c++ source file we are unhappy
      @param filename		Offending smart input file
      @param lineno		Approximate line number of offending input file
  */
  void Start(const char* srcfile, int srcline, 
             const char* filename=NULL, int lineno=-1);

  /**
      We're done reporting an error.
      In the internal case, we will terminate!
  */
  void Stop();
};


// Pre-defined streams
extern ErrorStream Error;
extern ErrorStream Warn;
extern InternalStream Internal;


/**
    Initialize the global error streams.
*/
void InitErrorStreams();

#endif

