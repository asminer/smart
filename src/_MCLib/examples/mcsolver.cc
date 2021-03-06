
// A (hopefully) thin layer of glue between the
// Markov Chain Parsing library and the
// Markov Chain storage and solver library

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include "mcparse.h"
#include "mclib.h"
#include "lslib.h"
#include "graphlib.h"

#include "timerlib.h"

// #define CHECK_CLASSES

// ******************************************************************
// *                         my_timer class                         *
// ******************************************************************

class my_timer : public GraphLib::timer_hook {
  timer watch;
public:
  virtual void start(const char* w) {
    fprintf(stderr, "%30s...", w);
    watch.reset();
  }
  virtual void stop() {
    fprintf(stderr, "  %lf seconds\n", watch.elapsed_seconds());
  }

  static my_timer* getInstance() {
    static my_timer* inst = 0;
    if (0==inst) inst = new my_timer;
    return inst;
  };
};



// ******************************************************************
// *                      dryrun_parser  class                      *
// ******************************************************************

class dryrun_parser : public mc_builder {
protected:
  // switches
  static bool quiet;
  static LS_Options ssopts;
  static my_timer* stopwatch;

protected:
  bool is_discrete;
  long States;
  long SeenArcs;

  timer sw;
public:
  dryrun_parser(FILE* err);

  // set switches
  static inline void setQuiet() { 
    quiet = true; 
  }
  static inline bool  isQuiet() { 
    return quiet; 
  }
  static inline void useTimer() { 
    stopwatch = my_timer::getInstance(); 
  }
  static inline void useMethod(LS_Method m) {
    ssopts.method = m;
  }
  static inline void maxIters(long m) {
    if (m > 0) ssopts.max_iters = m;
  };
  static inline void relaxation(double r) {
    if (r<=0 || r>=2) return;
    ssopts.relaxation = r;
    if (r!=1.0)   ssopts.use_relaxation = 1;
    else          ssopts.use_relaxation = 0;
  }
  static inline void epsilon(double e) {
    if (e>0 && e<1) ssopts.precision = e;
  }

  virtual bool startDTMC(const char*);
  virtual bool startCTMC(const char*);
  virtual bool specifyStates(long ns);
  virtual bool startInitial() { return true; }
  virtual bool addInitial(long state, double weight) { return true; }
  virtual bool doneInitial() { return true; }

  virtual bool startEdges(long num_edges);
  inline virtual bool addEdge(long from, long to, double wt) {
    SeenArcs++;
    return true;
  }

  virtual bool doneEdges();
    
  virtual bool startMeasureCollection(solution_type which, int  time) {
      return true;
  }
  virtual bool startMeasureCollection(solution_type which, double time) {
      return true;
  } 
  virtual bool startMeasure(const char* name) {
      return true;
  }
  virtual bool addToMeasure(long state, double value) {
      return true;
  }
  virtual bool doneMeasure() {
      return true;
  }
  virtual bool doneMeasureCollection() {
      return true;
  }
  virtual bool assertClasses(long nc) {
      return true;
  }
  virtual bool assertAbsorbing(long st) {
      return true;
  }
  virtual bool assertTransient(long s) {
      return true;
  }
  virtual bool startRecurrentAssertion() {
      return true;
  }
  virtual bool assertRecurrent(long s) {
      return true;
  }
  virtual bool doneRecurrentAssertion() {
      return true;
  }
};

bool dryrun_parser::quiet = false;
LS_Options dryrun_parser::ssopts;
my_timer* dryrun_parser::stopwatch = 0;

// ******************************************************************
// *                     dryrun_parser  methods                     *
// ******************************************************************

dryrun_parser::dryrun_parser(FILE* err) : mc_builder(err)
{
  States = 0;
  SeenArcs = 0;
}

bool dryrun_parser::startDTMC(const char* name)
{
  if (name) printf("DTMC %s\n", name);
  is_discrete = true;
  return true;
}

bool dryrun_parser::startCTMC(const char* name)
{
  if (name) printf("CTMC %s\n", name);
  is_discrete = false;
  return true;
}

bool dryrun_parser::specifyStates(long ns)
{
  States = ns;
  sw.reset();
  return (ns>0);
}

