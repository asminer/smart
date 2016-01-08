
// $Id$

#include "rgr_expl.h"
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


stateset* expl_reachgraph::EX(bool revTime, const stateset* p) const
{
  DCASSERT(edges);

  if (0==p) return 0; // propogate an earlier error
  const expl_stateset* ep = dynamic_cast <const expl_stateset*> (p);
  if (0==ep) return incompatibleOperand("EX");
  const intset& ip = ep->getExplicit(); 

  intset* answer = new intset(ip.getSize());
  answer->removeAll();

  if (revTime) {
    edges->getForward(ip, *answer);
  } else {
    edges->getBackward(ip, *answer);
  }

  return new expl_stateset(p->getParent(), answer);
}

stateset* expl_reachgraph::EU(bool revTime, const stateset* p, const stateset* q) const
{
  DCASSERT(edges);

  if (0==q) return 0; // propogate an earlier error

  const expl_stateset* ep = dynamic_cast <const expl_stateset*> (p);
  const expl_stateset* eq = dynamic_cast <const expl_stateset*> (q);

  if (0==eq)        return incompatibleOperand(p ? "EU" : "EF");

  const intset& iq = eq->getExplicit(); 

  if (0==p) {
    //
    // Easy special case of EF q
    //

    intset* answer = new intset(iq);

    if (reportCTL()) {
      long iters;
      if (revTime) {
        for (iters = 1; edges->getForward(*answer, *answer); iters++);
        reportIters("EP", iters);
      } else {
        for (iters = 1; edges->getBackward(*answer, *answer); iters++);
        reportIters("EF", iters);
      }
    } else {
      if (revTime) {
        while (edges->getForward(*answer, *answer)); 
      } else {
        while (edges->getBackward(*answer, *answer)); 
      }
    }

    return new expl_stateset(q->getParent(), answer);
  }

  //
  // E p U q
  //

  if (0==ep)   return incompatibleOperand("EU");
  const intset& ip = ep->getExplicit(); 

  intset* answer = new intset(iq.getSize());

  intset tmp(iq.getSize());

  reportIters(revTime ? "ES" : "EU", _EU(revTime, ip, iq, *answer, tmp) );

  return new expl_stateset(q->getParent(), answer);
}

stateset* expl_reachgraph::unfairEG(bool revTime, const stateset* p) const
{
  DCASSERT(edges);

  if (0==p) return 0; // propogate an earlier error
  const expl_stateset* ep = dynamic_cast <const expl_stateset*> (p);
  if (0==ep) return incompatibleOperand("EX");
  const intset& ip = ep->getExplicit(); 

  intset* answer = new intset(ip.getSize());

  intset tmp(ip.getSize());

  reportIters(revTime ? "EH" : "EG", unfair_EG(revTime, ip, *answer, tmp) );

  return new expl_stateset(p->getParent(), answer);
}

long expl_reachgraph::_EU(bool revTime, const intset& p, const intset& q,
  intset &r, intset &tmp) const
{
  DCASSERT(edges);

  r.assignFrom(q);
  for (long iters=1; ; iters++) {
    tmp.removeAll();
    if (revTime)  edges->getForward(r, tmp);
    else          edges->getBackward(r, tmp);
    // intersect with p 
    tmp *= p;

    // tmp are newly discovered states satisfying E p U q.
    // If tmp is a subset of r, then we won't add anything and we can stop

    if (tmp <= r) {
      return iters;
    }

    // There are new states, add them and loop

    r += tmp;

  }
}

long expl_reachgraph::unfair_EG(bool rT, const intset& p, intset &r, intset &tmp) const
{
  DCASSERT(edges);

  if (rT) {
    //
    // Determine set of source (initial) states...
    //
    const indexed_reachset* irs = smart_cast <const indexed_reachset*> (getParent()->getRSS());
    DCASSERT(irs);
    irs->getInitial(tmp);
  } else {
    //
    // Determine set of absorbing (no outgoing edges) states...
    //
    edges->noOutgoingEdges(tmp); 
  }
  //
  // ...that satisfy p
  //
  tmp *= p;
  intset* absorbP = tmp.isEmpty() ? 0 : new intset(tmp);

  //
  // Do usual EG p iteration, except add back absorbP states every time
  //

  // r_0 = p

  r.assignFrom(p);
  long iters;
  for (iters=1; ; iters++) {

    tmp.removeAll();
    if (rT) edges->getForward(r, tmp);
    else    edges->getBackward(r, tmp);
    if (absorbP) {
      tmp += *absorbP;
    }
    tmp *= p;

    // tmp is r_i+1, states satisfying p and reaching states in r_i

    if (tmp == r) break;  // converged!

    r.assignFrom(tmp);
  }

  delete absorbP;
  return iters;
}

void expl_reachgraph::setInitial(LS_Vector &init)
{
  DCASSERT(0==init.d_value);
  DCASSERT(0==init.f_value);
  initial = init;
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

