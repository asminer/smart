
// $Id$

#include <stdlib.h>
#include <stdio.h>

#include "hyper.h"
#include "mc_absorb.h"
#include "intset.h"

// #define DEBUG_VANISH

// ******************************************************************
// *                           Front  end                           *
// ******************************************************************

MCLib::Markov_chain* MCLib::startAbsorbingMC(bool disc, long nt, long na)
{
  return new mc_absorb(disc, nt, na);
}

// ******************************************************************
// *                       mc_absorb  methods                       *
// ******************************************************************

// public

mc_absorb::mc_absorb(bool d, long ns, long na)
  : mc_base(d, ns, 0)
{
  stop_index = (long*) malloc (sizeof(long));
  num_classes = 0;

  stop_index[0] = ns;  // number of transients
  num_absorbing = na;
  num_states = ns + na;

  h = new hypersparse_matrix();
}

mc_absorb::~mc_absorb()
{
}

long mc_absorb::addState()
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
  long handle = stop_index[0];
  num_states++;
  stop_index[0]++;
  if (rowsums) {
    EnlargeRowsums(stop_index[0]);
    rowsums[stop_index[0]-1] = 0.0;
  }
  return handle;
}

long mc_absorb::addAbsorbing()
{
  if (finished) {
    throw MCLib::error(MCLib::error::Finished_Mismatch);
  }
  num_absorbing++;
  num_states++;
  return -num_absorbing;
}

bool mc_absorb::addEdge(long from, long to, double v)
{
  if (finished) {
    throw MCLib::error(MCLib::error::Finished_Mismatch);
  }
  if (v<=0) {
    throw MCLib::error(MCLib::error::Bad_Rate);
  }
  if ( from<0 ) {
    throw MCLib::error(MCLib::error::Wrong_Type);
  }

  if ( to < 0 ) {
    // transient to absorbing edge
    if (rowsums) rowsums[from] += v;
    return h->AddElement(from, to, v);
  }

  // Must be a transient to transient edge.

  try {
    bool ok = g->addEdge(from, to, v);
    if (rowsums) rowsums[from] += v;
    return ok;
  }
  catch (GraphLib::error e) {
    throw MCLib::error(e);
  }
}

void mc_absorb::finish(const finish_options &o, renumbering &r)
{
  if (finished) {
    throw MCLib::error(MCLib::error::Finished_Mismatch);
  }

  if (o.report) o.report->start("Converting TA to static");
  h->ConvertToStatic(false, !o.Will_Clear);
  if (o.report) o.report->stop();

  if (!g->isByRows()) {
    g->transpose(o.report);
  }

  if (rowsums) {
    if (o.report) o.report->start("Normalizing rows");
    row_normalizer foo(rowsums);
    g->traverseAll(foo);
    h->NormalizeRows(rowsums);
    free(rowsums);
    rowsums_alloc = 0;
    if (o.report) o.report->stop();
  } // if row_sums

  if (o.report) o.report->start("Computing diagonals");
  if (ood_size < g->getNumNodes()) {
    oneoverd = (float*) realloc(oneoverd, g->getNumNodes() * sizeof(float));
    ood_size = g->getNumNodes();
  } 
  long hh, hi;
  if (!h->getFirstRow(hi, hh)) hi = g->getNumNodes();
  for (long i=0; i<g->getNumNodes(); i++) {
    double x = 0;
    g->getRowSum(i, x);
    if (i == hi) {
      h->getRowSum(hh, x);
      if (!h->getNextRow(hi, hh)) hi = g->getNumNodes();
    }
    if (x) {
      if (x > maxdiag)  maxdiag = x;
      oneoverd[i] = 1.0 / x;
    } else {
      oneoverd[i] = 0.0;
    }
  };
  if (o.report) o.report->stop();

  // Can all states reach an absorbing one?
  if (o.Verify_Absorbing && stop_index[0]>0) {
    if (g->isByRows()) {
      g->transpose(o.report);
    }
    if (o.report) o.report->start("Checking absorbing-ness");
    checkAbsorbing(o);
  }

  try {
    g->finish(o);
  }
  catch (GraphLib::error e) {
    throw MCLib::error(e);
  }

  if (o.report) o.report->start("Renumbering states");
  for (long a=0; a<h->num_edges; a++) {
    h->column_index[a] = (stop_index[0]-1) - h->column_index[a];
  }
  r.setAbsorbRenumber();
  if (o.report) o.report->stop();
 
  finalize(Absorbing);
  baseFinish();
}