bool dryrun_parser::startEdges(long num_edges)
{
  SeenArcs = 0;
  if (!quiet) {
    fprintf(errlog, "Reading ");
    fprintf(errlog, is_discrete ? "DTMC" : "CTMC");
    fprintf(errlog,  " with %ld states", States);
    if (num_edges>0) {
      fprintf(errlog, ", %ld edges", num_edges);
    }
    if (getFilename()) {
      fprintf(errlog, " from file %s", getFilename());
    }
    fprintf(errlog, "\n");
  }
  return true;
}

bool dryrun_parser::doneEdges()
{
  if (!quiet) {
    fprintf(errlog, "Read %ld edges, took %lf seconds\n", SeenArcs, sw.elapsed_seconds());
  }
  return true;
}

// ******************************************************************
// *                      solver_parser  class                      *
// ******************************************************************

class solver_parser : public dryrun_parser {
  GraphLib::dynamic_summable<double>* G;
  MCLib::Markov_chain* mc;   // Markov chain
  GraphLib::node_renumberer* Ren;
  // GraphLib::static_classifier C;
  float* initial;    // initial vector.
  long initsize;    // dimension of initial vector.
  long* initindex;  // indexes (or null) for initial sparse vector.
  LS_Vector p0;    // completed initial vector.
  double* solvector;
  solution_type last_solved;
  double last_dtime;
  int last_ltime;
  int num_msrs;
  double reward;
  bool infinity_reward;
  long first_recurrent;
public:
  solver_parser(FILE* err);
  virtual ~solver_parser();

protected:
  void clearOld();

public:
  virtual bool specifyStates(long ns);

  virtual bool startInitial();
  virtual bool addInitial(long state, double weight);
  virtual bool doneInitial() { return true; }
protected:
  bool finishInitial();

public:
  virtual bool startEdges(long num_edges);
  virtual bool addEdge(long from, long to, double wt);
  virtual bool doneEdges();

protected:
  void fillInit();  // copy initial distribution to solution vector
  bool solveDTMCTransient(int t);
  bool solveCTMCTransient(double t);
  bool solveSteady();
  bool solveAccumulated();
  inline bool makeVector() {
    if (solvector) return true;
    solvector = (double*) malloc(States * sizeof(double));
    if (solvector) return true;
    fprintf(errlog, "Error: couldn't allocate solution vector\n");
    return false;
  }

public:
  virtual bool startMeasureCollection(solution_type which, int  time);
  virtual bool startMeasureCollection(solution_type which, double time);
  virtual bool doneMeasureCollection();
  virtual bool startMeasure(const char* name);
  virtual bool addToMeasure(long state, double value);
  virtual bool doneMeasure();
  virtual bool assertClasses(long nc);
  virtual bool assertAbsorbing(long st);
  virtual bool assertTransient(long st);
  virtual bool startRecurrentAssertion() {
      first_recurrent = -1;
      return true;
  }
  virtual bool assertRecurrent(long s);
  virtual bool doneRecurrentAssertion() {
      return true;
  }
protected:
  inline void failedAssertion() {
    fprintf(errlog, "Assertion failed ");
    if (getFilename()) fprintf(errlog, "in file %s ", getFilename());
    fprintf(errlog, "at line %ld:\n\t", getLinenumber());
  }
};

// ******************************************************************
// *                     solver_parser  methods                     *
// ******************************************************************

solver_parser::solver_parser(FILE* err) : dryrun_parser(err)
{
  G = 0;
  mc = 0;
  Ren = 0;
  initial = 0;
  initsize = 0;
  initindex = 0;
  solvector = 0;
  last_solved = None;
}

solver_parser::~solver_parser()
{
  clearOld();
}

void solver_parser::clearOld()
{
  delete Ren;
  free(initial);
  free(initindex);
  delete mc;
  delete G;
  free(solvector);
  G = 0;
  mc = 0;
  Ren = 0;
  initial = 0;
  initindex = 0;
  initsize = 0;
  solvector = 0;
  last_solved = None;
}


bool solver_parser::specifyStates(long ns)
{
  clearOld();
  if (!dryrun_parser::specifyStates(ns)) return false;
  return startInitial();
}

bool solver_parser::startInitial()
{
  initsize = States;
  initial = (float*) malloc(States * sizeof(float));
  for (long i=0; i<States; i++) initial[i] = 0;
  return true;
}

bool solver_parser::addInitial(long state, double weight)
{
  if ( (state < 0) || (state >= States) ) {
    startError();
    fprintf(errlog, "illegal state index %ld", state);
    doneError();
    return false;
  };
  if (initial[state]) {
    startError();
    fprintf(errlog, "duplicate initial probability for state %ld", state);
    doneError();
    return false;
  }
  initial[state] = weight;
  return true;
}

