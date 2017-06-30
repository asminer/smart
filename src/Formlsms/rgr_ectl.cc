
#include "rgr_ectl.h"
#include "rss_indx.h"

#include "../Streams/streams.h"
#include "../include/heap.h"
#include "../Modules/expl_ssets.h"

// external library
#include "../_IntSets/intset.h"

// ******************************************************************
// *                                                                *
// *             ectl_reachgraph::CTL_traversal methods             *
// *                                                                *
// ******************************************************************

ectl_reachgraph::CTL_traversal::CTL_traversal(long NS)
 : BF_with_queue(NS)
{
  size = NS;
  obligations = new long[NS];
}

ectl_reachgraph::CTL_traversal::~CTL_traversal()
{
  delete[] obligations;
}

bool ectl_reachgraph::CTL_traversal::visit(long, long dest, const void*)
{
  if (num_obligations(dest) <= 0) return false;
  remove_obligation(dest);
  if (one_step) return false;
  if (0==num_obligations(dest)) {
    queuePush(dest);
  }
  return false;
}

void ectl_reachgraph::CTL_traversal::init_queue_from(const intset &p)
{
  DCASSERT(obligations);
  queueReset();
  for (long i = p.getSmallestAfter(-1); i>=0; i = p.getSmallestAfter(i)) {
    obligations[i] = 0;
    queuePush(i);
  }
}

void ectl_reachgraph::CTL_traversal::init_queue_complement(const intset &p)
{
  DCASSERT(obligations);
  queueReset();
  long next_in_rset = p.getSmallestAfter(-1);
  for (long i=0; i<size; i++) {
    if (i == next_in_rset) {
      next_in_rset = p.getSmallestAfter(next_in_rset);
      continue;
    }
    obligations[i] = 0;
    queuePush(i);
  }
}

void ectl_reachgraph::CTL_traversal::set_obligations(const intset &p, int value)
{
  DCASSERT(obligations);
  for (long i = p.getSmallestAfter(-1); i>=0; i = p.getSmallestAfter(i)) {
    obligations[i] = value;
  }
}

void ectl_reachgraph::CTL_traversal::reset_zero_obligations(int newval)
{
  DCASSERT(obligations);
  for (long i=0; i<size; i++) {
    if (obligations[i]) continue;
    obligations[i] = newval;
  }
}

void ectl_reachgraph::CTL_traversal::add_obligations(const intset &p)
{
  DCASSERT(obligations);
  for (long i = p.getSmallestAfter(-1); i>=0; i = p.getSmallestAfter(i)) {
    obligations[i]++;
  }
}


void ectl_reachgraph::CTL_traversal::restrict_paths(const intset &p, int value)
{
  DCASSERT(obligations);
  long next_in_rset = p.getSmallestAfter(-1);
  for (long i=0; i<size; i++) {
    if (i == next_in_rset) {
      next_in_rset = p.getSmallestAfter(next_in_rset);
      continue;
    }
    obligations[i] = value;
  }
}

void ectl_reachgraph::CTL_traversal::get_met_obligations(intset &x) const
{
  DCASSERT(obligations);
  for (long i=0; i<size; i++) {
    if (0==obligations[i]) {
      x.addElement(i);
    }
  }
}


// ******************************************************************
// *                                                                *
// *                    ectl_reachgraph  methods                    *
// *                                                                *
// ******************************************************************

ectl_reachgraph::ectl_reachgraph()
{
  TH = 0;
}

ectl_reachgraph::~ectl_reachgraph()
{
  delete TH;
}

void ectl_reachgraph::getDeadlocked(intset &r) const
{
  r.removeAll();
}

const intset& ectl_reachgraph::getInitial() const
{
  //
  // Determine set of source (initial) states...
  //
  const indexed_reachset* irs = smart_cast <const indexed_reachset*> (getParent()->getRSS());
  DCASSERT(irs);
  return irs->getInitial();
}

void ectl_reachgraph::getTSCCsSatisfying(intset &p) const
{
  p.removeAll();
}


stateset* ectl_reachgraph::EX(bool revTime, const stateset* p)
{
  const char* CTLOP = revTime ? "EY" : "EX";
  if (0==p) return 0; // propogate an earlier error
  const expl_stateset* ep = dynamic_cast <const expl_stateset*> (p);
  if (0==ep) return incompatibleOperand(CTLOP);

  const intset& ip = ep->getExplicit(); 
  if (!TH) TH = new CTL_traversal(ip.getSize());

  // Explore from p states
  TH->init_queue_from(ip);

  // set obligations to 1 AFTER we fill the queue
  TH->fill_obligations(1);

  // Traverse!
  startTraverse(CTLOP);
  TH->setOneStep(true);
  traverse(revTime, *TH);
  stopTraverse(CTLOP);

  // Build answer
  intset* answer = new intset(TH->getSize());
  answer->removeAll();
  TH->get_met_obligations(*answer);
  return new expl_stateset(p->getParent(), answer);
}


