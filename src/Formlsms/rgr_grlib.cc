
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
// *                                                                *
// *                    grlib_reachgraph methods                    *
// *                                                                *
// ******************************************************************

grlib_reachgraph::grlib_reachgraph(GraphLib::digraph* g) : InEdges(), OutEdges()
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

  DCASSERT(0==InEdges.num_rows);
  DCASSERT(0==OutEdges.num_rows);
}

grlib_reachgraph::~grlib_reachgraph()
{
  InEdges.destroy();
  OutEdges.destroy();

  // in case we still have it
  delete edges;

  // in case we still have it:
  delete[] initial.index;

  // deadlocks should be destroyed automagically here;
  // no need to do anything special, right?
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
  if (!edges->isFinished()) {
    // Finish the graph 
    GraphLib::digraph::finish_options o;
    edges->finish(o);
  }

  GraphLib::digraph::const_matrix cE;
  if (!edges->exportFinished(cE)) {
    // why did this fail?
    // we should throw something
    // or fail with an internal error message
    DCASSERT(0);
  }

  if (cE.is_transposed) {
    InEdges.copyFrom(cE);
    OutEdges.transposeFrom(cE);
  } else {
    InEdges.transposeFrom(cE);
    OutEdges.copyFrom(cE);
  }

  free(InEdges.value);
  InEdges.value = 0;
  free(OutEdges.value);
  OutEdges.value = 0;

  delete edges;  
  edges = 0;

#ifdef DEVELOPMENT_CODE
  /*
    Sanity checks 
  */
  DCASSERT(InEdges.is_transposed);
  DCASSERT(!OutEdges.is_transposed);
  long ns = 0;
  RSS->getNumStates(ns);
  DCASSERT(ns == InEdges.num_rows);
  DCASSERT(ns == OutEdges.num_rows);
  if (InEdges.num_rows) {
    DCASSERT(InEdges.rowptr);
    if (InEdges.rowptr[InEdges.num_rows]) {
      DCASSERT(InEdges.colindex);
    }
  }
  if (OutEdges.num_rows) {
    DCASSERT(OutEdges.rowptr);
    if (OutEdges.rowptr[OutEdges.num_rows]) {
      DCASSERT(OutEdges.colindex);
    }
  }
#endif

}

void grlib_reachgraph::getNumArcs(long &na) const
{
  if (InEdges.num_rows) {
    DCASSERT(InEdges.rowptr);
    na = InEdges.rowptr[InEdges.num_rows];
  } else {
    // no states.  could happen I guess.
    na = 0;
  }
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
  //
  // HUGE TBD: rewrite this using InEdges and OutEdges
  //

  long num_states = InEdges.num_rows;
  if (state_lldsm::tooManyStates(num_states, &os))  return;

  long na;
  getNumArcs(na);
  if (graph_lldsm::tooManyArcs(na, &os))            return;

  bool by_rows = (graph_lldsm::INCOMING != opt.STYLE);
  const char* row;
  const char* col;
  row = "From state ";
  col = "To state ";
  if (!by_rows) SWAP(row, col);

  const GraphLib::digraph::matrix &Edges = by_rows ? OutEdges : InEdges;

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


  // Sorted list for current row/col
  std::set<long> coll;

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
    coll.clear();
    CHECK_RANGE(0, I.getIndex(), Edges.num_rows);
    for (long z=Edges.rowptr[I.getIndex()]; z<Edges.rowptr[I.getIndex()+1]; z++) {
      coll.insert( I.index2ord(Edges.colindex[z]) );
    } // for z

    //
    // Display row/column
    //
    std::set<long>::const_iterator pos;
    for (pos=coll.begin(); pos != coll.end(); ++pos) {
      os.Put('\t');
      switch (opt.STYLE) {
        case graph_lldsm::DOT:
            os << "s" << I.getI() << " -> s" << *pos << ";";
            break;

        case graph_lldsm::TRIPLES:
            os << I.getI() << " " << *pos;
            break;

        default:
            os << col;
            if (opt.NODE_NAMES) {
              I.copyState(st, *pos);
              RSS->showState(os, st);
            } else {
              os << *pos;
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

  const long num_states = InEdges.num_rows;
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
      for (long z=OutEdges.rowptr[visit]; z<OutEdges.rowptr[visit+1]; z++) {
        long next = OutEdges.colindex[z];
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
    for (long z=OutEdges.rowptr[visit]; z<OutEdges.rowptr[visit+1]; z++) {
      long next = OutEdges.colindex[z];
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
    // easy peasy - use InEdges to count incoming edges
    DCASSERT(InEdges.rowptr);

    for (long i=0; i<InEdges.num_rows; i++) {
      TH.set_obligations(i, InEdges.rowptr[i+1] - InEdges.rowptr[i]);
    }

  } else {
    // easy peasy - use OutEdges to count outgoing edges
    DCASSERT(OutEdges.rowptr);

    for (long i=0; i<OutEdges.num_rows; i++) {
      TH.set_obligations(i, OutEdges.rowptr[i+1] - OutEdges.rowptr[i]);
    }
  }
}

void grlib_reachgraph::traverse(bool rt, bool one_step, traverse_helper &TH) const
{
  if (rt) _traverse(one_step, OutEdges, TH);
  else    _traverse(one_step, InEdges, TH);
}

void grlib_reachgraph::showRawMatrix(OutputStream &os, const GraphLib::digraph::matrix &E)
{
  const char* rptr = (E.is_transposed) ? "column pointers" : "row pointers";
  const char* cind = (E.is_transposed) ? "row index      " : "column index";

  os << "      size: " << E.num_rows << "\n";
  os << "      " << rptr << ": [";
  os.PutArray(E.rowptr, E.num_rows+1);
  os << "]\n";

  os << "      " << cind << ": [";
  os.PutArray(E.colindex, E.rowptr ? E.rowptr[E.num_rows] : 0);
  os << "]\n";
}

void grlib_reachgraph::_traverse(bool one_step, const GraphLib::digraph::matrix &E,
  ectl_reachgraph::traverse_helper &TH)
{
        while (TH.queue_nonempty()) {
            long s = TH.queue_pop();
            // explore edges to/from s
            for (long z=E.rowptr[s]; z<E.rowptr[s+1]; z++) {
                long t = E.colindex[z];
                if (TH.num_obligations(t) <= 0) continue;
                TH.remove_obligation(t);
                if (one_step) continue;
                if (0==TH.num_obligations(t)) {
                    TH.queue_push(t);
                }
            } // for z
        } // while queue not empty
}


