
// $Id$

/*
    A centralized set of classes to handle output streams in Smart
*/

#ifndef OUTPUT_H
#define OUTPUT_H

#include <ostream>

class OutputStream {
public:
  bool active;
protected:
  std::ostream* display;
  std::ostream* deflt;
public:
  OutputStream(std::ostream* deflt);

  std::ostream& Out() { return *display; }

  void SwitchDisplay(std::ostream* out);
};

template <class DATA>
OutputStream& operator<< (OutputStream& s, DATA data) 
{
  if (s.active) s.Out() << data;
  return s;
}

// Pre-defined output streams
extern OutputStream Output;
extern OutputStream Verbose;
extern OutputStream Report;


/**
    Initialize the global output streams.
*/
void InitOutputStreams();

#endif

