
// $Id$

#include "fsm_llm.h"
#include "graph_llm.h"
#include "../ExprLib/mod_inst.h"
#include "../ExprLib/mod_vars.h"
#include "../ExprLib/exprman.h"
#include "../include/heap.h"

#include "../Modules/expl_states.h"
#include "../Modules/expl_ssets.h"
#include "../Modules/biginttype.h"

// External libs
#include "statelib.h"
#include "graphlib.h"
#include "lslib.h"
#include "intset.h"
#include "timerlib.h"

// #define DEBUG_EG

// ******************************************************************
// *                                                                *
// *                         fsm_lib  class                         *
// *                                                                *
// ******************************************************************

class fsm_lib : public library {
public:
  fsm_lib() : library(false) { }
  virtual const char* getVersionString() const {
    return GraphLib::Version();
  }
  virtual bool hasFixedPointer() const { 
    return true; 
  }
};

// ******************************************************************
// *                                                                *
// *                                                                *
// *                       generic_fsm  class                       *
// *                                                                *
// *                                                                *
// ******************************************************************

/**
    New class for fsm formalisms, uses reachset and process classes.

    Huge TBD here.
*/
class generic_fsm : public graph_lldsm {
  public:
    generic_fsm(reachset* rss);
    virtual ~generic_fsm();

    virtual long getNumStates() const;
    virtual void getNumStates(result& count) const;
    virtual void showStates(bool internal) const;

#ifdef NEW_STATESETS
    virtual stateset* getReachable() const;
    virtual stateset* getPotential(expr* p) const;
    virtual stateset* getInitialStates() const;
#else
    virtual void getReachable(result &ss) const;
    virtual void getPotential(expr* p, result &ss) const;
    virtual void getInitialStates(result &x) const;
#endif

    // TBD - more methods eventually
  protected:
    virtual const char* getClassName() const { return "generic_fsm"; }

  private:
    reachset* RSS;
    // tbd - process here
};

// ******************************************************************
// *                                                                *
// *                      generic_fsm  methods                      *
// *                                                                *
// ******************************************************************

generic_fsm::generic_fsm(reachset* rss) : graph_lldsm(FSM)
{
  DCASSERT(rss);
  RSS = rss;
  RSS->setParent(this);
}

generic_fsm::~generic_fsm()
{
  Delete(RSS);
}

long generic_fsm::getNumStates() const
{
  DCASSERT(RSS);
  long ns;
  RSS->getNumStates(ns);
  return ns;
}

void generic_fsm::getNumStates(result& count) const
{
  DCASSERT(RSS);
  RSS->getNumStates(count);
}

void generic_fsm::showStates(bool internal) const
{
  DCASSERT(RSS);
  if (internal) {
    RSS->showInternal(em->cout());
  } else {
    shared_state* st = new shared_state(parent);
    RSS->showStates(em->cout(), stateDisplayOrder(), st);
    Delete(st);
  }
}

#ifdef NEW_STATESETS

stateset* generic_fsm::getReachable() const
{
  return RSS ? RSS->getReachable() : 0;
}

stateset* generic_fsm::getPotential(expr* p) const
{
  return RSS ? RSS->getPotential(p) : 0;
}

stateset* generic_fsm::getInitialStates() const
{
  return RSS ? RSS->getInitialStates() : 0;
}

#else

void generic_fsm::getReachable(result &ss) const
{
  DCASSERT(RSS);
  ss.setPtr(RSS->getReachable());
}

void generic_fsm::getPotential(expr* p, result &ss) const
{
  DCASSERT(RSS);
  ss.setPtr(RSS->getPotential(p));
}

void generic_fsm::getInitialStates(result &x) const
{
  DCASSERT(RSS);
  x.setPtr(RSS->getInitialStates());
}

#endif

// ******************************************************************
// *                                                                *
// *                           Front  end                           *
// *                                                                *
// ******************************************************************

void InitFSMLibs(exprman* em)
{
  static fsm_lib* fsml = 0;
 
  if (0==fsml) {
    fsml = new fsm_lib;
    em->registerLibrary(fsml);
  }
}

graph_lldsm* StartGenericFSM(graph_lldsm::reachset* rss)
{
  if (0==rss) return 0;
  return new generic_fsm(rss);
}

void FinishGenericFSM(lldsm* rs, LS_Vector &init) // TBD!
{
  generic_fsm* fsm = dynamic_cast <generic_fsm*> (rs);
  if (0==fsm) return;
  // fsm->FinishExpl(init, rg);
  // Clever method calls here
}

