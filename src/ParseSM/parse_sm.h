
/** \file parse_sm.h

    Minimalist front-end parser for smart language files.

*/

#ifndef PARSE_SM_H
#define PARSE_SM_H

#include <stdio.h>

/** Module for parsing Smart input files.
*/
class parse_module {
protected:
  // TO DO: allow "batch" processing, and return a block of statements?

  bool compiler_ready;
  bool lex_temporal_operators;

  // TBD: move these to trace sources
  bool temporal_operator_option;
  bool minimum_trace_option;
public:
  parse_module();

  /** Initialize compiler.
      Call this after the environment is set, but before
      invoking the Parser.
  */
  void Initialize();

  /** Parse several input files, in order.
        @param  env     Environment for parser to use.
                        If NULL, errors and warnings will either
                        continue or fail silently.
        @param  files   Array of input filenames.
        @param  count   Number of input filenames.
        @return 0 on success?
  */
  int ParseSmartFiles(const char** files, int count);

  /** Parse a single file.
      This allows for fancy-pants stuff, since the
      input file could be built from a file descriptor.
      In other words, if you want send an input file via
      a pipe or a socket, you can use this one by
      building the input file appropriately :^)
        @param  file  "file" to read from.
        @param  name  Filename string to use for errors.
                      Use "-" to display "standard input".
                      Use a string with a leading space to
                      avoid printing "file" in error messages.
        @return 0 on success?
  */
  int ParseSmartFile(FILE* file, const char* name);

  const location& where() const;

  // Are we lexing the temporal operators?
  inline bool scanForTemporal() const {
    return lex_temporal_operators && temporal_operator_option;
  }

  inline void stopLexingTemporal() {
    lex_temporal_operators = false;
  }

  inline void startLexingTemporal() {
    lex_temporal_operators = true;
  }

  inline bool computeMinimumTrace() const {
    return temporal_operator_option & minimum_trace_option;
  }


};

#endif
