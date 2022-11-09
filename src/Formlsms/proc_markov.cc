
#include "proc_markov.h"

#include "../Options/options.h"
#include "../Options/optman.h"
#include "../Options/opt_enum.h"
#include "../Options/radio_opt.h"

#include "../ExprLib/startup.h"
#include "../ExprLib/mod_inst.h"
#include "../ExprLib/mod_vars.h"
#include "../ExprLib/exprman.h"

// External libs
#include "../_MCLib/mclib.h"
#include "../_LSLib/lslib.h"
#include "../_Timer/timerlib.h"

// ******************************************************************
// *                                                                *
// *                     markov_process statics                     *
// *                                                                *
// ******************************************************************

LS_Options* markov_process::lsopts = 0;
unsigned markov_process::solver;
named_msg markov_process::report;
unsigned markov_process::access = markov_process::BY_COLUMNS;
markov_process::reporter* markov_process::my_timer = 0;

// ******************************************************************
// *                                                                *
// *                     markov_process methods                     *
// *                                                                *
// ******************************************************************

markov_process::markov_process()
{
}

markov_process::~markov_process()
{
}

const LS_Options& markov_process::getSolverOptions()
{
  DCASSERT(lsopts);
  CHECK_RANGE(0, solver, NUM_SOLVERS);
  // fix the option values that are not automatically linked
  lsopts[solver].use_relaxation = (lsopts[solver].relaxation != 1.0);
  return lsopts[solver];
}

void markov_process::startTransientReport(timer& watch, double t) const
{
  if (!report.startReport()) return;
  report.report() << "Starting transient solver, t=" << t << "\n";
  report.stopIO();
  watch.reset();
}

void markov_process::stopTransientReport(timer& watch, long iters) const
{
  if (!report.startReport()) return;
  report.report() << "Transient solver: ";
  report.report() << watch.elapsed_seconds() << " seconds, ";
  report.report() << iters << " iterations\n";
  report.stopIO();
}

void markov_process::startSteadyReport(timer& watch) const
{
  if (!report.startReport()) return;
  report.report() << "Solving steady-state distribution using ";
  report.report() << getSolver() << "\n";
  report.stopIO();
  watch.reset();
}

void markov_process::stopSteadyReport(timer& watch, long iters) const
{
  if (!report.startReport()) return;
  report.report() << "Solved  steady-state distribution\n";
  report.report() << "\t" << watch.elapsed_seconds() << " seconds";
  report.report() << " required for " << getSolver() << "\n";
  if (iters > 0) {
    report.report() << "\t" << iters << " iterations";
    report.report() << " required for " << getSolver() << "\n";
  }
  em->stopIO();
}

void markov_process::startTTAReport(timer& watch) const
{
  if (!report.startReport()) return;
  report.report() << "Solving time to absorption using ";
  report.report() << getSolver() << "\n";
  report.stopIO();
  watch.reset();
}

void markov_process::stopTTAReport(timer& watch, long iters) const
{
  if (!report.startReport()) return;
  report.report() << "Solved  time to absorption\n";
  report.report() << "\t" << watch.elapsed_seconds() << " seconds";
  report.report() << " required for " << getSolver() << "\n";
  if (iters > 0) {
    report.report() << "\t" << iters << " iterations";
    report.report() << " required for " << getSolver() << "\n";
  }
  report.stopIO();
}

void markov_process::startAccumulatedReport(timer& watch, double t) const
{
  if (!report.startReport()) return;
  report.report() << "Starting accumulated solver, t=" << t << "\n";
  report.stopIO();
  watch.reset();
}

void markov_process::stopAccumulatedReport(timer& watch, long iters) const
{
  if (!report.startReport()) return;
  report.report() << "Accumulated solver: ";
  report.report() << watch.elapsed_seconds() << " seconds, ";
  report.report() << iters << " iterations\n";
  report.stopIO();
}

void markov_process::startRevTransReport(timer& watch, double t) const
{
  if (!report.startReport()) return;
  report.report() << "Starting reverse transient solver, t=" << t << "\n";
  report.stopIO();
  watch.reset();
}

