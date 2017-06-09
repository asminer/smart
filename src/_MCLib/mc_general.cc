
#include <stdlib.h>
#include <stdio.h>

#include "hyper.h"
#include "mc_general.h"

// #include "sccs.h"

// #define DEBUG_CLASSIFY
// #define DEBUG_SPLIT

// ******************************************************************
// *                           Front  end                           *
// ******************************************************************

Old_MCLib::Markov_chain* 
Old_MCLib::startUnknownMC(bool disc, long states, long edges)
{
  return new mc_general(disc, states, edges);
}

// ******************************************************************
// *                                                                *
// *                       mc_splitter  class                       *
// *                                                                *
// ******************************************************************

class mc_splitter : public GraphLib::generic_graph::element_visitor {
  long num_transient;
  hypersparse_matrix* Etr;
public:
  mc_splitter(long nt, hypersparse_matrix* E) {
    num_transient = nt;
    Etr = E;
  }
  virtual bool visit(long from, long to, void* wt);
};

bool mc_splitter::visit(long from, long to, void* wt)
{
  float* rate = (float*) wt;
  if (from >= num_transient || to < num_transient) {
#ifdef DEBUG_SPLIT
    printf("Edge %ld:%ld:%f stays\n", from, to, rate[0]);
#endif
    return false;
  }
#ifdef DEBUG_SPLIT
  printf("Edge %ld:%ld:%f moves\n", from, to, rate[0]);
#endif
  Etr->AddElement(from, to, rate[0]);
  return true;
}

// ******************************************************************
// *                       mc_general methods                       *
// ******************************************************************


mc_general::mc_general(bool disc, long ns, long ne)
  : mc_base(disc, ns, ne)
{
  num_states = ns;
}

mc_general::~mc_general()
{
}

long mc_general::addState()
{
  if (finished) {
    throw Old_MCLib::error(Old_MCLib::error::Finished_Mismatch);
  }
  try {
    g->addNode();
  }
  catch (GraphLib::error e) {
    throw Old_MCLib::error(e);
  }
  long handle = num_states;
  num_states++;
  if (rowsums) {
    EnlargeRowsums(num_states);
    rowsums[handle] = 0.0;
  }
  return handle;
}

long mc_general::addAbsorbing()
{
  return mc_general::addState();
}

bool mc_general::addEdge(long from, long to, double v)
{
  if (finished) {
    throw Old_MCLib::error(Old_MCLib::error::Finished_Mismatch);
  }
  if (v<=0) {
    throw Old_MCLib::error(Old_MCLib::error::Bad_Rate);
  }
  try {
    bool ok = g->addEdge(from, to, v);
    if (rowsums) rowsums[from] += v;
    return ok;
  }
  catch (GraphLib::error e) {
    throw Old_MCLib::error(e);
  }
}