bool solver_parser::finishInitial()
{
  long truncsize = 0;
  long nnz = 0;
  long i;
  double total = 0.0;

  // reorder the vector
  if (Ren && Ren->changes_something()) {
    bool* fixme = new bool[States];
    for (i=States-1; i>=0; i--)  fixme[i] = (i != Ren->new_number(i));
    for (i=0; i<States; i++) if (fixme[i]) {
      float last = initial[i];
      long p = i;
      while (1) {
        p = Ren->new_number(p);
        SWAP(last, initial[p]);
        fixme[p] = false;
        if (p==i) break;
      } // while 1
    } // for i
    delete[] fixme;
  }

  // Get stats
  for (i=0; i<States; i++) if (initial[i]) {
    nnz++;
    truncsize = i;
    total += initial[i];
  }
  truncsize++;
  for (i=0; i<truncsize; i++)  initial[i] /= total;  // normalize
  if (nnz*2<truncsize) {
    // use sparse encoding
    initindex = (long*) malloc(nnz * sizeof(long));
    nnz = 0;
    for (i=0; i<States; i++) if (initial[i]) {
      initindex[nnz] = i;
      initial[nnz] = initial[i];
      nnz++;
    }
    initial = (float*) realloc(initial, nnz * sizeof(float));
    initsize = nnz;
  } else {
    // truncate
    initial = (float*) realloc(initial, truncsize * sizeof(float));
    initsize = truncsize;
  }
  p0.size = initsize;
  p0.index = initindex;
  p0.d_value = 0;
  p0.f_value = initial;
  return true;
}


bool solver_parser::startEdges(long num_edges)
{
  if (num_edges<0) num_edges = 0;
  G = new GraphLib::dynamic_summable<double> (is_discrete, true);
  if (0==G) {
    fprintf(errlog, "Couldn't start graph\n");
    return false;
  }
  G->addNodes(States);
  return dryrun_parser::startEdges(num_edges);
}

bool solver_parser::addEdge(long fs, long ts, double p)
{
  if (0==G) return false;
  try {
    bool dup = G->addEdge(fs, ts, p);
    if (!dup) return dryrun_parser::addEdge(fs, ts, p);

    fprintf(errlog,  "Warning: summing duplicate edge %ld:%ld ", fs, ts);
    const char* fn = getFilename();
    if (fn) fprintf(errlog, "in file %s ", fn);
    long ln = getLinenumber();
    if (ln>0) fprintf(errlog, "at line %ld", ln);
    fprintf(errlog, "\n");
    return dryrun_parser::addEdge(fs, ts, p);
  }
  catch (MCLib::error e) {
    startError();
    fprintf(errlog, "MCLib: %s", e.getString());
    doneError();
    return false;
  }
}

bool solver_parser::doneEdges()
{
  using namespace MCLib;

  dryrun_parser::doneEdges();
  sw.reset();
  try {
    GraphLib::abstract_classifier* ac = G->determineSCCs(0, 1, true, stopwatch); 
    GraphLib::static_classifier C;
    Ren = ac->buildRenumbererAndStatic(C);
    DCASSERT(Ren);
    delete ac;
    G->renumberNodes(*Ren);
    mc = new Markov_chain(is_discrete, *G, C, stopwatch);
  }
  catch (error e) {
    fprintf(errlog, "Error finishing chain: %s\n", e.getString());
    return false;
  }

  if (!finishInitial()) return false;

  const GraphLib::static_classifier &C = mc->getStateClassification();
  int nc = C.getNumClasses();
  if (!quiet) {
    if (is_discrete)  fprintf(errlog, "DTMC classes:\n");
    else              fprintf(errlog, "CTMC classes:\n");
    for (long i=0; i<nc; i++) {
      if (0==i) fprintf(errlog, "\tTransient:\n");
      else if (1==i) fprintf(errlog, "\tAbsorbing:\n");
      else fprintf(errlog, "\tRecurrent class %ld:\n", i);
      fprintf(errlog, "\t\t%ld states\n", C.sizeOfClass(i));
      if (i>1) fprintf(errlog, "\t\tperiod is %ld\n", mc->computePeriodOfClass(i));
    }
    fprintf(errlog, "Chain finalization took %lf seconds\n", sw.elapsed_seconds());
  } // if !quiet

  return true;
}

