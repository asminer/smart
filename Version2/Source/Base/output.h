
// $Id$

/*
    A centralized set of classes to handle output streams in Smart
*/

#ifndef OUTPUT_H
#define OUTPUT_H

#include <ostream>

class OutputStream {
protected:
  bool ready;
  std::ostream* display;
  std::ostream* deflt;
public:
  OutputStream(std::ostream* deflt);

  std::ostream& Out() { return *display; }

  void SwitchDisplay(std::ostream* out);

  inline bool IsActive() const { return ready; }
  inline void flush() const { if (ready) display->flush(); }

  void Activate();
  void Deactivate();

  template <class DATA>
  inline void Put(DATA d) { if (ready) (*display) << d; }
};

inline OutputStream& operator<< (OutputStream& s, bool data) 
{
  s.Put(data);
  return s;
}

inline OutputStream& operator<< (OutputStream& s, int data) 
{
  s.Put(data);
  return s;
}

inline OutputStream& operator<< (OutputStream& s, float data) 
{
  s.Put(data);
  return s;
}

inline OutputStream& operator<< (OutputStream& s, double data) 
{
  s.Put(data);
  return s;
}

inline OutputStream& operator<< (OutputStream& s, char* data) 
{
  s.Put(data);
  return s;
}

inline OutputStream& operator<< (OutputStream& s, const char* data) 
{
  s.Put(data);
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

