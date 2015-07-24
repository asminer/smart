
// $Id$

#include "mc_llm.h"
#include "../Timers/timers.h"
#include "../Options/options.h"
#include "../ExprLib/mod_inst.h"
#include "../ExprLib/mod_vars.h"
#include "../ExprLib/exprman.h"
#include "../include/heap.h"

#include "../Modules/expl_states.h"
#include "../Modules/statesets.h"
#include "../Modules/statevects.h"


// External libs
#include "mclib.h"
#include "lslib.h"
#include "statelib.h"
#include "intset.h"
#include "rng.h"

// #define DEBUG
// #define DEBUG_EG

// #define DEBUG_NUMERICAL_ITERATIONS

// ******************************************************************
// *                                                                *
// *                      markov_lldsm methods                      *
// *                                                                *
// ******************************************************************

LS_Options* markov_lldsm::lsopts = 0;
int markov_lldsm::solver;
named_msg markov_lldsm::report;
int markov_lldsm::access = markov_lldsm::BY_COLUMNS;


markov_lldsm::markov_lldsm(bool discr) : stochastic_lldsm(discr ? DTMC : CTMC)
{
  watch = makeTimer();
}

markov_lldsm::~markov_lldsm()
{
  doneTimer(watch);
}

const LS_Options& markov_lldsm::getSolverOptions()
{
  DCASSERT(lsopts);
  CHECK_RANGE(0, solver, NUM_SOLVERS);
  // fix the option values that are not automatically linked
  lsopts[solver].use_relaxation = (lsopts[solver].relaxation != 1.0);
  return lsopts[solver];
}

void markov_lldsm::startTransientReport(double t) const
{
  if (!report.startReport()) return;
  report.report() << "Starting transient solver, t=" << t << "\n";
  report.stopIO();
  DCASSERT(watch);
  watch->reset();
}

void markov_lldsm::stopTransientReport(long iters) const
{
  if (!report.startReport()) return;
  DCASSERT(watch);
  report.report() << "Transient solver: ";
  report.report() << watch->elapsed() << " seconds, ";
  report.report() << iters << " iterations\n";
  report.stopIO();
}

void markov_lldsm::startSteadyReport() const
{
  if (!report.startReport()) return;
  report.report() << "Solving steady-state distribution using ";
  report.report() << getSolver() << "\n";
  report.stopIO();
  DCASSERT(watch);
  watch->reset();
}

void markov_lldsm::stopSteadyReport(long iters) const
{
  if (!report.startReport()) return;
  DCASSERT(watch);
  report.report() << "Solved  steady-state distribution\n";
  report.report() << "\t" << watch->elapsed() << " seconds";
  report.report() << " required for " << getSolver() << "\n";
  if (iters > 0) {
    report.report() << "\t" << iters << " iterations";
    report.report() << " required for " << getSolver() << "\n";
  }
  em->stopIO();
}

void markov_lldsm::startTTAReport() const
{
  if (!report.startReport()) return;
  report.report() << "Solving time to absorption using ";
  report.report() << getSolver() << "\n";
  report.stopIO();
  DCASSERT(watch);
  watch->reset();
}

void markov_lldsm::stopTTAReport(long iters) const
{
  if (!report.startReport()) return;
  DCASSERT(watch);
  report.report() << "Solved  time to absorption\n";
  report.report() << "\t" << watch->elapsed() << " seconds";
  report.report() << " required for " << getSolver() << "\n";
  if (iters > 0) {
    report.report() << "\t" << iters << " iterations";
    report.report() << " required for " << getSolver() << "\n";
  }
  report.stopIO();
}

void markov_lldsm::startAccumulatedReport(double t) const
{
  if (!report.startReport()) return;
  report.report() << "Starting accumulated solver, t=" << t << "\n";
  report.stopIO();
  DCASSERT(watch);
  watch->reset();
}

void markov_lldsm::stopAccumulatedReport(long iters) const
{
  if (!report.startReport()) return;
  DCASSERT(watch);
  report.report() << "Accumulated solver: ";
  report.report() << watch->elapsed() << " seconds, ";
  report.report() << iters << " iterations\n";
  report.stopIO();
}

void markov_lldsm::startRevTransReport(double t) const
{
  if (!report.startReport()) return;
  report.report() << "Starting reverse transient solver, t=" << t << "\n";
  report.stopIO();
  DCASSERT(watch);
  watch->reset();
}

void markov_lldsm::stopRevTransReport(long iters) const
{
  if (!report.startReport()) return;
  DCASSERT(watch);
  report.report() << "Reverse transient solver: ";
  report.report() << watch->elapsed() << " seconds, ";
  report.report() << iters << " iterations\n";
  report.stopIO();
}


const char* markov_lldsm::getSolver() 
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
// *                       mc_reporter  class                       *
// *                                                                *
// ******************************************************************

class mc_reporter : public GraphLib::generic_graph::timer {
  const exprman* em;
  named_msg report;
  ::timer* watch;
public:
  mc_reporter(const exprman* The_em);
  virtual ~mc_reporter();
  virtual void start(const char* w);
  virtual void stop();

  inline mc_reporter* switchMe() { return report.isActive() ? this : 0; }
};

mc_reporter::mc_reporter(const exprman* The_em) 
: GraphLib::generic_graph::timer() 
{
  em = The_em;
  option* parent = em ? em->findOption("Report") : 0;
  report.Initialize(parent, 
    "mc_finish",
    "When set, performance details for Markov chain finalization steps are reported.",
    false
  );
  watch = makeTimer();
}

mc_reporter::~mc_reporter()
{
  doneTimer(watch);
}

void mc_reporter::start(const char* w)
{
  if (!report.startReport()) {
    DCASSERT(0);
    return;
  }
  
  report.report() << w;
  long written = strlen(w);
  report.report().Pad('.', 30-written);
  watch->reset();
}

void mc_reporter::stop()
{
  report.report() << " " << watch->elapsed() << " seconds\n";
  report.stopIO();
}


// ******************************************************************
// *                                                                *
// *                                                                *
// *                       explicit_mc  class                       *
// *                                                                *
// * Abstract base class, uses external Markov chain library for MC *
// *                                                                *
// ******************************************************************

class explicit_mc : public markov_lldsm {

