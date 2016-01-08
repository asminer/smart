
// $Id$

#include "rgr_grlib.h"
#include "rss_indx.h"

#include "../Streams/streams.h"
#include "../include/heap.h"
#include "../Modules/expl_ssets.h"

// external library
#include "intset.h"

// ******************************************************************
// *                                                                *
// *                    expl_reachgraph  methods                    *
// *                                                                *
// ******************************************************************

expl_reachgraph::expl_reachgraph(GraphLib::digraph* g)
{
  edges = g;
  // clear the initial vector
  initial.size = 0;
  initial.index = 0;
  initial.d_value = 0;
  initial.f_value = 0;
}

expl_reachgraph::~expl_reachgraph()
{
  delete edges;

  // in case we still have it:
  delete[] initial.index;
}

void expl_reachgraph::Finish(state_lldsm::reachset* RSS)
{
  // Transfer the initial state, if we can
  if (initial.size) {
    indexed_reachset* irs = dynamic_cast <indexed_reachset*> (RSS);
    if (irs) {
      irs->setInitial(initial);
      initial.size = 0;
      initial.index = 0;
    }
  }

  DCASSERT(edges);
  if (edges->isFinished()) return;

  // Finish the graph 
  GraphLib::digraph::finish_options o;
  o.Store_By_Rows = true;
  o.Will_Clear = false;
  edges->finish(o);
}

void expl_reachgraph::getNumArcs(long &na) const
{
  DCASSERT(edges);
  na = edges->getNumEdges();
}

void expl_reachgraph::showInternal(OutputStream &os) const
{
  os << "Internal representation for graph:\n";

  GraphLib::digraph::matrix m;
  if (!edges->exportFinished(m)) {
    os << "  Couldn't export graph to matrix\n";
    return;
  } 

  const char* rptr = (m.is_transposed) ? "column pointers" : "row pointers";
  const char* cind = (m.is_transposed) ? "row index      " : "column index";

  os << "  " << rptr << ": [";
  os.PutArray(m.rowptr, edges->getNumNodes()+1);
  os << "]\n";

  os << "  " << cind << ": [";
  os.PutArray(m.colindex, edges->getNumEdges());
  os << "]\n";
}

void expl_reachgraph::showArcs(OutputStream &os, state_lldsm::reachset* RSS, 
  state_lldsm::display_order ord, shared_state* st) const 
{
  long na = edges->getNumEdges();
  long num_states = edges->getNumNodes();

  if (state_lldsm::tooManyStates(num_states, true))  return;
  if (graph_lldsm::tooManyArcs(na, true))            return;

  bool by_rows = (graph_lldsm::OUTGOING == graph_lldsm::graphDisplayStyle());
  const char* row;
  const char* col;
  row = "From state ";
  col = "To state ";
  if (!by_rows) SWAP(row, col);

  // TBD : try/catch around this
  indexed_reachset::indexed_iterator &I 
  = dynamic_cast <indexed_reachset::indexed_iterator &> (RSS->iteratorForOrder(ord));


  switch (graph_lldsm::graphDisplayStyle()) {
    case graph_lldsm::DOT:
        os << "digraph fsm {\n";
        for (I.start(); I; I++) { 
          os << "\ts" << I.index();
          if (graph_lldsm::displayGraphNodeNames()) {
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
    switch (graph_lldsm::graphDisplayStyle()) {
      case graph_lldsm::INCOMING:
      case graph_lldsm::OUTGOING:
          os << row;
          if (graph_lldsm::displayGraphNodeNames()) {
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
    if (by_rows)   ok = foo.buildOutgoing(edges, I.getIndex());
    else           ok = foo.buildIncoming(edges, I.getIndex());

    // were we successful?
    if (!ok) {
      // out of memory, bail
      showError("Not enough memory to display reachability graph.");
      break;
    }

    // display row/column
    for (long z=0; z<foo.last; z++) {
      os.Put('\t');
      switch (graph_lldsm::graphDisplayStyle()) {
        case graph_lldsm::DOT:
            os << "s" << foo.index[z] << " -> s" << I.getI() << ";";
            break;

        case graph_lldsm::TRIPLES:
            os << foo.index[z] << " " << I.getI();
            break;

        default:
            os << col;
            if (graph_lldsm::displayGraphNodeNames()) {
              I.copyState(st, foo.index[z]);
              RSS->showState(os, st);
            } else {
              os << foo.index[z];
            }
      }
      os.Put('\n');
    } // for z

    os.flush();
  } // for i
  if (graph_lldsm::DOT == graph_lldsm::graphDisplayStyle()) {
    os << "}\n";
  }
  os.flush();
}

void expl_reachgraph::setInitial(LS_Vector &init)
{
  DCASSERT(0==init.d_value);
  DCASSERT(0==init.f_value);
  initial = init;
}

bool expl_reachgraph::forward(const intset& p, intset &r) const
{
  DCASSERT(edges);
  return edges->getForward(p, r);
}

bool expl_reachgraph::backward(const intset& p, intset &r) const
{
  DCASSERT(edges);
  return edges->getBackward(p, r);
}

void expl_reachgraph::absorbing(intset &r) const
{
  DCASSERT(edges);
  //
  // Determine set of absorbing (no outgoing edges) states...
  //
  edges->noOutgoingEdges(r); 
}

void expl_reachgraph::source(intset &r) const
{
  //
  // Determine set of source (initial) states...
  //
  const indexed_reachset* irs = smart_cast <const indexed_reachset*> (getParent()->getRSS());
  DCASSERT(irs);
  irs->getInitial(r);
}

// ******************************************************************
// *                                                                *
// *           expl_reachgraph::sparse_row_elems  methods           *
// *                                                                *
// ******************************************************************

expl_reachgraph::sparse_row_elems
::sparse_row_elems(const indexed_reachset::indexed_iterator &i)
 : I(i)
{
  alloc = 0;
  last = 0;
  index = 0;
}

expl_reachgraph::sparse_row_elems::~sparse_row_elems()
{
  free(index);
}

bool expl_reachgraph::sparse_row_elems::Enlarge(int ns)
{
  if (ns < alloc)   return true;
  if (0==alloc)     alloc = 16;
  while ((alloc < 256) && (alloc <= ns))  alloc *= 2;
  while (alloc <= ns)                     alloc += 256;
  index = (long*) realloc(index, alloc * sizeof(long));
  if (0==index)  return false;
  return true;
}

bool expl_reachgraph::sparse_row_elems
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

bool expl_reachgraph::sparse_row_elems
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

bool expl_reachgraph::sparse_row_elems
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

