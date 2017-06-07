
#include "rgr_grlib.h"
#include "rss_indx.h"

#include "../Streams/streams.h"
#include "../ExprLib/exprman.h"
#include "../ExprLib/engine.h"
#include "../include/heap.h"
#include "../Modules/expl_ssets.h"
#include "../Modules/biginttype.h"

// Alas, I have succumbed to STL
#include <set>

// External libs

#include "../_Timer/timerlib.h"

// ******************************************************************
// *                    grlib_display_row  class                    *
// ******************************************************************


class grlib_display_row : public GraphLib::BF_graph_traversal {
  public:
    grlib_display_row(indexed_reachset::indexed_iterator &_I, std::set<long> &_coll);

    void set_row(long r);

    virtual bool hasNodesToExplore();
    virtual long getNextToExplore();

    virtual bool visit(long, long dest, void*);

  private:
    indexed_reachset::indexed_iterator &I;
    std::set<long> &coll;

    long row;
};

// ******************************************************************
// *                   grlib_display_row  methods                   *
// ******************************************************************

grlib_display_row::grlib_display_row(indexed_reachset::indexed_iterator &_I, 
  std::set<long> &_coll) : I(_I), coll(_coll)
{
  row = -1;
}

void grlib_display_row::set_row(long r)
{
  row = r;
  coll.clear();
}

bool grlib_display_row::hasNodesToExplore()
{
  return row >= 0;
}

long grlib_display_row::getNextToExplore()
{
  long next = row;
  row = -1;
  return next;
}

bool grlib_display_row::visit(long, long dest, void*)
{
  coll.insert( I.index2ord(dest) );
  return false;
}

// ******************************************************************
// *                   grlib_display_style  class                   *
// ******************************************************************

/**
  For displaying graphs in various styles.
  Defaults are here, in the base class.
*/
class grlib_display_style {
  public:
    grlib_display_style(OutputStream &_os, state_lldsm::reachset* _RSS,
      indexed_reachset::indexed_iterator &_I, shared_state* _st, 
      bool state_names);

    virtual ~grlib_display_style();

    virtual void header();
    virtual void start_row(long row);
    virtual void show_edge(long src, long dest);
    virtual void finish_row();
    virtual void footer();

  protected:
    inline OutputStream &out() { return os; }

    virtual void showState(long s);
     
  protected:
    indexed_reachset::indexed_iterator &I;
    bool show_state_names;
  private:
    OutputStream &os;
    state_lldsm::reachset* RSS;
    shared_state* st;
};

// ******************************************************************
// *                  grlib_display_style  methods                  *
// ******************************************************************

grlib_display_style::grlib_display_style(OutputStream &_os, 
  state_lldsm::reachset* _RSS, indexed_reachset::indexed_iterator &_I, 
  shared_state* _st, bool state_names) : I(_I), os(_os)
{
  RSS = _RSS;
  st = _st;
  show_state_names = state_names;
}

grlib_display_style::~grlib_display_style()
{
}

void grlib_display_style::header()
{
  out() << "Reachability graph:\n";
}

void grlib_display_style::start_row(long row)
{
  out() << "From state ";
  showState(row);
  out() << ":\n";
}

void grlib_display_style::show_edge(long src, long dest)
{
  out() << "\tTo state ";
  showState(dest);
  out() << "\n";
}

void grlib_display_style::finish_row()
{
  out().flush();
}

void grlib_display_style::footer()
{
}

void grlib_display_style::showState(long s)
{
  // s is the state, native to the graph
  if (show_state_names) {
    I.copyState(st, s);
    RSS->showState(os, st);
  } else {
    out() << s;
  }
}

// ******************************************************************
// *                    grlib_display_dot  class                    *
// ******************************************************************

class grlib_display_dot : public grlib_display_style {
  public:
    grlib_display_dot(OutputStream &_os, state_lldsm::reachset* _RSS,
      indexed_reachset::indexed_iterator &_I, shared_state* _st, 
      bool state_names);

    virtual void header();
    virtual void start_row(long row);
    virtual void show_edge(long src, long dest);
    virtual void footer();
};

// ******************************************************************
// *                   grlib_display_dot  methods                   *
// ******************************************************************

