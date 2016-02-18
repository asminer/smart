
// $Id$ 

#include "proc_meddly.h"
#include "rss_meddly.h"

// ******************************************************************
// *                                                                *
// *                     mclib_process  methods                     *
// *                                                                *
// ******************************************************************

meddly_process::meddly_process(meddly_encoder* wrap)
{
  mxd_wrap = wrap;
  proc = 0;
  num_states = 0;
}

meddly_process::~meddly_process()
{
  Delete(mxd_wrap);
  Delete(proc);
}

void meddly_process::attachToParent(stochastic_lldsm* p, LS_Vector &init, state_lldsm::reachset* rss)
{
  DCASSERT(rss);

  process::attachToParent(p, init, rss);

  rss->getNumStates(num_states); 
  // TBD
}

long meddly_process::getNumStates() const
{
  return num_states;
}

void meddly_process::showProc(OutputStream &os, 
  const graph_lldsm::reachgraph::show_options &opt, 
  state_lldsm::reachset* RSS, shared_state* st) const
{
  if (getParent()->tooManyStates(&os))  return;
  if (getParent()->tooManyArcs(&os))    return;
   
  meddly_reachset* mrss = smart_cast <meddly_reachset*> (RSS);
  DCASSERT(mrss);

  mrss->buildIndexSet();

  DCASSERT(proc);
  
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


  // TBD : try/catch around this:
  meddly_reachset::lexical_iter &I
  = dynamic_cast <meddly_reachset::lexical_iter &> (RSS->iteratorForOrder(opt.ORDER));

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
        os << getParent()->getNumStates() << "\n";
        os << getParent()->getNumArcs() << "\n";
        break;

    default:
        os << "Markov chain:\n";
  }

  long i;
  for (I.start(), i=0; I; I++, i++) {
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
            os << i;
          }
          os << ":\n";

      default:
          // nothing
          break;
    }

    //
    // Iterate over this row/col
    //
    if (by_rows) {
      proc->startIteratorRow(I.getCurrentMinterm());
    } else {
      proc->startIteratorCol(I.getCurrentMinterm());
    }
    for (; !proc->isIterDone(); proc->incIter() ) {
      const int* tmt;
      if (by_rows) {
        tmt = proc->getIterPrimedMinterm();
      } else {
        tmt = proc->getIterUnprimedMinterm();
      }
      os.Put('\t');

      float rate = 0;
      proc->getIterValue(rate);

      switch (opt.STYLE) {
        case graph_lldsm::DOT:
            os << "s" << mrss->getMintermIndex(tmt) << " -> s" << i;
            if (!opt.RG_ONLY) {
              os << " [label=\"" << rate << "\"]";
            }
            os << ";";
            break;

        case graph_lldsm::TRIPLES:
            os << mrss->getMintermIndex(tmt) << " " << i;
            if (!opt.RG_ONLY) {
              os << " " << rate;
            }
            break;

        default:
            os << col;
            if (opt.NODE_NAMES) {
              mrss->MddMinterm2State(tmt, st);
              RSS->showState(os, st);
            } else {
              os << mrss->getMintermIndex(tmt);
            }
            if (!opt.RG_ONLY) {
              os << " : " << rate;
            }
      } // switch
      os.Put('\n');

    } // for iterator

    os.flush();

  } // for I

  if (graph_lldsm::DOT == opt.STYLE) {
    os << "}\n";
  }
}

void meddly_process::showInternal(OutputStream &os) const
{
  os << "Internal process representation (using MEDDLY):\n";
  mxd_wrap->showNodeGraph(os, proc);
  os.flush();
}

void meddly_process::getNumClasses(long &count) const
{
  DCASSERT(0);
}

void meddly_process::showClasses(OutputStream &os, state_lldsm::reachset* rss, 
      shared_state* st) const
{
  DCASSERT(0);
}

bool meddly_process::isTransient(long st) const
{
  DCASSERT(0);
  return false;
}

statedist* meddly_process::getInitialDistribution() const
{
  DCASSERT(0);
  return 0;
}

