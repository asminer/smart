
// $Id$

/**
    A centralized set of classes to handle I/O streams in Smart.
*/

#ifndef STREAMS_H
#define STREAMS_H

#include <stdio.h>

/** Abstract base class for output.
    All output goes to the buffer, for speed.
    (This unfies all output streams.)
*/
class OutputStream {
protected:
  char* buffer;
  int bufsize;
  int buftop;
  bool ready;
  const char* floatformat;
  const char* doubleformat;
  const char* doublewidthformat;
  const char* doubleprecformat;
protected:
  void ExpandBuffer(int newsize);
  inline char* bufptr() { return buffer+buftop; }
  inline int bufspace() { return bufsize-buftop; }
  void DetermineRealFormat();
public:
  OutputStream();
  virtual ~OutputStream();

  inline bool IsActive() const { return ready; }

  void Pad(int spaces);
  void Put(bool data);
  void Put(char data);
  void Put(int data);
  void Put(float data);
  void Put(double data);
  void Put(const char* data);

  void Put(bool data, int width);
  void Put(int data, int width);
  void Put(double data, int width);
  void Put(const char* data, int width);

  void Put(double data, int width, int prec);

  /// Force stream to dump buffer.
  virtual void flush() = 0;
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

inline OutputStream& operator<< (OutputStream& s, const char* data) 
{
  s.Put(data);
  return s;
}

/** String stream.
    Used by sprint.
*/
class StringStream : public OutputStream {
public:
  StringStream();
  
  char* GetString() const;
  virtual void flush();
};

/** Output (to the "display") stream.
    Note that error streams are derived from this class.
*/
class DisplayStream : public OutputStream {
protected:
  FILE* deflt;
  FILE* display;
public:
  DisplayStream(FILE* deflt);
  virtual ~DisplayStream();

  void SwitchDisplay(FILE* out);

  void Activate();
  void Deactivate();

  virtual void flush();
};

/**
    Used for centralized error reporting.
*/
class ErrorStream : public DisplayStream {
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

  /** 
      Used to print further messages about the last error.
      For instance, when doing a stack dump due to a runtime error.
      @param filename	Offending input file
      @param lineno	Approximate line number
  */
  void Continue(const char* filename = NULL, int lineno=-1);
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


/**
  Used for input.
*/
class InputStream {
protected:
  FILE* deflt;
  FILE* input;
public:
  InputStream(FILE* in);

  void SwitchInput(FILE* in);
  inline bool IsDefault() { return (input==deflt); }

  // return true on success.
  inline bool Get(char &x) { x = getc(input); return (x!=EOF); }
  inline bool Get(int &x) { return (1==fscanf(input, "%d", &x)); }
  inline bool Get(double &x) { return (1==fscanf(input, "%lf", &x)); }
};



// Pre-defined output streams
extern DisplayStream Output;
extern DisplayStream Verbose;
extern DisplayStream Report;
// Pre-defined error streams
extern ErrorStream Error;
extern ErrorStream Warning;
extern InternalStream Internal;

// Pre-defined input stream
extern InputStream Input;

/**
    Initialize the global output streams.
*/
void InitStreams();

#endif

