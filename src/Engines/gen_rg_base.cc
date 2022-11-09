
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
  if (report.startReport()) {
    report.report() << "Generating " << whatproc;
    report.newLine();
    report.report() << "for ";
    if (mdl.Name()) {
      report.report() << "model " << mdl.Name();
    } else {
      mdl.Print(report.report(), 0);
    }
    return true;
  }
  return false;
}

bool process_generator::stopGen(bool err, const hldsm& mdl,
                                const char* pr, const timer &w)
{
  if (report.startReport()) {
    if (err)  report.report() << "Incomplete ";
    else      report.report() << "Generated  ";
    report.report() << pr << " for ";
    if (mdl.Name()) {
      report.report() << "model " << mdl.Name();
    } else {
      mdl.Print(report.report(), 0);
    }
    report.report() << "\n\t" << w.elapsed_seconds() << " seconds ";
    if (err)  report.report() << "until error\n";
    else      report.report() << "required for generation\n";
    return true;
  }
  return false;
}

bool process_generator::startCompact(const hldsm& mdl, const char* whatproc)
{
  if (report.startReport()) {
    report.report() << "Finalizing " << whatproc;
    report.newLine();
    report.report() << "for ";
    if (mdl.Name()) {
      report.report() << "model " << mdl.Name();
    } else {
      mdl.Print(report.report(), 0);
    }
    return true;
  }
  return false;
}

bool process_generator
::stopCompact(const char* name, const char* wp, const timer &w, const lldsm* p)
{
  if (report.startReport()) {
    report.report() << "Finalized  " << wp;
    if (name) {
      report.report() << " for model " << name << "\n";
    } else {
      report.report() << "\n";
    }
    report.report() << "\t" << w.elapsed_seconds();
    report.report() << " seconds required for finalization\n";
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
  option* report = em->findOption("Report");
  if (report) report->addChecklistItem(
    "procgen",
    "When set, process generation performance is reported.",
    process_generator::report
  );

  option* debug = em->findOption("Debug");
  if (debug) debug->addChecklistItem(
    "procgen",
    "When set, process generation details are displayed.",
    process_generator::debug
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
