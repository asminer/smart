
/**
    A centralized set of classes to handle I/O streams in Smart.
*/

#ifndef STREAMS_H
#define STREAMS_H


#define OLD_STREAMS

class io_environ;
class location;
class shared_string;

#ifdef OLD_STREAMS

#include <stdio.h>

/** Abstract base class for output.
    All output goes to the buffer, for speed.
    (This unfies all output streams.)
*/
class OutputStream {
public:
  enum real_format {
    RF_GENERAL,
    RF_FIXED,
    RF_SCIENTIFIC
  };
protected:
  char* buffer;
  int bufsize;
  int buftop;
  bool ready;
private:
  const char* floatformat;
  const char* doubleformat;
  const char* doublewidthformat;
  const char* doubleprecformat;
  const io_environ* parent;
  char* intbuf;
  shared_string* thousands;
protected:
  void ExpandBuffer(int newsize);
  inline char* bufptr() { return buffer+buftop; }
  inline int bufspace() { return bufsize-buftop; }
  void Panic(const char* msg) const;
public:
  OutputStream();
  void SetParent(const io_environ* p);
  virtual ~OutputStream();

  inline bool IsActive() const { return ready; }

  void Pad(char space, int num_spaces);
  void Put(bool data);
  void Put(char data);
  void Put(int data);
  void Put(long data);
  void Put(unsigned long data);
  void Put(long long data);
  void PutHex(unsigned char data);
  void PutHex(unsigned long data);
  inline void PutAddr(void* data) { PutHex((unsigned long)data); }
  void Put(float data);
  void Put(double data);
  void Put(const char* data);

  void Put(bool data, int width);
  void Put(long data, int width);
  void Put(unsigned long data, int width);
  void Put(double data, int width);
  void Put(char data, int width);
  void Put(const char* data, int width);
  /// Makes comma-separated integers.
  void PutInteger(const char* data, int width);

  void Put(double data, int width, int prec);

  /// super handy- print an array!
  template <class DATA>
  void PutArray(DATA* array, int elements) {
    int i;
    if (elements) Put(array[0]);
    for (i=1; i<elements; i++) {
      Put(',');
      Put(' ');
      Put(array[i]);
    }
  }

  /// Specialized put: for files
  void PutFile(const char* fn, int ln);
  /// Specialized put: for memory usage
  void PutMemoryCount(size_t bytes, int prec);

  /// Force stream to dump buffer.
  virtual void flush() = 0;
  /// Check if we should flush the buffer.
  virtual void can_flush();

  void SetRealFormat(real_format rf);
  real_format GetRealFormat() const;

  inline shared_string* & linkThousands() {
      return thousands;
  }
};

template <class TYPE>
inline OutputStream& operator<< (OutputStream& s, TYPE data)
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
  inline const char* ReadString() const { return buffer; }
  virtual void flush();
};

/** Output (to the "display") stream.
*/
class DisplayStream : public OutputStream {
protected:
  static const int display_buffer_size;
  FILE* deflt;
  FILE* display;
public:
  DisplayStream();
  DisplayStream(FILE* out);
  void Initialize(FILE* out);

  virtual ~DisplayStream();

  void SwitchDisplay(FILE* out);

  inline FILE* getDisplay() { return display; }  // be careful with this

  void Activate();
  void Deactivate();

  virtual void flush();

  inline void Check() {
    if (ready) if (buftop>display_buffer_size) flush();
  }
  virtual void can_flush();
};

#else

#include "../Utils/outstream.h"
typedef     std::ostream   OutputStream;

#endif

/** Centralized i/o and error reporting class.

    Errors should be reported the following way:
      (1) call StartInternal(), StartError(), or StartWarning()
      (2) directly print any location details, e.g., "in function foo"
      (3) call either CausedBy() or NoCause(), which will finish the line
      (4) directly print any details, with line breaks by calling NewLine().
      (5) call Stop() when done.
*/
class io_environ {
  int WhichError;
  int  sigx;
  bool catchterm;
  int indents;
public:
#ifdef OLD_STREAMS
  DisplayStream Output;
  DisplayStream Report;     // 0
  DisplayStream Warning;    // 1
  DisplayStream Error;      // 2
  DisplayStream Internal;   // 3
#else
  outputStream Output;
  outputStream Report;
  outputStream Warning;
  outputStream Error;
  outputStream Internal;
#endif
public:
  io_environ();
  virtual ~io_environ();

  inline void WaitTerm() {
    catchterm = true;
  }

  void ResumeTerm();

  inline bool caughtTerm() const { return sigx; }

  inline void raiseTerm(int s) {
    if (0==sigx) {
      sigx = s;
      if (!catchterm) ResumeTerm();
    }
  }

  /** Used to start reporting an internal error.
        @param  srcfile  Which source (i.e., c++ file) is unhappy
        @param  srcline  Where in c++ source file we are unhappy
        @return true on success.
  */
  virtual bool StartInternal(const char* srcfile, int srcline);

  /** Used to start reporting an error.
        @return true on success.
  */
  virtual bool StartError(const location& L, const char* text);

  /** Used to start reporting a warning.
        @return true on success.
  */
  virtual bool StartWarning(const location& L, const char* text);

  /** Used to start reporting an error.
        @return true on success.
  */
  virtual bool StartError();

  /** Used to start reporting a warning.
        @return true on success.
  */
  virtual bool StartWarning();


  /** Print the causing filename and line number.
      Usually this is called after "StartError()"
      but before displaying the error details.
        @param  fn  File name.  If null, will not be displayed.
        @param  ln  Line number.  If negative, will not be displayed.
  */
  virtual void CausedBy(const char* fn, int ln);

  /** No filename or line number cause for the error.
  */
  virtual void NoCause();

  /** Change the indent level.
      Affects NewLine() methods.
        @param  delta   Amount to add to indentation level.
  */
  virtual void ChangeIndent(int delta);

  /** Used to continue a warning/error on another line.
      This allows us to indent consistently.
      Does nothing if no warning/error report is "active".
  */
  virtual void NewLine();

  /** Used to continue a report on another line.
      Does nothing if no report is "active".
        @param  what  Report "name", for consistency.
  */
  virtual void NewLine(const char* what);

  /** Used to start reporting other information.
        @param  what  Report "name", for consistency.
        @return true on success
  */
  virtual bool StartReport(const char* what);

  /** Done reporting error/warning/other information.
      May safely be called several times.
  */
  virtual void Stop();

  /** Function called when done with an Internal error.
      Default behavior is to call exit(1).
  */
  virtual void Exit();

  /** Function called when there is a fatal error
      in the stream library itself.
      Default behavior is to write the message to stderr,
      then call exit(2).
        @param  msg  A diagnostic message.
  */
  virtual void Panic(const char* msg) const;
};


/** Associate signal catchers with this io environment.

    @param  e   io environment to use for signal catching.
                If 0, signals will not be caught
                (the default before this is called).
                An internal error occurs if you try to assign
                signal catching to more than one io environment.

*/
void CatchSignals(io_environ *e);



#endif