// ******************************************************************
// *                                                                *
// * OOOOO L     DDD         SSSS TTTTT U   U FFFFF FFFFF           *
// * O   O L     D  D       S       T   U   U F     F               *
// * O   O L     D   D       SSS    T   U   U FFF   FFF             *
// * O   O L     D  D           S   T   U   U F     F               *
// * OOOOO LLLLL DDD        SSSS    T    UUU  F     F               *
// *                                                                *
// ******************************************************************

// ******************************************************************
// *                                                                *
// *                                                                *
// *                       explicit_fsm class                       *
// *                                                                *
// *    Abstract base class, uses external graph library for FSM    *
// *                                                                *
// ******************************************************************

class explicit_fsm : public graph_lldsm {

    class sparse_row_elems : public GraphLib::generic_graph::element_visitor {
      int alloc;
      const long* invmap;
      bool incoming;
      bool overflow;
    public:
      int last;
      long* index;
    public:
      sparse_row_elems(const long* invmap);
      virtual ~sparse_row_elems();

    protected:
      bool Enlarge(int ns);
    public:
      bool buildIncoming(GraphLib::digraph* g, int i);
      bool buildOutgoing(GraphLib::digraph* g, int i);

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
      }
    };

    
    class statetype : public GraphLib::generic_graph::element_visitor {
    public:
      enum {
        Deadlocked,
        Absorbing,
        Other
      } status;

      statetype() { status = Deadlocked; }
      virtual ~statetype() { };
      virtual bool visit(long from, long to, void*) {
        if (from != to) {
          status = Other;
          return true;
        }
        status = Absorbing;
        return false;
      }
    };

    class pot_visit : public state_lldsm::state_visitor {
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
  LS_Vector initial;
  GraphLib::digraph* edges;
  GraphLib::generic_graph::matrix raw_edges;
public:
  explicit_fsm(const LS_Vector& init, GraphLib::digraph* g);
  explicit_fsm(long ns);
  virtual ~explicit_fsm();

protected:
  void Finish(LS_Vector& init, GraphLib::digraph* g);
 
public:

  virtual long getNumStates() const;
  virtual void showStates(bool internal) const;
#ifdef NEW_STATESETS
  virtual stateset* getReachable() const;
  virtual stateset* getPotential(expr* p) const;
  virtual stateset* getInitialStates() const;
#else
  virtual void getReachable(result &ss) const;
  virtual void getPotential(expr* p, result &ss) const;
  virtual void getInitialStates(result &x) const;
#endif

  // graph requirements
  virtual long getNumArcs() const;
  virtual void showArcs(bool internal) const;
#ifndef NEW_STATESETS
  virtual void countPaths(const intset &src, const intset &dest, result& count);
#endif
  virtual bool requireByRows(const named_msg* rep);
  virtual bool requireByCols(const named_msg* rep);
  virtual long getOutgoingEdges(long from, ObjectList <int> *e) const;
  virtual long getIncomingEdges(long from, ObjectList <int> *e) const;
  virtual bool getOutgoingCounts(long* a) const;

  // checkable requirements
  virtual bool isAbsorbing(long st) const;
  virtual bool isDeadlocked(long st) const;

#ifdef NEW_STATESETS
  virtual void findDeadlockedStates(stateset *ss) const;
  virtual bool forward(const stateset* p, stateset* r) const;
  virtual bool backward(const stateset* p, stateset* r) const;
#else
  virtual void findDeadlockedStates(stateset &ss) const;
  virtual bool forward(const intset &p, intset &r) const;
  virtual bool backward(const intset &p, intset &r) const;
#endif

  virtual bool dumpDot(OutputStream &s) const;
  
protected:
  /** Build display order mapping.
      Takes NATURAL, DISCOVERY, LEXICAL into account.
        @param  map   Array for the mapping.
                      On return, map[i] will give the
                      index of the ith state to display.
  */
  virtual void BuildStateMapping(long* map) const = 0;

  /** Display a state.
        @param  s         Stream to write to.
        @param  i         Index of the state to display,
                          according to Markov chain numbering.
        @param  internal  Internal representation, or human readable?
  */
  virtual void ShowState(OutputStream &s, long i, bool internal) const = 0;

  bool transposeEdges(const named_msg* rep, bool byrows);
};

// ******************************************************************
// *             explicit_fsm::sparse_row_elems methods             *
// ******************************************************************

explicit_fsm::sparse_row_elems
::sparse_row_elems(const long* im)
{
  alloc = 0;
  last = 0;
  index = 0;
  invmap = im;
}

explicit_fsm::sparse_row_elems::~sparse_row_elems()
{
  free(index);
}

bool explicit_fsm::sparse_row_elems::Enlarge(int ns)
{
  if (ns < alloc)   return true;
  if (0==alloc)     alloc = 16;
  while ((alloc < 256) && (alloc <= ns))  alloc *= 2;
  while (alloc <= ns)                     alloc += 256;
  index = (long*) realloc(index, alloc * sizeof(long));
  if (0==index)  return false;
  return true;
}

