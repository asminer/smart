
#include "proc_mclib.h"

#include "../Streams/streams.h"
#include "../include/heap.h"
#include "../Modules/expl_ssets.h"
#include "../Modules/statevects.h"

// external library
#include "../_IntSets/intset.h"

bool statusOK(exprman* em, const LS_Output &o, const char* who) 
{
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

bool status(exprman* em, MCLib::error e, const char* who) 
{
    switch (e.getCode()) {
      case MCLib::error::Out_Of_Memory:
          if (em->startError()) {
            em->noCause();
            em->cerr() << "Insufficient memory for Markov chain ";
            em->cerr() << who << " solver";
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

// ******************************************************************
// *                                                                *
// *                     mclib_process  methods                     *
// *                                                                *
// ******************************************************************

mclib_process::mclib_process(bool discrete, GraphLib::dynamic_graph* _G)
{
  is_discrete = discrete;
  G = _G;
  VC = 0;

  chain = 0;
  initial = 0;
  
  trap = -1;
  accept = -1;
}

// ******************************************************************

mclib_process::mclib_process(MCLib::vanishing_chain* vc)
{
  is_discrete = vc->isDiscrete();
  G = 0;
  VC = vc;

  chain = 0;
  initial = 0;
  
  trap = -1;
  accept = -1;
}

// ******************************************************************

mclib_process::~mclib_process()
{
  delete G;
  delete VC;
  delete chain;
  Delete(initial);
}

// ******************************************************************

GraphLib::node_renumberer* mclib_process::initChain(GraphLib::dynamic_graph *g)
{
  // Classify states
  GraphLib::timer_hook *sw = my_timer ? my_timer->switchMe() : 0;
  GraphLib::abstract_classifier* ac = g->determineSCCs(0, 1, true, sw);
  GraphLib::static_classifier C;
  GraphLib::node_renumberer *Ren = ac->buildRenumbererAndStatic(C);
  
  // Renumber states
  DCASSERT(Ren);
  g->renumberNodes(*Ren);
  
  // Build chain; try using doubles
  GraphLib::dynamic_summable <double> *Gd = 
    dynamic_cast < GraphLib::dynamic_summable <double> *> (g);

  if (Gd) {
    chain = new MCLib::Markov_chain(is_discrete, *Gd, C, sw);
  }

  if (0==chain) {
    // try using floats
    GraphLib::dynamic_summable <float> *Gf = 
      dynamic_cast < GraphLib::dynamic_summable <float> *> (g);

    if (Gf) {
      chain = new MCLib::Markov_chain(is_discrete, *Gf, C, sw);
    }
  }
    
  if (0==chain) {
    if (em->startInternal(__FILE__, __LINE__)) {
        em->noCause();
        em->internal() << "Couldn't convert graph to Markov chain: bad graph type?\n";
        em->stopIO();
    }
  }

  // 
  // Cleanup
  //
  delete ac;

  return Ren;
}

// ******************************************************************

void mclib_process::attachToParent(stochastic_lldsm* p, LS_Vector &init, state_lldsm::reachset* rss)
{
  process::attachToParent(p, init, rss);

  // Finish rss
  indexed_reachset* irs = smart_cast <indexed_reachset*> (rss);
  DCASSERT(irs);
  irs->Finish();

  //
  // Finish MC
  //
  GraphLib::node_renumberer *Ren = 0;
  if (G) {
    Ren = initChain(G);
  } else if (VC) {
    Ren = initChain(& VC->TT() );
  }
  DCASSERT(Ren);

  //
  // Done with G, VC
  //
  delete G;
  G = 0;
  delete VC;
  VC = 0;

  //
  // Renumber states
  //
  irs->Renumber(Ren);
  if (trap >= 0) {
    trap = Ren->new_number(trap);
  }
  if (accept >= 0) {
    accept = Ren->new_number(accept);
  }
  initial = new statedist(p, init, Ren);

  // Copy initial states to RSS
  intset initial_set( chain->getNumStates() );  
  initial->greater_than(0, &initial_set);
  irs->setInitial(initial_set);

  // NEAT TRICK!!!
  // Set the reachability graph, using 
  // a thin wrapper around the Markov chain.
  p->setRGR( new mclib_reachgraph(this) );

  //
  // Done with Ren
  //
  delete Ren;
}

// ******************************************************************

long mclib_process::getNumStates() const
{
  DCASSERT(chain);
  return chain->getNumStates();
}

// ******************************************************************

void mclib_process::getNumClasses(long &count) const
{
  DCASSERT(chain);
  const GraphLib::static_classifier& C = chain->getStateClassification();
  count = C.getNumClasses() - 2 + C.sizeOfClass(1);
  // There's always one class for all transient states (#0)
  // and one class for all absorbing states (#1).  Subtract those,
  // and add back the number of absorbing states.
}

// ******************************************************************

void mclib_process::showClasses(OutputStream &os, state_lldsm::reachset* RSS, 
  shared_state* st) const
{
  // TBD

  DCASSERT(0);

/*
  // TBD : try/catch around this
  indexed_reachset::indexed_iterator &I = 
    dynamic_cast <indexed_reachset::indexed_iterator &> (
      RSS->iteratorForOrder(state_lldsm::NATURAL)
    );

  const GraphLib::static_classifier& C = chain->getStateClassification();

  long nr = C.getNumClasses()-2;
  long na = C.sizeOfClass(1);

  long trans = chain->getNumTransient();
  if (trans) {
    em->cout() << "Transient states:\n";
    long snum = chain->getFirstTransient();
    for (long i = trans; i; i--) {
      em->cout().Put('\t');
      I.copyState(st, snum);
      RSS->showState(os, st);
      em->cout().Put('\n');
      em->cout().Check();
      snum++;
    }
  }
  if (nr) for (long c=1; c<=nr; c++) {
    em->cout() << "Recurrent class " << c << ":\n";
    long snum = chain->getFirstRecurrent(c);
    for (long i = chain->getRecurrentSize(c); i; i--) {
      em->cout().Put('\t');
      I.copyState(st, snum);
      RSS->showState(os, st);
      em->cout().Put('\n');
      em->cout().Check();
      snum++;
    }
  }
  if (na) {
    em->cout() << "Absorbing states:\n";
    long snum = chain->getFirstAbsorbing();
    for (long i = na; i; i--) {
      em->cout().Put('\t');
      I.copyState(st, snum);
      RSS->showState(os, st);
      em->cout().Put('\n');
      em->cout().Check();
      snum++;
    }
  }
  */
}

// ******************************************************************

bool mclib_process::isTransient(long st) const
{
  DCASSERT(chain);
  const GraphLib::static_classifier& C = chain->getStateClassification();
  return C.isNodeInClass(st, 0);
}

// ******************************************************************

statedist* mclib_process::getInitialDistribution() const
{
  return Share(initial);
}

// ******************************************************************

long mclib_process::getOutgoingWeights(long from, long* to, double* w, long n) const
{
  DCASSERT(0);
  return 0;

  // TBD

  /*
  simple_outedges thing(to, w, n);
  chain->traverseFrom(from, thing);
  return thing.edges;
  */
}

// ******************************************************************

bool mclib_process
::computeTransient(double t, double* probs, double* aux1, double* aux2) const
{
  if (0==chain || 0==probs || 0==aux1)  return false;


  try {
    timer w;
    if (is_discrete) {
      MCLib::Markov_chain::DTMC_transient_options opts;
      opts.vm_result = aux1;
      opts.accumulator = aux2;

      int it = int(t);
      startTransientReport(w, it);
      chain->computeTransient(it, probs, opts);
      stopTransientReport(w, opts.multiplications);

      opts.vm_result = 0;
      opts.accumulator = 0;
    } else {
      MCLib::Markov_chain::CTMC_transient_options opts;
      opts.vm_result = aux1;
      opts.accumulator = aux2;

      startTransientReport(w, t); 
      chain->computeTransient(t, probs, opts);
      stopTransientReport(w, opts.multiplications);

      opts.vm_result = 0;
      opts.accumulator = 0;
    }
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

// ******************************************************************

bool mclib_process::computeAccumulated(double t, const double* p0, double* n,
                                  double* aux, double* aux2) const
{
  if (0==chain || 0==p0 || 0==n || 0==aux || 0==aux2) return false;

  try {
    timer w;
    if (is_discrete) {
      MCLib::Markov_chain::DTMC_transient_options opts;
      opts.vm_result = aux;
      opts.accumulator = aux2;

      int it = int(t);
      startAccumulatedReport(w, it);
      chain->accumulate(it, p0, n, opts);
      stopAccumulatedReport(w, opts.multiplications);

      opts.vm_result = 0;
      opts.accumulator = 0;
    } else {
      MCLib::Markov_chain::CTMC_transient_options opts;
      opts.vm_result = aux;
      opts.accumulator = aux2;

      startAccumulatedReport(w, t); 
      chain->accumulate(t, p0, n, opts);
      stopAccumulatedReport(w, opts.multiplications);

      opts.vm_result = 0;
      opts.accumulator = 0;
    }
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

// ******************************************************************

bool mclib_process::computeSteadyState(double* probs) const
{
  if (0==chain) return false;
  if (0==probs) return false;

  try {
    LS_Output outdata;
    LS_Vector ls_init;
    timer w;
    startSteadyReport(w); 
    DCASSERT(initial);
    initial->ExportTo(ls_init);
    chain->computeInfinityDistribution(ls_init, probs, getSolverOptions(), outdata);
    stopSteadyReport(w, outdata.num_iters);
    return statusOK(em, outdata, "steady-state");
  }
  catch (MCLib::error e) {
    return status(em, e, "steady-state");
  }
}

// ******************************************************************

bool mclib_process::computeTimeInStates(const double* p0, double* x) const
{
  if (0==chain || 0==p0 || 0==x) return false;

  LS_Output outdata;
  LS_Vector p0vect;
  p0vect.size = chain->getNumStates();
  p0vect.index = 0;
  p0vect.d_value = p0;
  p0vect.f_value = 0;
  try {
    timer w;
    startTTAReport(w);
    chain->computeTTA(p0vect, x, getSolverOptions(), outdata);
    stopTTAReport(w, outdata.num_iters);
    return statusOK(em, outdata, "time in states");
  }
  catch (MCLib::error e) {
    return status(em, e, "time in states");
  }
}

// ******************************************************************

bool mclib_process::computeClassProbs(const double* p0, double* x) const
{
  if (0==chain || 0==p0 || 0==x) return false;

  LS_Output outdata;
  LS_Vector p0vect;
  p0vect.size = chain->getNumStates();
  p0vect.index = 0;
  p0vect.d_value = p0;
  p0vect.f_value = 0;
  try {
    timer w;
    startTTAReport(w);
    chain->computeFirstRecurrentProbs(p0vect, x, getSolverOptions(), outdata);
    stopTTAReport(w, outdata.num_iters);
    return statusOK(em, outdata, "class probabilities");
  }
  catch (MCLib::error e) {
    return status(em, e, "class probabilities");
  }
}

// ******************************************************************

bool mclib_process::randomTTA(rng_stream &st, long &state, const stateset* F,
                          long maxt, long &elapsed)
{
  const expl_stateset* final = dynamic_cast <const expl_stateset*> (F);
  DCASSERT(final);
  DCASSERT(chain);
  if (chain->isContinuous()) {
    if (em->startInternal(__FILE__, __LINE__)) {
      em->noCause();
      em->internal() << "Can't simulate discrete-time random walk on a CTMC.";
      em->stopIO();
    }
    return false;
  }

  try {
    elapsed = chain->randomWalk(st, state, &final->getExplicit(), maxt, 1.0);
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

// ******************************************************************

bool mclib_process::randomTTA(rng_stream &st, long &state, const stateset* F,
                          double maxt, double &elapsed)
{
  const expl_stateset* final = dynamic_cast <const expl_stateset*> (F);
  DCASSERT(chain);
  if (chain->isDiscrete()) {
    if (em->startInternal(__FILE__, __LINE__)) {
      em->noCause();
      em->internal() << "Can't simulate continuous-time random walk on a DTMC.";
      em->stopIO();
    }
    return false;
  }

  try {
    elapsed = chain->randomWalk(st, state, &final->getExplicit(), maxt);
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

//
// For phase?
// 

// ******************************************************************

bool mclib_process::computeDiscreteTTA(double epsilon, long maxsize, 
  discrete_pdf &dist) const
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
    dist.reset(0, -1, 0, 1.0);
    return true;
  }

  const GraphLib::static_classifier& C = chain->getStateClassification();
  DCASSERT(C.isNodeInClass(acc_state, 1));

  try {
    // int goal = chain->getClassOfState(acc_state);
    long goal = -acc_state;
    MCLib::Markov_chain::DTMC_distribution_options opts;
    opts.epsilon = epsilon;
    opts.setMaxSize(maxsize);
    LS_Vector ls_init;
    DCASSERT(initial);
    initial->ExportTo(ls_init);
    chain->computeDiscreteDistTTA(opts, ls_init, goal, dist);
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


// ******************************************************************

bool mclib_process::computeContinuousTTA(double dt, double epsilon, 
  long maxsize, discrete_pdf &dist) const
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
    dist.reset(0, -1, 0, 1.0);
    return true;
  }

  try {
    // int goal = chain->getClassOfState(acc_state);
    long goal = -acc_state;
    MCLib::Markov_chain::CTMC_distribution_options opts;
    opts.epsilon = epsilon;
    opts.setMaxSize(maxsize);
    LS_Vector ls_init;
    DCASSERT(initial);
    initial->ExportTo(ls_init);
    chain->computeContinuousDistTTA(opts, ls_init, goal, dt, dist);
    return true;
  }
  catch (MCLib::error e) {
    if (em->startError()) {
      em->noCause();
      em->cerr() << "Couldn't compute continuous TTA: ";
      em->cerr() << e.getString();
      em->stopIO();
    }
    return false;
  }
}


// ******************************************************************

bool mclib_process::reachesAcceptBy(double t, double* x) const
{
  DCASSERT(chain);

  //
  // Set x to be all zeroes, except for the accepting state
  //
  for (long i=chain->getNumStates()-1; i>=0; i--) x[i] = 0;

  long acc_state = getAcceptingState();
  if (acc_state < 0) {
    // Degenerate case - no accepting state,
    // so nothing will reach it
    return true;
  }
  x[acc_state] = 1;

  if (t<=0) return true;

  try {
    timer w;
    if (is_discrete) {
      MCLib::Markov_chain::DTMC_transient_options opts;
      int it = int(t);
      startRevTransReport(w, it);
      chain->reverseTransient(it, x, opts);
      stopRevTransReport(w, opts.multiplications);
    } else {
      MCLib::Markov_chain::CTMC_transient_options opts;
      startRevTransReport(w, t); 
      chain->reverseTransient(t, x, opts);
      stopRevTransReport(w, opts.multiplications);
    }
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






//
// For reachgraphs
//

// ******************************************************************

void mclib_process::showInternal(OutputStream &os) const
{
  // TBD

  os << "Internal representation for Markov chain:\n";
  os << "  Explicit representation using MCLib.  Cannot display yet, sorry\n";
  return;
}

// ******************************************************************

void mclib_process::showProc(OutputStream &os, 
  const graph_lldsm::reachgraph::show_options &opt, 
  state_lldsm::reachset* RSS, shared_state* st) const
{
  /*
  long na = chain->getNumArcs();
  long num_states = chain->getNumStates();

  if (state_lldsm::tooManyStates(num_states, &os))  return;
  if (graph_lldsm::tooManyArcs(na, &os))            return;

  bool by_rows = (graph_lldsm::OUTGOING == opt.STYLE);
  const char* row;
  const char* col;
  if (opt.RG_ONLY) {
    row = "From state ";
    col = "To state ";
    if (!by_rows) SWAP(row, col);
  } else {
    row = by_rows ? "Row " : "Column ";
    col = "";
  }

  // TBD : try/catch around this
  indexed_reachset::indexed_iterator &I 
  = dynamic_cast <indexed_reachset::indexed_iterator &> (RSS->iteratorForOrder(opt.ORDER));


  switch (opt.STYLE) {
    case graph_lldsm::DOT:
        os << "digraph mc {\n";
        for (I.start(); I; I++) { 
          os << "\ts" << I.index();
          if (opt.NODE_NAMES) {
            I.copyState(st);
            os << " [label=\"";
            RSS->showState(os, st);
            os << "\"]";
          }
          os << ";\n";
          os.flush();
        } // for i
        os << "\n";
        break;

    case graph_lldsm::TRIPLES:
        os << num_states << "\n";
        os << na << "\n";
        break;

    default:
        if (opt.RG_ONLY) {
          os << "Reachability graph:\n";
        } else {
          os << "Markov chain:\n";
        }
  }

  sparse_row_elems foo(I);

  for (I.start(); I; I++) {
    switch (opt.STYLE) {
      case graph_lldsm::INCOMING:
      case graph_lldsm::OUTGOING:
          os << row;
          if (opt.NODE_NAMES) {
            I.copyState(st);
            RSS->showState(os, st);
          } else {
            os << I.getI();
          }
          os << ":\n";

      default:
          // nothing
          break;
    }

    bool ok;
    if (by_rows)   ok = foo.buildOutgoing(chain, I.getIndex());
    else           ok = foo.buildIncoming(chain, I.getIndex());

    // were we successful?
    if (!ok) {
      // out of memory, bail
      showError("Not enough memory to display Markov chain.");
      break;
    }

    // display row/column
    for (long z=0; z<foo.last; z++) {
      os.Put('\t');
      switch (opt.STYLE) {
        case graph_lldsm::DOT:
            os << "s" << foo.index[z] << " -> s" << I.getI();
            if (!opt.RG_ONLY) {
              os << " [label=\"" << foo.value[z] << "\"]";
            }
            os << ";";
            break;

        case graph_lldsm::TRIPLES:
            os << foo.index[z] << " " << I.getI();
            if (!opt.RG_ONLY) {
              os << " " << foo.value[z];
            }
            break;

        default:
            os << col;
            if (opt.NODE_NAMES) {
              I.copyState(st, foo.index[z]);
              RSS->showState(os, st);
            } else {
              os << foo.index[z];
            }
            if (!opt.RG_ONLY) {
              os << " : " << foo.value[z];
            }
      }
      os.Put('\n');
    } // for z

    os.flush();
  } // for i
  if (graph_lldsm::DOT == opt.STYLE) {
    os << "}\n";
  }
  os.flush();
  */
}

// ******************************************************************

void mclib_process::getDeadlocked(intset &r) const
{
  //
  // Remove from r any state with some outgoing edge.
  // For a DTMC, that's everything.
  // For a CTMC, that's any state except absorbing ones.
  //
  DCASSERT(chain);
  if (chain->isDiscrete()) {
    r.removeAll();
    return;
  }
  //
  // CTMC case
  //
  const GraphLib::static_classifier& C = chain->getStateClassification();

  // Remove transients
  r.removeRange(C.firstNodeOfClass(0), C.lastNodeOfClass(0));

  // Remove all others
  r.removeRange(C.firstNodeOfClass(2), chain->getNumStates()-1);
}

// ******************************************************************

void mclib_process::getTSCCsSatisfying(intset &p) const 
{
  DCASSERT(chain);
  const GraphLib::static_classifier& C = chain->getStateClassification();

  //
  // Remove transient states from p
  //
  p.removeRange(C.firstNodeOfClass(0), C.lastNodeOfClass(0));

  //
  // For each recurrent class, check if ALL states in the class satisfy p.
  // If not, remove all states in the class from p.
  //

  //
  // We do this by complementing the set, and searching for the smallest
  // element in !p to see if it is in the class or not.  If not,
  // then all states in the class satisfy p.
  //
  p.complement();
  for (long c=2; c<C.getNumClasses(); c++) {
    long nextnotp = p.getSmallestAfter( C.firstNodeOfClass(c)-1 );
    if (nextnotp < 0) continue; // not found
    if (nextnotp > C.lastNodeOfClass(c)) continue;
    //
    // Remove all states in this class from p,
    // which means *adding* them to !p.
    //
    p.addRange(C.firstNodeOfClass(c), C.lastNodeOfClass(c));
  } // for c
  p.complement();
}

#ifdef USE_OLD_TRAVERSE_HELPER

// ******************************************************************

void mclib_process::count_edges(bool, traverse_helper&) const 
{
      throw "not implemented";
}

// ******************************************************************

void mclib_process::traverse(bool rt, bool one_step, traverse_helper &TH) const 
{
      throw "not implemented";
}

#else

// ******************************************************************

void mclib_process::count_edges(bool rt, ectl_reachgraph::CTL_traversal &TH) const 
{
  DCASSERT(chain);
  for (long i=0; i<getNumStates(); i++) {
    TH.set_obligations(i, chain->getNumEdgesFor(rt, i) );
  }
}

// ******************************************************************

void mclib_process::traverse(bool rt, GraphLib::BF_graph_traversal &T) const 
{
  DCASSERT(chain);
  if (rt) chain->traverseOutgoing(T);
  else    chain->traverseIncoming(T);
}

#endif


// ******************************************************************
// *                                                                *
// *            mclib_process::sparse_row_elems  methods            *
// *                                                                *
// ******************************************************************
/*
mclib_process::sparse_row_elems
::sparse_row_elems(const indexed_reachset::indexed_iterator &i)
 : I(i)
{
  alloc = 0;
  last = 0;
  index = 0;
  value = 0;
}

mclib_process::sparse_row_elems::~sparse_row_elems()
{
  free(index);
  free(value);
}

bool mclib_process::sparse_row_elems::Enlarge(int ns)
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

bool mclib_process::sparse_row_elems
::buildIncoming(Old_MCLib::Markov_chain* chain, int i)
{
  overflow = false;
  incoming = true;
  last = 0;
  chain->traverseTo(i, *this);
  if (overflow) return false;
  HeapSortAbstract(this, last);
  return true;
}

bool mclib_process::sparse_row_elems
::buildOutgoing(Old_MCLib::Markov_chain* chain, int i)
{
  overflow = false;
  incoming = false;
  last = 0;
  chain->traverseFrom(i, *this);
  if (overflow) return false;
  HeapSortAbstract(this, last);
  return true;
}

bool mclib_process::sparse_row_elems
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
  index[last] = I.index2ord(z);
  value[last] = ((float*) label)[0];
  last++;
  return false;
}
*/
// ******************************************************************
// *                                                                *
// *                    mclib_reachgraph methods                    *
// *                                                                *
// ******************************************************************

mclib_reachgraph::mclib_reachgraph(mclib_process* MC)
{
  chain = Share(MC);
  DCASSERT(chain);
}

mclib_reachgraph::~mclib_reachgraph()
{
  Delete(chain);
}

void mclib_reachgraph::getNumArcs(long &na) const
{
  DCASSERT(chain);
  chain->getNumArcs(na);
}

void mclib_reachgraph::showInternal(OutputStream &os) const
{
  DCASSERT(chain);
  chain->showInternal(os);
}

void mclib_reachgraph::showArcs(OutputStream &os, const show_options &opt, 
  state_lldsm::reachset* RSS, shared_state* st) const
{
  DCASSERT(chain);
  chain->showProc(os, opt, RSS, st);
}

#ifdef USE_OLD_TRAVERSE_HELPER
void mclib_reachgraph::count_edges(bool rt, traverse_helper&TH) const 
{
  DCASSERT(chain);
  chain->count_edges(rt, TH);
}

void mclib_reachgraph::traverse(bool rt, bool one_step, traverse_helper &TH) const 
{
  DCASSERT(chain);
  chain->traverse(rt, one_step, TH);
}

#else

void mclib_reachgraph::count_edges(bool rt, CTL_traversal &T) const 
{
  DCASSERT(chain);
  chain->count_edges(rt, T);
}

void mclib_reachgraph::traverse(bool rt, GraphLib::BF_graph_traversal &T) const 
{
  DCASSERT(chain);
  chain->traverse(rt, T);
}

#endif

void mclib_reachgraph::getDeadlocked(intset &r) const
{
  DCASSERT(chain);
  chain->getDeadlocked(r);
}

void mclib_reachgraph::getTSCCsSatisfying(intset &p) const
{
  DCASSERT(chain);
  chain->getTSCCsSatisfying(p);
}