void markov_process::stopRevTransReport(timer& watch, long iters) const
{
  if (!report.startReport()) return;
  report.report() << "Reverse transient solver: ";
  report.report() << watch.elapsed_seconds() << " seconds, ";
  report.report() << iters << " iterations\n";
  report.stopIO();
}

void markov_process::startReachAcceptReport(timer& watch) const
{
  if (!report.startReport()) return;
  report.report() << "Solving `reaches accepting' probabilities using ";
  report.report() << getSolver() << "\n";
  report.stopIO();
  watch.reset();
}

void markov_process::stopReachAcceptReport(timer& watch, long iters) const
{
  if (!report.startReport()) return;
  report.report() << "Solved  `reaches accepting' probabilities\n";
  report.report() << "\t" << watch.elapsed_seconds() << " seconds";
  report.report() << " required for " << getSolver() << "\n";
  if (iters > 0) {
    report.report() << "\t" << iters << " iterations";
    report.report() << " required for " << getSolver() << "\n";
  }
  em->stopIO();
}


const char* markov_process::getSolver()
{
  switch (solver) {
    case GAUSS_SEIDEL:    return "Gauss-Seidel";
    case JACOBI:          return "Jacobi";
    case ROW_JACOBI:      return "Row Jacobi";
    default:              return "unknown solver";
  }
}

// ******************************************************************
// *                                                                *
// *                markov_process::reporter methods                *
// *                                                                *
// ******************************************************************

markov_process::reporter::reporter(const exprman* The_em)
: GraphLib::timer_hook()
{
  em = The_em;
  option* parent = em ? em->findOption("Report") : 0;
  if (parent) parent->addChecklistItem(
    "mc_finish",
    "When set, performance details for Markov chain finalization steps are reported.",
    report, false
  );
}

markov_process::reporter::~reporter()
{
}

void markov_process::reporter::start(const char* w)
{
  if (!report.startReport()) {
    DCASSERT(0);
    return;
  }

  report.report() << w;
  long written = strlen(w);
  report.report().Pad('.', 30-written);
  watch.reset();
}

void markov_process::reporter::stop()
{
  report.report() << " " << watch.elapsed_seconds() << " seconds\n";
  report.stopIO();
}



// ******************************************************************
// *                                                                *
// *                          mc_lib class                          *
// *                                                                *
// ******************************************************************

class mc_lib : public library {
public:
  mc_lib() : library(false, false) { }
  virtual const char* getVersionString() const {
    return MCLib::Version();
  }
  virtual bool hasFixedPointer() const {
    return true;
  }
};

// ******************************************************************
// *                                                                *
// *                          ls_lib class                          *
// *                                                                *
// ******************************************************************

class ls_lib : public library {
public:
  ls_lib() : library(false, false) { }
  virtual const char* getVersionString() const {
    return LS_LibraryVersion();
  }
  virtual bool hasFixedPointer() const {
    return true;
  }
};

// ******************************************************************
// *                                                                *
// *                                                                *
// *                         Initialization                         *
// *                                                                *
// *                                                                *
// ******************************************************************

class init_markovproc : public initializer {
  public:
    init_markovproc();
    virtual bool execute();

    option_manager* makeSubsettings(unsigned i, bool auxvectors);
};
init_markovproc the_markovproc_initializer;

init_markovproc::init_markovproc() : initializer("init_markovproc")
{
  usesResource("em");
}

