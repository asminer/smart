
// $Id$

#include "api.h"
#include "compile.h"

#include "../Base/api.h"
#include "../Base/docs.h"
#include "../Language/api.h"
#include "../Engines/api.h"

// These are defined in smart.l
extern char **inputFiles;
extern int numInputFiles;
extern int whichInputFile;
extern char* filename;

// in fnlib.cc
extern char** environment; 

void InitLexer(int filecount, char** files)
{
  numInputFiles = filecount;
  inputFiles = files;
  whichInputFile = 1;
  filename = NULL;

  // Add lexer-related documentation
  AddTopic("#include", "A source file can include other source files using the #include preprocessing directive, as in C.  An #include directive is ignored if it causes a circular dependency.");

  AddTopic("comments", "Source files can contain C and C++ style comments, which are stripped by Smart's lexer.  The rules are:\n  (1) Characters on a line following \"//\" are ignored.\n  (2) Characters between \"/*\" and \"*/\" are ignored.");
}

void HelpScreen()
{
  Output << "SMART version " << _VERSION << "\n";
  Output << "Usage : \n";
  Output << "smart <file1> <file2> ... <filen>\n";
  Output << "      Use the filename `-' to denote standard input\n";
  Output << "\n";  
  Output.flush();
}

void smart_exit()
{
  // we're going to bail
  Output.flush();
}

int smart_main(int argc, char *argv[], char *env[])
{
  if (argc<2) {
    HelpScreen();
    return 0;
  }
  environment = env;

  // Initialize modules

  StartOptions();

  InitBase();
  InitLanguage();
  InitLexer(argc, argv);
  InitCompiler();
  InitEngines();

  SortOptions();
  Output.flush();

  yyparse();

  smart_exit();
  return 0;
}