void solver_parser::fillInit()
{
  for (long i=0; i<States; i++) solvector[i] = 0.0;
  if (initindex) {
    // sparse
    for (long z=0; z<initsize; z++)
      solvector[initindex[z]] = initial[z];
  } else {
    // truncated full
    for (long z=0; z<initsize; z++)
      solvector[z] = initial[z];
  }
}

bool solver_parser::solveDTMCTransient(int n)
{
  using namespace MCLib;
  if (n<0) return false;
  try {
    Markov_chain::DTMC_transient_options foo;
    if ((Transient == last_solved) && (last_ltime <= n)) {
      if (!quiet) {
        fprintf(errlog,  "Computing distribution at time %d", n);
        fprintf(errlog,  " starting from time %d\n", last_ltime);
      }
      mc->computeTransient(n-last_ltime, solvector, foo);
    } else {
      if (!quiet) {
        fprintf(errlog,  "Computing distribution at time %d\n", n);
      }
      fillInit();
      mc->computeTransient(n, solvector, foo);
    }
    if (!quiet) {
      fprintf(errlog,  "\tSteps: %ld\n", foo.multiplications);
    }
    printf("Time %d:\n", n);
    last_ltime = n;
    last_solved = Transient;
    return true;
  }
  catch (error e) {
    fprintf(errlog,  "Error: %s\n", e.getString());
    return false;
  }
}

bool solver_parser::solveCTMCTransient(double t)
{
  using namespace MCLib;
  if (t<0) return false;
  try {
    Markov_chain::CTMC_transient_options foo;
    foo.ssprec = 1e-8;
    if ((Transient == last_solved) && (last_dtime <= t)) {
      if (!quiet) {
        fprintf(errlog,  "Computing distribution at time %lf", t);
        fprintf(errlog,  " starting from time %lf\n", last_dtime);
      }
      mc->computeTransient(t-last_dtime, solvector, foo);
    } else {
      if (!quiet) {
        fprintf(errlog,  "Computing distribution at time %lf\n", t);
      }
      fillInit();
      mc->computeTransient(t, solvector, foo);
    }
    if (!quiet) {
      fprintf(errlog, "\tq=%lf \tRight=%ld \tSteps=%ld\n",
              foo.q, foo.poisson_right, foo.multiplications);
    }
    printf("Time %lf\n", t );
    last_dtime = t;
    last_solved = Transient;
    return true;
  }
  catch (error e) {
    fprintf(errlog,  "Error: %s\n", e.getString());
    return false;
  }
}

bool solver_parser::solveSteady()
{
  using namespace MCLib;
  if (Steady_state == last_solved) return true;
  try {
    if (!quiet) {
      fprintf(errlog,  "Starting ");
      switch (ssopts.method) {
        case LS_Gauss_Seidel:
            fprintf(errlog,  "Gauss-Seidel");
            break;
        case LS_Row_Jacobi: 
            fprintf(errlog,  "row Jacobi");
            break;
        case LS_Jacobi: 
            fprintf(errlog,  "Jacobi");
            break;
        default:
            fprintf(errlog,  "Unknown solver");
      } // switch
      if (ssopts.use_relaxation) {
        fprintf(errlog,  " with relaxation %lf", ssopts.relaxation);
      }
      fprintf(errlog,  "\n");
    }
    sw.reset();
    LS_Output bar;
    mc->computeInfinityDistribution(p0, solvector, ssopts, bar);
    if (!quiet) {
      fprintf(errlog,  "Done, %ld iterations, ", bar.num_iters);
      fprintf(errlog,  "%lf seconds\n", sw.elapsed_seconds());
    }
    last_solved = Steady_state;
    printf("Steady:\n");
    return true;
  }
  catch (error e) {
    fprintf(errlog,  "Error: %s\n", e.getString());
    return false;
  }
}

bool solver_parser::solveAccumulated()
{
  using namespace MCLib;
  if (Accumulated == last_solved) return true;
  last_solved = Accumulated;

  try {
    sw.reset();
    LS_Output bar;
    mc->computeTTA(p0, solvector, ssopts, bar);
    if (!quiet) {
      fprintf(errlog,  "Done, %ld iterations, ", bar.num_iters);
      fprintf(errlog,  "%lf seconds\n", sw.elapsed_seconds());
    }
    printf("Accumulated:\n");
    return true;
  }
  catch (error e) {
    fprintf(errlog,  "Error: %s\n", e.getString());
    return false;
  }
}

