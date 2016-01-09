
// $Id$

#include "rgr_mclib.h"
#include "rss_indx.h"

#include "../Streams/streams.h"
#include "../include/heap.h"
#include "../Modules/expl_ssets.h"

// external library
#include "intset.h"

// ******************************************************************
// *                                                                *
// *                    mclib_reachgraph methods                    *
// *                                                                *
// ******************************************************************

mclib_reachgraph::mclib_reachgraph(MCLib::Markov_chain* mc)
{
  chain = mc;
}

mclib_reachgraph::~mclib_reachgraph()
{
// TBD PROBABLY NOT:
//
//  delete chain;
//
// ASSUMING we have another abstract class, process, owned by stoch_llm.
}

/*
  
  TBD : NOPE

    We should have another abstract class, "process", owned by stoch_llm.
    Interface TBD.
    But there will be an "explicit mc" derived class that contains
    a MCLib::Markov_chain, and likely a Finish mechanism for that
    one also.


*/
/*
void mclib_reachgraph::Finish(state_lldsm::reachset* RSS)
{
  
  indexed_reachset* irs = dynamic_cast <indexed_reachset*> (RSS);

  // Transfer the initial state, if we can
  if (initial.size) {
    if (irs) {
      irs->setInitial(initial);
      initial.size = 0;
      initial.index = 0;
    }
  }

  // shrink RSS
  if (irs) irs->Finish();

  DCASSERT(chain);
  if (chain->isFinished()) return;

  // Finish the Markov chain

}
*/

void mclib_reachgraph::getNumArcs(long &na) const
{
  DCASSERT(chain);
  na = chain->getNumArcs();
}

void mclib_reachgraph::showInternal(OutputStream &os) const
{
  os << "Internal representation for Markov chain:\n";
  os << "  Explicit representation using MCLib.  Cannot display, sorry\n";
  return;
}

void mclib_reachgraph::showArcs(OutputStream &os, state_lldsm::reachset* RSS, 
  state_lldsm::display_order ord, shared_state* st) const 
{
  long na = chain->getNumArcs();
  long num_states = chain->getNumStates();

  if (state_lldsm::tooManyStates(num_states, true))  return;
  if (graph_lldsm::tooManyArcs(na, true))            return;

  bool by_rows = (graph_lldsm::OUTGOING == graph_lldsm::graphDisplayStyle());
  const char* row;
  if (graph_lldsm::displayGraphNodeNames()) {
    row = by_rows ? "From " : "To ";
  } else {
    row = by_rows ? "Row " : "Column ";
  }

  // TBD : try/catch around this
  indexed_reachset::indexed_iterator &I 
  = dynamic_cast <indexed_reachset::indexed_iterator &> (RSS->iteratorForOrder(ord));


  switch (graph_lldsm::graphDisplayStyle()) {
    case graph_lldsm::DOT:
        os << "digraph mc {\n";
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
        os << "Markov chain:\n";
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
      switch (graph_lldsm::graphDisplayStyle()) {
        case graph_lldsm::DOT:
            os << "s" << foo.index[z] << " -> s" << I.getI();
            os << " [label=\"" << foo.value[z] << "\"];";
            break;

        case graph_lldsm::TRIPLES:
            os << foo.index[z] << " " << I.getI() << " " << foo.value[z];
            break;

        default:
            if (graph_lldsm::displayGraphNodeNames()) {
              I.copyState(st, foo.index[z]);
              RSS->showState(os, st);
            } else {
              os << foo.index[z];
            }
            os << " : " << foo.value[z];
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

bool mclib_reachgraph::forward(const intset& p, intset &r) const
{
  DCASSERT(chain);
  return chain->getForward(p, r);
}

bool mclib_reachgraph::backward(const intset& p, intset &r) const
{
  DCASSERT(chain);
  return chain->getBackward(p, r);
}

void mclib_reachgraph::absorbing(intset &r) const
{
  DCASSERT(chain);
  //
  // Determine set of absorbing (no outgoing edges) states...
  //
  r.removeAll();
  if (chain->isDiscrete()) return;
  long fa = chain->getFirstAbsorbing();
  if (fa < 0) return;
  long la = chain->getNumStates();
  r.addRange(fa, la-1);
}

// ******************************************************************
// *                                                                *
// *           mclib_reachgraph::sparse_row_elems methods           *
// *                                                                *
// ******************************************************************

mclib_reachgraph::sparse_row_elems
::sparse_row_elems(const indexed_reachset::indexed_iterator &i)
 : I(i)
{
  alloc = 0;
  last = 0;
  index = 0;
}

mclib_reachgraph::sparse_row_elems::~sparse_row_elems()
{
  free(index);
}

bool mclib_reachgraph::sparse_row_elems::Enlarge(int ns)
{
  if (ns < alloc)   return true;
  if (0==alloc)     alloc = 16;
  while ((alloc < 256) && (alloc <= ns))  alloc *= 2;
  while (alloc <= ns)                     alloc += 256;
  index = (long*) realloc(index, alloc * sizeof(long));
  if (0==index)  return false;
  return true;
}

bool mclib_reachgraph::sparse_row_elems
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

bool mclib_reachgraph::sparse_row_elems
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

bool mclib_reachgraph::sparse_row_elems
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