void mc_absorb::clear()
{
  clearKeepAbsorbing();
  num_absorbing = 0;
}

void mc_absorb::clearKeepAbsorbing()
{
  g->clear();
  h->Clear();
  stop_index[0] = 0;
  finished = false;
}

void mc_absorb
::AbsorbingRatesToMCRow(const double* n, long from, Markov_chain* R) const
{
  if (!finished) {
    throw MCLib::error(MCLib::error::Finished_Mismatch);
  }
  if (0==n) {
    throw MCLib::error(MCLib::error::Null_Vector);
  }

  // Vector matrix multiply
  long z = h->row_pointer[0];
  for (long r = 0; r<h->num_rows; r++) {
    double ni = n[h->row_index[r]];
    for (; z<h->row_pointer[r+1]; z++) {
      long to = h->column_index[z] - stop_index[0];
      double rate = ni * h->value[z];
#ifdef DEBUG_VANISH
      printf("Adding t->v*->t MC edge %ld : %ld : %lf", from, to, rate);
      bool dup = R->addEdge(from, to, rate);
      if (dup) printf(" (dup)\n"); else printf("\n");
#else 
      R->addEdge(from, to, rate);     
#endif
    } // for z
  } // for r
}


void mc_absorb
::AbsorbingRatesToRow(const double* n, long from, hypersparse_matrix* A) const
{
  if (!finished) {
    throw MCLib::error(MCLib::error::Finished_Mismatch);
  }
  if (0==n) {
    throw MCLib::error(MCLib::error::Null_Vector);
  }

  // Vector matrix multiply
  long z = h->row_pointer[0];
  for (long r = 0; r<h->num_rows; r++) {
    double ni = n[h->row_index[r]];
    for (; z<h->row_pointer[r+1]; z++) {
      long to = h->column_index[z] - stop_index[0];
      double rate = ni * h->value[z];
#ifdef DEBUG_VANISH
      printf("Adding t->v*->t HS edge %ld : %ld : %lf\n", from, to, rate);
#endif
      A->AddElement(from, to, rate);     
    } // for z
  } // for r
}

// protected

// Requires TT to be stored "by columns", TA to be stored "by rows".
void mc_absorb::checkAbsorbing(const finish_options &o) 
{
  if (0==stop_index[0]) return;
  long count = 0;
  long* queue = 0;
  if (o.Use_Compact_Sets) {
    intset reaches(stop_index[0]);
    queue = (long*) malloc(stop_index[0] * sizeof(long));
    if (0==queue) {
      throw MCLib::error(MCLib::error::Out_Of_Memory);
    }
    reaches.removeAll();
    for (long i=h->num_rows-1; i>=0; i--) {
      count += g->getReachable(h->row_index[i], reaches, queue);
    }
  } else {
    bool* reaches = (bool*) malloc(stop_index[0] * sizeof(bool));
    queue = (long*) malloc(stop_index[0] * sizeof(long));
    if (0==reaches || 0==queue) {
      free(reaches);
      free(queue);
      throw MCLib::error(MCLib::error::Out_Of_Memory);
    }
    for (long i=stop_index[0]-1; i>=0; i--) reaches[i] = false;
    for (long i=h->num_rows-1; i>=0; i--) {
      count += g->getReachable(h->row_index[i], reaches, queue);
    }
    free(reaches);
  }
  free(queue);
  
  if (count >= stop_index[0]) return;
  finalize(Error_type);
  throw MCLib::error(MCLib::error::Wrong_Type);
}