grlib_display_dot::grlib_display_dot(OutputStream &_os, 
  state_lldsm::reachset* _RSS, indexed_reachset::indexed_iterator &_I, 
  shared_state* _st, bool state_names)
 : grlib_display_style(_os, _RSS, _I, _st, state_names)
{
}

void grlib_display_dot::header()
{
  out() << "digraph fsm {\n";
  for (I.start(); I; I++) { 
    out() << "\ts" << I.index();
    out() << " [label=\"";
    showState(I.index());
    out() << "\"];\n";
    out().flush();
  } // for i
  out() << "\n";
}

void grlib_display_dot::start_row(long row)
{
}

void grlib_display_dot::show_edge(long src, long dest)
{
  out() << "\ts" << src << " -> s" << dest << ";\n";
}

void grlib_display_dot::footer()
{
  out() << "}\n";
  out().flush();
}


// ******************************************************************
// *                  grlib_display_triples  class                  *
// ******************************************************************

class grlib_display_triples : public grlib_display_style {
  public:
    grlib_display_triples(OutputStream &_os, state_lldsm::reachset* _RSS,
      indexed_reachset::indexed_iterator &_I, shared_state* _st, 
      long _num_states, long _num_arcs);

    virtual void header();
    virtual void start_row(long row);
    virtual void show_edge(long src, long dest);

  private:
    long num_states;
    long num_arcs;
};

// ******************************************************************
// *                 grlib_display_triples  methods                 *
// ******************************************************************

grlib_display_triples::grlib_display_triples(OutputStream &_os, 
  state_lldsm::reachset* _RSS, indexed_reachset::indexed_iterator &_I, 
  shared_state* _st, long _num_states, long _num_arcs)
 : grlib_display_style(_os, _RSS, _I, _st, false)
{
  num_states = _num_states;
  num_arcs = _num_arcs;
}

void grlib_display_triples::header()
{
  out() << num_states << "\n";
  out() << num_arcs << "\n";
}

void grlib_display_triples::start_row(long row)
{
}

void grlib_display_triples::show_edge(long src, long dest)
{
  out() << "\t" << src << " " << dest << "\n";
}


// ******************************************************************
// *                  grlib_display_incoming class                  *
// ******************************************************************

class grlib_display_incoming : public grlib_display_style {
  public:
    grlib_display_incoming(OutputStream &_os, state_lldsm::reachset* _RSS,
      indexed_reachset::indexed_iterator &_I, shared_state* _st, 
      bool _show_states);

    virtual void start_row(long row);
    virtual void show_edge(long src, long dest);
};

// ******************************************************************
// *                 grlib_display_incoming methods                 *
// ******************************************************************

grlib_display_incoming::grlib_display_incoming(OutputStream &_os, 
  state_lldsm::reachset* _RSS, indexed_reachset::indexed_iterator &_I, 
  shared_state* _st, bool state_names)
: grlib_display_style(_os, _RSS, _I, _st, state_names)
{
}

void grlib_display_incoming::start_row(long row)
{
  out() << "To state ";
  showState(row);
  out() << ":\n";
}

void grlib_display_incoming::show_edge(long src, long dest)
{
  out() << "\tFrom state ";
  showState(dest);
  out() << "\n";
}



// ******************************************************************
// *                                                                *
// *                    grlib_reachgraph methods                    *
// *                                                                *
// ******************************************************************

grlib_reachgraph::grlib_reachgraph(GraphLib::dynamic_digraph* g) : InEdges(), OutEdges()
{
  edges = g;
  // clear the initial vector
  initial.size = 0;
  initial.index = 0;
  initial.d_value = 0;
  initial.f_value = 0;

  DCASSERT(0==InEdges.getNumNodes());
  DCASSERT(0==OutEdges.getNumNodes());
}

grlib_reachgraph::~grlib_reachgraph()
{
  // in case we still have it
  delete edges;

  // in case we still have it:
  delete[] initial.index;

  // deadlocks, InEdges, and OutEdges should be destroyed 
  // automagically here; no need to do anything special, right?
}

