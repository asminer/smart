
/*
    Main program for integer constraint programming.

    Basically, our job is to
      (1)  Initialize modules
      (2)  Process command-line arguments
      (3)  Start the parser
      (4)  Cleanup
*/

#include "../Options/optman.h"
#include "../ExprLib/startup.h"
#include "../ExprLib/exprman.h"
#include "../ParseICP/parse_icp.h"

// #define DEBUG_MSRS

// ============================================================

class first_init : public startup {
  public:
    first_init(exprman* em);
    virtual bool execute();
  private:
    exprman* hold_em;
};

// ============================================================

first_init::first_init(exprman* _em)
 : startup("first_init")
{
  buildsResource("em");
  hold_em = _em;
}

bool first_init::execute()
{
  em = hold_em;
  DCASSERT(em);
  return true;
}

// ============================================================

void InitOptions(option_manager* om)
{
  if (0==om)  return;
  om->addChecklistOption("Report",
    "Switches to control what information, if any, is written to the report stream."
  );
  om->addChecklistOption("Debug",
    "Switches to control what low-level debugging information, if any, is written to the report stream."
  );
}

void SolveMeasures(outputStream& s, parse_module* pm)
{
  pm->Finish();
  traverse_data x(traverse_data::Compute);
  result answer;
  x.answer = &answer;

#ifdef DEBUG_MSRS
  s << "Measures to solve:\n";
  for (int i=0; i<pm->num_measures; i++) {
    s << "\t" << pm->measure_names[i] << " : ";
    if (pm->measure_calls[i])
      pm->measure_calls[i]->Print(s.stream());
    else
      s << "null";
    s << "\n";
  }
#endif

  for (int i=0; i<pm->num_measures; i++) {
    SafeCompute(pm->measure_calls[i], x);
    const type* t = pm->em->SafeType(pm->measure_calls[i]);
    s << pm->measure_names[i] << ": ";
    t->print(s.stream(), answer);
    s << "\n";
  }
  // solve measures here...
}

int main(int argc, const char** argv, const char** env)
{
  //
  // Initialize modules
  //

  // Stream module initialization
    /*
  io_environ myio;
  CatchSignals(&myio);
  DisplayStream& cout = myio.Output;
  */

  // Option module initialization
  option_manager* om = MakeOptionManager();
  InitOptions(om);

  // Expression module initialization
  // exprman* em = Initialize_Expressions(&myio, om);
  exprman* em = Initialize_Expressions(om);

  // Bootstrap startups, and run them
  first_init the_first_init(em);
  if ( ! startup::executeAll() ) {
    internal_error E(__FILE__, __LINE__);
    E << "Deadlock in startups";
    return -1;
  }

  // Parser initialization
  parse_module pm(em);
  pm.Initialize();

  // Finalize
  em->finalize();

  //
  // Process command line, start parser
  //

  outputStream& out = outputStream::globalOut();

  int code = 0;
  if (argc < 2) {
    // ==================================================================
    out << "\nICP version 0.1\n";
    out << "\nSupporting libraries:\n";
    if (em)   em->printLibraryVersions(out.stream());
    else      out << "\nERROR, no expression manager available\n";
    out << "\n";
    out << "Usage : \n";
    out << "icp <file1> <file2> ... <filen>\n";
    out << "      Use the filename `-' to denote standard input\n";
    out << "\n";
    // ==================================================================
  } else {
    code = pm.ParseICPFiles(argv+1, argc-1);
    if (0==code) {
      SolveMeasures(out, &pm);
    }
  }

  //
  // Cleanup
  //
  delete om;
  destroyExpressionManager(em);

  return 0;
}