bool solver_parser::startMeasureCollection(solution_type which, int  time)
{
  num_msrs = 0;
  if (!makeVector()) return false;
  switch (which) {
    case Transient:
      if (is_discrete)  return solveDTMCTransient(time);
      else              return solveCTMCTransient(time);

    case Steady_state:  return solveSteady();

    case Accumulated:   return solveAccumulated();

    default:            return false;
  };
  return false;
}

bool solver_parser::startMeasureCollection(solution_type which, double time)
{
  num_msrs = 0;
  if (!makeVector()) return false;
  switch (which) {
    case Transient:
      if (is_discrete) {
        startError();
        fprintf(errlog, "DTMC model requires integer time");
        doneError();
        return false;
      } else {
        return solveCTMCTransient(time);
      }

    case Steady_state:  return solveSteady();

    case Accumulated:   return solveAccumulated();

    default:            return false;
  };
  return false;
}

bool solver_parser::doneMeasureCollection()
{
  if (num_msrs) return true;
  const GraphLib::static_classifier &C = mc->getStateClassification();
  // no measures; display vector
  switch (last_solved) {
    case Transient:
    case Steady_state:
        printf("[");
        for (long i=0; i<States; i++) {
          if (i) printf(", ");
          long j = (Ren) ? Ren->new_number(i) : i;
          printf("%2.8lf", solvector[j]);
        }
        printf("]\n");
        return true;

    case Accumulated:
        printf("[");
        for (long i=0; i<States; i++) {
          if (i) printf(", ");
          long j = (Ren) ? Ren->new_number(i) : i;
          if (C.isNodeInClass(j, 0))  printf("%lf", solvector[j]);
          else                        printf("infinity");
        }
        printf("]\n");
        return true;

    default:
        return false;
  } // switch
  return false;
}

bool solver_parser::startMeasure(const char* name) 
{
  reward = 0;
  infinity_reward = false;
  printf("  %s: ", name);
  num_msrs++;
  return true;
}

bool solver_parser::addToMeasure(long state, double value) 
{
  const GraphLib::static_classifier &C = mc->getStateClassification();
  if (state<0 || state>=States) {
    printf("\n");
    startError();
    fprintf(errlog, "bad state index");
    doneError();
    return false;
  }
  if (0==value) return true;
  long i = (Ren) ? Ren->new_number(state) : state;
  if (Accumulated == last_solved) {
    if (C.isNodeInClass(i, 0))  reward += solvector[i] * value;
    else                        infinity_reward = true;
  } else {
    reward += solvector[i] * value;
  }
  return true;
}

bool solver_parser::doneMeasure() 
{
  if (infinity_reward)  printf("infinity\n"); 
  else                  printf("%lf\n", reward);
  return true;
}

bool solver_parser::assertClasses(long nc)
{
  const GraphLib::static_classifier &C = mc->getStateClassification();
  if (C.getNumClasses() == nc) {
    if (!quiet) fprintf(errlog, "Assertion CLASSES %ld passed\n", nc);
    return true;
  }
  failedAssertion();
  fprintf(errlog, 
    "expected %ld classes, got %ld\n", 
    nc, C.getNumClasses()
  );
  return false;
}

bool solver_parser::assertAbsorbing(long s)
{
  const GraphLib::static_classifier &C = mc->getStateClassification();
  long i = Ren ? Ren->new_number(s) : s;
  if (C.isNodeInClass(i, 1)) {
    if (!quiet) fprintf(errlog, "Assertion ABSORBING %ld passed\n", s);
    return true;
  }
  failedAssertion();
  fprintf(errlog, "state %ld is not absorbing\n", s);
  return false;
}

bool solver_parser::assertTransient(long s)
{
  const GraphLib::static_classifier &C = mc->getStateClassification();
  long i = Ren ? Ren->new_number(s) : s;
  if (C.isNodeInClass(i, 0)) {
    if (!quiet) fprintf(errlog, "Assertion TRANSIENT %ld passed\n", s);
    return true;
  }
  failedAssertion();
  fprintf(errlog, "state %ld is not transient\n", s);
  return false;
}