    class simple_outedges : public GraphLib::generic_graph::element_visitor {
    public:
      long edges;
      long* to;
      double* weights;
      long edge_alloc;
      simple_outedges(long* t, double* w, long n) {
        edges = 0;
        to = t;
        weights = w;
        edge_alloc = n;
      }
      virtual bool visit(long f, long t, void* label) {
        if (edges < edge_alloc) {
          to[edges] = t;
          weights[edges] = ((float*)label)[0];
        }
        edges++;
        return false;
      }
    };

    class sparse_row_elems : public GraphLib::generic_graph::element_visitor {
      int alloc;
      const long* invmap;
      bool incoming;
      bool overflow;
    public:
      int last;
      long* index;
      double* value;
    public:
      sparse_row_elems(const long* invmap);
      virtual ~sparse_row_elems();

    protected:
      bool Enlarge(int ns);
    public:
      bool buildIncoming(MCLib::Markov_chain* chain, int i);
      bool buildOutgoing(MCLib::Markov_chain* chain, int i);
   
    // for element_visitor
      virtual bool visit(long from, long to, void*);    

    // for heapsort
      inline int Compare(long i, long j) const {
        CHECK_RANGE(0, i, last);
        CHECK_RANGE(0, j, last);
        return SIGN(index[i] - index[j]);
      }

      inline void Swap(long i, long j) {
        CHECK_RANGE(0, i, last);
        CHECK_RANGE(0, j, last);
        SWAP(index[i], index[j]);
        SWAP(value[i], value[j]);
      }
    };

    class pot_visit : public lldsm::state_visitor {
      expr* p;
      intset &pset;
      result tmp;
      bool ok;
    public:
      pot_visit(const hldsm* mdl, expr* _p, intset &ps);
      inline bool isOK() const { return ok; }
      virtual bool visit();
    };

protected:
  long num_states;
  statedist* initial;
  MCLib::Markov_chain* chain;
  static mc_reporter* my_timer;
  friend void InitMCLibs(exprman* em);

public:
  explicit_mc(LS_Vector& init, MCLib::Markov_chain* mc);
  explicit_mc(bool dtmc, long ns);
  virtual ~explicit_mc();

protected:
  void setChain(statedist* init, MCLib::Markov_chain* mc);

public:
  virtual long getNumStates(bool show) const;
  virtual void getReachable(result &ss) const;
  virtual void getPotential(expr* p, result &ss) const;
  virtual long getNumArcs(bool show) const;
  virtual void showInitial() const;
  virtual long getNumClasses(bool show) const;
  virtual void getClass(long cl, intset &statelist) const;
  virtual bool isTransient(long st) const;
  virtual bool isAbsorbing(long st) const;
  virtual bool isDeadlocked(long st) const;

  virtual void findDeadlockedStates(stateset &ss) const;
  virtual bool forward(const intset &p, intset &r) const;
  virtual bool backward(const intset &p, intset &r) const;

  virtual bool isFairModel() const { return true; }
  virtual void getTSCCsSatisfying(stateset &p) const;
  
  virtual statedist* getInitialDistribution() const;
  virtual long getOutgoingWeights(long from, long* to, double* w, long n) const;

  virtual void getInitialStates(result &x) const;
  virtual bool computeTransient(double t, double* p, double* a, double* b) const;
  virtual bool computeAccumulated(double t, const double* p0, double* n,
                                  double* aux, double* aux2) const;
  virtual bool computeSteadyState(double* probs) const;
  virtual bool computeTimeInStates(const double* p0, double* x) const;
  virtual bool computeClassProbs(const double* p0, double* x) const;
  virtual bool dumpDot(OutputStream &s) const;

  virtual bool computeDiscreteTTA(double epsilon, double* &dist, int &N) const;
  virtual bool computeContinuousTTA(double epsilon, double dt, double* &dist, int &N) const;

  virtual bool reachesAcceptBy(double t, double* x) const;

  virtual bool randomTTA(rng_stream &st, long &state, const stateset &final,
                          long maxt, long &elapsed);
  virtual bool randomTTA(rng_stream &st, long &state, const stateset &final,
                          double maxt, double &elapsed);

protected:
  /** Build display order mapping.
      Takes NATURAL, DISCOVERY, LEXICAL into account.
        @param  map   Array for the mapping.
                      On return, map[i] will give the
                      index of the ith state to display.
  */
  virtual void BuildStateMapping(long* map) const = 0;

  /** Display a state.
        @param  s   Stream to write to.
        @param  i   Index of the state to display,
                    according to Markov chain numbering.
  */
  virtual void ShowState(OutputStream &s, long i) const = 0;

  // void reorderInitial(const long* ren);

  inline bool statusOK(const LS_Output &o, const char* who) const {
    switch (o.status) {
      case LS_Success:
          return true;

      case LS_No_Convergence:
          if (em->startWarning()) {
            em->noCause();
            em->warn() << "Markov chain linear solver (for ";
            em->warn() << who << ") did not converge";
            em->stopIO();
          }
          return true;

      case LS_Out_Of_Memory:
          if (em->startError()) {
            em->noCause();
            em->cerr() << "Insufficient memory for Markov chain ";
            em->cerr() << who << " solver";
            em->stopIO();
          }
          return false;

      case LS_Wrong_Format:
          if (em->startError()) {
            em->noCause();
            em->cerr() << "Wrong matrix format for Markov chain linear solver";
            em->stopIO();
          }
          return false;

      default:
          if (em->startInternal(__FILE__, __LINE__)) {
            em->noCause();
            em->internal() << "Unexpected error";
            em->stopIO();
          }
    } // switch
    return false;
  }

  inline bool status(MCLib::error e, const char* who) const {
    switch (e.getCode()) {
      case MCLib::error::Out_Of_Memory:
          if (em->startError()) {
            em->noCause();
            em->cerr() << "Insufficient memory for Markov chain ";
            em->cerr() << who << " solver";
            em->stopIO();
          }
          break;

      case MCLib::error::Wrong_Format:
          if (em->startError()) {
            em->noCause();
            em->cerr() << "Wrong matrix format for Markov chain linear solver";
            em->stopIO();
          }
          break;

      case MCLib::error::Null_Vector:
          if (em->startError()) {
            em->noCause();
            em->cerr() << "Initial probability vector required for Markov chain solver";
            em->stopIO();
          }
          break;

      default:
        if (em->startInternal(__FILE__, __LINE__)) {
            em->noCause();
            em->internal() << "Unexpected error: " << e.getString();
            em->stopIO();
        }
    } // switch
    return false;
  }
};