bool explicit_fsm::sparse_row_elems
::buildIncoming(GraphLib::digraph* edges, int i)
{
  overflow = false;
  incoming = true;
  last = 0;
  edges->traverseTo(i, *this);
  if (overflow) return false;
  HeapSortAbstract(this, last);
  return true;
}

bool explicit_fsm::sparse_row_elems
::buildOutgoing(GraphLib::digraph* edges, int i)
{
  overflow = false;
  incoming = false;
  last = 0;
  edges->traverseFrom(i, *this);
  if (overflow) return false;
  HeapSortAbstract(this, last);
  return true;
}

bool explicit_fsm::sparse_row_elems
::visit(long from, long to, void* )
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
  last++;
  return false;
}

// ******************************************************************
// *                explicit_fsm::pot_visit  methods                *
// ******************************************************************

explicit_fsm::pot_visit::pot_visit(const hldsm* mdl, expr* _p, intset &ps)
 : state_visitor(mdl), pset(ps)
{
  p = _p;
  p->PreCompute();
  x.answer = &tmp;
  pset.removeAll();
  ok = true;
}

bool explicit_fsm::pot_visit::visit()
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
// *                      explicit_fsm methods                      *
// ******************************************************************

explicit_fsm::explicit_fsm(const LS_Vector& init, GraphLib::digraph* rg)
 : graph_lldsm(FSM)
{
  DCASSERT(rg);
  edges = rg;
  num_states = edges->getNumNodes();
  initial = init;
  DCASSERT(0==init.d_value);
  DCASSERT(0==init.f_value);
  raw_edges.rowptr = 0;
  raw_edges.colindex = 0;
  raw_edges.value = 0;
}

explicit_fsm::explicit_fsm(long ns) : graph_lldsm(FSM)
{
  num_states = ns;
  edges = 0;
  raw_edges.rowptr = 0;
  raw_edges.colindex = 0;
  raw_edges.value = 0;
}

explicit_fsm::~explicit_fsm()
{
  delete edges;
  delete[] initial.index;
}

void explicit_fsm::Finish(LS_Vector& init, GraphLib::digraph* rg)
{
  DCASSERT(rg);
  DCASSERT(rg->getNumNodes() == num_states);
  edges = rg;
  initial = init;
  DCASSERT(0==init.d_value);
  DCASSERT(0==init.f_value);

  // Finish FSM
  GraphLib::digraph::finish_options o;
  o.Store_By_Rows = true;  // for now... what is better for CTL?
  o.Will_Clear = false;
  edges->finish(o);
}

long explicit_fsm::getNumStates() const
{
  return num_states;
    
}

void explicit_fsm::showStates(bool internal) const
{
  if (!em->hasIO()) return;

  if (internal) {

    for (long i=0; i<num_states; i++) {
      em->cout() << "State " << i << " internal: ";
      ShowState(em->cout(), i, true);
      em->cout() << "\n";
      em->cout().Check();
    } // for i
    em->cout().flush();

  } else {
    if (0==num_states) return;
    if (tooManyStates(num_states, true)) return;

    long* map = 0;
    if (NATURAL != stateDisplayOrder()) {
      map = new long[num_states];
      BuildStateMapping(map);
    }

    for (long i=0; i<num_states; i++) {
      long mi = map ? map[i] : i; 
      em->cout() << "State " << i << ": ";
      ShowState(em->cout(), mi, false);
      em->cout() << "\n";
      em->cout().Check();
    } // for i
    delete[] map;
    em->cout().flush();
  }

  
}

#ifdef NEW_STATESETS

stateset* explicit_fsm::getReachable() const
{
  intset* all = new intset(num_states);
  all->addAll();
  return new expl_stateset(this, all);
}

stateset* explicit_fsm::getPotential(expr* p) const
{
  intset* all = new intset(num_states);
  if (p) {
    pot_visit pv(GetParent(), p, *all);
    visitStates(pv);
    if (!pv.isOK()) {
      delete all;
      return 0;
    } 
  } else {
    all->removeAll();
  }
  return new expl_stateset(this, all);
}

stateset* explicit_fsm::getInitialStates() const
{
  DCASSERT(edges);
  intset* initss = new intset(num_states);
  initss->removeAll();
  
  if (initial.index) {
    for (long z=0; z<initial.size; z++)
      initss->addElement(initial.index[z]);
  } 

  return new expl_stateset(this, initss);
}

#else

void explicit_fsm::getReachable(result &rs) const
{
  if (0==num_states) {
    rs.setNull();
    return;
  }
  intset* all = new intset(num_states);
  all->addAll();
  rs.setPtr(new stateset(this, all));
}

