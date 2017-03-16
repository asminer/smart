
// $Id$

#include "rgr_grlib.h"
#include "rss_indx.h"

#include "../Streams/streams.h"
#include "../ExprLib/exprman.h"
#include "../ExprLib/engine.h"
#include "../include/heap.h"
#include "../Modules/expl_ssets.h"
#include "../Modules/biginttype.h"

// External libs

#include "timerlib.h"

// ******************************************************************
// *                                                                *
// *                    grlib_reachgraph methods                    *
// *                                                                *
// ******************************************************************

grlib_reachgraph::grlib_reachgraph(GraphLib::digraph* g) : OutEdges()
{
  edges = g;
  // clear the initial vector
  initial.size = 0;
  initial.index = 0;
  initial.d_value = 0;
  initial.f_value = 0;
  // determine deadlocked states
  DCASSERT(edges);
  deadlocks.resetSize(edges->getNumNodes());
  edges->noOutgoingEdges(deadlocks); 

  // Static incoming edges
  InEdges.num_rows = 0;
  InEdges.rowptr = 0;
  InEdges.colindex = 0;
  InEdges.value = 0;
}

grlib_reachgraph::~grlib_reachgraph()
{
  // in case we still have it
  delete edges;

  // in case we still have it:
  delete[] initial.index;

  // deadlocks should be destroyed automagically here

  // InEdges belongs to edges, don't delete it

  OutEdges.destroy();
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
  if (edges->isFinished()) return;

  // Finish the graph 
  GraphLib::digraph::finish_options o;
  o.Store_By_Rows = false;
  edges->finish(o);
  
  if (!edges->exportFinished(InEdges)) {
    // why did this fail?
    // we should throw something
    DCASSERT(0);
  }

  DCASSERT(InEdges.is_transposed);
#ifdef DEVELOPMENT_CODE
  long ns = 0;
  RSS->getNumStates(ns);
  DCASSERT(ns == InEdges.num_rows);
  if (InEdges.num_rows) {
    DCASSERT(InEdges.rowptr);
    if (InEdges.rowptr[InEdges.num_rows]) {
      DCASSERT(InEdges.colindex);
    }
  }
#endif
}

void grlib_reachgraph::getNumArcs(long &na) const
{
  DCASSERT(edges);
  DCASSERT(InEdges.rowptr);
  na = edges->getNumEdges();
}

void grlib_reachgraph::showInternal(OutputStream &os) const
{
  os << "Internal representation for graph:\n";

  GraphLib::digraph::const_matrix m;
  if (!edges->exportFinished(m)) {
    os << "  Couldn't export graph to matrix\n";
    return;
  } 

  const char* rptr = (InEdges.is_transposed) ? "column pointers" : "row pointers";
  const char* cind = (InEdges.is_transposed) ? "row index      " : "column index";

  os << "  " << rptr << ": [";
  os.PutArray(InEdges.rowptr, edges->getNumNodes()+1);
  os << "]\n";

  os << "  " << cind << ": [";
  os.PutArray(InEdges.colindex, edges->getNumEdges());
  os << "]\n";
}

