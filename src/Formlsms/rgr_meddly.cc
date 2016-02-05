
// $Id$

#include "rgr_meddly.h"
#include "rss_meddly.h"

// ******************************************************************
// *                                                                *
// *                  meddly_monolithic_rg methods                  *
// *                                                                *
// ******************************************************************

meddly_monolithic_rg::meddly_monolithic_rg(meddly_encoder* wrap)
{
  vars = 0;
  mxd_wrap = wrap;
  edges = 0;
  states = 0;
  convert_to_actual = false;
}

meddly_monolithic_rg::~meddly_monolithic_rg()
{
  Delete(vars);
  Delete(edges);
  Delete(mxd_wrap);
  Delete(states);
}

void meddly_monolithic_rg::attachToParent(graph_lldsm* p, state_lldsm::reachset* rss)
{
  graph_lldsm::reachgraph::attachToParent(p, rss);

  meddly_reachset* mrss = smart_cast <meddly_reachset*> (rss);
  DCASSERT(mrss);

  vars = mrss->shareVars();
  states = mrss->copyStates();

  if (convert_to_actual) {
    shared_ddedge* actual = buildActualEdges();
    Delete(edges);
    edges = actual;
    uses_potential = false;
    convert_to_actual = false;
  }
}

void meddly_monolithic_rg::setPotential(shared_ddedge* nsf)
{
  edges = nsf;
  uses_potential = true;
}

void meddly_monolithic_rg::setActual(shared_ddedge* nsf)
{
  edges = nsf;
  uses_potential = false;
}

void meddly_monolithic_rg::scheduleConversionToActual()
{
  convert_to_actual = uses_potential;
}

void meddly_monolithic_rg::getNumArcs(result &na) const 
{
  getNumArcsTemplate(na);
}

void meddly_monolithic_rg::getNumArcs(long &na) const 
{
  getNumArcsTemplate(na);
}

void meddly_monolithic_rg::showInternal(OutputStream &os) const 
{
  os << "Internal process representation (using MEDDLY):\n";
  mxd_wrap->showNodeGraph(os, edges);
  os.flush();
}

void meddly_monolithic_rg::showArcs(OutputStream &os, const show_options &opt, 
      state_lldsm::reachset* RSS, shared_state* st) const
{
  if (getParent()->tooManyStates(&os))  return;
  if (getParent()->tooManyArcs(&os))    return;
   
  meddly_reachset* mrss = smart_cast <meddly_reachset*> (RSS);
  DCASSERT(mrss);

  mrss->buildIndexSet();

  bool by_rows = (graph_lldsm::OUTGOING == opt.STYLE);
  const char* row;
  const char* col;
  row = "From state ";
  col = "To state ";
  if (!by_rows) SWAP(row, col);

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
        os << "Reachability graph:\n";
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
      edges->startIteratorRow(I.getCurrentMinterm());
    } else {
      edges->startIteratorCol(I.getCurrentMinterm());
    }
    for (; !edges->isIterDone(); edges->incIter() ) {
      const int* tmt;
      if (by_rows) {
        tmt = edges->getIterPrimedMinterm();
      } else {
        tmt = edges->getIterUnprimedMinterm();
      }
      os.Put('\t');

      switch (opt.STYLE) {
        case graph_lldsm::DOT:
            os << "s" << mrss->getMintermIndex(tmt) << " -> s" << i << ";";
            break;

        case graph_lldsm::TRIPLES:
            os << mrss->getMintermIndex(tmt) << " " << i;
            break;

        default:
            os << col;
            if (opt.NODE_NAMES) {
              mrss->MddMinterm2State(tmt, st);
              RSS->showState(os, st);
            } else {
              os << mrss->getMintermIndex(tmt);
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

