
// $Id$

/*
    A centralized set of classes to handle the various errors
    and output streams in Smart
*/

#ifndef ERRORS_H
#define ERRORS_H

#include <sstream>

/**
    Used for centralized error reporting.
*/
class ErrorStream {
protected:
  std::ostringstream *out;
  bool ready;
  const char* errortype;
  bool active;
  std::ostream* display;
public:
  ErrorStream(const char* et);

  std::ostream& Out() { return *out; }
  bool IsReady() const { return ready; }
  void Activate() { active = true; }
  void Deactivate() { active = false; }
  void SetDisplay(std::ostream *d) { display = d; }

  /**
      Used to start reporting an error.
      @param filename	Offending input file to smart
      @param lineno	Approximate line number of offending input file
  */
  void Start(char* filename=NULL, int lineno=-1);

  /**
      We're done reporting an error.
  */
  void Stop();
};

template <class DATA>
ErrorStream& operator<< (ErrorStream& s, DATA data) 
{
  if (s.IsReady()) s.Out() << data;
  return s;
}

/**
     Used for internal errors.
*/
class InternalStream : public ErrorStream {
public:
  InternalStream(const char* et) : ErrorStream(et) { };
  /**
  	Used to start reporting an error.

      @param srcfile		Which source (i.e., c++ file) is unhappy
      @param srcline		Where in c++ source file we are unhappy
      @param filename		Offending smart input file
      @param lineno		Approximate line number of offending input file
  */
  void Start(char* srcfile, int srcline, char* filename=NULL, int lineno=-1);
};

// Pre-defined streams
extern ErrorStream Error;
extern ErrorStream Warn;
extern InternalStream Internal;


/**
    Initialize the global error streams.
*/
void InitStreams();

#endif

