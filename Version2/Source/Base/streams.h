
// $Id$

/**
    A centralized set of classes to handle I/O streams in Smart.
*/

#ifndef STREAMS_H
#define STREAMS_H

#include <stdio.h>

/** Basic output stream.
    Note that error streams are derived from this class.
*/
class OutputStream {
protected:
  bool ready;
  FILE* deflt;
  FILE* display;
public:
  OutputStream(FILE* deflt);

  void SwitchDisplay(FILE* out);

  void Activate();
  void Deactivate();
  bool IsActive() { return ready; }

  void Put(bool data);
  void Put(char data);
  void Put(int data);
  void Put(float data);
  void Put(double data);
  void Put(const char* data);

/*
  void Put(bool data, int width);
  void Put(int data, int width);
  void Put(double data, int width);
  void Put(const char* data, int width);

  void Put(double data, int width, int prec);
  */

  inline void flush() { fflush(display); }
};

inline OutputStream& operator<< (OutputStream& s, bool data) 
{
  s.Put(data);
  return s;
}

inline OutputStream& operator<< (OutputStream& s, char data) 
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

/*
inline OutputStream& operator<< (OutputStream& s, char* data) 
{
  s.Put(data);
  return s;
}
*/

inline OutputStream& operator<< (OutputStream& s, const char* data) 
{
  s.Put(data);
  return s;
}

/**
    Used for centralized error reporting.
*/
class ErrorStream : public OutputStream {
protected:
  bool active;
  const char* errortype;
public:
  ErrorStream(const char* et, FILE* deflt);

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
  InternalStream(const char* et, FILE* deflt) : ErrorStream(et, deflt) { }
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



class InputStream {
protected:
  bool ready;
  FILE* stream;
public:
  InputStream(FILE* deflt);

  void SwitchInput(FILE* in);

  void Activate();
  void Deactivate();

  bool Get(bool &data);
  bool Get(int &data);
  bool Get(double &data);
  bool Get(char* &data);
};




// Pre-defined output streams
extern OutputStream Output;
extern OutputStream Verbose;
extern OutputStream Report;
// Pre-defined error streams
extern ErrorStream Error;
extern ErrorStream Warn;
extern InternalStream Internal;


/**
    Initialize the global output streams.
*/
void InitStreams();

#endif

