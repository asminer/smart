
/** \file parse_icp.h

    Minimalist front-end parser for ICP language files.

*/

#ifndef PARSE_ICP_H
#define PARSE_ICP_H

#include <stdio.h>

class expr;

/** Module for parsing ICP input files.
*/
class parse_module {
public:
  bool compiler_ready;

public:
  int num_measures;
  char** measure_names;
  expr** measure_calls;

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
  int ParseICPFiles(const char** files, int count);

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

        @return  0 on success?
  */
  int ParseICPFile(FILE* file, const char* name);

  void Finish();

  const location& where() const;
};

#endif