stateset* ectl_reachgraph::AX(bool revTime, const stateset* p)
{
  const char* CTLOP = revTime ? "AY" : "AX";
  if (0==p) return 0; // propogate an earlier error
  const expl_stateset* ep = dynamic_cast <const expl_stateset*> (p);
  if (0==ep) return incompatibleOperand(CTLOP);

  const intset& ip = ep->getExplicit(); 
  if (!TH) TH = new CTL_traversal(ip.getSize());

  // Explore from p states
  TH->init_queue_from(ip);

  // set obligations AFTER we fill the queue
  count_edges(revTime, *TH);

  // Traverse!
  startTraverse(CTLOP);
  TH->setOneStep(true);
  traverse(revTime, *TH);
  stopTraverse(CTLOP);

  // Build answer
  intset* answer = new intset(TH->getSize());
  answer->removeAll();
  TH->get_met_obligations(*answer);
  return new expl_stateset(p->getParent(), answer);
}


stateset* ectl_reachgraph::EU(bool revTime, const stateset* p, const stateset* q)
{
  if (0==q) return 0; // propogate an earlier error

  const expl_stateset* ep = dynamic_cast <const expl_stateset*> (p);
  const expl_stateset* eq = dynamic_cast <const expl_stateset*> (q);

  const char* CTLOP = revTime 
    ?   ( p ? "ES" : "EP" )
    :   ( p ? "EU" : "EF" )
  ;

  if (0==eq) return incompatibleOperand(CTLOP);

  const intset& iq = eq->getExplicit(); 
  if (!TH) TH = new CTL_traversal(iq.getSize());

  // obligations to 1
  TH->fill_obligations(1);

  // if p is specified, then restrict paths to satisfying p
  if (p) {
    if (0==ep)   return incompatibleOperand(CTLOP);
    const intset& ip = ep->getExplicit(); 
    TH->restrict_paths(ip);
  }

  // Explore from q states
  TH->init_queue_from(iq);

  // Traverse!
  startTraverse(CTLOP);
  TH->setOneStep(false);
  traverse(revTime, *TH);
  stopTraverse(CTLOP);

  // Build answer
  intset* answer = new intset(TH->getSize());
  answer->removeAll();
  TH->get_met_obligations(*answer);
  return new expl_stateset(q->getParent(), answer);
}


stateset* ectl_reachgraph::unfairAU(bool revTime, const stateset* p, const stateset* q)
{
  if (0==q) return 0; // propogate an earlier error

  const expl_stateset* ep = dynamic_cast <const expl_stateset*> (p);
  const expl_stateset* eq = dynamic_cast <const expl_stateset*> (q);

  const char* CTLOP = revTime 
    ?   ( p ? "AS" : "AP" )
    :   ( p ? "AU" : "AF" )
  ;

  if (0==eq) return incompatibleOperand(CTLOP);

  const intset& iq = eq->getExplicit(); 
  if (!TH) TH = new CTL_traversal(iq.getSize());

  // obligations to edge counts
  count_edges(revTime, *TH);
  // Add imaginary edges to initial states
  if (revTime) TH->add_obligations(getInitial());
  // need to adjust any deadlocks
  TH->reset_zero_obligations(1);

  // if p is specified, then restrict paths to satisfying p
  if (p) {
    if (0==ep)   return incompatibleOperand(CTLOP);
    const intset& ip = ep->getExplicit(); 
    TH->restrict_paths(ip);
  }

  // Explore from q states
  TH->init_queue_from(iq);

  // Traverse!
  startTraverse(CTLOP);
  TH->setOneStep(false);
  traverse(revTime, *TH);
  stopTraverse(CTLOP);

  // Build answer
  intset* answer = new intset(TH->getSize());
  answer->removeAll();
  TH->get_met_obligations(*answer);
  return new expl_stateset(q->getParent(), answer);
}


stateset* ectl_reachgraph::fairAU(bool revTime, const stateset* p, const stateset* q)
{
  if (0==q) return 0; // propogate an earlier error

  const expl_stateset* ep = dynamic_cast <const expl_stateset*> (p);
  const expl_stateset* eq = dynamic_cast <const expl_stateset*> (q);

  const char* CTLOP = revTime 
    ?   ( p ? "AS" : "AP" )
    :   ( p ? "AU" : "AF" )
  ;

  if (0==eq) return incompatibleOperand(CTLOP);

  const intset& iq = eq->getExplicit(); 
  if (!TH) TH = new CTL_traversal(iq.getSize());

  // Based on !A p U q =  EG !q  OR  E p!q U !p!q

  // First, determine (fair) EG !q
  
  intset* answer = new intset(iq);
  answer->complement();
  fair_EG_helper(revTime, *answer, CTLOP);
  
  // answer is now (fair) EG !q.

  if (p) {
    //
    // Determine E p!q U !p!q
    //

    if (0==ep)   return incompatibleOperand(CTLOP);
    const intset& ip = ep->getExplicit(); 

    // obligations to 1
    TH->fill_obligations(1);

    //
    // Restrict paths to p!q
    //

    intset tmp(ip);
    // tmp = p
    tmp.complement();
    // tmp = !p
    tmp += iq;
    // tmp = !p + q
    tmp.complement();
    // tmp is now p!q
    TH->restrict_paths(tmp);

    //
    // Initialize queue to !p!q
    //
    tmp = ip;
    // tmp = p
    tmp += iq;
    // tmp = p+q
    tmp.complement();
    // tmp = !p!q
    TH->init_queue_from(tmp);

    //
    // Traverse
    //
    startTraverse(CTLOP);
    TH->setOneStep(false);
    traverse(revTime, *TH);
    stopTraverse(CTLOP);

    // add E p!q U !p!q to answer
    TH->get_met_obligations(*answer);
  }

  // answer is now EG !q  OR  E p!q U !p!q.
  // complement it and return
  answer->complement();
  return new expl_stateset(q->getParent(), answer);
}