bool solver_parser::assertRecurrent(long s)
{
  const GraphLib::static_classifier &C = mc->getStateClassification();
  if (first_recurrent>=0) {
    long j = Ren ? Ren->new_number(first_recurrent) : first_recurrent;
    long c = C.classOfNode(j);
    long i = Ren ? Ren->new_number(s) : s;
    if (C.isNodeInClass(i, c)) {
      if (!quiet) fprintf(errlog, "Assertion RECURRENT %ld passed\n", s);
      return true;
    }
    failedAssertion();
    if (C.isNodeInClass(i, 0)) {
      fprintf(errlog, "state %ld is not recurrent\n", s);
    } else {
      fprintf(errlog, "state %ld is in a different recurrent class than %ld\n", s, first_recurrent);
    }
    return false;
  }
  // just check if we're recurrent
  long i = Ren ? Ren->new_number(s) : s;
  if (C.classOfNode(i) < 2) {
    failedAssertion();
    fprintf(errlog, "state %ld is not recurrent\n", s);
    return false;
  }
  if (!quiet) fprintf(errlog, "Assertion RECURRENT %ld passed\n", s);
  first_recurrent = s;
  return true;
}

// ******************************************************************
// *                           front  end                           *
// ******************************************************************


int Usage(const char* name)
{
  fprintf(stderr, "\nMarkov chain solver\n");
  fprintf(stderr, "\t%s\n", GraphLib::Version());
  fprintf(stderr, "\t%s\n", LS_LibraryVersion());
  // fprintf(stderr, "\t%s\n", Old_MCLib::Version());
  fprintf(stderr, "\t%s\n", mc_builder::getParserVersion());
  fprintf(stderr, "\nUsage: %s [-switches] <files>\n", name);
  fprintf(stderr, "\tIf no files are specified, then input is taken from standard input\n");
  fprintf(stderr, "\nSwitches:\n");
  fprintf(stderr, "\t?: Print usage and exit\n");
  fprintf(stderr, "\td: Dry run; parse the input file but do not compute anything\n");
  fprintf(stderr, "\tq: quiet (no diagnostic messages, except errors)\n");
  fprintf(stderr, "\tt: time Markov chain operations\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "\tr: Jacobi by rows\n");
  fprintf(stderr, "\tj: Jacobi by vector-matrix multiply\n");
  fprintf(stderr, "\tg: Gauss-Seidel (by rows)\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "\te x: desired precision (epsilon)\n");
  fprintf(stderr, "\tm x: maximum number of iterations\n");
  fprintf(stderr, "\tw x: relaxation parameter (default 1.0)\n");
  fprintf(stderr, "\n");
  return 1;
}


int main(int argc, char** argv)
{
  const char* name = argv[0];
  // process command line
  int ch;
  bool dry_run = false;
  for (;;) {
    ch = getopt(argc, argv, "?dqtczrjge:m:w:");
    if (ch<0) break;
    switch (ch) {
      case 'd':
          dry_run = true;
          continue;

      case 'q':
          dryrun_parser::setQuiet();
          continue;

      case 't':
          dryrun_parser::useTimer();
          continue;

      case 'r':
          dryrun_parser::useMethod(LS_Row_Jacobi);
          continue;

      case 'j':
          dryrun_parser::useMethod(LS_Jacobi);
          continue;

      case 'g':
          dryrun_parser::useMethod(LS_Gauss_Seidel);
          continue;

      case 'e':
          if (optarg) dryrun_parser::epsilon( atof(optarg) );
          continue;

      case 'm':
          if (optarg) dryrun_parser::maxIters( atoi(optarg) );
          continue;

      case 'w':
          if (optarg) dryrun_parser::relaxation( atof(optarg) );
          continue;

      default:
        return Usage(name);
    } // switch
  }; // infinite loop

  dryrun_parser* foo = dry_run 
                        ? new dryrun_parser(stderr) 
                        : new solver_parser(stderr);
  
  bool ok;
  if (argc <= optind) {
    ok = foo->parse_file(stdin, 0);
    if (!ok) return 1;
  } else {
    for (int i=optind; i<argc; i++) {
      FILE* infile = fopen(argv[i], "r");
      if (0==infile) {
        fprintf(stderr, "Couldn't open file %s\n", argv[i]);
        return 2;
      }
      ok = foo->parse_file(infile, argv[i]);
      if (!ok) return 1;
      fclose(infile);
    }
  }
  foo->done_parsing();
  delete foo;
  return 0;
}