void grlib_reachgraph::showArcs(OutputStream &os, const show_options &opt, 
  state_lldsm::reachset* RSS, shared_state* st) const
{
  //
  // HUGE TBD: rewrite this using InEdges and OutEdges
  //

  long na = edges->getNumEdges();
  long num_states = edges->getNumNodes();

  if (state_lldsm::tooManyStates(num_states, &os))  return;
  if (graph_lldsm::tooManyArcs(na, &os))            return;

  bool by_rows = (graph_lldsm::OUTGOING == opt.STYLE);
  const char* row;
  const char* col;
  row = "From state ";
  col = "To state ";
  if (!by_rows) SWAP(row, col);

  // TBD : try/catch around this
  indexed_reachset::indexed_iterator &I 
  = dynamic_cast <indexed_reachset::indexed_iterator &> (RSS->iteratorForOrder(opt.ORDER));


  switch (opt.STYLE) {
    case graph_lldsm::DOT:
        os << "digraph fsm {\n";
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
        os << "Reachability graph:\n";
  }

  sparse_row_elems foo(I);

  for (I.start(); I; I++) {
    //
    // Start of another row/col
    //
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

    //
    // Build the row/col
    //
    bool ok;
    if (by_rows)   ok = foo.buildOutgoing(edges, I.getIndex());
    else           ok = foo.buildIncoming(edges, I.getIndex());

    // were we successful?
    if (!ok) {
      // out of memory, bail
      showError("Not enough memory to display reachability graph.");
      break;
    }

    //
    // Display row/column
    //
    for (long z=0; z<foo.last; z++) {
      os.Put('\t');
      switch (opt.STYLE) {
        case graph_lldsm::DOT:
            os << "s" << foo.index[z] << " -> s" << I.getI() << ";";
            break;

        case graph_lldsm::TRIPLES:
            os << foo.index[z] << " " << I.getI();
            break;

        default:
            os << col;
            if (opt.NODE_NAMES) {
              I.copyState(st, foo.index[z]);
              RSS->showState(os, st);
            } else {
              os << foo.index[z];
            }
      }
      os.Put('\n');
    } // for z

    os.flush();
  } // for I
  if (graph_lldsm::DOT == opt.STYLE) {
    os << "}\n";
  }
  os.flush();
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
  //
  // TBD - rewrite using InEdges and OutEdges
  //

  DCASSERT(edges);

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
  const long num_states = edges->getNumNodes();
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

  // TBD - is this necessary?
  // number of paths from src to dest equals
  // number of paths from dest to src in transposed graph
  //
  // so we should be able to do this either way

  // force us by rows
  if (edges->isByCols()) {
    if (!transposeEdges(&numpaths_report, false)) {
      count.setNull();
      return;
    }
  } 

  // export graph edges
  DCASSERT(edges->isByRows());
  GraphLib::generic_graph::const_matrix rg;
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


/*

stateset* grlib_reachgraph::unfairAEF(bool rT, const stateset* p, const stateset* q)
{
  if (0==p || 0==q) return 0; // propogate an earlier error
  const expl_stateset* ep = dynamic_cast <const expl_stateset*> (p);
  const expl_stateset* eq = dynamic_cast <const expl_stateset*> (q);
  if (0==ep || 0==eq) return incompatibleOperand(rT ? "AEP" : "AEF");

  const intset& ip = ep->getExplicit();
  const intset& iq = eq->getExplicit();

  if (rT == true) {
    return notImplemented("unfairAEP");
  }

  // force us by columns
  if (edges->isByRows()) {
    if (!transposeEdges(&ctl_report, false)) {
      if (em->startError()) {
        em->noCause();
        em->cerr() << "Couldn't transpose graph\n";
        em->stopIO();
      }
      return 0;
    }
  } 

  // initialize queue
  long* queue = new long[ip.getSize()];
  long qfront = 0;
  long qtail = 0;

  // initialize required notifications: arc counts.
  long* required = new long[ip.getSize()];
  for (long s=ip.getSize()-1; s>=0; s--) required[s] = 0;
  outgoingCounter mycounter(required);
  edges->traverseAll(mycounter);

  // adjust for goal, controllable, and deadlocked states.
  for (long s=ip.getSize()-1; s>=0; s--) {
    if (0==required[s] || ip.contains(s)) {
      required[s] = 1;
    }
    if (iq.contains(s)) {
      required[s] = 0;
      // add to queue
      CHECK_RANGE(0, qtail, ip.getSize());
      queue[qtail] = s;
      qtail++;
    }
  } // for s

  
  // Explore loop
  incomingEdges inList;
  while (qfront < qtail) {

    // remove some state from queue
    CHECK_RANGE(0, qfront, ip.getSize());
    long s = queue[qfront];
    qfront++;

    // notify all edges to state s
    inList.Clear();
    edges->traverseTo(s, inList);
    for (long z=0; z<inList.Length(); z++) {
      long r = inList.Item(z);
      CHECK_RANGE(0, r, ip.getSize());
      if (0==required[r]) continue;
      required[r]--;
      if (required[r]) continue;
      // add r to queue
      CHECK_RANGE(0, qtail, ip.getSize());
      queue[qtail] = r;
      qtail++;
    } // for z

  } // while queue not empty

  
  // everything that was ever in the queue, is a solution
  intset* ir = new intset(ip.getSize());
  ir->removeAll();
  DCASSERT(qtail <= ip.getSize());
  for (long z=0; z<qtail; z++) {
    ir->addElement(queue[z]);
  }

  // cleanup
  delete[] required;
  delete[] queue;
  return new expl_stateset(getParent(), ir);
}


//
// For ectl_reachgraph
//

bool grlib_reachgraph::forward(const intset& p, intset &r) const
{
  DCASSERT(edges);
  return edges->getForward(p, r);
}

bool grlib_reachgraph::backward(const intset& p, intset &r) const
{
  DCASSERT(edges);
  return edges->getBackward(p, r);
}

void grlib_reachgraph::getDeadlocked(intset &r) const
{
  r.assignFrom(deadlocks);
}

*/

//
// Helpers
//

bool grlib_reachgraph::transposeEdges(const named_msg* rep, bool byrows)
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



//
// NEW STUFF
//

void grlib_reachgraph::getDeadlocked(intset &r) const
{
  r.assignFrom(deadlocks);
}

void grlib_reachgraph::need_reverse_time()
{
  DCASSERT(InEdges.is_transposed);
  DCASSERT(InEdges.rowptr);
  DCASSERT(InEdges.colindex);
  if (0==OutEdges.num_rows) {
    OutEdges.transposeFrom(InEdges);
  }
  DCASSERT(!OutEdges.is_transposed);
  DCASSERT(OutEdges.rowptr);
  DCASSERT(OutEdges.colindex);
}


void grlib_reachgraph::count_edges(bool rt, traverse_helper &TH) const
{
  if (rt) {
    // easy peasy - use InEdges to count incoming edges
    DCASSERT(InEdges.rowptr);

    for (long i=0; i<InEdges.num_rows; i++) {
      TH.set_obligations(i, InEdges.rowptr[i+1] - InEdges.rowptr[i]);
    }
    return;
  }

  if (OutEdges.rowptr) {
    // easy peasy - use OutEdges to count outgoing edges

    for (long i=0; i<OutEdges.num_rows; i++) {
      TH.set_obligations(i, OutEdges.rowptr[i+1] - OutEdges.rowptr[i]);
    }
    return;
  }

  DCASSERT(InEdges.rowptr);
  DCASSERT(InEdges.colindex);

  // Dang, we have to actually count them
  TH.fill_obligations(0);
  for (long z=InEdges.rowptr[InEdges.num_rows]-1; z>=0; z--) {
    TH.add_obligation( InEdges.colindex[z] );
  }
}

void grlib_reachgraph::traverse(bool rt, bool one_step, traverse_helper &TH) const
{
  if (rt) _traverse(one_step, OutEdges, TH);
  else    _traverse(one_step, InEdges, TH);
}

// ******************************************************************
// *                                                                *
// *           grlib_reachgraph::sparse_row_elems methods           *
// *                                                                *
// ******************************************************************

grlib_reachgraph::sparse_row_elems
::sparse_row_elems(const indexed_reachset::indexed_iterator &i)
 : I(i)
{
  alloc = 0;
  last = 0;
  index = 0;
}

grlib_reachgraph::sparse_row_elems::~sparse_row_elems()
{
  free(index);
}

bool grlib_reachgraph::sparse_row_elems::Enlarge(int ns)
{
  if (ns < alloc)   return true;
  if (0==alloc)     alloc = 16;
  while ((alloc < 256) && (alloc <= ns))  alloc *= 2;
  while (alloc <= ns)                     alloc += 256;
  index = (long*) realloc(index, alloc * sizeof(long));
  if (0==index)  return false;
  return true;
}

bool grlib_reachgraph::sparse_row_elems
::buildIncomingUnsorted(GraphLib::digraph* edges, int i)
{
  overflow = false;
  incoming = true;
  last = 0;
  edges->traverseTo(i, *this);
  return !overflow;
}

bool grlib_reachgraph::sparse_row_elems
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

bool grlib_reachgraph::sparse_row_elems
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

bool grlib_reachgraph::sparse_row_elems
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
  index[last] = I.index2ord(z);
  last++;
  return false;
}