mc_reporter* explicit_mc::my_timer = 0;

// ******************************************************************
// *             explicit_mc::sparse_row_elems  methods             *
// ******************************************************************

explicit_mc::sparse_row_elems
::sparse_row_elems(const long* im)
{
  alloc = 0;
  last = 0;
  index = 0;
  value = 0;
  invmap = im;
}

explicit_mc::sparse_row_elems::~sparse_row_elems()
{
  free(index);
  free(value);
}

bool explicit_mc::sparse_row_elems::Enlarge(int ns)
{
  if (ns < alloc)   return true;
  if (0==alloc)     alloc = 16;
  while ((alloc < 256) && (alloc <= ns))  alloc *= 2;
  while (alloc <= ns)                     alloc += 256;
  index = (long*) realloc(index, alloc * sizeof(long));
  value = (double*) realloc(value, alloc * sizeof(double));
  if (0==index || 0==value)  return false;
  return true;
}

bool explicit_mc::sparse_row_elems
::buildIncoming(MCLib::Markov_chain* chain, int i)
{
  overflow = false;
  incoming = true;
  last = 0;
  chain->traverseTo(i, *this);
  if (overflow) return false;
  HeapSortAbstract(this, last);
  return true;
}

bool explicit_mc::sparse_row_elems
::buildOutgoing(MCLib::Markov_chain* chain, int i)
{
  overflow = false;
  incoming = false;
  last = 0;
  chain->traverseFrom(i, *this);
  if (overflow) return false;
  HeapSortAbstract(this, last);
  return true;
}

bool explicit_mc::sparse_row_elems
::visit(long from, long to, void* label)
{
  if (last >= alloc) {
    if (!Enlarge(last+1)) {
      overflow = true;
      return true;
    }
  }
  long z;
  if (incoming) z = from;
  else          z = to;
  index[last] = invmap ? invmap[z] : z;
  value[last] = ((float*) label)[0];
  last++;
  return false;
}

// ******************************************************************
// *                 explicit_mc::pot_visit methods                 *
// ******************************************************************

explicit_mc::pot_visit::pot_visit(const hldsm* mdl, expr* _p, intset &ps)
 : state_visitor(mdl), pset(ps)
{
  p = _p;
  p->PreCompute();
  x.answer = &tmp;
  pset.removeAll();
  ok = true;
}

bool explicit_mc::pot_visit::visit()
{
  p->Compute(x);
  if (!tmp.isNormal()) {
    ok = false;
    return true;
  }
  if (tmp.getBool()) pset.addElement(x.current_state_index);
  return false;
}

// ******************************************************************
// *                      explicit_mc  methods                      *
// ******************************************************************

explicit_mc::explicit_mc(LS_Vector& init, MCLib::Markov_chain* mc)
 : markov_lldsm(mc->isDiscrete())
{
  chain = mc;
  initial = new statedist(this, init, 0);
  num_states = chain->getNumStates();
}

explicit_mc::explicit_mc(bool dtmc, long ns) : markov_lldsm(dtmc)
{
  chain = 0;
  initial = 0;
  num_states = ns;
}

explicit_mc::~explicit_mc()
{
  delete chain;
  delete initial;
}

void explicit_mc::setChain(statedist* init, MCLib::Markov_chain* mc)
{
  DCASSERT(0==chain);
  DCASSERT(mc);
  DCASSERT(Type() == (mc->isDiscrete() ? DTMC : CTMC) );
  DCASSERT(mc->getNumStates() == num_states);
  chain = mc;
  initial = init;
}

long explicit_mc::getNumStates(bool show) const
{
  if (!show || !em->hasIO() || 0==num_states)  return num_states;

  if (tooManyStates(num_states, show)) return num_states;
  
  long* map = 0;
  if (NATURAL != display_order) {
    map = new long[num_states];
    BuildStateMapping(map);
  }

  for (long i=0; i<num_states; i++) {
    long mi = map ? map[i] : i; 
    em->cout() << "State " << i << ": ";
    ShowState(em->cout(), mi);
    em->cout() << "\n";
    em->cout().Check();
  } // for i
  delete[] map;
  em->cout().flush();
  return num_states;
}

void explicit_mc::getReachable(result &rs) const
{
  if (0==num_states) {
    rs.setNull();
    return;
  }
  intset* all = new intset(num_states);
  all->addAll();
  rs.setPtr(new stateset(this, all));
}

void explicit_mc::getPotential(expr* p, result &ss) const
{
  if (0==p) {
    ss.setNull();
    return;
  }
  if (0==num_states) {
    ss.setNull();
    return;
  }
  intset* all = new intset(num_states);
  pot_visit pv(GetParent(), p, *all);
  visitStates(pv);
  if (pv.isOK()) {
    ss.setPtr(new stateset(this, all));
  } else {
    delete all;
    ss.setNull();
  }
}

