
#include "rgr_meddly.h"

#ifdef HAVE_MEDDLY_H

// ******************************************************************
// *                                                                *
// *                  meddly_monolithic_rg methods                  *
// *                                                                *
// ******************************************************************

meddly_monolithic_rg::meddly_monolithic_rg(shared_domain* v, meddly_encoder* wrap)
{
  vars = v;
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

  if (0==vars) vars = mrss->shareVars();
  states = mrss->copyStates();

  if (convert_to_actual) {
    shared_ddedge* actual = buildActualEdges();
    Delete(edges);
    edges = actual;
    uses_potential = false;
    convert_to_actual = false;
  }
}

void meddly_monolithic_rg::setEdges(shared_ddedge* nsf)
{
  DCASSERT(nsf);
  if (nsf->getForest() == mxd_wrap->getForest()) {
    edges = nsf;
    return;
  }

  //
  // Different forests.  Make a copy.
  //

  Delete(edges);  // probably zero but this is safest
  edges = newMxdEdge();
  MEDDLY::apply(MEDDLY::COPY, nsf->E, edges->E);
  Delete(nsf);
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
  os << "Internal reachability graph representation (using MEDDLY):\n";
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

  DCASSERT(edges);
  
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


stateset* meddly_monolithic_rg::EX(bool revTime, const stateset* p) 
{
  const meddly_stateset* mp = dynamic_cast <const meddly_stateset*> (p);
  if (0==mp) return incompatibleOperand(revTime ? "EY" : "EX");
  const shared_ddedge* mpe = mp->getStateDD();
  DCASSERT(mpe);

  shared_ddedge* ans = mrss->newMddEdge();

  try {
    _EX(revTime, mpe, ans);
    return new meddly_stateset(mp, ans);
  } 
  catch (sv_encoder::error err) { 
    Delete(ans);
    throw convert(err);
  }
}

stateset* meddly_monolithic_rg::AX(bool revTime, const stateset* p) 
{
  //
  // Uses AX p = !EX !p
  //

  const meddly_stateset* mp = dynamic_cast <const meddly_stateset*> (p);
  if (0==mp) return incompatibleOperand(revTime ? "EY" : "EX");
  const shared_ddedge* mpe = mp->getStateDD();
  DCASSERT(mpe);

  shared_ddedge* notp = mrss->newMddEdge();
  DCASSERT(notp);

  MEDDLY::apply( MEDDLY::COMPLEMENT, mpe->E, notp->E );

  shared_ddedge* ans = mrss->newMddEdge();
  DCASSERT(ans);

  try {
    _EX(revTime, notp, ans);
    Delete(notp);
    notp = 0;
    MEDDLY::apply( MEDDLY::COMPLEMENT, ans->E, ans->E );
    return new meddly_stateset(mp, ans);
  }
  catch (sv_encoder::error err) { 
    Delete(notp);
    Delete(ans);
    throw convert(err);
  }
}

stateset* meddly_monolithic_rg
::EU(bool revTime, const stateset* p, const stateset* q)
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

  try {
    _EU(revTime, mpe, mqe, ans);
    return new meddly_stateset(mq, ans);
  } 
  catch (sv_encoder::error err) { 
    Delete(ans);
    throw convert(err);
  }
}

stateset* meddly_monolithic_rg
::unfairAU(bool revTime, const stateset* p, const stateset* q)
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

  //
  // Uses !ApUq = E (p !q) U (!p !q)  OR  EG !q
  //

  //
  // First, determine EG !q
  //
  shared_ddedge* notq = mrss->newMddEdge();
  DCASSERT(notq);
  MEDDLY::apply( MEDDLY::COMPLEMENT, mqe->E, notq->E );

  shared_ddedge* egnotq = mrss->newMddEdge();
  DCASSERT(egnotq);
  _unfairEG(revTime, notq, egnotq);

  
  if (0==mpe) {
    //
    // p is true, which means the set satisfying
    // E (p!q) U (!p!q) must be empty.
    // So, the answer is !EG!q 
    Delete(notq);
    MEDDLY::apply( MEDDLY::COMPLEMENT, egnotq->E, egnotq->E );
    return new meddly_stateset(mq, egnotq);
  }


  //
  // Determine E (p!q) U (!p!q)
  //
  shared_ddedge* pnotq = mrss->newMddEdge();
  DCASSERT(pnotq);
  MEDDLY::apply( MEDDLY::INTERSECTION, mpe->E, notq->E, pnotq->E );

  shared_ddedge* notpnotq = mrss->newMddEdge();
  DCASSERT(notpnotq);
  MEDDLY::apply( MEDDLY::COMPLEMENT, mpe->E, notpnotq->E );
  MEDDLY::apply( MEDDLY::INTERSECTION, notpnotq->E, notq->E, notpnotq->E );

  shared_ddedge* eupart = mrss->newMddEdge();
  DCASSERT(eupart);

  _EU(revTime, pnotq, notpnotq, eupart);

  //
  // "Or" them, and invert
  //
  shared_ddedge* ans = mrss->newMddEdge();
  DCASSERT(ans);
  MEDDLY::apply( MEDDLY::UNION, egnotq->E, eupart->E, ans->E );
  MEDDLY::apply( MEDDLY::COMPLEMENT, ans->E, ans->E );

  //
  // Cleanup
  //
  Delete(notq);
  Delete(pnotq);
  Delete(notpnotq);
  Delete(eupart);
  Delete(egnotq);

  return new meddly_stateset(mq, ans);
}

stateset* meddly_monolithic_rg::unfairEG(bool revTime, const stateset* p)
{
  const meddly_stateset* mp = dynamic_cast <const meddly_stateset*> (p);
  if (0==mp) return incompatibleOperand(revTime ? "EH" : "EG");
  const shared_ddedge* mpe = mp->getStateDD();
  DCASSERT(mpe);

  shared_ddedge* ans = mrss->newMddEdge();
  DCASSERT(ans);

  try {
    _unfairEG(revTime, mpe, ans);
    return new meddly_stateset(mp, ans);
  } 
  catch (sv_encoder::error err) { 
    Delete(ans);
    throw convert(err);
  }
  
}

stateset* meddly_monolithic_rg::AG(bool revTime, const stateset* p) 
{
  //
  // Uses AG p = !EF !p
  //

  const meddly_stateset* mp = dynamic_cast <const meddly_stateset*> (p);
  if (0==mp) return incompatibleOperand(revTime ? "EY" : "EX");
  const shared_ddedge* mpe = mp->getStateDD();
  DCASSERT(mpe);

  shared_ddedge* notp = mrss->newMddEdge();
  DCASSERT(notp);

  MEDDLY::apply( MEDDLY::COMPLEMENT, mpe->E, notp->E );

  shared_ddedge* ans = mrss->newMddEdge();
  DCASSERT(ans);

  try {
    _EU(revTime, 0, notp, ans); // EF !p = E true U !p
    Delete(notp);
    notp = 0;
    MEDDLY::apply( MEDDLY::COMPLEMENT, ans->E, ans->E );
    return new meddly_stateset(mp, ans);
  }
  catch (sv_encoder::error err) { 
    Delete(notp);
    Delete(ans);
    throw convert(err);
  }
}


meddly_encoder* meddly_monolithic_rg::newMxdWrapper(const char* n, 
  MEDDLY::forest::range_type t, MEDDLY::forest::edge_labeling ev) const
{
  DCASSERT(vars);
  MEDDLY::forest* f = vars->createForest(true, t, ev, getMxdForest()->getPolicies() );
  return mxd_wrap->copyWithDifferentForest(n, f);
}

#endif
