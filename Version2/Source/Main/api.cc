
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

void ShowVersion()
{
  Output << "SMART version " << _VERSION << "\n";
  Output.flush();
}

void HelpScreen()
{
  Output << "Usage : \n";
  Output << "smart <file1> <file2> ... <filen>\n";
  Output << "      Use the filename `-' to denote standard input\n";
  Output << "\n";  
  Output.flush();
}


int smart_main(int argc, char *argv[])
{
  ShowVersion();
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

  Output << "Done.\n";
  Output.flush();

  return 0;
}