long explicit_mc::getNumArcs(bool show) const
{
  DCASSERT(chain);
  long na = chain->getNumArcs();

  if (!show || !em->hasIO())            return na;
  if (tooManyStates(num_states, show))  return na;
  if (tooManyArcs(na, show))            return na;

  long* map = 0;
  long* invmap = 0;
  if (NATURAL != display_order) {
    map = new long[num_states];
    BuildStateMapping(map);
    invmap = new long[num_states];
    for (long i=0; i<num_states; i++) invmap[map[i]] = i;
  }

  bool by_rows = (graph_display_style != INCOMING);
  const char* row;
  if (display_graph_node_names)   row = by_rows ? "From " : "To ";
  else                            row = by_rows ? "Row " : "Column ";

  switch (graph_display_style) {
    case DOT:
        em->cout() << "digraph mc {\n";
        for (long i=0; i<num_states; i++) {
          long mi = map ? map[i] : i;
          em->cout() << "\ts" << i;
          if (display_graph_node_names) {
            em->cout() << " [label=\"";
            ShowState(em->cout(), mi);
            em->cout() << "\"]";
          }
          em->cout() << ";\n";
          em->cout().Check();
        } // for i
        em->cout() << "\n";
        break;

    case TRIPLES:
        em->cout() << num_states << "\n";
        em->cout() << na << "\n";
        break;

    default:
        em->cout() << "Markov chain:\n";
  }

  sparse_row_elems foo(invmap);

  for (long i=0; i<num_states; i++) {
    long h = map ? map[i] : i;
    CHECK_RANGE(0, h, num_states);
    switch (graph_display_style) {
      case INCOMING:
      case OUTGOING:
          em->cout() << row;
          if (display_graph_node_names)   ShowState(em->cout(), h);
          else                            em->cout() << i;
          em->cout() << ":\n";
          break;
    }

    bool ok;
    if (by_rows)  ok = foo.buildOutgoing(chain, h);
    else          ok = foo.buildIncoming(chain, h);

    // were we successful?
    if (!ok) {
      // out of memory, bail
      if (em->startError()) {
        em->noCause();
        em->cerr() << "Not enough memory to display Markov chain.";
        em->stopIO();
      }
      break;
    }

    // display row/column
    for (long z=0; z<foo.last; z++) {
      em->cout().Put('\t');
      switch (graph_display_style) {
        case DOT:
            em->cout() << "s" << i << " -> s" << foo.index[z];
            em->cout() << " [label=\"" << foo.value[z] << "\"];";
            break;

        case TRIPLES:
            em->cout() << i << " " << foo.index[z] << " " << foo.value[z];
            break;

        default:
            if (display_graph_node_names) {
              long h = map ? map[foo.index[z]] : foo.index[z];
              CHECK_RANGE(0, h, num_states);
              ShowState(em->cout(), h);
            } else {
              em->cout() << foo.index[z];
            }
            em->cout() << " : " << foo.value[z];
      }
      em->cout().Put('\n');
    } // for z

    em->cout().Check();
  } // for i
  delete[] invmap;
  delete[] map;
  if (DOT == graph_display_style) {
    em->cout() << "}\n";
  }
  em->cout().flush();

  return na;
}


void explicit_mc::showInitial() const
{
  // build initial distribution
  double* init = new double[num_states];
  initial->ExportTo(init);

  // build state mapping
  long* map = 0;
  if (NATURAL != display_order) {
    map = new long[num_states];
    BuildStateMapping(map);
  }


  // Display it
  em->cout() << "Initial distribution:\n";

  for (long i=0; i<num_states; i++) {
    long j = map ? map[i] : i;
    if (init[j]) {
      em->cout() << "\tState " << i << " prob " << init[j] << "\n";
      em->cout().Check();
    }
  }

  // cleanup
  delete[] map;
  delete[] init;
}


long explicit_mc::getNumClasses(bool show) const
{
  DCASSERT(chain);
  long nr = chain->getNumClasses();
  long na = chain->getNumAbsorbing();
  long nc = nr + na;
  if (!show || !em->hasIO())  return nc;

  long trans = chain->getNumTransient();
  if (trans) {
    em->cout() << "Transient states:\n";
    long st = chain->getFirstTransient();
    for (long i = trans; i; i--) {
      em->cout().Put('\t');
      ShowState(em->cout(), st);
      em->cout().Put('\n');
      em->cout().Check();
      st++;
    }
  }
  if (nr) for (long c=1; c<=nr; c++) {
    em->cout() << "Recurrent class " << c << ":\n";
    long st = chain->getFirstRecurrent(c);
    for (long i = chain->getRecurrentSize(c); i; i--) {
      em->cout().Put('\t');
      ShowState(em->cout(), st);
      em->cout().Put('\n');
      em->cout().Check();
      st++;
    }
  }
  if (na) {
    em->cout() << "Absorbing states:\n";
    long st = chain->getFirstAbsorbing();
    for (long i = na; i; i--) {
      em->cout().Put('\t');
      ShowState(em->cout(), st);
      em->cout().Put('\n');
      em->cout().Check();
      st++;
    }
  }
  return nc;
}

void explicit_mc::getClass(long cl, intset &statelist) const
{
  statelist.removeAll();
  if (0==cl) {
    long ft = chain->getFirstTransient();
    long lt = ft + chain->getNumTransient() - 1;
    statelist.addRange(ft, lt);
    return;
  }
  long NRC = chain->getNumClasses();
  if (cl > NRC) {
    // absorbing state
    long a = cl - NRC;
    statelist.addElement(a + chain->getFirstAbsorbing());
    return;
  }
  long frc = chain->getFirstRecurrent(cl);
  long lrc = frc + chain->getRecurrentSize(cl) - 1;
  statelist.addRange(frc, lrc);
}

bool explicit_mc::isTransient(long st) const
{
  DCASSERT(chain);
  return chain->isTransientState(st);
}

bool explicit_mc::isAbsorbing(long st) const
{
  DCASSERT(chain);
  return chain->isAbsorbingState(st);
}

bool explicit_mc::isDeadlocked(long st) const
{
  DCASSERT(chain);
  if (chain->isDiscrete()) return false;
  return chain->isAbsorbingState(st);
}

void explicit_mc::findDeadlockedStates(stateset &ss) const
{
  DCASSERT(chain);
  if (chain->isDiscrete()) {
    // every state has at least one outgoing edge.
    ss.changeExplicit().removeAll();
  } else {
    // a state is deadlocked iff it is absorbing.
    long fa = chain->getFirstAbsorbing();
    if (fa < 0)  ss.changeExplicit().removeAll();
    else         ss.changeExplicit().removeRange(0, fa-1);
  }
}

bool explicit_mc::forward(const intset &p, intset &r) const
{
  DCASSERT(chain);
  return chain->getForward(p, r);
}

bool explicit_mc::backward(const intset &p, intset &r) const
{
  DCASSERT(chain);
  return chain->getBackward(p, r);
}

