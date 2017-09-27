
#include "../ExprLib/mod_vars.h"

#include "rgr_meddly.h"

#ifdef HAVE_MEDDLY_H

#include "meddly_expert.h"

// ******************************************************************
// *                                                                *
// *                   meddly_trace_data methods                    *
// *                                                                *
// ******************************************************************

meddly_trace_data::meddly_trace_data()
{
}

meddly_trace_data::~meddly_trace_data()
{
  for (int i = 0; i < stages.Length(); i++) {
    Delete(stages.Item(i));
  }
}

void meddly_trace_data::AppendStage(shared_ddedge* s)
{
  stages.Append(s);
}

int meddly_trace_data::Length() const
{
  return stages.Length();
}

const shared_ddedge* meddly_trace_data::getStage(int i) const
{
  return stages.ReadItem(i);
}

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


stateset* meddly_monolithic_rg::EX(bool revTime, const stateset* p, trace_data* td)
{
  const meddly_stateset* mp = dynamic_cast <const meddly_stateset*> (p);
  if (0==mp) return incompatibleOperand(revTime ? "EY" : "EX");
  const shared_ddedge* mpe = mp->getStateDD();
  DCASSERT(mpe);

  shared_ddedge* ans = mrss->newMddEdge();

  try {
    if (nullptr == td) {
      _EX(revTime, mpe, ans);
    }
    else {
      // Keep the necessary data for trace generation later
      meddly_trace_data* mtd = dynamic_cast<meddly_trace_data*>(td);
      if (mtd == nullptr) {
        // TODO: To be implemented
        return nullptr;
      }
      List<shared_ddedge> qs;
      _EX(revTime, mpe, ans, &qs);
      DCASSERT(qs.Length() == 2);
      for (int i = 0; i < qs.Length(); i++) {
        shared_ddedge* sd = qs.Item(i);
        mtd->AppendStage(Share(sd));
        Delete(sd);
      }
    }

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
::EU(bool revTime, const stateset* p, const stateset* q, trace_data* td)
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
    if (nullptr == td) {
      _EU(revTime, mpe, mqe, ans);
    }
    else {
      // Keep the intermediate data for trace generation later
      meddly_trace_data* mtd = dynamic_cast<meddly_trace_data*>(td);
      if (nullptr == mtd) {
        // TODO: To be implemented
        DCASSERT(false);
        return nullptr;
      }
      List<shared_ddedge> qs;
      _EU(revTime, mpe, mqe, ans, &qs);
      DCASSERT(qs.Length() >= 2);

      // No need to keep the first stage due to fixed point
      Delete(qs.Item(0));
      for (int i = 1; i < qs.Length(); i++) {
        shared_ddedge* sd = qs.Item(i);
        mtd->AppendStage(Share(sd));
        Delete(sd);
      }
    }

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

stateset* meddly_monolithic_rg::unfairEG(bool revTime, const stateset* p, trace_data* td)
{
  const meddly_stateset* mp = dynamic_cast <const meddly_stateset*> (p);
  if (0==mp) return incompatibleOperand(revTime ? "EH" : "EG");
  const shared_ddedge* mpe = mp->getStateDD();
  DCASSERT(mpe);

  shared_ddedge* ans = mrss->newMddEdge();
  DCASSERT(ans);

  try {
    if (nullptr == td) {
      _unfairEG(revTime, mpe, ans);
    }
    else {
      // Keep the intermediate data for trace generation later
      meddly_trace_data* mtd = dynamic_cast<meddly_trace_data*>(td);
      if (nullptr == mtd) {
        // TODO: To be implemented
        DCASSERT(false);
        return nullptr;
      }
      List<shared_ddedge> qs;
      _unfairEG(revTime, mpe, ans, &qs);

      for (int i = 0; i < qs.Length(); i++) {
        shared_ddedge* sd = qs.Item(i);
        mtd->AppendStage(Share(sd));
        Delete(sd);
      }
    }
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

void meddly_monolithic_rg::traceEX(bool revTime, const stateset* p, const trace_data* td, List<stateset>* ans)
{
  const meddly_stateset* mp = dynamic_cast <const meddly_stateset*> (p);
  if (0==mp) {
    incompatibleOperand(revTime ? "EY" : "EX");
    return;
  }
  const shared_ddedge* mpe = mp->getStateDD();
  DCASSERT(mpe);
  const meddly_trace_data* mtd = dynamic_cast<const meddly_trace_data*>(td);
  DCASSERT(mtd);

  try {
    List<shared_ddedge> es;
    _traceEX(revTime, mpe, mtd, &es);

    for (int i = 0; i < es.Length(); i++) {
      shared_ddedge* e = es.Item(i);
      ans->Append(new meddly_stateset(mp, Share(e)));
      Delete(e);
    }
  }
  catch (sv_encoder::error err) {
    throw convert(err);
  }
}

void meddly_monolithic_rg::traceEU(bool revTime, const stateset* p, const trace_data* td, List<stateset>* ans)
{
  const meddly_stateset* mp = dynamic_cast <const meddly_stateset*> (p);
  if (0==mp) {
    incompatibleOperand(revTime ? "ES" : "EU");
    return;
  }
  const shared_ddedge* mpe = mp->getStateDD();
  DCASSERT(mpe);
  const meddly_trace_data* mtd = dynamic_cast<const meddly_trace_data*>(td);
  DCASSERT(mtd);

  try {
    List<shared_ddedge> es;
    _traceEU(revTime, mpe, mtd, &es);

    for (int i = 0; i < es.Length(); i++) {
      shared_ddedge* e = es.Item(i);
      ans->Append(new meddly_stateset(mp, Share(e)));
      Delete(e);
    }
  }
  catch (sv_encoder::error err) {
    throw convert(err);
  }
}

void meddly_monolithic_rg::traceEG(bool revTime, const stateset* p, const trace_data* td, List<stateset>* ans)
{
  const meddly_stateset* mp = dynamic_cast <const meddly_stateset*> (p);
  if (0==mp) {
    incompatibleOperand(revTime ? "EH" : "EG");
    return;
  }
  const shared_ddedge* mpe = mp->getStateDD();
  DCASSERT(mpe);
  const meddly_trace_data* mtd = dynamic_cast<const meddly_trace_data*>(td);
  DCASSERT(mtd);

  try {
    List<shared_ddedge> es;
    _traceEG(revTime, mpe, mtd, &es);

    for (int i = 0; i < es.Length(); i++) {
      shared_ddedge* e = es.Item(i);
      ans->Append(new meddly_stateset(mp, Share(e)));
      Delete(e);
    }
  }
  catch (sv_encoder::error err) {
    throw convert(err);
  }
}

stateset* meddly_monolithic_rg::attachWeight(const stateset* p) const
{
  return mrss->attachWeight(p);
}

meddly_encoder* meddly_monolithic_rg::newMxdWrapper(const char* n, 
  MEDDLY::forest::range_type t, MEDDLY::forest::edge_labeling ev) const
{
  DCASSERT(vars);
  MEDDLY::forest* f = vars->createForest(true, t, ev, getMxdForest()->getPolicies() );
  return mxd_wrap->copyWithDifferentForest(n, f);
}

// ******************************************************************
// *                                                                *
// *               meddly_monolithic_min_rg methods                 *
// *                                                                *
// ******************************************************************

meddly_monolithic_min_rg::meddly_monolithic_min_rg(shared_domain* v, meddly_encoder* wrap)
  : meddly_monolithic_rg(v, wrap),
    evmxd_wrap(nullptr)
{
}

meddly_encoder* meddly_monolithic_min_rg::getEvmxdWrap()
{
  if (nullptr == evmxd_wrap) {
    MEDDLY::forest* foo = vars->createForest(
      true, MEDDLY::forest::INTEGER, MEDDLY::forest::EVPLUS
    );
    evmxd_wrap = mxd_wrap->copyWithDifferentForest("EV+MxD", foo);
  }
  return evmxd_wrap;
}

shared_ddedge* meddly_monolithic_min_rg::newEvmxdEdge()
{
  return new shared_ddedge(getEvmxdWrap()->getForest());
}

stateset* meddly_monolithic_min_rg::EX(bool revTime, const stateset* p, trace_data* td)
{
  stateset* wp = attachWeight(p);
  meddly_stateset* mp = dynamic_cast <meddly_stateset*> (wp);
  if (0==mp) return incompatibleOperand(revTime ? "EY" : "EX");
  const shared_ddedge* mpe = mp->getStateDD();
  DCASSERT(mpe);

  shared_ddedge* ans = mrss->newEvmddEdge();

  try {
    // Keep the necessary data for trace generation later
    meddly_trace_data* mtd = dynamic_cast<meddly_trace_data*>(td);
    if (mtd == nullptr) {
      // TODO: To be implemented
      return nullptr;
    }
    List<shared_ddedge> qs;
    _EX(revTime, mpe, ans, &qs);
    DCASSERT(qs.Length() == 2);
    for (int i = 0; i < qs.Length(); i++) {
      shared_ddedge* sd = qs.Item(i);
      mtd->AppendStage(Share(sd));
      Delete(sd);
    }

    stateset* ss = new meddly_stateset(mp, ans);
    Delete(mp);
    return ss;
  }
  catch (sv_encoder::error err) {
    Delete(mp);
    Delete(ans);
    throw convert(err);
  }
}

stateset* meddly_monolithic_min_rg
::EU(bool revTime, const stateset* p, const stateset* q, trace_data* td)
{
  //
  // Grab p in a form we can use
  //
  meddly_stateset* mp = 0;
  const shared_ddedge* mpe = 0;
  if (p) {
    stateset* wp = attachWeight(p);
    mp = dynamic_cast<meddly_stateset*>(wp);
    if (0==mp) return incompatibleOperand(revTime ? "ES" : "EU");
    mpe = mp->getStateDD();
    DCASSERT(mpe);
  }

  //
  // Grab q in a form we can use
  //
  stateset* wq = attachWeight(q);
  meddly_stateset* mq = dynamic_cast <meddly_stateset*> (wq);
  if (0==mq) return incompatibleOperand(revTime ? "ES" : "EU");
  const shared_ddedge* mqe = mq->getStateDD();
  DCASSERT(mqe);

  // Result
  shared_ddedge* ans = mrss->newEvmddEdge();
  DCASSERT(ans);

  try {
    // Keep the intermediate data for trace generation later
    meddly_trace_data* mtd = dynamic_cast<meddly_trace_data*>(td);
    if (nullptr == mtd) {
      // TODO: To be implemented
      DCASSERT(false);
      return nullptr;
    }
    List<shared_ddedge> qs;
    _EU(revTime, mpe, mqe, ans, &qs);
    DCASSERT(qs.Length() == 2);

    for (int i = 0; i < qs.Length(); i++) {
      shared_ddedge* sd = qs.Item(i);
      mtd->AppendStage(Share(sd));
      Delete(sd);
    }

    stateset* ss = new meddly_stateset(mq, ans);
    Delete(mp);
    Delete(mq);
    return ss;
  }
  catch (sv_encoder::error err) {
    Delete(mp);
    Delete(mq);
    Delete(ans);
    throw convert(err);
  }
}

stateset* meddly_monolithic_min_rg::unfairEG(bool revTime, const stateset* p, trace_data* td)
{
  stateset* wp = attachWeight(p);
  meddly_stateset* mp = dynamic_cast <meddly_stateset*> (wp);
  if (0==mp) return incompatibleOperand(revTime ? "EH" : "EG");
  const shared_ddedge* mpe = mp->getStateDD();
  DCASSERT(mpe);

  shared_ddedge* ans = mrss->newEvmddEdge();
  DCASSERT(ans);

  try {
    // Keep the intermediate data for trace generation later
    meddly_trace_data* mtd = dynamic_cast<meddly_trace_data*>(td);
    if (nullptr == mtd) {
      // TODO: To be implemented
      DCASSERT(false);
      return nullptr;
    }
    List<shared_ddedge> qs;
    _unfairEG(revTime, mpe, ans, &qs);

    for (int i = 0; i < qs.Length(); i++) {
      shared_ddedge* sd = qs.Item(i);
      mtd->AppendStage(Share(sd));
      Delete(sd);
    }
    Delete(mp);
    return new meddly_stateset(mp, ans);
  }
  catch (sv_encoder::error err) {
    Delete(mp);
    Delete(ans);
    throw convert(err);
  }

}

void meddly_monolithic_min_rg::_EX(bool revTime, const shared_ddedge* p, shared_ddedge* ans, List<shared_ddedge>* extra)
{
  if (revTime) {
    mxd_wrap->postImage(p, edges, ans);
  } else {
    mxd_wrap->preImage(p, edges, ans);
  }

  extra->Append(Share(ans));
  extra->Append(Share(const_cast<shared_ddedge*>(p)));
}

void meddly_monolithic_min_rg::_EU(bool revTime, const shared_ddedge* p, const shared_ddedge* q,
  shared_ddedge* ans, List<shared_ddedge>* extra)
{
  MEDDLY::minimum_witness_opname::minimum_witness_args args(p == nullptr ? nullptr : p->E.getForest(),
    q->E.getForest(), edges->E.getForest(), ans->E.getForest());

  if (revTime) {
    // TODO: To be implemented
    return;
  }

  MEDDLY::specialized_operation* op = MEDDLY::CONSGTRAINT_BACKWARD_DFS->buildOperation(&args);
  op->compute(p->E, q->E, edges->E, ans->E);
  {
    shared_ddedge* t = mrss->newEvmddEdge();
    t->E = p->E;
    extra->Append(t);
  }
  extra->Append(Share(ans));
}

void meddly_monolithic_min_rg::_unfairEG(bool revTime, const shared_ddedge* p,
    shared_ddedge* ans, List<shared_ddedge>* extra)
{
  if (revTime) {
    // TODO: To be implemented
    DCASSERT(false);
    return;
  }

  shared_ddedge* tc = newEvmxdEdge();
  MEDDLY::apply(MEDDLY::COPY, edges->E, tc->E);
  MEDDLY::apply(MEDDLY::POST_PLUS, tc->E, p->E, tc->E);

  MEDDLY::minimum_witness_opname::minimum_witness_args args(p->E.getForest(),
    tc->E.getForest(), edges->E.getForest(), tc->E.getForest());
  MEDDLY::specialized_operation* tcOp = MEDDLY::TRANSITIVE_CLOSURE_DFS->buildOperation(&args);
  tcOp->compute(p->E, tc->E, edges->E, tc->E);

  shared_ddedge* cycles = mrss->newEvmddEdge();
  MEDDLY::apply(MEDDLY::CYCLE, tc->E, cycles->E);
  {
    // Increas by 1 because of repeating states in the cycle
    long ev = 0;
    cycles->E.getEdgeValue(ev);
    cycles->E.setEdgeValue(ev + 1);
  }

  _EU(revTime, p, cycles, ans, extra);

  Delete(tc);
}

void meddly_monolithic_min_rg::_traceEX(bool revTime, const shared_ddedge* p, const meddly_trace_data* mtd, List<shared_ddedge>* ans)
{
  DCASSERT(mtd->Length() == 2);
  // EX p
  // mtd[0]: cost to verify st |= EX p
  // mtd[1]: cost to verify st |= p

  shared_ddedge* f = mrss->newEvmddEdge();
  DCASSERT(f);

  shared_ddedge* g = mrss->newEvmddEdge();
  MEDDLY::apply( MEDDLY::INTERSECTION, p->E, mtd->getStage(0)->E, g->E );
  if (p->E != g->E) {
    // p must be included in mtd[0]
    DCASSERT(false);
    return;
  }

  if (revTime) {
    mxd_wrap->preImage(p, edges, f);
  } else {
    mxd_wrap->postImage(p, edges, f);
  }

  long ev = -1;
  f->E.getEdgeValue(ev);
  DCASSERT(ev > 2);
  f->E.setEdgeValue(ev - 2);

  // f := f /\ mtd[1]
  MEDDLY::apply( MEDDLY::INTERSECTION, f->E, mtd->getStage(1)->E, f->E );
  // f := select(f)
  MEDDLY::apply( MEDDLY::SELECT, f->E, f->E );

  {
    shared_ddedge* t = mrss->newEvmddEdge();
    DCASSERT(t);
    t->E = p->E;
    ans->Append(t);
  }
  ans->Append(Share(f));

  //
  // Cleanup
  //
  Delete(f);
}

void meddly_monolithic_min_rg::_traceEU(bool revTime, const shared_ddedge* p, const meddly_trace_data* mtd, List<shared_ddedge>* ans)
{
  DCASSERT(mtd->Length() == 2);
  // E p U q
  // mtd[0]: cost of st |= p
  // mtd[1]: cost of st |= E p U q

  shared_ddedge* f = mrss->newEvmddEdge();
  DCASSERT(f);
  f->E = p->E;

  shared_ddedge* g = mrss->newEvmddEdge();
  MEDDLY::apply( MEDDLY::INTERSECTION, f->E, mtd->getStage(1)->E, g->E );
  if (f->E != g->E) {
    // p must be included in mtd[1]
    DCASSERT(false);
    return;
  }

  while (true) {
    g->E = f->E;
    g->E.setEdgeValue(0);
    MEDDLY::apply( MEDDLY::INTERSECTION, g->E, mtd->getStage(0)->E, g->E );

    long ev1 = 0;   // Cost to verify p
    g->E.getEdgeValue(ev1);
    long ev2 = 0;   // Cost to verify E p U q
    f->E.getEdgeValue(ev2);
    if (0 == g->E.getNode() || ev2 <= ev1) {
      shared_ddedge* t = mrss->newEvmddEdge();
      DCASSERT(t);
      t->E = f->E;
      ans->Append(t);
      break;
    }

    if (revTime) {
      mxd_wrap->preImage(f, edges, g);
    } else {
      mxd_wrap->postImage(f, edges, g);
    }
    g->E.setEdgeValue(ev2 - ev1);

    MEDDLY::apply( MEDDLY::INTERSECTION, g->E, mtd->getStage(1)->E, g->E );

    long ev3 = 0;
    g->E.getEdgeValue(ev3);
    DCASSERT(0 == g->E.getNode() || ev3 >= ev2 - ev1);
    if (0 == g->E.getNode() || ev3 > ev2 - ev1) {
      shared_ddedge* t = mrss->newEvmddEdge();
      DCASSERT(t);
      t->E = f->E;
      ans->Append(t);
      break;
    }

    {
      shared_ddedge* t = mrss->newEvmddEdge();
      DCASSERT(t);
      t->E = f->E;
      t->E.setEdgeValue(ev1);
      ans->Append(t);
    }

    // f := select(g)
    MEDDLY::apply( MEDDLY::SELECT, g->E, f->E );
  }

  // Cleanup
  Delete(f);
  Delete(g);
}

void meddly_monolithic_min_rg::_traceEG(bool revTime, const shared_ddedge* p, const meddly_trace_data* mtd, List<shared_ddedge>* ans)
{
  DCASSERT(mtd->Length() == 2);
  // EG p
  // mtd[0]: cost to verify st |= p
  // mtd[1]: cost to verify st |= EG p

  // Generate a handle trace
  _traceEU(revTime, p, mtd, ans);
  DCASSERT(ans->Length() > 0);

  // Generate a cycle trace
  // A cycle is expressed as a finite sequence of states
  // where the first and last states are the same

  // The first state of the cycle
  shared_ddedge* first = mrss->newEvmddEdge();
  first->E = ans->Item(ans->Length() - 1)->E;

  // The last state of the cycle
  shared_ddedge* last = mrss->newEvmddEdge();
  last->E = first->E;
  last->E.setEdgeValue(1);

  // Update the cost of the last state in EU fragment
  // because we verify st |= p
  // not st is in a p-cycle
  MEDDLY::apply(MEDDLY::INTERSECTION, last->E, mtd->getStage(0)->E, ans->Item(ans->Length() - 1)->E);

  shared_ddedge* f = mrss->newEvmddEdge();
  f->E = last->E;
  shared_ddedge* g = mrss->newEvmddEdge();
  List<shared_ddedge> cache;
  // Backward
  while (g->E != first->E) {
    {
      shared_ddedge* t = mrss->newEvmddEdge();
      t->E = f->E;
      cache.Append(t);
    }
    mxd_wrap->preImage(f, edges, f);
    long ev = 0;
    f->E.getEdgeValue(ev);
    f->E.setEdgeValue(ev - 1);
    MEDDLY::apply( MEDDLY::PLUS, f->E, mtd->getStage(0)->E, f->E );

    MEDDLY::apply( MEDDLY::INTERSECTION, f->E, last->E, g->E );
  }
  // Forward
  f->E = first->E;
  long ev1 = 0;  // Cost of the current state
  ans->ReadItem(ans->Length() - 1)->E.getEdgeValue(ev1);
  for (int i = cache.Length() - 1; i >= 1; i--) {
    long ev2 = 0;   // The remaining cost
    f->E.getEdgeValue(ev2);

    mxd_wrap->postImage(f, edges, f);
    f->E.setEdgeValue(ev2 - ev1);

    MEDDLY::apply( MEDDLY::INTERSECTION, f->E, cache.ReadItem(i)->E, f->E );
    MEDDLY::apply( MEDDLY::SELECT, f->E, f->E );

    {
      shared_ddedge* t = mrss->newEvmddEdge();
      t->E = f->E;
      t->E.setEdgeValue(0);
      MEDDLY::apply( MEDDLY::INTERSECTION, t->E, mtd->getStage(0)->E, t->E );
      ans->Append(t);
      t->E.getEdgeValue(ev1);
    }
  }
  ans->Append(last);

  Delete(first);
  Delete(f);
  Delete(g);
  for (int i = 0; i < cache.Length(); i++) {
    Delete(cache.Item(i));
  }
}

#endif
