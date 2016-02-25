
// $Id$

#include "gen_rg_base.h"

#include "../ExprLib/startup.h"
#include "../ExprLib/engine.h"
#include "../ExprLib/exprman.h"
#include "../ExprLib/mod_inst.h"

#include "../Options/options.h"

#include "timerlib.h"

// **************************************************************************
// *                                                                        *
// *                       process_generator  methods                       *
// *                                                                        *
// **************************************************************************

named_msg process_generator::report;
named_msg process_generator::debug;
int process_generator::remove_vanishing;

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

bool process_generator::stopGen(bool err, const char* n, 
                                const char* pr, const timer &w)
{
  if (report.startReport()) {
    if (err)  report.report() << "Incomplete ";
    else      report.report() << "Generated  ";
    if (n) {
      report.report() << pr << " for model " << n << "\n";
    } else {
      report.report() << "\n";
    }
    report.report() << "\t" << w.elapsed_seconds() << " seconds ";
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
  option* debug = em->findOption("Debug");

  process_generator::report.Initialize(report,
    "procgen",
    "When set, process generation performance is reported.",
    false
  );

  process_generator::debug.Initialize(debug,
    "procgen",
    "When set, process generation details are displayed.",
    false
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

  /*
    Vanishing elimiation styles - as an option
  */
  radio_button** rlist = new radio_button*[2];
  rlist[process_generator::BY_PATH] = new radio_button(
    "BY_PATH", 
    "Recursively search vanishing paths until tangibles are reached.",
    process_generator::BY_PATH
  );
  rlist[process_generator::BY_SUBGRAPH] = new radio_button(
    "BY_SUBGRAPH",
    "Explore vanishing portions of graph, then eliminate; can handle vanishing cycles.",
    process_generator::BY_SUBGRAPH
  );
  process_generator::remove_vanishing = process_generator::BY_SUBGRAPH;

  em->addOption(
    MakeRadioOption(
      "RemoveVanishing",
      "Method to remove vanishing states",
      rlist, 2, process_generator::remove_vanishing
    )
  );

  return true;
}