void explicit_mc::getTSCCsSatisfying(stateset &p) const
{
  p.changeExplicit().complement();
  DCASSERT(chain); 
  long nt = chain->getNumTransient();
  if (nt) {
#ifdef DEBUG_EG
    em->cout() << "Removing transients, states 0.." << nt-1 << "\n";
#endif
    p.changeExplicit().addRange(0, nt-1);
  }
  for (long c=1; c<=chain->getNumClasses(); c++) {
    long fs = chain->getFirstRecurrent(c);
    long ls = fs + chain->getRecurrentSize(c) - 1;
    long nz = p.getExplicit().getSmallestAfter(fs-1);
    if (nz < 0) continue;
    if (nz > ls) continue;
#ifdef DEBUG_EG
    em->cout() << "Removing class "<< c <<", states "<< fs <<".."<< ls << "\n";
#endif
    p.changeExplicit().addRange(fs, ls);
  } // for c
  p.changeExplicit().complement();
}

statedist* explicit_mc::getInitialDistribution() const
{
  return Share(initial);
}

long explicit_mc::
getOutgoingWeights(long from, long* to, double* w, long n) const
{
  simple_outedges thing(to, w, n);
  chain->traverseFrom(from, thing);
  return thing.edges;
}

void explicit_mc::getInitialStates(result &x) const
{
  long ns = chain->getNumStates();
  if (0==ns) {
    x.setNull();
    return;
  }

  intset* initss = new intset(ns);
  DCASSERT(initial);
  initial->greater_than(0, initss);

  x.setPtr(new stateset(this, initss));
}

bool explicit_mc
 ::computeTransient(double t, double* probs, double* aux1, double* aux2) const
{
  if (0==chain || 0==probs || 0==aux1)  return false;
  MCLib::Markov_chain::transopts opts;
  opts.kill_aux_vectors = false;
  opts.vm_result = aux1;
  opts.accumulator = aux2;

  try {
    startTransientReport(t); 
    chain->computeTransient(t, probs, opts);
    stopTransientReport(opts.Steps);
    return true;
  }
  catch (MCLib::error e) {
    if (em->startInternal(__FILE__, __LINE__)) {
      em->noCause();
      em->internal() << "Unexpected error: ";
      em->internal() << e.getString();
      em->stopIO();
    }
    return false;
  }
}

bool explicit_mc::computeAccumulated(double t, const double* p0, double* n,
                                  double* aux, double* aux2) const
{
  if (0==chain || 0==p0 || 0==n || 0==aux || 0==aux2) return false;

  MCLib::Markov_chain::transopts opts;
  opts.vm_result = aux;
  opts.accumulator = aux2;
  opts.kill_aux_vectors = false;

  try {
    startAccumulatedReport(t);
    chain->accumulate(t, p0, n, opts);
    stopAccumulatedReport(opts.Steps);
    return true;
  }
  catch (MCLib::error e) {
    if (em->startInternal(__FILE__, __LINE__)) {
      em->noCause();
      em->internal() << "Unexpected error: ";
      em->internal() << e.getString();
      em->stopIO();
    }
    return false;
  }
}

bool explicit_mc::computeSteadyState(double* probs) const
{
  if (0==chain) return false;
  if (0==probs) return false;

  startSteadyReport(); 
  LS_Output outdata;
  try {
    LS_Vector ls_init;
    DCASSERT(initial);
    initial->ExportTo(ls_init);
    chain->computeSteady(ls_init, probs, getSolverOptions(), outdata);
    stopSteadyReport(outdata.num_iters);
    return statusOK(outdata, "steady-state");
  }
  catch (MCLib::error e) {
    return status(e, "steady-state");
  }
}

bool explicit_mc
::computeTimeInStates(const double* p0, double* x) const
{
  if (0==chain || 0==p0 || 0==x) return false;

  startTTAReport();
  LS_Output outdata;
  LS_Vector p0vect;
  p0vect.size = chain->getNumStates();
  p0vect.index = 0;
  p0vect.d_value = p0;
  p0vect.f_value = 0;
  try {
    chain->computeTTA(p0vect, x, getSolverOptions(), outdata);
    stopTTAReport(outdata.num_iters);
    return statusOK(outdata, "time in states");
  }
  catch (MCLib::error e) {
    return status(e, "time in states");
  }
}

bool explicit_mc
::computeClassProbs(const double* p0, double* x) const
{
  if (0==chain || 0==p0 || 0==x) return false;

  startTTAReport();
  LS_Output outdata;
  LS_Vector p0vect;
  p0vect.size = chain->getNumStates();
  p0vect.index = 0;
  p0vect.d_value = p0;
  p0vect.f_value = 0;
  try {
    chain->computeClassProbs(p0vect, x, getSolverOptions(), outdata);
    stopTTAReport(outdata.num_iters);
    return statusOK(outdata, "class probabilities");
  }
  catch (MCLib::error e) {
    return status(e, "class probabilities");
  }
}

bool explicit_mc::dumpDot(OutputStream &s) const
{
  DCASSERT(chain);
  long ns = chain->getNumStates();
  s << "digraph mc {\n";
  for (long i=0; i<ns; i++) {
    s << "\ts" << i << " [label=\"";
    ShowState(s, i);
    s << "\"];\n";
    s.can_flush();
  } // for i
  s << "\n";

  sparse_row_elems foo(0);
  for (long i=0; i<ns; i++) {
    bool ok = foo.buildOutgoing(chain, i);
    // were we successful?
    if (!ok) {
      // out of memory, bail
      if (em->startError()) {
        em->noCause();
        em->cerr() << "Not enough memory to write dot file.";
        em->stopIO();
      }
      return false;
    }

    // display row
    for (long z=0; z<foo.last; z++) {
      s << "\ts" << i << " -> s" << foo.index[z];
      s << " [label=\"" << foo.value[z] << "\"];\n";
    } // for z
    s.can_flush();
  } // for i
  s << "}\n";
  return true;
}

bool explicit_mc::computeDiscreteTTA(double epsilon, double* &dist, int &N) const
{
  DCASSERT(chain);
  if (chain->isContinuous()) {
    if (em->startInternal(__FILE__, __LINE__)) {
      em->noCause();
      em->internal() << "Can't compute discrete TTA on a CTMC.";
      em->stopIO();
    }
    return false;
  }
  
  long acc_state = getAcceptingState();
  if (acc_state < 0) {
    // Degenerate case - no accepting state
    // This should be a distribution of "infinity"
    dist = 0;
    N = 0;
    return true;
  }

  try {
    int goal = chain->getClassOfState(acc_state);
    MCLib::Markov_chain::distopts opts;
    LS_Vector ls_init;
    DCASSERT(initial);
    initial->ExportTo(ls_init);
    chain->computeDiscreteDistTTA(ls_init, opts, goal, epsilon, dist, N);
    return true;
  }
  catch (MCLib::error e) {
    if (em->startError()) {
      em->noCause();
      em->cerr() << "Couldn't compute discrete TTA: ";
      em->cerr() << e.getString();
      em->stopIO();
    }
    return false;
  }
}

