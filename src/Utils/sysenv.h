
/**
    A centralized object for systemy stuff
    like output, error, reporting streams,
    catching signals, exiting cleanly.
*/

#ifndef SYSENV_H
#define SYSENV_H

#include <fstream>

class system_environ {
        /*
         * Element 0: Output
         * Element 1: Report
         * Element 2: Warning
         * Element 3: Error
         * Element 4: Internal
         */
        ofstream outstream[5];
        /*
         * Index of active error stream, in the outstream array.
         * Use 0 for unused.
         */
        unsigned errindex;

        bool interactive;

    public:
        system_environ();

};



/** Centralized i/o and error reporting class.

    Errors should be reported the following way:
      (1) call StartInternal(), StartError(), or StartWarning()
      (2) directly print any location details, e.g., "in function foo"
      (3) call either CausedBy() or NoCause(), which will finish the line
      (4) directly print any details, with line breaks by calling NewLine().
      (5) call Stop() when done.
*/
class io_environ {
  bool interactive;
  int WhichError;
  int  sigx;
  bool catchterm;
  int indents;
public:
  // InputStream Input;
  DisplayStream Output;
  DisplayStream Report;     // 0
  DisplayStream Warning;    // 1
  DisplayStream Error;      // 2
  DisplayStream Internal;   // 3
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

  // inline bool IsInteractive() const { return interactive; }
  inline void SetInteractive() { interactive = true; }
  inline void SetBatch() { interactive = false; }

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


/** Centralized documentation formatting class.

    Derive a class from this one, to have documentation
    formatted in different, fancy ways (e.g., html, LaTeX).

    The main idea is to dump strings into the formatter,
    using the comfortable OutputStream interface,
    and it will automatically wrap lines around, and
    produce generally pleasing results.

    Future additions, if necessary:
      A tabular environment?
      Font changes?
*/
class doc_formatter {
public:
  doc_formatter();
  virtual ~doc_formatter();

  /// For writing text.
  virtual OutputStream& Out() = 0;

  /// Start a new section, with given name.
  virtual void section(const char* name) = 0;

  /** Signifies the start of a heading.
      This should be called when starting to display
      an "entry", e.g., a function header.
  */
  virtual void begin_heading() = 0;

  /// Signifies the end of a heading.
  virtual void end_heading() = 0;

  /// Increase level of indentation.
  virtual void begin_indent() = 0;

  /// Decrease level of indentation.
  virtual void end_indent() = 0;

  /** Start a "description" environment.
      This is a list of the form:
        item_1  description of item 1
        item_2  description of item 2, possibly so long that
                it is forced to wrap around.

        @param  width  Width (in chars) of the widest "item".
  */
  virtual void begin_description(int width) = 0;

  /** Set the next item in the list.
      Does nothing if we are not within a description environment.
        @param  str  The next item.
  */
  virtual void item(const char* str) = 0;

  /// end a "description" environment.
  virtual void end_description() = 0;

  /// eject the current "page" of text.
  virtual void eject_page() = 0;

  // Not strictly documentation formatting, but it needs to go somewhere...

  /** Determine if a string matches a keyword (for searches).
      Default behavior: check if the string contains the keyword
      (ignoring case).
      Derive a class from this and override if you want other behavior.
  */
  virtual bool Matches(const char* str, const char* keyword) const;
};


/** Build a plain text formatter.
      @param  width  Page width, in characters (80 is "standard").
      @param  out  Ultimately, where to write the text.
*/
doc_formatter* MakeTextFormatter(int width, OutputStream &out);

#endif