void explicit_fsm::getPotential(expr* p, result &ss) const
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

void explicit_fsm::getInitialStates(result &x) const
{
  DCASSERT(edges);
  if (0==num_states) {
    x.setNull();
    return;
  }
  intset* initss = new intset(num_states);
  initss->removeAll();
  
  if (initial.index) {
    for (long z=0; z<initial.size; z++)
      initss->addElement(initial.index[z]);
  } 

  x.setPtr(new stateset(this, initss));
}

#endif

long explicit_fsm::getNumArcs() const
{
  DCASSERT(edges);
  return edges->getNumEdges();
}

void explicit_fsm::showArcs(bool internal) const
{
  DCASSERT(edges);
  if (internal) {
    em->cout() << "Internal representation for graph:\n";

    GraphLib::digraph::matrix m;
    if (!edges->exportFinished(m)) {
      em->cout() << "  Couldn't export graph to matrix\n";
      return;
    } 

    const char* rptr = (m.is_transposed) ? "column pointers" : "row pointers";
    const char* cind = (m.is_transposed) ? "row index      " : "column index";

    em->cout() << "  " << rptr << ": [";
    em->cout().PutArray(m.rowptr, num_states+1);
    em->cout() << "]\n";

    em->cout() << "  " << cind << ": [";
    em->cout().PutArray(m.colindex, edges->getNumEdges());
    em->cout() << "]\n";
    
    return;
  }

  // TBD - what to do for internal?

  long na = edges->getNumEdges();

  if (!em->hasIO())                     return;
  if (tooManyStates(num_states, true))  return;
  if (tooManyArcs(na, true))            return;

  long* map = 0;
  long* invmap = 0;
  if (NATURAL != stateDisplayOrder()) {
    map = new long[num_states];
    BuildStateMapping(map);
    invmap = new long[num_states];
    for (long i=0; i<num_states; i++) invmap[map[i]] = i;
  }

  bool by_rows = (OUTGOING == graphDisplayStyle());
  const char* row;
  const char* col;
  row = "From state ";
  col = "To state ";
  if (!by_rows) SWAP(row, col);

  switch (graphDisplayStyle()) {
    case DOT:
        em->cout() << "digraph fsm {\n";
        for (long i=0; i<num_states; i++) {
          long mi = map ? map[i] : i;
          em->cout() << "\ts" << i;
          if (displayGraphNodeNames()) {
            em->cout() << " [label=\"";
            ShowState(em->cout(), mi, false);
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
        em->cout() << "Reachability graph:\n";
  }

  sparse_row_elems foo(invmap);

  for (long i=0; i<num_states; i++) {
    long h = map ? map[i] : i;
    CHECK_RANGE(0, h, num_states);
    switch (graphDisplayStyle()) {
      case INCOMING:
      case OUTGOING:
          em->cout() << row;
          if (displayGraphNodeNames())   ShowState(em->cout(), h, false);
          else                            em->cout() << i;
          em->cout() << ":\n";

      default:
          // nothing
          break;
    }

    bool ok;
    if (by_rows)   ok = foo.buildOutgoing(edges, h);
    else           ok = foo.buildIncoming(edges, h);

    // were we successful?
    if (!ok) {
      // out of memory, bail
      if (em->startError()) {
        em->noCause();
        em->cerr() << "Not enough memory to display reachability graph.";
        em->stopIO();
      }
      break;
    }

    // display row/column
    for (long z=0; z<foo.last; z++) {
      em->cout().Put('\t');
      switch (graphDisplayStyle()) {
        case DOT:
            em->cout() << "s" << foo.index[z] << " -> s" << i << ";";
            break;

        case TRIPLES:
            em->cout() << foo.index[z] << " " << i;
            break;

        default:
            em->cout() << col;
            if (displayGraphNodeNames()) {
              long h = map ? map[foo.index[z]] : foo.index[z];
              CHECK_RANGE(0, h, num_states);
              ShowState(em->cout(), h, false);
            } else {
              em->cout() << foo.index[z];
            }
      }
      em->cout().Put('\n');
    } // for z

    em->cout().Check();
  } // for i
  delete[] invmap;
  delete[] map;
  if (DOT == graphDisplayStyle()) {
    em->cout() << "}\n";
  }
  em->cout().flush();
}

#ifndef NEW_STATESETS

void explicit_fsm
::countPaths(const intset &src, const intset &dest, result& count)
{
  DCASSERT(edges);
  if (0==num_states) {
    count.setNull();
    return;
  }

  // build backward set from dest, with card "nb".
  timer sw;
  if (numpaths_report.startReport()) {
    numpaths_report.report() << "Building backward set\n";
    numpaths_report.stopIO();
  }
  intset back(num_states);
  long nb = 0;
  if (edges->isByCols()) {
    back.removeAll();
    for (long s=dest.getSmallestAfter(-1); s>=0; s=dest.getSmallestAfter(s)) {
      nb += edges->getReachable(s, back);
    } // for s
  } else {
    back = dest;
    while (edges->getBackward(back, back));
    nb = back.cardinality();
  } // else not by columns
  if (numpaths_report.startReport()) {
    numpaths_report.report() << "Built    backward set, took " << sw.elapsed_seconds();
    numpaths_report.report() << " seconds\n";
    numpaths_report.stopIO();
  }

  // force us by rows
  if (edges->isByCols()) {
    if (!transposeEdges(&numpaths_report, false)) {
      count.setNull();
      return;
    }
  } 

  // export graph edges
  DCASSERT(edges->isByRows());
  GraphLib::generic_graph::matrix rg;
  if (!edges->exportFinished(rg)) {
    DCASSERT(0);
  } else {
    DCASSERT(!rg.is_transposed);
    DCASSERT(rg.rowptr);
    DCASSERT(rg.colindex);
  }

  bigint acc;

  sw.reset();
  if (numpaths_report.startReport()) {
    numpaths_report.report() << "Counting paths\n";
    numpaths_report.stopIO();
  }

  // Initialize paths
  long* shortpc = new long[num_states];
  const long VISITING = -1;
  const long UNBOUNDED = -2;
  const long AS_BIGINT = -3;
  for (long i=0; i<num_states; i++) {
    shortpc[i] = dest.contains(i) ? 1 : 0;
  } 
  bigint** paths = new bigint*[num_states];
  for (long i=0; i<num_states; i++) {
    paths[i] = 0;
  } 

  // Initialize stack
  long* stack = new long[1+nb];
  long stack_top = 0;
  for (long s=src.getSmallestAfter(-1); s>=0; s=src.getSmallestAfter(s)) {
    if (back.contains(s)) {
      // Push(s)
      CHECK_RANGE(0, stack_top, 1+nb);
      stack[stack_top++] = s;
    }
  } // for s


  // Depth first search, starting from "src" states.
  while (stack_top) {
    // Pop(visit)
    stack_top--;
    CHECK_RANGE(0, stack_top, 1+nb);
    long visit = stack[stack_top];

    // null pathcount: push again, and push "reachable" children
    if (0==shortpc[visit]) {
      // Push(visit)
      CHECK_RANGE(0, stack_top, 1+nb);
      stack[stack_top++] = visit;

      shortpc[visit] = VISITING;

      // for all outgoing edges from this state
      for (long z=rg.rowptr[visit]; z<rg.rowptr[visit+1]; z++) {
        long next = rg.colindex[z];
        if (!back.contains(next)) continue;
        if (0==shortpc[next]) {
          // Push(next)
          CHECK_RANGE(0, stack_top, 1+nb);
          stack[stack_top++] = next;
        } else {
          // already visited this state, check for graph cycle
          if ((VISITING == shortpc[next]) || (UNBOUNDED == shortpc[next])) {
            // negative: either we're still visiting next (cycle),
            // or we've determined #paths from next is unbounded.
            // either way, the number of paths from here is unbounded.
            shortpc[visit] = UNBOUNDED;
          }
        } // else 0!=shortpc[next]
      } // for z
      continue;
    } // if 0==shortpc[visit]

    if (shortpc[visit] != VISITING) continue; // already know #paths from here

    // should have #paths computed for all children, add them up   
    acc.set_si(0);
    for (long z=rg.rowptr[visit]; z<rg.rowptr[visit+1]; z++) {
      long next = rg.colindex[z];
      if (shortpc[next] >= 0) {
        acc.add_ui(shortpc[next]);
        continue;
      }
      if ((VISITING == shortpc[next]) || (UNBOUNDED == shortpc[next])) {
        acc.set_si(UNBOUNDED);
        break;
      }
      DCASSERT(AS_BIGINT == shortpc[next]);
      DCASSERT(paths[next]);
      acc.add(acc, *paths[next]);
    } // for z
    
    // save the result
    if (acc.fits_slong()) {
      shortpc[visit] = acc.get_si();
    } else {
      shortpc[visit] = AS_BIGINT;
      paths[visit] = new bigint(acc);
    }
  } // while stack not empty

 
  // Have #paths for every state, add them up for src states
  acc.set_si(0);
  for (long s=src.getSmallestAfter(-1); s>=0; s=src.getSmallestAfter(s)) {
    if (shortpc[s] >= 0) {
      acc.add_ui(shortpc[s]);
      continue;
    }
    DCASSERT(VISITING != shortpc[s]);
    if (UNBOUNDED == shortpc[s]) {
      acc.set_si(UNBOUNDED);
      break;
    }
    DCASSERT(AS_BIGINT == shortpc[s]);
    DCASSERT(paths[s]);
    acc.add(acc, *paths[s]);
  } // for s

  if (numpaths_report.startReport()) {
    numpaths_report.report() << "Counted  paths, took ";
    numpaths_report.report() << sw.elapsed_seconds() << " seconds\n";
    numpaths_report.stopIO();
  }

  // cleanup
  sw.reset();
  if (numpaths_report.startReport()) {
    numpaths_report.report() << "Cleaning up\n";
    numpaths_report.stopIO();
  }
  delete[] stack;
  for (long i=0; i<num_states; i++) Delete(paths[i]);
  delete[] paths;
  delete[] shortpc;
  if (numpaths_report.startReport()) {
    numpaths_report.report() << "Cleanup took ";
    numpaths_report.report() << sw.elapsed_seconds() << " seconds\n";
    numpaths_report.stopIO();
  }

  if (acc.cmp_si(0) < 0) {
    count.setInfinity(1);
  } else {
    count.setPtr(new bigint(acc));
  }
}

#endif

bool explicit_fsm::requireByRows(const named_msg* rep)
{
  if (edges->isByRows())  return true;
  if (transposeEdges(rep, true)) {
    return edges->exportFinished(raw_edges);
  }
  return false;
}

bool explicit_fsm::requireByCols(const named_msg* rep)
{
  if (edges->isByCols())  return true;
  if (transposeEdges(rep, false)) {
    return edges->exportFinished(raw_edges);
  }
  return false;
}

long explicit_fsm::getOutgoingEdges(long from, ObjectList <int> *e) const
{
  if (raw_edges.is_transposed) return -1;
  DCASSERT(raw_edges.rowptr);
  if (0==raw_edges.colindex) return 0;  // no edges!
  DCASSERT(raw_edges.colindex);
  CHECK_RANGE(0, from, edges->getNumNodes());

  if (e) {
    for (long z=raw_edges.rowptr[from]; z<raw_edges.rowptr[from+1]; z++) {
      e->Append(raw_edges.colindex[z]);
    }
  }
  return raw_edges.rowptr[from+1] - raw_edges.rowptr[from];
}

long explicit_fsm::getIncomingEdges(long to, ObjectList <int> *e) const
{
  if (!raw_edges.is_transposed) return -1;
  DCASSERT(raw_edges.rowptr);
  if (0==raw_edges.colindex) return 0;  // no edges!
  DCASSERT(raw_edges.colindex);
  CHECK_RANGE(0, to, edges->getNumNodes());

  if (e) {
    for (long z=raw_edges.rowptr[to]; z<raw_edges.rowptr[to+1]; z++) {
      e->Append(raw_edges.colindex[z]);
    }
  }
  return raw_edges.rowptr[to+1] - raw_edges.rowptr[to];
}

bool explicit_fsm::getOutgoingCounts(long* a) const
{
  DCASSERT(raw_edges.rowptr);
  if (0==raw_edges.colindex) return true; // no edges!
  DCASSERT(raw_edges.colindex);
  if (raw_edges.is_transposed) {
    for (long z=raw_edges.rowptr[edges->getNumNodes()]-1; z>=0; z--) {
      CHECK_RANGE(0, raw_edges.colindex[z], edges->getNumNodes());
      a[raw_edges.colindex[z]]++;
    } // for z
  } else {
    for (long z=edges->getNumNodes()-1; z>=0; z--) 
      a[z] += raw_edges.rowptr[z+1] - raw_edges.rowptr[z];
  }
  return true;
}

bool explicit_fsm::isAbsorbing(long st) const
{
  // only efficient if edges are stored "by rows".
  statetype foo;
  edges->traverseFrom(st, foo);
  return (statetype::Other != foo.status);
}

bool explicit_fsm::isDeadlocked(long st) const
{
  // only efficient if edges are stored "by rows".
  statetype foo;
  edges->traverseFrom(st, foo);
  return (statetype::Deadlocked == foo.status);
}

#ifdef NEW_STATESETS

void explicit_fsm::findDeadlockedStates(stateset* ss) const
{
  expl_stateset* ess = smart_cast <expl_stateset*> (ss);
  DCASSERT(ess);
  edges->noOutgoingEdges(ess->changeExplicit());
}

bool explicit_fsm::forward(const stateset* p, stateset* r) const
{
  const expl_stateset* ep = smart_cast <const expl_stateset*> (p);
  DCASSERT(ep);
  expl_stateset* er = smart_cast <expl_stateset*> (r);
  DCASSERT(er);
  DCASSERT(edges);
  return edges->getForward(ep->getExplicit(), er->changeExplicit());
}

bool explicit_fsm::backward(const stateset* p, stateset* r) const
{
  const expl_stateset* ep = smart_cast <const expl_stateset*> (p);
  DCASSERT(ep);
  expl_stateset* er = smart_cast <expl_stateset*> (r);
  DCASSERT(er);
  DCASSERT(edges);
  return edges->getBackward(ep->getExplicit(), er->changeExplicit());
}


#else

void explicit_fsm::findDeadlockedStates(stateset &ss) const
{
  edges->noOutgoingEdges(ss.changeExplicit());
}

bool explicit_fsm::forward(const intset &p, intset &r) const
{
  DCASSERT(edges);
  return edges->getForward(p, r);
}

bool explicit_fsm::backward(const intset &p, intset &r) const
{
  DCASSERT(edges);
  return edges->getBackward(p, r);
}

#endif

bool explicit_fsm::dumpDot(OutputStream &s) const
{
  DCASSERT(edges);
  s << "digraph fsm {\n";
  for (long i=0; i<num_states; i++) {
    s << "\ts" << i << " [label=\"";
    ShowState(s, i, false);
    s << "\"];\n";
    s.can_flush();
  } // for i
  s << "\n";

  sparse_row_elems foo(0);
  for (long i=0; i<num_states; i++) {
    bool ok = foo.buildOutgoing(edges, i);
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
      s << "\ts" << i << " -> s" << foo.index[z] << ";\n";
    } // for z
    s.can_flush();
  } // for i
  s << "}\n";
  return true;
}

bool explicit_fsm::transposeEdges(const named_msg* rep, bool byrows)
{
  timer sw;
  if (rep && rep->startReport()) {
    rep->report() << "Transposing edges\n";
    rep->stopIO();
  }

  // transpose edges 
  edges->unfinish();
  GraphLib::generic_graph::finish_options fo;
  fo.Store_By_Rows = byrows;
  fo.Will_Clear = false;
  try {
    edges->finish(fo);
    if (rep && rep->startReport()) {
      rep->report() << "Transposed  edges, took " << sw.elapsed_seconds();
      rep->report() << " seconds\n";
      rep->stopIO();
    }
    return true;
  }
  catch (GraphLib::error e) {
    if (em->startError()) {
      em->noCause();
      em->cerr() << "Couldn't transpose graph: ";
      em->cerr() << e.getString() << "\n";
      em->stopIO();
    }
    return false;
  }
}


// ******************************************************************
// *                                                                *
// *                         fsm_enum class                         *
// *                                                                *
// ******************************************************************

class fsm_enum : public explicit_fsm {
protected:
  long* state_handle;
  model_enum* states;
public:
  /// Constructor for enumerated graphs.
  fsm_enum(const LS_Vector& init, GraphLib::digraph* fsm, model_enum* st);
  virtual ~fsm_enum();
  virtual void visitStates(state_visitor &v) const;
  virtual shared_object* getEnumeratedState(long i) const;
protected:
  const char* getClassName() const { return "fsm_enum"; }
  virtual void BuildStateMapping(long* map) const;
  virtual void ShowState(OutputStream &s, long i, bool internal) const;
};

// ******************************************************************
// *                        fsm_enum methods                        *
// ******************************************************************

fsm_enum::fsm_enum(const LS_Vector& init, GraphLib::digraph* fsm, model_enum* st)
 : explicit_fsm(init, fsm)
{
  states = st;
  state_handle = new long[states->NumValues()];
  for (long j=0; j<states->NumValues(); j++) {
    const model_enum_value* st = states->ReadValue(j);
    CHECK_RANGE(0, st->GetIndex(), states->NumValues());
    state_handle[st->GetIndex()] = j;
  }
}

fsm_enum::~fsm_enum()
{
  Delete(states);
  delete[] state_handle;
}

void fsm_enum::visitStates(state_visitor &v) const
{
  DCASSERT(states);
  DCASSERT(v.state());
  for (v.index()=0; v.index() < states->NumValues(); v.index()++) {
    if (v.canSkipIndex()) continue;
    v.state()->set(states->GetIndex(), v.index());
    if (v.visit()) return;
  }
}

shared_object* fsm_enum::getEnumeratedState(long i) const
{
  DCASSERT(state_handle);
  return Share(states->GetValue(state_handle[i]));
}

void fsm_enum::BuildStateMapping(long* map) const
{
  // invert state_handle
  long* hs = new long[states->NumValues()];
  for (long i=0; i<states->NumValues(); i++) {
    CHECK_RANGE(0, state_handle[i], states->NumValues());
    hs[state_handle[i]] = i;
  }
  switch (stateDisplayOrder()) {
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

void fsm_enum::ShowState(OutputStream &s, long i, bool internal) const
{
  DCASSERT(state_handle);
  CHECK_RANGE(0, i, states->NumValues());
  if (internal) s << "(index " << state_handle[i] << ") ";
  const model_enum_value* st = states->ReadValue(state_handle[i]);
  DCASSERT(st);
  s.Put(st->Name());
}


// ******************************************************************
// *                                                                *
// *                         fsm_expl class                         *
// *                                                                *
// ******************************************************************

class fsm_expl : public explicit_fsm {
  StateLib::state_db* fullstates;
protected:
  long* state_handle;
  StateLib::state_coll* states;
public:
  fsm_expl(StateLib::state_db* st);
protected:
  virtual ~fsm_expl();
public:
  void FinishExpl(LS_Vector& init, GraphLib::digraph* fsm);
  inline StateLib::state_db* getAllStates() { return fullstates; }
  virtual void visitStates(state_visitor &v) const;
  virtual void reportMemUsage(exprman* em, const char* prefix) const;
protected:
  const char* getClassName() const { return "fsm_expl"; }
  virtual void BuildStateMapping(long* map) const;
  virtual void ShowState(OutputStream &s, long i, bool internal) const;

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

  inline const unsigned char* getInternal(long i, long &bytes) const {
    DCASSERT(false == parent->containsListVar());
    CHECK_RANGE(0, i, num_states);
    if (fullstates) {
      return fullstates->GetRawState(i, bytes);
    } else {
      DCASSERT(states);
      DCASSERT(state_handle);
      return states->GetRawState(state_handle[i], bytes);
    }
  }
};

// ******************************************************************
// *                        fsm_expl methods                        *
// ******************************************************************

fsm_expl::fsm_expl(StateLib::state_db* st) : explicit_fsm(st->Size())
{
  state_handle = 0;
  states = 0;
  fullstates = st;
  fullstates->ConvertToStatic(true);
}

fsm_expl::~fsm_expl()
{
  delete fullstates;
  delete states;
  delete[] state_handle;
}

void fsm_expl::FinishExpl(LS_Vector& init, GraphLib::digraph* fsm)
{
  // Compact states
  states = fullstates->TakeStateCollection();
  delete fullstates;
  fullstates = 0;
  state_handle = states->RemoveIndexHandles();

  Finish(init, fsm);
}

void fsm_expl::visitStates(state_visitor &v) const
{
  DCASSERT(v.state());
  for (v.index()=0; v.index() < num_states; v.index()++) {
    if (v.canSkipIndex()) continue;
    getState(v.index(), v.state());
    if (v.visit()) return;
  }
}

void fsm_expl::reportMemUsage(exprman* em, const char* prefix) const
{
  long mem = states ? states->ReportMemTotal() : 0;
  if (state_handle) mem += num_states * sizeof(long);
  em->report().Put(prefix);
  em->report().PutMemoryCount(mem, 3);
  em->report() << " required for state space storage\n";
  mem = edges ? edges->ReportMemTotal() : 0;
  em->report().Put(prefix);
  em->report().PutMemoryCount(mem, 3);
  em->report() << " required for reachability graph storage\n";
}

// Protected:

void fsm_expl::BuildStateMapping(long* map) const
{
  switch (stateDisplayOrder()) {
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

void fsm_expl::ShowState(OutputStream &s, long i, bool internal) const
{
  if (internal) {
    long bytes = 0;
    const unsigned char* ptr = getInternal(i, bytes);
    for (long b=0; b<bytes; b++) {
      s.PutHex(ptr[b]);
      s.Put(' ');
    }
  } else {
    shared_state* st = new shared_state(parent);
    getState(i, st);
    st->Print(s, 0);
    Delete(st);
  }
}

// ******************************************************************
// *                                                                *
// *                           Front  end                           *
// *                                                                *
// ******************************************************************

graph_lldsm* MakeEnumeratedFSM(LS_Vector &init, model_enum* ss, GraphLib::digraph* rg)
{
  return new fsm_enum(init, rg, ss);
}

graph_lldsm* StartExplicitFSM(StateLib::state_db* ss)
{
  return new fsm_expl(ss);
}

StateLib::state_db* GrabExplicitFSMStates(lldsm* rg)
{
  fsm_expl* fsm = dynamic_cast <fsm_expl*> (rg);
  if (0==fsm) return 0;
  return fsm->getAllStates();
}

void FinishExplicitFSM(lldsm* rs, LS_Vector &init, GraphLib::digraph* rg)
{
  if (0==rg) return;
  fsm_expl* fsm = dynamic_cast <fsm_expl*> (rs);
  if (0==fsm) return;
  fsm->FinishExpl(init, rg);
}