bool explicit_mc::
computeContinuousTTA(double dt, double epsilon, double* &dist, int &N) const
{
  DCASSERT(chain);
  if (chain->isDiscrete()) {
    if (em->startInternal(__FILE__, __LINE__)) {
      em->noCause();
      em->internal() << "Can't compute continuous TTA on a DTMC.";
      em->stopIO();
    }
    return false;
  }
  
  long acc_state = getAcceptingState();
  if (acc_state < 0) {
    // Degenerate case - no accepting state
    // This should be a distribution of "infinity"
    dist = 0;
    N = 0;
    return true;
  }

  try {
    int goal = chain->getClassOfState(acc_state);
    MCLib::Markov_chain::distopts opts;
    LS_Vector ls_init;
    DCASSERT(initial);
    initial->ExportTo(ls_init);
    chain->computeContinuousDistTTA(ls_init, opts, goal, dt, epsilon, dist, N);
    return true;
  }
  catch (MCLib::error e) {
    if (em->startError()) {
      em->noCause();
      em->cerr() << "Couldn't compute discrete TTA: ";
      em->cerr() << e.getString();
      em->stopIO();
    }
    return false;
  }
}

bool explicit_mc::reachesAcceptBy(double t, double* x) const
{
  DCASSERT(chain);

  //
  // Set x to be all zeroes, except for the accepting state
  //
  for (long i=0; i<num_states; i++) x[i] = 0;

  long acc_state = getAcceptingState();
  if (acc_state < 0) {
    // Degenerate case - no accepting state,
    // so nothing will reach it
    return true;
  }
  x[acc_state] = 1;

  if (t<=0) return true;

  MCLib::Markov_chain::transopts opts;

  try {
    startRevTransReport(t); 
    chain->reverseTransient(t, x, opts);
    stopRevTransReport(opts.Steps);
    return true;
  }
  catch (MCLib::error e) {
    if (em->startInternal(__FILE__, __LINE__)) {
      em->noCause();
      em->internal() << "Unexpected error: ";
      em->internal() << e.getString();
      em->stopIO();
    }
    return false;
  }
}

bool explicit_mc::randomTTA(rng_stream &st, long &state, const stateset &final,
                          long maxt, long &elapsed)
{
  DCASSERT(chain);
  if (chain->isContinuous()) {
    if (em->startInternal(__FILE__, __LINE__)) {
      em->noCause();
      em->internal() << "Can't simulate discrete-time random walk on a CTMC.";
      em->stopIO();
    }
    return false;
  }

  // TBD: an option for this part?
  // TBD: reporting for the transpose?
  if (chain->isEfficientByCols()) {
    try {
      chain->transpose();
    }
    catch (MCLib::error e) {
      if (em->startError()) {
        em->noCause();
        em->cerr() << "Couldn't transpose DTMC for random walk: ";
        em->cerr() << e.getString();
        em->stopIO();
      }
      return false;
    }
  }

  try {
    elapsed = chain->randomWalk(st, state, &final.getExplicit(), maxt, 1.0);
    return true;
  }
  catch (MCLib::error e) {
    if (em->startError()) {
      em->noCause();
      em->cerr() << "Couldn't simulate DTMC random walk: ";
      em->cerr() << e.getString();
      em->stopIO();
    }
    return false;
  }
}

bool explicit_mc::randomTTA(rng_stream &st, long &state, const stateset &final,
                          double maxt, double &elapsed)
{
  DCASSERT(chain);
  if (chain->isDiscrete()) {
    if (em->startInternal(__FILE__, __LINE__)) {
      em->noCause();
      em->internal() << "Can't simulate continuous-time random walk on a DTMC.";
      em->stopIO();
    }
    return false;
  }

  // TBD: an option for this part?
  // TBD: reporting for the transpose?
  if (chain->isEfficientByCols()) {
    try {
      chain->transpose();
    }
    catch (MCLib::error e) {
      if (em->startError()) {
        em->noCause();
        em->cerr() << "Couldn't transpose CTMC for random walk: ";
        em->cerr() << e.getString();
        em->stopIO();
      }
      return false;
    }
  }

  try {
    elapsed = chain->randomWalk(st, state, &final.getExplicit(), maxt);
    return true;
  }
  catch (MCLib::error e) {
    if (em->startError()) {
      em->noCause();
      em->cerr() << "Couldn't simulate CTMC random walk: ";
      em->cerr() << e.getString();
      em->stopIO();
    }
    return false;
  }
}


// ******************************************************************
// *                                                                *
// *                         mc_enum  class                         *
// *                                                                *
// ******************************************************************

class mc_enum : public explicit_mc {
protected:
  long* state_handle;
  model_enum* states;
public:
  /// Constructor for enumerated chains.
  mc_enum(LS_Vector& init, MCLib::Markov_chain* mc, model_enum* st);
  virtual ~mc_enum();
  virtual void visitStates(state_visitor &v) const;
  virtual shared_object* getEnumeratedState(long i) const;
protected:
  const char* getClassName() const { return "mc_enum"; }
  virtual void BuildStateMapping(long* map) const;
  virtual void ShowState(OutputStream &s, long i) const;
};

// ******************************************************************
// *                        mc_enum  methods                        *
// ******************************************************************

mc_enum::mc_enum(LS_Vector& init, MCLib::Markov_chain* mc, model_enum* st)
 : explicit_mc(init, mc)
{
  states = st;
  state_handle = new long[states->NumValues()];
  for (long j=0; j<states->NumValues(); j++) {
    const model_enum_value* st = states->ReadValue(j);
    CHECK_RANGE(0, st->GetIndex(), states->NumValues());
    state_handle[st->GetIndex()] = j;
  }
}

mc_enum::~mc_enum()
{
  Delete(states);
  delete[] state_handle;
}

