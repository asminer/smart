
// $Id$

/*
    A centralized set of classes to handle the various errors
    and output streams in Smart
*/

#ifndef ERRORS_H
#define ERRORS_H

#include <sstream>

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

  void Start(char* filename=NULL, int lineno=-1);
  void Stop();
};

template <class DATA>
ErrorStream& operator<< (ErrorStream& s, DATA data) 
{
  if (s.IsReady()) s.Out() << data;
  return s;
}

// Pre-defined streams
extern ErrorStream Error;
extern ErrorStream Warn;
extern ErrorStream Internal;


/**
    Initialize the global error streams.
*/
void InitStreams();

#endif

