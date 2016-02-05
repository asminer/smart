
// $Id$

#include "rgr_meddly.h"
#include "rss_meddly.h"
#include "../Modules/meddly_ssets.h"

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
  mrss = 0;
}

meddly_monolithic_rg::~meddly_monolithic_rg()
{
  Delete(vars);
  Delete(edges);
  Delete(mxd_wrap);
  Delete(states);
  Delete(mrss);
}

void meddly_monolithic_rg::attachToParent(graph_lldsm* p, state_lldsm::reachset* rss)
{
  graph_lldsm::reachgraph::attachToParent(p, rss);

  mrss = Share(smart_cast <meddly_reachset*> (rss));
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


stateset* meddly_monolithic_rg::EX(bool revTime, const stateset* p) const
{
  const meddly_stateset* mp = dynamic_cast <const meddly_stateset*> (p);
  if (0==mp) return incompatibleOperand(revTime ? "EY" : "EX");

  const shared_ddedge* mpe = mp->getStateDD();
  DCASSERT(mpe);
  shared_ddedge* ans = mrss->newMddEdge();

  try {
    if (revTime) {
      mxd_wrap->postImage(mpe, edges, ans);
    } else {
      mxd_wrap->preImage(mpe, edges, ans);
    }
    return new meddly_stateset(mp, ans);
  } 
  catch (sv_encoder::error err) { 
    Delete(ans);
    throw convert(err);
  }
}

stateset* meddly_monolithic_rg
::EU(bool revTime, const stateset* p, const stateset* q) const
{
  //
  // Grab p in a form we can use
  //
  const meddly_stateset* mp = 0;
  const shared_ddedge* mpe = 0;
  if (p) {
    mp = dynamic_cast <const meddly_stateset*> (p);
    if (0==mp) return incompatibleOperand(revTime ? "ES" : "EU");
    mpe = mp->getStateDD();
    DCASSERT(mpe);
  }

  //
  // Grab q in a form we can use
  //
  const meddly_stateset* mq = dynamic_cast <const meddly_stateset*> (q);
  if (0==mq) return incompatibleOperand(revTime ? "ES" : "EU");
  const shared_ddedge* mqe = mq->getStateDD();
  DCASSERT(mqe);

  // Result 
  shared_ddedge* ans = mrss->newMddEdge();
  DCASSERT(ans);

  //
  // Special case - if p=0 (means true), use saturation
  //
  if (0==mpe) {
    if (revTime) {
      mxd_wrap->postImageStar(mqe, edges, ans);
    } else {
      mxd_wrap->preImageStar(mqe, edges, ans);
    }
    return new meddly_stateset(mq, ans);
  }

  //
  // Non trivial p, use iteration
  // TBD - use saturation here as well
  //

  //
  // Auxiliary sets
  //
  shared_ddedge* prev = mrss->newMddConst(false); // prev := emptyset
  shared_ddedge* f = mrss->newMddEdge();

  DCASSERT(prev);
  DCASSERT(f);

  // answer := q
  ans->E = mqe->E;

  while (prev->E != ans->E) {

    // f := pre/post (answer)
    if (revTime) {
      mxd_wrap->postImage(ans, edges, f);
    } else {
      mxd_wrap->preImage(ans, edges, f);
    }

    // f := f ^ p
    MEDDLY::apply( MEDDLY::INTERSECTION, f->E, mpe->E, f->E );

    // prev := answer
    prev->E = ans->E;

    // answer := answer U f
    MEDDLY::apply( MEDDLY::UNION, ans->E, f->E, ans->E );

  } // iteration

  // Cleanup
  Delete(f);
  Delete(prev);
  return new meddly_stateset(mq, ans);
}

stateset* meddly_monolithic_rg::unfairEG(bool revTime, const stateset* p) const
{
  return 0;
}