void mc_enum::visitStates(state_visitor &v) const
{
  DCASSERT(states);
  DCASSERT(v.state());
  for (v.index()=0; v.index() < states->NumValues(); v.index()++) {
    if (v.canSkipIndex()) continue;
    v.state()->set(states->GetIndex(), v.index());
    if (v.visit()) return;
  }
}

shared_object* mc_enum::getEnumeratedState(long i) const
{
  DCASSERT(state_handle);
  return Share(states->GetValue(state_handle[i]));
}

void mc_enum::BuildStateMapping(long* map) const
{
  // invert state_handle
  long* hs = new long[states->NumValues()];
  for (long i=0; i<states->NumValues(); i++) {
    CHECK_RANGE(0, state_handle[i], states->NumValues());
    hs[state_handle[i]] = i;
  }
  switch (display_order) {
    case NATURAL:
      DCASSERT(0);
      break;

    case LEXICAL:
      states->MakeSortedMap(map);
      break;

    case DISCOVERY:
      for (long i=0; i<states->NumValues(); i++) map[i] = i;
      break;

    default:
      DCASSERT(0);
  }
  // apply inverse state handle
  for (long i=0; i<states->NumValues(); i++) map[i] = hs[map[i]];
  delete[] hs;
}

void mc_enum::ShowState(OutputStream &s, long i) const
{
  DCASSERT(state_handle);
  CHECK_RANGE(0, i, states->NumValues());
  const model_enum_value* st = states->ReadValue(state_handle[i]);
  DCASSERT(st);
  s.Put(st->Name());
}


// ******************************************************************
// *                                                                *
// *                         mc_expl  class                         *
// *                                                                *
// ******************************************************************

class mc_expl : public explicit_mc {
  StateLib::state_db* fullstates;
protected:
  long accept;
  long trap;
  long* state_handle;
  StateLib::state_coll* states;
public:
  mc_expl(bool dtmc, StateLib::state_db* st);
protected:
  virtual ~mc_expl();
public:
  void FinishExpl(LS_Vector& init, MCLib::Markov_chain* mc, 
    long acc, long trap);

  inline void FinishExpl(LS_Vector& init, MCLib::Markov_chain* mc) {
    FinishExpl(init, mc, -1, -1);
  }
  inline StateLib::state_db* getAllStates() { return fullstates; }
  virtual long getAcceptingState() const { return accept; }
  virtual long getTrapState() const { return trap; }
  virtual void visitStates(state_visitor &v) const;
  virtual void reportMemUsage(exprman* em, const char* prefix) const;
protected:
  const char* getClassName() const { return "mc_expl"; }
  virtual void BuildStateMapping(long* map) const;
  virtual void ShowState(OutputStream &s, long i) const;

  inline void getState(long i, shared_state* st) const {
    DCASSERT(false == parent->containsListVar());
    CHECK_RANGE(0, i, num_states);
    DCASSERT(st);
    if (fullstates) {
      fullstates->GetStateKnown(i, 
            st->writeState(), st->getStateSize());
    } else {
      DCASSERT(states);
      DCASSERT(state_handle);
      states->GetStateKnown(state_handle[i], 
            st->writeState(), st->getStateSize());
    }
  }
};

// ******************************************************************
// *                        mc_expl  methods                        *
// ******************************************************************

mc_expl::mc_expl(bool dtmc, StateLib::state_db* st)
: explicit_mc(dtmc, st->Size())
{
  fullstates = st;
  state_handle = 0;
  states = 0;
  accept = -2;
  trap = -3;
  fullstates->ConvertToStatic(true);
}

mc_expl::~mc_expl()
{
  delete fullstates;
  delete states;
  delete[] state_handle;
}

void mc_expl::FinishExpl(LS_Vector& init, MCLib::Markov_chain* mc, 
  long a, long t)
{
  accept = a;
  trap = t;

  // Compact states
  states = fullstates->TakeStateCollection();
  delete fullstates;
  fullstates = 0;
  state_handle = states->RemoveIndexHandles();

  // Finish MC
  MCLib::Markov_chain::finish_options opts;
  opts.Store_By_Rows = storeByRows();
  opts.Will_Clear = false;
  opts.report = my_timer ? my_timer->switchMe() : 0;
  MCLib::Markov_chain::renumbering r;
  mc->finish(opts, r);
  DCASSERT(mc->isEfficientByRows() == opts.Store_By_Rows);

  if (r.NoRenumbering()) {
    setChain(new statedist(this, init, 0), mc);
    return;
  }

  DCASSERT(r.GeneralRenumbering());
  const long* ren = r.GetGeneral();
  DCASSERT(ren);

  // Renumber accepting state, if any
  if (accept>=0) {
    CHECK_RANGE(0, accept, states->Size());
    accept = ren[accept];
  }
  // Renumber trap state, if any
  if (trap>=0) {
    CHECK_RANGE(0, trap, states->Size());
    trap = ren[trap];
  }

  // Build initial distribution (renumbered)
  setChain(new statedist(this, init, ren), mc);
  
  // Renumber state_handle array.
  long* aux = new long[states->Size()];
  for (long i=states->Size()-1; i>=0; i--) {
    long j = ren[i];
    CHECK_RANGE(0, j, states->Size());
    aux[j] = state_handle[i];
  } // for i

  delete[] state_handle;
  state_handle = aux;
}

void mc_expl::visitStates(state_visitor &v) const
{
  DCASSERT(v.state());
  for (v.index()=0; v.index() < num_states; v.index()++) {
    if (v.canSkipIndex()) continue;
    getState(v.index(), v.state());
    if (v.visit()) return;
  }
}

void mc_expl::reportMemUsage(exprman* em, const char* prefix) const
{
  long mem = states ? states->ReportMemTotal() : 0;
  if (state_handle) mem += states->Size() * sizeof(long);
  em->report().Put(prefix);
  em->report().PutMemoryCount(mem, 3);
  em->report() << " required for state space storage\n";
  mem = chain ? chain->ReportMemTotal() : 0;
  em->report().Put(prefix);
  em->report().PutMemoryCount(mem, 3);
  em->report() << " required for Markov chain storage\n";
}

// Protected:

