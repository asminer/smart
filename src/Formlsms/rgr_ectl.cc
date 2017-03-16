
// $Id$

#include "rgr_ectl.h"
#include "rss_indx.h"

#include "../Streams/streams.h"
#include "../include/heap.h"
#include "../Modules/expl_ssets.h"

// external library
#include "intset.h"

// ******************************************************************
// *                                                                *
// *            ectl_reachgraph::traverse_helper methods            *
// *                                                                *
// ******************************************************************

ectl_reachgraph::traverse_helper::traverse_helper(long NS)
{
  size = NS;
  obligations = new long[NS];
  queue = new long[NS];
  clear_queue();
}

ectl_reachgraph::traverse_helper::~traverse_helper()
{
  delete[] obligations;
  delete[] queue;
}

void ectl_reachgraph::traverse_helper::init_queue_from(const intset &p)
{
  DCASSERT(obligations);
  DCASSERT(queue);
  clear_queue();
  for (long i = p.getSmallestAfter(-1); i>=0; i = p.getSmallestAfter(i)) {
    obligations[i] = 0;
    queue_push(i);
  }
}

void ectl_reachgraph::traverse_helper::init_queue_complement(const intset &p)
{
  DCASSERT(obligations);
  clear_queue();
  long next_in_rset = p.getSmallestAfter(-1);
  for (long i=0; i<size; i++) {
    if (i == next_in_rset) {
      next_in_rset = p.getSmallestAfter(next_in_rset);
      continue;
    }
    obligations[i] = 0;
    queue_push(i);
  }
}

void ectl_reachgraph::traverse_helper::set_obligations(const intset &p, int value)
{
  DCASSERT(obligations);
  for (long i = p.getSmallestAfter(-1); i>=0; i = p.getSmallestAfter(i)) {
    obligations[i] = value;
  }
}

void ectl_reachgraph::traverse_helper::reset_zero_obligations(int newval)
{
  DCASSERT(obligations);
  for (long i=0; i<size; i++) {
    if (obligations[i]) continue;
    obligations[i] = newval;
  }
}

void ectl_reachgraph::traverse_helper::add_obligations(const intset &p)
{
  DCASSERT(obligations);
  for (long i = p.getSmallestAfter(-1); i>=0; i = p.getSmallestAfter(i)) {
    obligations[i]++;
  }
}


void ectl_reachgraph::traverse_helper::restrict_paths(const intset &p, int value)
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

void ectl_reachgraph::traverse_helper::get_met_obligations(intset &x) const
{
  DCASSERT(obligations);
  for (long i=0; i<size; i++) {
    if (0==obligations[i]) {
      x.addElement(i);
    }
  }
}

#ifdef DEBUG_ECTL
void ectl_reachgraph::traverse_helper::dump(FILE* out) const
{
  fprintf(out, "    queue: (");
  for (long i=0; i<queue_head; i++) {
    if (i) fprintf(out, ", ");
    fprintf(out, "%ld", queue[i]);
  }
  fprintf(out, ") ");
  for (long i=queue_head; i<queue_tail; i++) {
    if (i>queue_head) fprintf(out, ", ");
    fprintf(out, "%ld", queue[i]);
  }
  fprintf(out, "\n");
  fprintf(out, "    obligations: [%ld", obligations[0]);
  for (long i=1; i<size; i++) {
    fprintf(out, ", %ld", obligations[i]);
  }
  fprintf(out, "]\n");
}
#endif

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
  if (revTime) need_reverse_time();
  if (!TH) TH = new traverse_helper(ip.getSize());

  // Explore from p states
  TH->init_queue_from(ip);

  // set obligations to 1 AFTER we fill the queue
  TH->fill_obligations(1);

  // Traverse!
  startTraverse(CTLOP);
  traverse(revTime, true, *TH);
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

  // Explore from p states
  TH->init_queue_from(ep->getExplicit());

  // set obligations AFTER we fill the queue
  count_edges(revTime, *TH);

  // Traverse!
  startTraverse(CTLOP);
  traverse(revTime, true, *TH);
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
  if (revTime) need_reverse_time();
  if (!TH) TH = new traverse_helper(iq.getSize());

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
  traverse(revTime, false, *TH);
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
  if (revTime) need_reverse_time();
  if (!TH) TH = new traverse_helper(iq.getSize());

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
  traverse(revTime, false, *TH);
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
  if (revTime) need_reverse_time();
  if (!TH) TH = new traverse_helper(iq.getSize());

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
    traverse(revTime, false, *TH);
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
  if (revTime) need_reverse_time();
  if (!TH) TH = new traverse_helper(ip.getSize());

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
  traverse(revTime, false, *TH);
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
  if (revTime) need_reverse_time();
  if (!TH) TH = new traverse_helper(ip.getSize());

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
  if (revTime) need_reverse_time();
  if (!TH) TH = new traverse_helper(ip.getSize());

  // We're using !EF !p, so below is EF

  // obligations to 1
  TH->fill_obligations(1);

  // Explore from !p states
  TH->init_queue_complement(ip);

  // Traverse!
  startTraverse(CTLOP);
  traverse(revTime, false, *TH);
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

  if (revTime) need_reverse_time();
  if (!TH) TH = new traverse_helper(iq.getSize());

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
  traverse(revTime, false, *TH);
  stopTraverse(CTLOP);

  // Build answer
  intset* answer = new intset(TH->getSize());
  answer->removeAll();
  TH->get_met_obligations(*answer);
  return new expl_stateset(q->getParent(), answer);
}

