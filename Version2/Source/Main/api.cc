
// $Id$

#include "api.h"
#include "compile.h"

#include "../Base/api.h"
#include "../Language/api.h"

// These are defined in smart.l
extern char **inputFiles;
extern int numInputFiles;
extern int whichInputFile;
extern char* filename;

void InitLexer(int filecount, char** files)
{
  numInputFiles = filecount;
  inputFiles = files;
  whichInputFile = 1;
  filename = NULL;
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

int smart_main(int argc, char *argv[])
{
  if (argc<2) {
    HelpScreen();
    return 0;
  }

  // Initialize modules

  StartOptions();

  InitBase();
  InitLanguage();
  InitLexer(argc, argv);
  InitCompiler();

  SortOptions();
  Output.flush();

  yyparse();

  smart_exit();
  return 0;
}

