
// $Id$

#include "gen_rg_base.h"

#include "../ExprLib/engine.h"
#include "../ExprLib/exprman.h"
#include "../ExprLib/mod_inst.h"
#include "../Timers/timers.h"


// **************************************************************************
// *                                                                        *
// *                       process_generator  methods                       *
// *                                                                        *
// **************************************************************************

named_msg process_generator::report;
named_msg process_generator::debug;

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
                                const char* pr, const timer* w)
{
  if (report.startReport()) {
    double elapsed = (w) ? w->elapsed() : 0.0;
    if (err)  report.report() << "Incomplete ";
    else      report.report() << "Generated  ";
    if (n) {
      report.report() << pr << " for model " << n << "\n";
    } else {
      report.report() << "\n";
    }
    if (w) {
      report.report() << "\t" << elapsed << " seconds ";
      if (err)  report.report() << "until error\n";
      else      report.report() << "required for generation\n";
    }
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
::stopCompact(const char* name, const char* wp, const timer* w, const lldsm* p)
{
  if (report.startReport()) {
    double elapsed = (w) ? w->elapsed() : 0.0;
    report.report() << "Finalized  " << wp;
    if (name) {
      report.report() << " for model " << name << "\n";
    } else {
      report.report() << "\n";
    }
    if (w) {
      report.report() << "\t" << elapsed;
      report.report() << " seconds required for finalization\n";
    }
    if (p) p->reportMemUsage(em, "\t");
    return true;
  } 
  return false;
}

// **************************************************************************
// *                                                                        *
// *                               Front  end                               *
// *                                                                        *
// **************************************************************************

void InitializeProcGen(exprman* em)
{
  if (0==em)         return;

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
}

