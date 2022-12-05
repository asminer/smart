
#include "gen_rg_base.h"

#include "../ExprLib/startup.h"
#include "../ExprLib/engine.h"
#include "../ExprLib/exprman.h"
#include "../ExprLib/mod_inst.h"

#include "../Options/options.h"
#include "../Options/optman.h"

#include "../_Timer/timerlib.h"

// **************************************************************************
// *                                                                        *
// *                       process_generator  methods                       *
// *                                                                        *
// **************************************************************************

reporting_msg process_generator::report;
debugging_msg process_generator::debug;
unsigned process_generator::remove_vanishing;

process_generator::process_generator()
 : subengine()
{
}

process_generator::~process_generator()
{
}

bool process_generator::startGen(const hldsm& mdl, const char* whatproc)
{
  if (report.start()) {
    report << "Generating " << whatproc;
    report.newLine();
    report << "for ";
    if (mdl.Name()) {
      report << "model " << mdl.Name();
    } else {
      mdl.Print(report.stream());
    }
    return true;
  }
  return false;
}

bool process_generator::stopGen(bool err, const hldsm& mdl,
                                const char* pr, const timer &w)
{
  if (report.start()) {
    if (err)  report << "Incomplete ";
    else      report << "Generated  ";
    report << pr << " for ";
    if (mdl.Name()) {
      report << "model " << mdl.Name();
    } else {
      mdl.Print(report.stream());
    }
    report << "\n\t" << w.elapsed_seconds() << " seconds ";
    if (err)  report << "until error\n";
    else      report << "required for generation\n";
    return true;
  }
  return false;
}

bool process_generator::startCompact(const hldsm& mdl, const char* whatproc)
{
  if (report.start()) {
    report << "Finalizing " << whatproc;
    report.newLine();
    report << "for ";
    if (mdl.Name()) {
      report << "model " << mdl.Name();
    } else {
      mdl.Print(report.stream());
    }
    return true;
  }
  return false;
}

bool process_generator
::stopCompact(const char* name, const char* wp, const timer &w, const lldsm* p)
{
  if (report.start()) {
    report << "Finalized  " << wp;
    if (name) {
      report << " for model " << name << "\n";
    } else {
      report << "\n";
    }
    report << "\t" << w.elapsed_seconds();
    report << " seconds required for finalization\n";
    if (p) p->reportMemUsage(em, "\t");
    return true;
  }
  return false;
}

// ******************************************************************
// *                                                                *
// *                                                                *
// *                         Initialization                         *
// *                                                                *
// *                                                                *
// ******************************************************************

class init_procgen : public initializer {
  public:
    init_procgen();
    virtual bool execute();
};
init_procgen the_procgen_initializer;

init_procgen::init_procgen() : initializer("init_procgen")
{
  usesResource("em");
  buildsResource("procgen");
  buildsResource("engtypes");
}

bool init_procgen::execute()
{
  if (0==em)  return false;

  // Initialize options
  process_generator::report.initialize(em->OptMan(), "procgen",
    "When set, process generation performance is reported."
  );

  process_generator::debug.initialize(em->OptMan(), "procgen",
    "When set, process generation details are displayed."
  );

  engtype* ProcessGeneration = MakeEngineType(em,
      "ProcessGeneration",
      "Algorithm to use to generate the underlying process",
      engtype::Model
  );
  engine* ExplicitProcessGeneration = new engine(
      "EXPLICIT",
      "Explicit process generation"
  );
  RegisterEngine(ProcessGeneration, ExplicitProcessGeneration);
  engine* ExplicitProcessGenerationCOV = new engine(
      "EXPLICITCOV",
      "Explicit process generation"
  );
  RegisterEngine(ProcessGeneration, ExplicitProcessGenerationCOV);
  /*
    Vanishing elimiation styles - as an option
  */
  process_generator::remove_vanishing = process_generator::BY_SUBGRAPH;
  if (em->OptMan()) {
    option* rmvan = em->OptMan()->addRadioOption(
      "RemoveVanishing",
      "Method to remove vanishing states",
      2, process_generator::remove_vanishing
    );
    DCASSERT(rmvan);

    rmvan->addRadioButton(
      "BY_PATH",
      "Recursively search vanishing paths until tangibles are reached.",
      process_generator::BY_PATH
    );
    rmvan->addRadioButton(
      "BY_SUBGRAPH",
      "Explore vanishing portions of graph, then eliminate; can handle vanishing cycles.",
      process_generator::BY_SUBGRAPH
    );
  }


  return true;
}