void mc_expl::BuildStateMapping(long* map) const
{
  switch (display_order) {
    case NATURAL:
      DCASSERT(0);
      return;

    case LEXICAL:
      if (fullstates) {
        const StateLib::state_coll* coll = fullstates->GetStateCollection();
        LexicalSort(parent, coll, map);
      } else {
        DCASSERT(states);
        DCASSERT(state_handle);
        LexicalSort(parent, states, state_handle, map);
      }
      return;

    case DISCOVERY:
      if (state_handle) HeapSort(state_handle, map, num_states);
      return;

    default:
      DCASSERT(0);
  }
}

void mc_expl::ShowState(OutputStream &s, long i) const
{
  shared_state* st = new shared_state(parent);
  getState(i, st);
  st->Print(s, 0);
  Delete(st);
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
// *                           Front  end                           *
// *                                                                *
// ******************************************************************

void InitMCLibs(exprman* em)
{
  if (0==em)  return;
  
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

  if (0==explicit_mc::my_timer) {
    explicit_mc::my_timer = new mc_reporter(em);
  }

  if (markov_lldsm::lsopts) return;

  markov_lldsm::lsopts = new LS_Options[markov_lldsm::NUM_SOLVERS];
  markov_lldsm::lsopts[markov_lldsm::GAUSS_SEIDEL].method = LS_Gauss_Seidel;
  markov_lldsm::lsopts[markov_lldsm::JACOBI].method = LS_Jacobi;
  markov_lldsm::lsopts[markov_lldsm::ROW_JACOBI].method = LS_Row_Jacobi;

  //
  // Set up the radio buttons for the solvers
  //
  radio_button** solvers = new radio_button*[markov_lldsm::NUM_SOLVERS];

  solvers[markov_lldsm::GAUSS_SEIDEL] = new radio_button(
      "GAUSS_SEIDEL", "Gauss-Seidel", markov_lldsm::GAUSS_SEIDEL
  );
  solvers[markov_lldsm::JACOBI] = new radio_button(
      "JACOBI", "Jacobi, using matrix-vector multiply", 
      markov_lldsm::JACOBI
  );
  solvers[markov_lldsm::ROW_JACOBI] = new radio_button(
      "ROW_JACOBI", "Jacobi, visiting one matrix row at a time", 
      markov_lldsm::ROW_JACOBI
  );
   
  markov_lldsm::solver = markov_lldsm::GAUSS_SEIDEL;
  em->addOption(
    MakeRadioOption(
      "MCSolver",
      "Numerical method to use for solving linear systems during Markov chain analysis.",
      solvers, 3, markov_lldsm::solver
    )
  );
  
  //
  // Add settings for each solver radio button (cool, huh?)
  //
  for (int i=0; i<markov_lldsm::NUM_SOLVERS; i++) {
#ifdef DEBUG_NUMERICAL_ITERATIONS
    markov_lldsm::lsopts[i].debug = true;
#endif
    option_manager* settings = MakeOptionManager();
    settings->AddOption(
      MakeIntOption(
        "MinIters", 
        "Minimum number of iterations.  Guarantees that at least this many iterations will occur.",
        markov_lldsm::lsopts[i].min_iters, 0, 2000000000
      )
    );

    settings->AddOption(
      MakeIntOption(
        "MaxIters", 
        "Maximum number of iterations.  Once the minimum number of iterations has been reached, the solver will terminate if either the termination criteria has been met (see options for Precision), or the maximum number of iterations has been reached.",
        markov_lldsm::lsopts[i].max_iters, 0, 2000000000
      )
    );

    settings->AddOption(
      MakeRealOption(
        "Precision", 
        "Desired precision.  Solvers will run until each solution vector element has changed less than epsilon.  Relative or absolute precision may be used, see option TBD.",
        markov_lldsm::lsopts[i].precision, 
        true, false, 0.0,
        true, false, 1.0
      )
    );

    settings->AddOption(
      MakeRealOption(
        "Relaxation", 
        "Relaxation parameter to use (or start with).",
        markov_lldsm::lsopts[i].relaxation, 
        true, false, 0.0,
        true, false, 2.0
      )
    );

    // put these settings into the radio button

    settings->DoneAddingOptions();
    solvers[i]->makeSettings(settings);

    // Memory leak, because we never clean up settings, but probably ok
  } // for i




  option* report = em->findOption("Report");
  markov_lldsm::report.Initialize(report,
      "mc_solve",
      "When set, Markov chain solution performance is reported.",
      false
  );


  radio_button** alist = new radio_button*[2];
  alist[markov_lldsm::BY_COLUMNS] = new radio_button(
      "COLUMNS", 
      "Access to columns", 
      markov_lldsm::BY_COLUMNS
  );
  alist[markov_lldsm::BY_ROWS] = new radio_button(
      "ROWS", 
      "Access to rows", 
      markov_lldsm::BY_ROWS
  );
  em->addOption(
    MakeRadioOption("MCAccessBy",
      "Specifiy initial storage method for Markov chains: by rows (required for simulations) or by columns (required for certain linear solvers).",
      alist, 2, markov_lldsm::access
    )
  );
  
}

stochastic_lldsm* 
MakeEnumeratedMC(LS_Vector& init, model_enum* ss, MCLib::Markov_chain* mc)
{
  return new mc_enum(init, mc, ss);
}

stochastic_lldsm* StartExplicitMC(bool dtmc, StateLib::state_db* ss)
{
  return new mc_expl(dtmc, ss);
}

StateLib::state_db* GrabExplicitMCStates(lldsm* mc)
{
  mc_expl* xmc = dynamic_cast <mc_expl*> (mc);
  if (0==xmc) return 0;
  return xmc->getAllStates();
}

void FinishExplicitMC(lldsm* m, LS_Vector &i, MCLib::Markov_chain* mc)
{
  if (0==mc) return;
  mc_expl* xmc = dynamic_cast <mc_expl*> (m);
  if (0==xmc) return;
  xmc->FinishExpl(i, mc);
}

void FinishExplicitMC(lldsm* m, LS_Vector &i, long acc, long trap,
  MCLib::Markov_chain* mc)
{
  if (0==mc) return;
  mc_expl* xmc = dynamic_cast <mc_expl*> (m);
  if (0==xmc) return;
  xmc->FinishExpl(i, mc, acc, trap);
}
