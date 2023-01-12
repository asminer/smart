
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
#include "../Utils/textfmt.h"
#include "../Options/optman.h"

#include "../Utils/library.h"
#include "../Utils/initializer.h"
#include "../Utils/env.h"

#include "../ExprLib/functions.h"
#include "../ExprLib/values.h"
#include "../ExprLib/symb_tab.h"

#include "../ParseSM/parse_sm.h"

#include "../include/revision.h"

// ============================================================

class cmdline_error : public error_msg {
    public:
        cmdline_error();
};

cmdline_error::cmdline_error() : error_msg("ERROR")
{
    Out << ' ' << location::CMDLINE();
    newLine();
}

// ============================================================

class smart_init : public initializer {
        const char** env;
    public:
        smart_init(const char** env);
        static const char* getVersionString();
        static const char* getLongName();
    protected:
        virtual void execute();
};

// ============================================================

smart_init::smart_init(const char** _env) : initializer(__FILE__, 1, 0)
{
    env = _env;
    builds_resource(0, "env");
}

const char* smart_init::getVersionString()
{
    static char* version = 0;
    if (0==version) {
        std::stringstream str;
        str << "SMART";
#ifdef VERSION
        str << " version " << VERSION;
#endif
#ifdef DEVELOPMENT_CODE
        str << " (" << long(8*sizeof(void*)) << "-bit devel.)";
#else
        str << " (" << long(8*sizeof(void*)) << "-bit)";
#endif
        version = strdup(str.str().c_str());
    }
    return version;
}

const char* smart_init::getLongName()
{
    return "Stochastic Model-checking Analyzer for Reliability and Timing";
}

void smart_init::execute()
{
    set_object(0, new environ(getVersionString(), env), "env");
}

// ============================================================

int Usage()
{
    outputStream &out = outputStream::globalOut();
    out << "\n" << smart_init::getVersionString() << "\n";
    out << "\nSupporting libraries:\n";
    library::printLibraryVersions(out.stream());
    out << "\n";
    out << "Usage : \n";
    out << "smart <file1> <file2> ... <filen>\n";
    out << "      Use the filename `-' to denote standard input\n";
    out << "\n";
    out << "For full copyright information, type `smart -c'\n";
    out << "For help, view documentation with `smart -h keywords'\n";
    out << "\n";
    return 0;
}

int Copyrights()
{
    doc_formatter df(80, outputStream::globalOut());
    df.Out() << "\n";
    df.begin_heading();
    df.Out() << smart_init::getVersionString();
    if (SMART_DATE) {
        df.Out() << ", released " << SMART_DATE << "\n";
    }
    df.end_heading();
    df.begin_indent();
    df.Out() << smart_init::getLongName() << "\n";
    df.Out() << "Copyright (C) 2017-2018, Gianfranco Ciardo and Andrew Miner\n";
    df.Out() << "Released under the Apache License, version 2\n";
#ifdef PACKAGE_URL
    df.Out() << PACKAGE_URL << "\n";
#endif
    df.end_indent();
    library::printLibraryCopyrights(df);
    df.Out() << "\n";
    return 0;
}


int CmdLineHelp(const char** argv, int argc)
{
  symbol* help = symbol_table::findGlobal("help");
  if (0==help) {
    cmdline_error E;
    E << "No online help found\n";
    return 1;
  }
  if (help->Next()) {
    cmdline_error E;
    E << "Overloaded online help\n";
    return 1;
  }
  function* hf = smart_cast<function*>(help);
  DCASSERT(hf);
  result keyword;
  DCASSERT(type::find("string"));

  traverse_data x(traverse_data::Compute);

  if (0==argc) {
    type::find("string")->assignFromString(keyword, "");
    expr* foo = new value(location::NOWHERE(), type::find("string"), keyword);
    hf->Compute(x, &foo, 1);
    Delete(foo);
  } else {
    for (int i=0; i<argc; i++) {
      type::find("string")->assignFromString(keyword, argv[i]);
      expr* foo = new value(location::NOWHERE(), type::find("string"), keyword);
      hf->Compute(x, &foo, 1);
      Delete(foo);
    }
  }

  outputStream::globalOut() << "\n";
  return 0;
}

int process_args(parse_module& pm, int argc, const char** argv)
{
  if (argc < 2)
    return Usage();

  if (argc == 2 && argv[1][0] == '-' && argv[1][1] == 'c' && argv[1][2] == 0)
    return Copyrights();

  if (argv[1][0] == '-' && argv[1][1] == 'h' && argv[1][2] == 0)
    return CmdLineHelp(argv+2, argc-2);

  if (argv[1][0] == '-' && argv[1][1] == '?' && argv[1][2] == 0) {
    cmdline_error E;
    E << "Help system is now -h\n";
    return 1;
  }

  return pm.ParseSmartFiles(argv+1, argc-1);
}

int main(int argc, const char** argv, const char** env)
{
  // Run initializers
  static smart_init the_smart_init(env);
  initializer::execute_all(true);

  // Parser initialization
  parse_module pm;
  pm.Initialize();

  // Done adding types
  type::finalizeRegistry();

  // Process command line, start parser
  int code = process_args(pm, argc, argv);

  return code;
}