void grlib_reachgraph::attachToParent(graph_lldsm* p, state_lldsm::reachset* RSS)
{
  ectl_reachgraph::attachToParent(p, RSS);

  indexed_reachset* irs = dynamic_cast <indexed_reachset*> (RSS);

  // Transfer the initial state, if we can
  if (irs) irs->setInitial(initial);

  // shrink the rss
  if (irs) irs->Finish();

  DCASSERT(edges);

  if (edges->isByRows()) {
    edges->exportAndDestroy(OutEdges, 0);
    InEdges.transposeFrom(OutEdges);
  } else {
    edges->exportAndDestroy(InEdges, 0);
    OutEdges.transposeFrom(InEdges);
  }
  // determine deadlocked states
  deadlocks.resetSize(OutEdges.getNumNodes());
  OutEdges.emptyRows(deadlocks);

  delete edges;  
  edges = 0;

}

void grlib_reachgraph::getNumArcs(long &na) const
{
  na = OutEdges.getNumEdges();
}

void grlib_reachgraph::showInternal(OutputStream &os) const
{
  os << "Internal representation for graph:\n";

  os << "    InEdges matrix:\n";
  showRawMatrix(os, InEdges);

  os << "    OutEdges matrix:\n";
  showRawMatrix(os, OutEdges);
}

void grlib_reachgraph::showArcs(OutputStream &os, const show_options &opt, 
  state_lldsm::reachset* RSS, shared_state* st) const
{

  if (state_lldsm::tooManyStates(OutEdges.getNumNodes(), &os))    return;
  if (graph_lldsm::tooManyArcs(OutEdges.getNumEdges(), &os))      return;

  bool by_rows = (graph_lldsm::INCOMING != opt.STYLE);
  const GraphLib::static_graph &Edges = by_rows ? OutEdges : InEdges;

  // TBD : try/catch around this
  indexed_reachset::indexed_iterator &I 
  = dynamic_cast <indexed_reachset::indexed_iterator &> (RSS->iteratorForOrder(opt.ORDER));

  //
  // Set up the display style.
  // This cleans up the code below.
  //
  grlib_display_style* display = 0;
  switch (opt.STYLE) {
    case graph_lldsm::DOT:
      display = new grlib_display_dot(os, RSS, I, st, opt.NODE_NAMES);
      break;

    case graph_lldsm::TRIPLES:
      display = new grlib_display_triples(os, RSS, I, st, 
          OutEdges.getNumNodes(), OutEdges.getNumEdges());
      break;

    case graph_lldsm::INCOMING:
      display = new grlib_display_incoming(os, RSS, I, st, opt.NODE_NAMES);
      break;

    // case graph_lldsm::OUTGOING:
    default:
      display = new grlib_display_style(os, RSS, I, st, opt.NODE_NAMES); 
  }

  //
  // Stuff for building the current row/col
  //
  std::set<long> coll;
  grlib_display_row rowbuild(I, coll);


  //
  // Ready to display graph
  //

  display->header();

  for (I.start(); I; I++) {
    // 
    // Start of another row/col
    //
    display->start_row(I.getI());
    rowbuild.set_row(I.index());
    Edges.traverse(rowbuild);

    //
    // Display row/column
    //
    std::set<long>::const_iterator pos;
    for (pos=coll.begin(); pos != coll.end(); ++pos) {
      display->show_edge(I.getI(), *pos);
    }

    display->finish_row();
  } // for I
  display->footer();

  //
  // Cleanup
  //

  delete display;
}

void grlib_reachgraph::setInitial(LS_Vector &init)
{
  DCASSERT(0==init.d_value);
  DCASSERT(0==init.f_value);
  initial = init;
}