// THE DEAD STUFF BELOW
#if 0

stateset* ectl_reachgraph::EX(bool revTime, const stateset* p) const
{
  if (0==p) return 0; // propogate an earlier error
  const expl_stateset* ep = dynamic_cast <const expl_stateset*> (p);
  if (0==ep) return incompatibleOperand(revTime ? "EY" : "EX");
  const intset& ip = ep->getExplicit(); 

  intset* answer = new intset(ip.getSize());
  answer->removeAll();

  if (revTime) {
    forward(ip, *answer);
  } else {
    backward(ip, *answer);
  }

  return new expl_stateset(p->getParent(), answer);
}

stateset* ectl_reachgraph::EU(bool revTime, const stateset* p, const stateset* q) const
{
  if (0==q) return 0; // propogate an earlier error

  const expl_stateset* ep = dynamic_cast <const expl_stateset*> (p);
  const expl_stateset* eq = dynamic_cast <const expl_stateset*> (q);

  if (0==eq) {
    if (revTime)  return incompatibleOperand(p ? "ES" : "EP");
    else          return incompatibleOperand(p ? "EU" : "EF");
  }

  const intset& iq = eq->getExplicit(); 

  if (0==p) {
    //
    // Easy special case of EF q
    //

    intset* answer = new intset(iq);

    if (reportCTL()) {
      long iters;
      if (revTime) {
        for (iters = 1; forward(*answer, *answer); iters++);
        reportIters("EP", iters);
      } else {
        for (iters = 1; backward(*answer, *answer); iters++);
        reportIters("EF", iters);
      }
    } else {
      if (revTime) {
        while (forward(*answer, *answer)); 
      } else {
        while (backward(*answer, *answer)); 
      }
    }

    return new expl_stateset(q->getParent(), answer);
  }

  //
  // E p U q
  //

  if (0==ep)   return incompatibleOperand(revTime ? "ES" : "EU");
  const intset& ip = ep->getExplicit(); 

  intset* answer = new intset(iq.getSize());

  intset tmp(iq.getSize());

  reportIters(revTime ? "ES" : "EU", _EU(revTime, ip, iq, *answer, tmp) );

  return new expl_stateset(q->getParent(), answer);
}

stateset* ectl_reachgraph::unfairEG(bool revTime, const stateset* p) const
{
  if (0==p) return 0; // propogate an earlier error
  const expl_stateset* ep = dynamic_cast <const expl_stateset*> (p);
  if (0==ep) return incompatibleOperand(revTime ? "EH" : "EG");
  const intset& ip = ep->getExplicit(); 

  intset* answer = new intset(ip.getSize());

  intset tmp(ip.getSize());

  reportIters(revTime ? "EH" : "EG", unfair_EG(revTime, ip, *answer, tmp) );

  return new expl_stateset(p->getParent(), answer);
}

stateset* ectl_reachgraph::fairEG(bool revTime, const stateset* p) const
{
  if (0==p) return 0; // propogate an earlier error
  const expl_stateset* ep = dynamic_cast <const expl_stateset*> (p);
  if (0==ep) return incompatibleOperand(revTime ? "EH" : "EG");
  const intset& ip = ep->getExplicit(); 

  intset* answer = new intset(ip.getSize());

  intset tmp(ip.getSize());

  reportIters(revTime ? "EH" : "EG", fair_EG(revTime, ip, *answer, tmp) );

  return new expl_stateset(p->getParent(), answer);
}

//
// Helpers
//

long ectl_reachgraph::_EU(bool revTime, const intset& p, const intset& q,
  intset &r, intset &tmp) const
{
  if (&q != &r) r.assignFrom(q);
  for (long iters=1; ; iters++) {
    tmp.removeAll();
    if (revTime)  forward(r, tmp);
    else          backward(r, tmp);
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

long ectl_reachgraph::unfair_EG(bool rT, const intset& p, intset &r, intset &tmp) const
{
  if (rT) getInitial(tmp);
  else    getDeadlocked(tmp);
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
    if (rT) forward(r, tmp);
    else    backward(r, tmp);
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

long ectl_reachgraph::fair_EG(bool rT, const intset& p, intset &r, intset &tmp) const
{
  // Start with TSCCs satisfying p
  r.assignFrom(p);
  getTSCCsSatisfying(r);

  // Add source / absorbing states satisfying p
  if (rT) getInitial(tmp);
  else    getDeadlocked(tmp);
  tmp *= p;
  r += tmp;

  // add everything that reaches / reachable from this set,
  // along states satisfying p
  return _EU(rT, p, r, r, tmp);
}

#endif

