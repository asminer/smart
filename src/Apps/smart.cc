
/*
    Main program for Smart.
    
    Basically, our job is to
      (1)  Initialize modules
      (2)  Process command-line arguments
      (3)  Start the parser
      (4)  Cleanup

    Release history:

    3.2: December 2012
            switched everything to autotools
            (autoconf and automake)

    3.1: switched because UCR is using version number 2

    2.1, under development
      Redesign of source to improve modularity.
      Added monolithic "expression manager" to allow registration
      of types, operations, and solution engines at run time;
      different applications can register different things.
  
    2.0, internal release spring 2008
      Re-implementation of nearly everything from version 1.0
*/

#include "config.h"
#include "../Streams/streams.h"
#include "../Options/options.h"
#include "../ExprLib/exprman.h"
#include "../ExprLib/startup.h"
#include "../ExprLib/functions.h"
#include "../SymTabs/symtabs.h"
#include "../ParseSM/parse_sm.h"

#include "../include/revision.h"

// ============================================================

class first_init : public initializer {
  public:
    first_init(exprman* em, symbol_table* st, const char** env);
    virtual bool execute();
    static const char* getVersionString();
  private:
    exprman* hold_em;
    symbol_table* hold_st;
    const char** hold_env;
};

// ============================================================

first_init::first_init(exprman* _em, symbol_table* _st, const char** _env)
 : initializer("first_init")
{
  buildsResource("em");
  buildsResource("st");
  buildsResource("env");
  buildsResource("version");
  hold_em = _em;
  hold_st = _st;
  hold_env = _env;
}

bool first_init::execute()
{
  em = hold_em;
  st = hold_st;
  env = hold_env;
  version = getVersionString();

  DCASSERT(em);
  DCASSERT(st);
  DCASSERT(env);
  DCASSERT(version);
  return true;
}

const char* first_init::getVersionString()
{
  static char* version = 0;
  if (0==version) {
    StringStream str;
    str << "SMART";
#ifdef VERSION
    str << " version " << VERSION;
#endif
#ifdef DEVELOPMENT_CODE
    str << " (" << long(8*sizeof(void*)) << "-bit devel.)";
#else
    str << " (" << long(8*sizeof(void*)) << "-bit)";
#endif
    version = str.GetString();
  }
  return version;
}

// ============================================================

void InitOptions(option_manager* om)
{
  if (0==om)  return;
  om->AddOption( 
    MakeChecklistOption("Report", "Switches to control what reports, if any, are written to the report stream.")
  );
  om->AddOption( 
    MakeChecklistOption("Debug", "Switches to control what low-level debugging information, if any, is written to the report stream.")
  );
  om->AddOption(
    MakeChecklistOption("Warning", "Switches to control which warning messages are displayed and which are suppressed.")
  );
}

int Usage(exprman* em)
{
  if (0==em) return 1;
  DisplayStream& cout = em->cout();
  cout << "\n" << first_init::getVersionString() << "\n";
  cout << "\nSupporting libraries:\n";
  em->printLibraryVersions(cout);
  cout << "\n";
  cout << "Usage : \n";
  cout << "smart <file1> <file2> ... <filen>\n";
  cout << "      Use the filename `-' to denote standard input\n";
  cout << "\n";  
  cout << "For full copyright information, type `smart -c'\n";  
  cout << "For help, view documentation with `smart -h keywords'\n";
  cout << "\n";  
  return 0;
}

int Copyrights(exprman* em)
{
  if (0==em) return 1;
  doc_formatter* df = MakeTextFormatter(80, em->cout());
  df->Out() << "\n";
  df->begin_heading();
  df->Out() << first_init::getVersionString();
  df->end_heading();
  df->begin_indent();
  df->begin_description(15);
  df->item("Design:");
  df->Out() << "Gianfranco Ciardo and Andrew Miner";
  // Uncomment this when we add names to Implementation.
  // df->item("Lead developer:");
  //df->Out() << "Andrew Miner";
  df->item("Implementation:");
  // Add to and alphabetize this list of names.
  df->Out() << "Andrew Miner";
  df->end_description();
  if (SMART_DATE) {
    df->Out() << "Released " << SMART_DATE << "\n";
  }
#ifdef PACKAGE_URL
  df->Out() << PACKAGE_URL << "\n";
#endif
  df->end_indent();
  em->printLibraryCopyrights(df);
  delete df;
  em->cout() << "\n";
  return 0;
}


int CmdLineHelp(exprman* em, symbol_table* st, const char** argv, int argc)
{
  if (0==em) return 1;

  symbol* help = st->FindSymbol("help");
  if (0==help) {
    em->startError();
    em->noCause();
    em->cerr() << "No online help found\n";
    em->stopIO();
    return 1;
  }
  if (help->Next()) {
    em->startError();
    em->noCause();
    em->cerr() << "Overloaded online help\n";
    em->stopIO();
    return 1;
  }
  function* hf = smart_cast<function*>(help);
  DCASSERT(hf);
  result keyword;
  DCASSERT(em->STRING);

  traverse_data x(traverse_data::Compute);

  if (0==argc) {
    em->STRING->assignFromString(keyword, "");
    expr* foo = em->makeLiteral(0, -1, em->STRING, keyword);
    hf->Compute(x, &foo, 1);
    Delete(foo);
  } else {
    for (int i=0; i<argc; i++) {
      em->STRING->assignFromString(keyword, argv[i]);
      expr* foo = em->makeLiteral(0, -1, em->STRING, keyword);
      hf->Compute(x, &foo, 1);
      Delete(foo);
    }
  }

  em->cout() << "\n";
  return 0;
}

int process_args(parse_module& pm, exprman* em, symbol_table* st,
  int argc, const char** argv)
{
  if (argc < 2) 
    return Usage(em);
  
  if (argc == 2 && argv[1][0] == '-' && argv[1][1] == 'c' && argv[1][2] == 0)
    return Copyrights(em);

  if (argv[1][0] == '-' && argv[1][1] == 'h' && argv[1][2] == 0) 
    return CmdLineHelp(em, st, argv+2, argc-2);
  
  if (argv[1][0] == '-' && argv[1][1] == '?' && argv[1][2] == 0) {
    if (0==em) return 1;
    em->startError();
    em->noCause();
    em->cerr() << "Help system is now -h\n";
    em->stopIO();
    return 1;
  }
  
  return pm.ParseSmartFiles(argv+1, argc-1);
}

int main(int argc, const char** argv, const char** env)
{
  io_environ myio;
  CatchSignals(&myio);

  // Options
  option_manager* om = MakeOptionManager();
  InitOptions(om);

  // Expressions
  exprman* em = Initialize_Expressions(&myio, om);

  // Start the symbol table for builtin functions
  symbol_table* st = MakeSymbolTable(); 

  // Bootstrap initializers, and run them
  first_init the_first_init(em, st, env);
  if ( ! initializer::executeAll() ) {
    if (em->startInternal(__FILE__, __LINE__)) {
      em->cerr() << "Deadlock in initializers";
      em->stopIO();
    }
    return -1;
  }

  // Parser initialization
  parse_module pm(em);
  pm.SetBuiltins(st);
  pm.Initialize();

  // finalize expression manager
  em->finalize();

  // Process command line, start parser
  int code = process_args(pm, em, st, argc, argv);

  //
  // Cleanup
  //
  delete om;
  destroyExpressionManager(em);

  return code;
}
