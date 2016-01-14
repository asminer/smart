
// $Id$

#include "proc_mclib.h"

#include "../Streams/streams.h"
#include "../include/heap.h"
#include "../Modules/expl_ssets.h"
#include "../Modules/statevects.h"

// external library
#include "intset.h"

// ******************************************************************
// *                                                                *
// *                     mclib_process  methods                     *
// *                                                                *
// ******************************************************************

mclib_process::mclib_process(MCLib::Markov_chain* mc)
{
  chain = mc;
  initial = 0;
  //
  trap = -1;
  accept = -1;
}

mclib_process::~mclib_process()
{
  delete chain;
  Delete(initial);
}

void mclib_process::attachToParent(stochastic_lldsm* p, LS_Vector &init, state_lldsm::reachset* rss)
{
  process::attachToParent(p, init, rss);

  // Finish rss
  indexed_reachset* irs = smart_cast <indexed_reachset*> (rss);
  DCASSERT(irs);
  irs->Finish();

  // Finish MC
  MCLib::Markov_chain::finish_options opts;
  opts.Store_By_Rows = storeByRows();
  opts.Will_Clear = false;
  opts.report = my_timer ? my_timer->switchMe() : 0;
  MCLib::Markov_chain::renumbering r;
  chain->finish(opts, r);
  DCASSERT(chain->isEfficientByRows() == opts.Store_By_Rows);

  if (r.GeneralRenumbering()) {
    const long* ren = r.GetGeneral();
    DCASSERT(ren);

    // Renumber states
    irs->Renumber(ren);

    // TBD - renumber accepting state

    // TBD - renumber trap state

    initial = new statedist(p, init, ren);
  } else {
    DCASSERT(r.NoRenumbering());

    initial = new statedist(p, init, 0);
  }
    
  // Copy initial states to RSS
  intset initial_set( chain->getNumStates() );  
  initial->greater_than(0, &initial_set);
  irs->setInitial(initial_set);

  // NEAT TRICK!!!
  // Set the reachability graph, using 
  // a thin wrapper around the Markov chain.
  p->setRGR( new mclib_reachgraph(this) );
}

long mclib_process::getNumStates() const
{
  DCASSERT(chain);
  return chain->getNumStates();
}

void mclib_process::getNumClasses(long &count) const
{
  // TBD
  DCASSERT(0);
  count = -1;
}

void mclib_process::showClasses(OutputStream &os, shared_state* st) const
{
  // TBD
  DCASSERT(0);
}

bool mclib_process::isTransient(long st) const
{
  // TBD
  DCASSERT(0);
}

statedist* mclib_process::getInitialDistribution() const
{
  return Share(initial);
}

//
// For reachgraphs
//

void mclib_process::showInternal(OutputStream &os) const
{
  os << "Internal representation for Markov chain:\n";
  os << "  Explicit representation using MCLib.  Cannot display, sorry\n";
  return;
}

void mclib_process::showArcs(OutputStream &os, state_lldsm::reachset* RSS, 
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

// ******************************************************************
// *                                                                *
// *            mclib_process::sparse_row_elems  methods            *
// *                                                                *
// ******************************************************************

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

bool mclib_process::sparse_row_elems
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

void mclib_reachgraph::showArcs(OutputStream &os, state_lldsm::reachset* RSS, 
      state_lldsm::display_order ord, shared_state* st) const
{
  DCASSERT(chain);
  chain->showArcs(os, RSS, ord, st);
}

bool mclib_reachgraph::forward(const intset& p, intset &r) const
{
  DCASSERT(chain);
  return chain->forward(p, r);
}

bool mclib_reachgraph::backward(const intset& p, intset &r) const
{
  DCASSERT(chain);
  return chain->backward(p, r);
}

void mclib_reachgraph::absorbing(intset &r) const
{
  DCASSERT(chain);
  chain->absorbing(r);
}
    
void mclib_reachgraph::getTSCCsSatisfying(intset &p) const
{
  DCASSERT(chain);
  chain->getTSCCsSatisfying(p);
}