bool init_markovproc::execute()
{
  if (0==em)  return false;

  static mc_lib* mcl = 0;
  static ls_lib* lsl = 0;

  if (0==mcl) {
    mcl = new mc_lib;
    em->registerLibrary(mcl);
  }
  if (0==lsl) {
    lsl = new ls_lib;
    em->registerLibrary(lsl);
  }

  if (0==markov_process::my_timer) {
    markov_process::my_timer = new markov_process::reporter(em);
  }

  DCASSERT(0==markov_process::lsopts);

  markov_process::lsopts = new LS_Options[markov_process::NUM_SOLVERS];
  markov_process::lsopts[markov_process::GAUSS_SEIDEL].method = LS_Gauss_Seidel;
  markov_process::lsopts[markov_process::JACOBI].method = LS_Jacobi;
  markov_process::lsopts[markov_process::ROW_JACOBI].method = LS_Row_Jacobi;

  markov_process::solver = markov_process::GAUSS_SEIDEL;

  //
  // Set up the radio buttons for the solvers
  //
  if (em->OptMan()) {
    option* solvers = em->OptMan()->addRadioOption(
      "MCSolver",
      "Numerical method to use for solving linear systems during Markov chain analysis.",
      3, markov_process::solver
    );

    option_enum* currsolv = solvers->addRadioButton(
      "GAUSS_SEIDEL", "Gauss-Seidel",
      markov_process::GAUSS_SEIDEL
    );
    currsolv->makeSettings(
            makeSubsettings(markov_process::GAUSS_SEIDEL, false)
    );

    currsolv = solvers->addRadioButton(
      "JACOBI", "Jacobi, using matrix-vector multiply",
      markov_process::JACOBI
    );
    currsolv->makeSettings(
            makeSubsettings(markov_process::JACOBI, true)
    );

    currsolv = solvers->addRadioButton(
      "ROW_JACOBI", "Jacobi, visiting one matrix row at a time",
      markov_process::ROW_JACOBI
    );
    currsolv->makeSettings(
            makeSubsettings(markov_process::ROW_JACOBI, true)
    );


    option* mcby = em->OptMan()->addRadioOption("MCAccessBy",
      "Specifiy initial storage method for Markov chains: by rows (required for simulations) or by columns (required for certain linear solvers).",
      2, markov_process::access
    );

    mcby->addRadioButton(
      "COLUMNS",
      "Access to columns",
      markov_process::BY_COLUMNS
    );
    mcby->addRadioButton(
      "ROWS",
      "Access to rows",
      markov_process::BY_ROWS
    );
  }

  markov_process::solver = markov_process::GAUSS_SEIDEL;
  markov_process::access = markov_process::BY_COLUMNS;

  option* report = em->findOption("Report");
  if (report) report->addChecklistItem(
      "mc_solve",
      "When set, Markov chain solution performance is reported.",
      markov_process::report, false
  );

  return true;
}

option_manager* init_markovproc::makeSubsettings(unsigned i, bool auxvectors)
{
#ifdef DEBUG_NUMERICAL_ITERATIONS
    markov_process::lsopts[i].debug = true;
#endif
    option_manager* settings = MakeOptionManager();
    settings->addIntOption(
        "MinIters",
        "Minimum number of iterations.  Guarantees that at least this many iterations will occur.",
        markov_process::lsopts[i].min_iters, 0, 2000000000
    );
    markov_process::lsopts[i].min_iters = 10;

    settings->addIntOption(
        "MaxIters",
        "Maximum number of iterations.  Once the minimum number of iterations has been reached, the solver will terminate if either the termination criteria has been met (see options for Precision), or the maximum number of iterations has been reached.",
        markov_process::lsopts[i].max_iters, 0, 2000000000
    );
    markov_process::lsopts[i].max_iters = 5000;

    settings->addRealOption(
        "Precision",
        "Desired precision.  Solvers will run until each solution vector element has changed less than epsilon.  Relative or absolute precision may be used, see option TBD.",
        markov_process::lsopts[i].precision,
        true, false, 0.0,
        true, false, 1.0
    );
    markov_process::lsopts[i].precision = 1e-5;


    settings->addRealOption(
        "Relaxation",
        "Relaxation parameter to use (or start with).",
        markov_process::lsopts[i].relaxation,
        true, false, 0.0,
        true, false, 2.0
    );
    markov_process::lsopts[i].relaxation = 0.98;

    //
    // Option for auxiliary vectors, only applies to solvers
    // that use auxiliary vectors like JACOBI
    //
    if (auxvectors) {

      settings->addBoolOption(
          "FloatsForAuxVectors",
          "Should we use floats (instead of doubles) for any auxiliary solution vectors.",
          markov_process::lsopts[i].float_vectors
      );
    }
    markov_process::lsopts[i].float_vectors = false;

    // That's all
    settings->DoneAddingOptions();

    return settings;
}