void grlib_reachgraph
::countPaths(const stateset* src_ss, const stateset* dest_ss, result& count)
{
  // check types of statesets
  const expl_stateset* src_ess = dynamic_cast <const expl_stateset*> (src_ss);
  const expl_stateset* dest_ess = dynamic_cast <const expl_stateset*> (dest_ss);
  if (0==src_ess || 0==dest_ess) {
    if (em->startError()) {
      em->causedBy(0);
      em->cerr() << "Explicit reachability graph expecting explicit statesets for path count";
      em->stopIO();
    }
    count.setNull();
    return;
  }
  const intset& src = src_ess->getExplicit();
  const intset& dest = dest_ess->getExplicit();

  //
  // build backward set from dest, with card "nb".
  //
  timer sw;
  if (numpaths_report.startReport()) {
    numpaths_report.report() << "Building backward set\n";
    numpaths_report.stopIO();
  }

  const long num_states = InEdges.getNumNodes();
  intset back(num_states);
  {
    traverse_helper TH(num_states);
    TH.fill_obligations(1);
    TH.init_queue_from(dest);
    traverse(false, false, TH);
    TH.get_met_obligations(back);
  }
  long nb = back.cardinality();

  if (numpaths_report.startReport()) {
    numpaths_report.report() << "Built    backward set, took " << sw.elapsed_seconds();
    numpaths_report.report() << " seconds\n";
    numpaths_report.stopIO();
  }

  //
  // Ok, time to start counting paths
  //

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

  const long* outrowptr = OutEdges.RowPointer();
  const long* outcolindex = OutEdges.ColumnIndex();

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
      for (long z=outrowptr[visit]; z<outrowptr[visit+1]; z++) {
        long next = outcolindex[z];
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
    for (long z=outrowptr[visit]; z<outrowptr[visit+1]; z++) {
      long next = outcolindex[z];
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



void grlib_reachgraph::getDeadlocked(intset &r) const
{
  r.assignFrom(deadlocks);
}

void grlib_reachgraph::need_reverse_time()
{
  // Nothing
}


void grlib_reachgraph::count_edges(bool rt, traverse_helper &TH) const
{
  if (rt) {
    for (long i=0; i<InEdges.getNumNodes(); i++) {
      TH.set_obligations(i, InEdges.getNumEdgesFor(i));
    }
  } else {
    for (long i=0; i<OutEdges.getNumNodes(); i++) {
      TH.set_obligations(i, OutEdges.getNumEdgesFor(i));
    }
  }
}

void grlib_reachgraph::traverse(bool rt, bool one_step, traverse_helper &TH) const
{
  if (rt) _traverse(one_step, OutEdges, TH);
  else    _traverse(one_step, InEdges, TH);
}

void grlib_reachgraph::showRawMatrix(OutputStream &os, const GraphLib::static_graph &E)
{
  const char* rptr = (E.isByCols()) ? "column pointers" : "row pointers";
  const char* cind = (E.isByCols()) ? "row index      " : "column index";

  os << "      size: " << E.getNumNodes() << "\n";
  os << "      " << rptr << ": [";
  os.PutArray(E.RowPointer(), E.getNumNodes()+1);
  os << "]\n";

  os << "      " << cind << ": [";
  os.PutArray(E.ColumnIndex(), E.getNumEdges());
  os << "]\n";
}


void grlib_reachgraph::_traverse(bool one_step, const GraphLib::static_graph &E,
  ectl_reachgraph::traverse_helper &TH)
{
  if (one_step) {
    mygraphtraverse<true> foo(TH);
    E.traverse(foo);
  } else {
    mygraphtraverse<false> foo(TH);
    E.traverse(foo);
  }
}

// ******************************************************************
// *                                                                *
// *           grlib_reachgraph::mygraphtraverse  methods           *
// *                                                                *
// ******************************************************************

template <bool ONESTEP>
grlib_reachgraph::mygraphtraverse<ONESTEP>::mygraphtraverse(traverse_helper &th)
 : TH(th)
{
}

template <bool ONESTEP>
bool grlib_reachgraph::mygraphtraverse<ONESTEP>::hasNodesToExplore()
{
  return TH.queue_nonempty();
}

template <bool ONESTEP>
long grlib_reachgraph::mygraphtraverse<ONESTEP>::getNextToExplore()
{
  return TH.queue_pop();
}

template <bool ONESTEP>
bool grlib_reachgraph::mygraphtraverse<ONESTEP>::visit(long, long dest, void*)
{
  if (TH.num_obligations(dest) <= 0) return false;
  TH.remove_obligation(dest);
  if (ONESTEP) return false;
  if (0==TH.num_obligations(dest)) {
    TH.queue_push(dest);
  }
  return false;
}

