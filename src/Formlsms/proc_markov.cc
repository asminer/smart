
#include "proc_markov.h"
#include "../Options/options.h"
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
int markov_process::solver;
named_msg markov_process::report;
int markov_process::access = markov_process::BY_COLUMNS;
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
: GraphLib::generic_graph::timer_hook() 
{
  em = The_em;
  option* parent = em ? em->findOption("Report") : 0;
  report.Initialize(parent, 
    "mc_finish",
    "When set, performance details for Markov chain finalization steps are reported.",
    false
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
  mc_lib() : library(false) { }
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
  ls_lib() : library(false) { }
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

  //
  // Set up the radio buttons for the solvers
  //
  radio_button** solvers = new radio_button*[markov_process::NUM_SOLVERS];

  solvers[markov_process::GAUSS_SEIDEL] = new radio_button(
      "GAUSS_SEIDEL", "Gauss-Seidel", markov_process::GAUSS_SEIDEL
  );
  solvers[markov_process::JACOBI] = new radio_button(
      "JACOBI", "Jacobi, using matrix-vector multiply", 
      markov_process::JACOBI
  );
  solvers[markov_process::ROW_JACOBI] = new radio_button(
      "ROW_JACOBI", "Jacobi, visiting one matrix row at a time", 
      markov_process::ROW_JACOBI
  );
   
  markov_process::solver = markov_process::GAUSS_SEIDEL;
  em->addOption(
    MakeRadioOption(
      "MCSolver",
      "Numerical method to use for solving linear systems during Markov chain analysis.",
      solvers, 3, markov_process::solver
    )
  );
  
  //
  // Add settings for each solver radio button (cool, huh?)
  //
  for (int i=0; i<markov_process::NUM_SOLVERS; i++) {
#ifdef DEBUG_NUMERICAL_ITERATIONS
    markov_process::lsopts[i].debug = true;
#endif
    option_manager* settings = MakeOptionManager();
    settings->AddOption(
      MakeIntOption(
        "MinIters", 
        "Minimum number of iterations.  Guarantees that at least this many iterations will occur.",
        markov_process::lsopts[i].min_iters, 0, 2000000000
      )
    );

    settings->AddOption(
      MakeIntOption(
        "MaxIters", 
        "Maximum number of iterations.  Once the minimum number of iterations has been reached, the solver will terminate if either the termination criteria has been met (see options for Precision), or the maximum number of iterations has been reached.",
        markov_process::lsopts[i].max_iters, 0, 2000000000
      )
    );

    settings->AddOption(
      MakeRealOption(
        "Precision", 
        "Desired precision.  Solvers will run until each solution vector element has changed less than epsilon.  Relative or absolute precision may be used, see option TBD.",
        markov_process::lsopts[i].precision, 
        true, false, 0.0,
        true, false, 1.0
      )
    );

    settings->AddOption(
      MakeRealOption(
        "Relaxation", 
        "Relaxation parameter to use (or start with).",
        markov_process::lsopts[i].relaxation, 
        true, false, 0.0,
        true, false, 2.0
      )
    );

    // 
    // Option for auxiliary vectors, only applies to solvers
    // that use auxiliary vectors like JACOBI
    //
    if ( (markov_process::JACOBI == i) || (markov_process::ROW_JACOBI == i) ) {

      markov_process::lsopts[i].float_vectors = false;
      settings->AddOption(
        MakeBoolOption(
          "FloatsForAuxVectors",
          "Should we use floats (instead of doubles) for any auxiliary solution vectors.",
          markov_process::lsopts[i].float_vectors
        ) 
      );
    }

    // put these settings into the radio button

    settings->DoneAddingOptions();
    solvers[i]->makeSettings(settings);

    // Memory leak, because we never clean up settings, but probably ok
  } // for i





  option* report = em->findOption("Report");
  markov_process::report.Initialize(report,
      "mc_solve",
      "When set, Markov chain solution performance is reported.",
      false
  );


  radio_button** alist = new radio_button*[2];
  alist[markov_process::BY_COLUMNS] = new radio_button(
      "COLUMNS", 
      "Access to columns", 
      markov_process::BY_COLUMNS
  );
  alist[markov_process::BY_ROWS] = new radio_button(
      "ROWS", 
      "Access to rows", 
      markov_process::BY_ROWS
  );
  em->addOption(
    MakeRadioOption("MCAccessBy",
      "Specifiy initial storage method for Markov chains: by rows (required for simulations) or by columns (required for certain linear solvers).",
      alist, 2, markov_process::access
    )
  );
  
  return true;
}