// ******************************************************************
// *                                                                *
// *           grlib_reachgraph::outgoingCounter  methods           *
// *                                                                *
// ******************************************************************

/*
grlib_reachgraph::outgoingCounter::outgoingCounter(long* c)
{ 
  count = c; 
}

grlib_reachgraph::outgoingCounter::~outgoingCounter()
{ 
}

bool grlib_reachgraph::outgoingCounter::visit(long from, long to, void*) 
{ 
  count[from]++; 
  return false; 
}
*/

// ******************************************************************
// *                                                                *
// *            grlib_reachgraph::incomingEdges  methods            *
// *                                                                *
// ******************************************************************

/*
grlib_reachgraph::incomingEdges::incomingEdges()
{
  alloc = 0;
  last = 0;
  index = 0;
  overflow = false;
}

grlib_reachgraph::incomingEdges::~incomingEdges()
{
  free(index);
}

bool grlib_reachgraph::incomingEdges::visit(long from, long to, void* )
{
  if (last >= alloc) {
    if (!Enlarge(last+1)) {
      overflow = true;
      return true;
    }
  }
  index[last] = from;
  last++;
  return false;
}

bool grlib_reachgraph::incomingEdges::Enlarge(int ns)
{
  if (ns < alloc)   return true;
  if (0==alloc)     alloc = 16;
  while ((alloc < 256) && (alloc <= ns))  alloc *= 2;
  while (alloc <= ns)                     alloc += 256;
  index = (long*) realloc(index, alloc * sizeof(long));
  if (0==index)  return false;
  return true;
}

*/

