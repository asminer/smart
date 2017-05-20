
// $Id$

#include <stdlib.h>
#include <stdio.h>

#include "hyper.h"
#include "mc_irred.h"
#include "../_IntSets/intset.h"

// ******************************************************************
// *                           Front  end                           *
// ******************************************************************

MCLib::Markov_chain* 
MCLib::startIrreducibleMC(bool disc, long numStates, long numEdges)
{
  return new mc_irred(disc, numStates, numEdges);
}

// ******************************************************************
// *                        mc_irred methods                        *
// ******************************************************************

mc_irred::mc_irred(bool disc, long ns, long ne) : mc_base(disc, ns, ne)
{
  stop_index = (long*) malloc (2*sizeof(long));
  stop_index[0] = 0;
  num_classes = 1;  // assuming everything works out...
  stop_index[1] = num_states = ns;
}

mc_irred::~mc_irred()
{
}

long mc_irred::addState()
{
  if (finished) {
    throw MCLib::error(MCLib::error::Finished_Mismatch);
  }
  try {
    g->addNode();
  }
  catch (GraphLib::error e) {
    throw MCLib::error(e);
  }
  long handle = num_states;
  num_states++;
  stop_index[1]++;
  if (rowsums) {
    EnlargeRowsums(num_states);
    rowsums[num_states-1] = 0.0;
  }
  return handle;
}

long mc_irred::addAbsorbing()
{
  throw MCLib::error(MCLib::error::Wrong_Type);
}

bool mc_irred::addEdge(long from, long to, double v)
{
  if (finished) {
    throw MCLib::error(MCLib::error::Finished_Mismatch);
  }
  if (v<=0) {
    throw MCLib::error(MCLib::error::Bad_Rate);
  }

  try {
    bool ok = g->addEdge(from, to, v);
    if (rowsums) rowsums[from] += v;
    return ok;
  }
  catch (GraphLib::error e) {
    throw MCLib::error(e);
  }
}

void mc_irred::finish(const finish_options &o, renumbering &r)
{
  if (finished) {
    throw MCLib::error(MCLib::error::Finished_Mismatch);
  }

  if (!g->isByRows()) {
    g->transpose(o.report);
  }

  if (rowsums) {
    if (o.report) o.report->start("Normalizing rows");
    row_normalizer foo(rowsums);
    g->traverseAll(foo);
    free(rowsums);
    rowsums_alloc = 0;
    if (o.report) o.report->stop();
  } // if row_sums

  if (o.report) o.report->start("Computing diagonals");
  if (ood_size < g->getNumNodes()) {
    oneoverd = (float*) realloc(oneoverd, g->getNumNodes() * sizeof(float));
    ood_size = g->getNumNodes();
  } 
  for (long i=0; i<g->getNumNodes(); i++) {
    double x = 0;
    g->getRowSum(i, x);
    if (x) {
      if (x > maxdiag)  maxdiag = x;
      oneoverd[i] = 1.0 / x;
    }
  };
  if (o.report) o.report->stop();

  if (o.Verify_Irred) {
    bool ok;
    if (o.report) o.report->start("Forward irreducible");
    intset rs_is;
    bool*  rs_bv = 0;
    if (o.Use_Compact_Sets) {
      rs_is.resetSize(g->getNumNodes());
      rs_is.removeAll();
      ok = (g->getReachable(0, rs_is) == g->getNumNodes());
    } else {
      rs_bv = (bool*) malloc(g->getNumNodes() * sizeof(bool));
      if (0==rs_bv) {
        throw MCLib::error(MCLib::error::Out_Of_Memory);
      }
      for (long i=g->getNumNodes()-1; i>=0; i--) rs_bv[i] = false;
      ok = (g->getReachable(0, rs_bv) == g->getNumNodes());
    }
    if (o.report) o.report->stop();
    if (!ok) {
      finalize(Error_type);
      free(rs_bv);
      throw MCLib::error(MCLib::error::Wrong_Type);
    }

    g->transpose(o.report);

    if (o.report) o.report->start("Backward irreducible");
    if (o.Use_Compact_Sets) {
      rs_is.removeAll();
      ok = (g->getReachable(0, rs_is) == g->getNumNodes());
    } else {
      for (long i=g->getNumNodes()-1; i>=0; i--) rs_bv[i] = false;
      ok = (g->getReachable(0, rs_bv) == g->getNumNodes());
      free(rs_bv);
    }
    if (o.report) o.report->stop();
    if (!ok) {
      finalize(Error_type);
      throw MCLib::error(MCLib::error::Wrong_Type);
    }
  } // if Verify

  try {
    g->finish(o);
  }
  catch (GraphLib::error e) {
    throw MCLib::error(e);
  }

  // no state renumbering necessary
  r.setNoRenumber();
 
  finalize(Irreducible);
  baseFinish();
}

void mc_irred::clear()
{
  g->clear();
  finished = false;
  our_type = Unknown;
}

