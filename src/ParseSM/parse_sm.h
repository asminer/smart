
/** \file parse_sm.h

    Minimalist front-end parser for smart language files.

*/

#ifndef PARSE_SM_H
#define PARSE_SM_H

#include <stdio.h>

/// Defined in ExprLib module.
class exprman;

/// Defined in Options module.
class option_manager;

/// Defined in SymTabs module.
class symbol_table;

/** Module for parsing Smart input files.
*/
class parse_module {
public:
  exprman* em;
protected:
  symbol_table* builtins;

  // TO DO: allow "batch" processing, and return a block of statements?

  bool compiler_ready;
  bool lex_temporal_operators;
  bool temporal_operator_option;
  bool minimum_trace_option;
public:
  parse_module(exprman* em);

  inline void SetBuiltins(symbol_table* b) { builtins = b; }
  inline symbol_table* GetBuiltins() const { return builtins; }

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


public:
  inline option_manager* OptMan() { return em->OptMan(); }
  inline const type* FindOWDType(const char* s) const {
    DCASSERT(em);
    return em->findOWDType(s);
  }
  inline modifier FindModif(const char* s) const {
    DCASSERT(em);
    return em->findModifier(s);
  }
  inline void newLine(int delta=0) {
    DCASSERT(em);
    DCASSERT(em->hasIO());
    em->newLine(delta);
  }
  inline void changeIndent(int delta) {
    DCASSERT(em);
    em->changeIndent(delta);
  }
  inline bool startInternal(const char* file, int line) {
    DCASSERT(em);
    if (em->startInternal(file, line)) {
      em->causedBy(where());
      return true;
    }
    return false;
  }
  inline OutputStream& internal() {
    DCASSERT(em);
    DCASSERT(em->hasIO());
    return em->internal();
  }
  inline bool startError() {
    DCASSERT(em);
    if (em->startError()) {
      em->causedBy(where());
      return true;
    }
    return false;
  }
  inline OutputStream& cerr() {
    DCASSERT(em);
    DCASSERT(em->hasIO());
    return em->cerr();
  }
  inline bool startWarning() {
    DCASSERT(em);
    if (em->startWarning()) {
      em->causedBy(where());
      return true;
    }
    return false;
  }
  inline OutputStream& warn() {
    DCASSERT(em);
    DCASSERT(em->hasIO());
    return em->warn();
  }
  inline void stopError() {
    DCASSERT(em);
    if (em->hasIO()) em->stopIO();
  }
  inline void Exit() {
    DCASSERT(em);
    em->Exit();
  }
};

#endif