void mc_general::finish(const finish_options &o, renumbering &r)
{
  if (finished) {
    throw Old_MCLib::error(Old_MCLib::error::Finished_Mismatch);
  }

  if (!g->isByRows()) {
    g->transpose(o.report);
  }

  if (rowsums) {
    if (o.report) o.report->start("Normalizing rows");
    row_normalizer foo(rowsums);
    g->traverseAll(foo);
    free(rowsums);
    rowsums = 0;
    rowsums_alloc = 0;
    if (o.report) o.report->stop();
  } // if row_sums

  // Super special case: MC with one state
  if (1 == num_states) {
    // set this as an absorbing chain with no transient states
    num_classes = 0;
    stop_index = new long[1];
    stop_index[0] = 0;
    finalize(Absorbing);
    r.setNoRenumber();
    g->finish(o);
    baseFinish();
    return;
  }

  // determine SCCs here... 
  long* sccmap = (long*) malloc(num_states * sizeof(long));
  long* aux = (long*) malloc(num_states * sizeof(long));
  if (0==sccmap || 0==aux) {
    free(sccmap);
    free(aux);
    finalize(Error_type);
    throw Old_MCLib::error(Old_MCLib::error::Out_Of_Memory);
  }
  long num_tsccs;
  try {
    num_tsccs = g->computeTSCCs(o.report, o.SCCs_Optimize_Memory, sccmap, aux); 
  }
  catch (GraphLib::error e) {
    finalize(Error_type);
    free(sccmap);
    free(aux);
    throw Old_MCLib::error(e);
  }

#ifdef DEBUG_CLASSIFY
  printf("ComputeTSCCs returned %ld\n", num_tsccs);
  printf("Mapping of states to sccs:\n[%ld", sccmap[0]);
  for (int i=1; i<num_states; i++) printf(", %ld", sccmap[i]);
  printf("]\n");
#endif

  // Important special case: is this an irreducible chain?
  bool is_irred = true;
  long num_absorbing = 0;
  for (long i=0; i<num_states; i++) {
    is_irred = (1 == sccmap[i]);
    if (!is_irred) break;
  }
  if (is_irred) {
    finalize(Irreducible);
  } else {
    // Count the number of asborbing states
    for (long i=0; i<num_states; i++) {
      if (g->RowPtr(i)<0) num_absorbing++;
    }
#ifdef DEBUG_CLASSIFY
    printf("Counted %ld absorbing states\n", num_absorbing);
#endif

    // Check special case: absorbing chain
    if (num_absorbing == num_tsccs) {
      finalize(Absorbing);
    }
  }

  if (o.report) o.report->start("Renumbering states");

  // Count the number of states in each non-absorbing class
  num_classes = num_tsccs - num_absorbing;
  stop_index = new long[1+num_classes];
  for (long i=0; i<=num_classes; i++) stop_index[i] = 0;
  for (long i=0; i<num_states; i++) {
    if (g->RowPtr(i)>=0) stop_index[sccmap[i]]++;
  }
#ifdef DEBUG_CLASSIFY
  printf("Counted number of states per class:\n");
  printf("\tTransient: %ld\n", stop_index[0]);
  for (long i=1; i<=num_classes; i++) {
    printf("\tClass %3ld: %ld\n", i, stop_index[i]);
  }
#endif 

  // Determine starting index per class
  for (long i=1; i<=num_classes; i++) stop_index[i] += stop_index[i-1];
  for (long i=num_classes; i; i--) stop_index[i] = stop_index[i-1];
  stop_index[0] = 0;
#ifdef DEBUG_CLASSIFY
  printf("Starting index per class:\n[%ld", stop_index[0]);
  for (long i=1; i<=num_classes; i++) printf(", %ld", stop_index[i]);
  printf("]\n");
#endif

  // Renumber non-absorbing
  long num = 0;
  for (long i=0; i<num_states; i++) {
    if (g->RowPtr(i)>=0) {
      aux[i] = stop_index[sccmap[i]];
      stop_index[sccmap[i]]++;
      num++;
    }
  }
  // Renumber absorbing
  for (long i=0; i<num_states; i++) {
    if (g->RowPtr(i)<0) {
      aux[i] = num;
      num++;
    }
  }
  // Is renumbering necessary?
  if (IsIdentity(aux)) {
    free(aux);
    aux = 0;
  }
  // don't need this anymore
  free(sccmap);
  sccmap = 0;

#ifdef DEBUG_CLASSIFY
  if (aux) {
    printf("Renumber array: [%ld", aux[0]);
    for (long i=1; i<num_states; i++)
      printf(", %ld", aux[i]);
    printf("]\n");
  } else {
    printf("No renumbering necessary!\n");
  }
  printf("Stopping index per class:\n[%ld", stop_index[0]);
  for (long i=1; i<=num_tsccs; i++) printf(", %ld", stop_index[i]);
  printf("]\n");
#endif

  // renumber the graph, if necessary
  try {
    if (aux) { 
      g->renumber(aux);
      r.setGeneralRenumber(aux);
    } else {
      r.setNoRenumber();
    }
  }
  catch (GraphLib::error e) {
    finalize(Error_type);
    throw Old_MCLib::error(e);
  }

  if (o.report) {
    o.report->stop();
    o.report->start("Computing diagonals");
  }

  long nna = stop_index[num_classes];
  if (ood_size < nna) {
    oneoverd = (float*) realloc(oneoverd, nna * sizeof(float));
    ood_size = nna;
  } 
  for (long i=0; i < nna; i++) {
    double x = 0;
    g->getRowSum(i, x);
    if (x>0) {
      if (x > maxdiag)  maxdiag = x;
      oneoverd[i] = 1.0 / x;
    }
  };

  if (o.report) {
    o.report->stop();
    o.report->start("Rearranging matrix");
  }

  try {
    h = is_irred ? 0 : new hypersparse_matrix();
    if (stop_index[0]>0) {
      mc_splitter mysplit(stop_index[0], h);
      g->removeEdges(mysplit);
    }
    if (o.report) o.report->stop();

    // Finish the graph itself
    g->finish(o);
  }
  catch (GraphLib::error e) {
    finalize(Error_type);
    throw Old_MCLib::error(e);
  }

  // Finish transient to recurrent, if present
  if (h) {
    if (o.report) o.report->start("Converting TA to static");
    h->ConvertToStatic(false, !o.Will_Clear);
    if (o.report) o.report->stop();
  }

  if (Unknown == our_type)  finalize(Reducible);
  baseFinish();
}

void mc_general::clear()
{
  g->clear();
  h->Clear();
  finished = false;
  our_type = Unknown;
}