stateset* ectl_reachgraph::unfairEG(bool revTime, const stateset* p)
{
  if (0==p) return 0; // propogate an earlier error
  const expl_stateset* ep = dynamic_cast <const expl_stateset*> (p);
  if (0==ep) return incompatibleOperand(revTime ? "EH" : "EG");

  const intset& ip = ep->getExplicit(); 
  if (!TH) TH = new CTL_traversal(ip.getSize());

  // Use !AF !p
  count_edges(revTime, *TH);
  if (revTime) {
    // Add imaginary edge to initial states
    TH->add_obligations(getInitial());
  } else {
    // Add imaginary edge from deadlock states
    TH->reset_zero_obligations(1);
  }
  // start searching from !p
  TH->init_queue_complement(ip);

  // Traverse!
  startTraverse(revTime ? "EH" : "EG");
  TH->setOneStep(false);
  traverse(revTime, *TH);
  stopTraverse(revTime ? "EH" : "EG");

  // build answer
  intset* answer = new intset(TH->getSize());
  answer->removeAll();
  TH->get_met_obligations(*answer);
  // answer is AF !p, invert it
  answer->complement();
  return new expl_stateset(p->getParent(), answer);
}


stateset* ectl_reachgraph::fairEG(bool revTime, const stateset* p)
{
  const char* CTLOP = revTime ? "EH" : "EG";
  if (0==p) return 0; // propogate an earlier error
  const expl_stateset* ep = dynamic_cast <const expl_stateset*> (p);
  if (0==ep) return incompatibleOperand(CTLOP);

  const intset& ip = ep->getExplicit(); 
  if (!TH) TH = new CTL_traversal(ip.getSize());

  intset* answer = new intset(ip);
  fair_EG_helper(revTime, *answer, CTLOP);
  return new expl_stateset(p->getParent(), answer);
}

stateset* ectl_reachgraph::AG(bool revTime, const stateset* p)
{
  const char* CTLOP = revTime ? "AH" : "AG";
  if (0==p) return 0; // propogate an earlier error
  const expl_stateset* ep = dynamic_cast <const expl_stateset*> (p);
  if (0==ep) return incompatibleOperand(CTLOP);

  const intset& ip = ep->getExplicit(); 
  if (!TH) TH = new CTL_traversal(ip.getSize());

  // We're using !EF !p, so below is EF

  // obligations to 1
  TH->fill_obligations(1);

  // Explore from !p states
  TH->init_queue_complement(ip);

  // Traverse!
  startTraverse(CTLOP);
  TH->setOneStep(false);
  traverse(revTime, *TH);
  stopTraverse(CTLOP);

  // build answer
  intset* answer = new intset(ip.getSize());
  answer->removeAll();
  TH->get_met_obligations(*answer);
  // answer is now EF !p, so flip it
  answer->complement();

  return new expl_stateset(p->getParent(), answer);
}


stateset* ectl_reachgraph::unfairAEF(bool revTime, const stateset* p, const stateset* q) 
{
  if (0==p || 0==q) return 0; // propogate an earlier error

  const expl_stateset* ep = dynamic_cast <const expl_stateset*> (p);
  const expl_stateset* eq = dynamic_cast <const expl_stateset*> (q);

  const char* CTLOP = revTime ? "AEP" : "AEF";

  if (0==ep || 0==eq) return incompatibleOperand(CTLOP);

  const intset& ip = ep->getExplicit(); 
  const intset& iq = eq->getExplicit(); 

  if (!TH) TH = new CTL_traversal(ip.getSize());

  // obligations to edge counts...
  count_edges(revTime, *TH);

  // Fix deadlock states
  TH->reset_zero_obligations(1);
  // If we're in set p, then obligation is 1
  TH->set_obligations(ip, 1);
  // Start search from set q
  TH->init_queue_from(iq);

  // Traverse!
  startTraverse(CTLOP);
  TH->setOneStep(false);
  traverse(revTime, *TH);
  stopTraverse(CTLOP);

  // Build answer
  intset* answer = new intset(TH->getSize());
  answer->removeAll();
  TH->get_met_obligations(*answer);
  return new expl_stateset(q->getParent(), answer);
}

