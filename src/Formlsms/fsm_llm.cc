
// $Id$

#include "fsm_llm.h"
#include "check_llm.h"
#include "../Timers/timers.h"
#include "../ExprLib/mod_inst.h"
#include "../ExprLib/mod_vars.h"
#include "../ExprLib/exprman.h"
#include "../include/heap.h"

#include "../Modules/expl_states.h"
#include "../Modules/statesets.h"
#include "../Modules/biginttype.h"


// External libs
#include "statelib.h"
#include "graphlib.h"
#include "lslib.h"
#include "intset.h"

// #define DEBUG_EG


// ******************************************************************
// *                                                                *
// *                                                                *
// *                       explicit_fsm class                       *
// *                                                                *
// *    Abstract base class, uses external graph library for FSM    *
// *                                                                *
// ******************************************************************

class explicit_fsm : public checkable_lldsm {

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

    class findall : public GraphLib::generic_graph::element_visitor {
      bool dead;
      intset &ss;
    public:
      findall(bool d, intset &s) : ss(s) { dead = d; } 
      virtual ~findall() { };
      virtual bool visit(long from, long to, void*) {
        long remove = dead ? from : to;
#ifdef DEBUG_EG
        em->cout() << "\tremoving " << remove << "\n";
#endif
        ss.removeElement(remove);
        return ss.cardinality() == 0;
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

  virtual long getNumStates(bool show) const;
  virtual void getReachable(result &ss) const;
  virtual void getPotential(expr* p, result &ss) const;
  virtual void getInitialStates(result &x) const;

  // graph requirements
  virtual long getNumArcs(bool show) const;
  virtual void countPaths(const intset &src, const intset &dest, result& count);
  virtual bool requireByRows(const named_msg* rep);
  virtual bool requireByCols(const named_msg* rep);
  virtual long getOutgoingEdges(long from, ObjectList <int> *e) const;
  virtual long getIncomingEdges(long from, ObjectList <int> *e) const;
  virtual bool getOutgoingCounts(long* a) const;

  // checkable requirements
  virtual bool isAbsorbing(long st) const;
  virtual bool isDeadlocked(long st) const;

  virtual void findDeadlockedStates(stateset &ss) const;
  virtual bool forward(const intset &p, intset &r) const;
  virtual bool backward(const intset &p, intset &r) const;

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
        @param  s   Stream to write to.
        @param  i   Index of the state to display,
                    according to Markov chain numbering.
  */
  virtual void ShowState(OutputStream &s, long i) const = 0;

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
 : checkable_lldsm(FSM)
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

explicit_fsm::explicit_fsm(long ns) : checkable_lldsm(FSM)
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

long explicit_fsm::getNumStates(bool show) const
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

long explicit_fsm::getNumArcs(bool show) const
{
  DCASSERT(edges);
  long na = edges->getNumEdges();

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
  const char* col;
  row = "From state ";
  col = "To state ";
  if (!by_rows) SWAP(row, col);

  if (DOT == graph_display_style) {
    em->cout() << "digraph fsm {\n";
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
  } else {
    em->cout() << "Reachability graph:\n";
  }

  sparse_row_elems foo(invmap);

  for (long i=0; i<num_states; i++) {
    long h = map ? map[i] : i;
    CHECK_RANGE(0, h, num_states);
    if (DOT != graph_display_style) {
      em->cout() << row;
      if (display_graph_node_names)  ShowState(em->cout(), h);
      else        em->cout() << i;
      em->cout() << ":\n";
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
      if (DOT == graph_display_style) {
        em->cout() << "s" << i << " -> s" << foo.index[z] << ";";
      } else {
        em->cout() << col;
        if (display_graph_node_names) {
          long h = map ? map[foo.index[z]] : foo.index[z];
          CHECK_RANGE(0, h, num_states);
          ShowState(em->cout(), h);
        } else em->cout() << foo.index[z];
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

void explicit_fsm
::countPaths(const intset &src, const intset &dest, result& count)
{
  DCASSERT(edges);
  if (0==num_states) {
    count.setNull();
    return;
  }

  // build backward set from dest, with card "nb".
  timer* sw = makeTimer();
  if (numpaths_report.startReport()) {
    numpaths_report.report() << "Building backward set\n";
    numpaths_report.stopIO();
    sw->reset();
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
    numpaths_report.report() << "Built    backward set, took " << sw->elapsed();
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

  if (numpaths_report.startReport()) {
    numpaths_report.report() << "Counting paths\n";
    numpaths_report.stopIO();
    sw->reset();
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
    numpaths_report.report() << sw->elapsed() << " seconds\n";
    numpaths_report.stopIO();
  }

  // cleanup
  if (numpaths_report.startReport()) {
    numpaths_report.report() << "Cleaning up\n";
    numpaths_report.stopIO();
    sw->reset();
  }
  delete[] stack;
  for (long i=0; i<num_states; i++) Delete(paths[i]);
  delete[] paths;
  delete[] shortpc;
  if (numpaths_report.startReport()) {
    numpaths_report.report() << "Cleanup took ";
    numpaths_report.report() << sw->elapsed() << " seconds\n";
    numpaths_report.stopIO();
  }
  doneTimer(sw);

  if (acc.cmp_si(0) < 0) {
    count.setInfinity(1);
  } else {
    count.setPtr(new bigint(acc));
  }
}


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

void explicit_fsm::findDeadlockedStates(stateset &ss) const
{
  findall foo(true, ss.changeExplicit());
  edges->traverseAll(foo);
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


bool explicit_fsm::dumpDot(OutputStream &s) const
{
  DCASSERT(edges);
  s << "digraph fsm {\n";
  for (long i=0; i<num_states; i++) {
    s << "\ts" << i << " [label=\"";
    ShowState(s, i);
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
  timer* sw = 0;
  if (rep && rep->startReport()) {
    rep->report() << "Transposing edges\n";
    rep->stopIO();
    sw = makeTimer();
  }

  // transpose edges 
  edges->unfinish();
  GraphLib::generic_graph::finish_options fo;
  fo.Store_By_Rows = byrows;
  fo.Will_Clear = false;
  try {
    edges->finish(fo);
    if (rep && rep->startReport()) {
      rep->report() << "Transposed  edges, took " << sw->elapsed();
      rep->report() << " seconds\n";
      rep->stopIO();
    }
    doneTimer(sw);
    return true;
  }
  catch (GraphLib::error e) {
    if (em->startError()) {
      em->noCause();
      em->cerr() << "Couldn't transpose graph: ";
      em->cerr() << e.getString() << "\n";
      em->stopIO();
    }
    doneTimer(sw);
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
  virtual void ShowState(OutputStream &s, long i) const;
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

void fsm_enum::ShowState(OutputStream &s, long i) const
{
  DCASSERT(state_handle);
  CHECK_RANGE(0, i, states->NumValues());
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

void fsm_expl::ShowState(OutputStream &s, long i) const
{
  shared_state* st = new shared_state(parent);
  getState(i, st);
  st->Print(s, 0);
  Delete(st);
}

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

checkable_lldsm* MakeEnumeratedFSM(LS_Vector &init, model_enum* ss, GraphLib::digraph* rg)
{
  return new fsm_enum(init, rg, ss);
}

checkable_lldsm* StartExplicitFSM(StateLib::state_db* ss)
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
