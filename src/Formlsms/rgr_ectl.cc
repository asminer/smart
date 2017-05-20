
// $Id$

#include "rgr_ectl.h"
#include "rss_indx.h"

#include "../Streams/streams.h"
#include "../include/heap.h"
#include "../Modules/expl_ssets.h"

// external library
#include "../_IntSets/intset.h"

// ******************************************************************
// *                                                                *
// *                    ectl_reachgraph  methods                    *
// *                                                                *
// ******************************************************************

ectl_reachgraph::ectl_reachgraph()
{
}

ectl_reachgraph::~ectl_reachgraph()
{
}

void ectl_reachgraph::getDeadlocked(intset &r) const
{
  r.removeAll();
}

void ectl_reachgraph::getInitial(intset &r) const
{
  //
  // Determine set of source (initial) states...
  //
  const indexed_reachset* irs = smart_cast <const indexed_reachset*> (getParent()->getRSS());
  DCASSERT(irs);
  irs->getInitial(r);
}

void ectl_reachgraph::getTSCCsSatisfying(intset &p) const
{
  p.removeAll();
}

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


