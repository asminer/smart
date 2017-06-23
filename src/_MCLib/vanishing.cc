
/**
  Implementation of Markov_chain class.
*/

#include "mclib.h"

#define DEBUG_GROUPBYSOURCE
#define DEBUG_ELIMINATE

//------------------------------------------------------------

#ifdef DEBUG_ELIMINATE
  #define USES_IOSTREAM
#endif

#ifdef DEBUG_GROUPBYSOURCE
  #define USES_IOSTREAM
#endif


#ifdef USES_IOSTREAM
#include <iostream>
using namespace std;

void showSet(const intset &S)
{
  bool comma = false;
  cout << "{";
  for (long i=S.getSmallestAfter(-1); i>=0; i=S.getSmallestAfter(i)) {
    if (comma) cout << ", ";
    cout << i;
    comma = true;
  }
  cout << "}";
}

#endif



// ======================================================================
// |                                                                    |
// |                 vanishing_chain::pairlist  methods                 |
// |                                                                    |
// ======================================================================

MCLib::vanishing_chain::pairlist::pairlist()
{
  pairarray = 0;
  alloc_pairs = 0;
  last_pair = -1;
}

MCLib::vanishing_chain::pairlist::~pairlist()
{
  free(pairarray);
}

void MCLib::vanishing_chain::pairlist::addItem(long i, double wt)
{
  //
  // Check last pair for duplicates; and add weights
  //
  if (last_pair>=0) {
      if (pairarray[last_pair].index == i) {
        pairarray[last_pair].weight += wt;
        return;
      }
  }

  if (last_pair+1 >= alloc_pairs) {
    long newalloc = alloc_pairs + 256;
    pair* newpairs = (pair*) realloc(pairarray, newalloc * sizeof(pair));
    if (0==newpairs) {
      throw error(error::Out_Of_Memory);
    }
    alloc_pairs = newalloc;
    pairarray = newpairs;
  }
  last_pair++;
  pairarray[last_pair].index = i;
  pairarray[last_pair].weight = wt;
}

void MCLib::vanishing_chain::pairlist::clear()
{
  last_pair = -1;
}


// ======================================================================
// |                                                                    |
// |                 vanishing_chain::edgelist  methods                 |
// |                                                                    |
// ======================================================================

MCLib::vanishing_chain::edgelist::edgelist()
{
  edgearray = 0;
  alloc_edges = 0;
  last_edge = -1;
}

MCLib::vanishing_chain::edgelist::~edgelist()
{
  free(edgearray);
}

void MCLib::vanishing_chain::edgelist::addEdge(long from, long to, double wt)
{
  //
  // Check last edge for duplicates; and add weights
  //
  if (last_edge>=0) {
    if (edgearray[last_edge].from == from) {
      if (edgearray[last_edge].to == to) {
        edgearray[last_edge].weight += wt;
        return;
      }
    }
  }

  if (last_edge+1 >= alloc_edges) {
    long newalloc = alloc_edges + 256;
    edge* newedges = (edge*) realloc(edgearray, newalloc * sizeof(edge));
    if (0==newedges) {
      throw error(error::Out_Of_Memory);
    }
    alloc_edges = newalloc;
    edgearray = newedges;
  }
  last_edge++;
  edgearray[last_edge].from = from;
  edgearray[last_edge].to = to;
  edgearray[last_edge].weight = wt;
}

void MCLib::vanishing_chain::edgelist::groupBySource()
{
#ifdef DEBUG_GROUPBYSOURCE
  cout << "Grouping by source.  Original edges:\n\t";  
  for (long i=0; i<=last_edge; i++) {
    cout << "(" << edgearray[i].from << ", " << edgearray[i].to;
    cout << ", " << edgearray[i].weight << ") ";
  }
  cout << "\n";
#endif

  //
  // Rearrange edges so that all edges from the same
  // source state are contiguous.
  //
  for (long i=0; i<=last_edge; i++) {
    // Increase left from i, decrease right from end.
    // We'll swap left and right so that everything
    // next to edge i is from the same source.
    long left=i+1;
    long right=last_edge;
    while (left<right) {
      //
      // Advance left until it differs from i
      //
      for (; left<right; left++) {
        if (edgearray[left].from != edgearray[i].from) break;
      }
      //
      // Advance right until it equals i
      //
      for (; left<right; right--) {
        if (edgearray[right].from == edgearray[i].from) break;
      }
      if (left >= right) break;
      swapedges(left, right);  
      i = left;
    }
  } // for i

#ifdef DEBUG_GROUPBYSOURCE
  cout << "Finished grouping edges by source.  Edges:\n\t";  
  for (long i=0; i<=last_edge; i++) {
    cout << "(" << edgearray[i].from << ", " << edgearray[i].to;
    cout << ", " << edgearray[i].weight << ") ";
  }
  cout << "\n";
#endif
}

void MCLib::vanishing_chain::edgelist::clear()
{
  last_edge = -1;
}

void MCLib::vanishing_chain::edgelist::swapedges(long i, long j)
{
  SWAP(edgearray[i].from, edgearray[j].from);
  SWAP(edgearray[i].to, edgearray[j].to);
  SWAP(edgearray[i].weight, edgearray[j].weight);
}

// ======================================================================
// |                                                                    |
// |                      vanishing_chain  methods                      |
// |                                                                    |
// ======================================================================

MCLib::vanishing_chain::vanishing_chain(bool disc, long nt, long nv)
 : TT_graph(disc, true), VV_graph(disc, true)
{
  discrete = disc;
  addTangibles(nt);
  addVanishings(nv);
}

MCLib::vanishing_chain::~vanishing_chain()
{
  // tbd
}



void MCLib::vanishing_chain::eliminateVanishing(const LS_Options &opt)
{

  // We use the following class for graph traversal
  // ======================================================================
  class back_reachable : public GraphLib::BF_with_queue {
    public:
      back_reachable(intset &_reachable)
        : BF_with_queue(_reachable.getSize())
        , reachable(_reachable)
      {
        reachable.removeAll();
      }

      virtual ~back_reachable() { }

      virtual bool visit(long src, long dest, const void*) 
      {
        if (!reachable.contains(src)) {
          reachable.addElement(src);
          queuePush(src);
        }
        return false;
      }

    private:
      intset &reachable;
  }; // class back_reachable
  // ======================================================================

  //
  // Convert the VV_graph into a static matrix, by columns.
  //
  GraphLib::static_graph VV_bycols;
  if (VV_graph.isByRows()) {
    GraphLib::static_graph VV_byrows;
    VV_graph.exportToStatic(VV_byrows, 0);
    VV_bycols.transposeFrom(VV_byrows);
  } else {
    VV_graph.exportToStatic(VV_bycols, 0);
  }

  // TBD - do we need VV_byrows?

  //
  // Make sure that every vanishing state can eventually reach
  // a tangible one.  First, scan VT_edges and add all vanishing states
  // to the explore queue.  Then do a breadth-first reverse search from
  // those states in the VV_graph, and make sure we can reach everything.
  //
  intset escapable(getNumVanishing());
  back_reachable foo(escapable);
  for (long i=0; i<=VT_edges.last_edge; i++) {
    foo.visit(VT_edges.edgearray[i].from, 0, 0);
  }
  VV_bycols.traverse(foo);
  escapable.complement();
#ifdef DEBUG_ELIMINATE
  cout << "Vanishing states with no escape path: ";
  showSet(escapable);
  cout << "\n";
#endif
  if (escapable.getSmallestAfter(-1) >= 0) {
    // There is a vanishing state we cannot escape.
    // Throw an error.
    throw error(error::Loop_Of_Vanishing);
  }


  //
  // Preprocess TV edges
  //
  TV_edges.groupBySource();

  // TBD - allocate an initial vector of dimension #vanishing

  // loop over different TV_edge sources
  // for each T source
  //     fill initial vector from TV_edge destinations
  //     solve linear system with matrix VV, for "time in v"
  //     multiply solution vector by VT edges, will
  //         get a vector of dimension #tangible;
  //         nonzeroes are T destinations.
  //         add a TT Edge for this
  //


  //
  // Cleanup
  //
  VV_graph.clear();
}

// ==========================================================================================================================================================================
// OLD STUFF BELOW
// ==========================================================================================================================================================================

#if 0
#include <stdlib.h>
#include <stdio.h>

#include "mclib.h"
#include "hyper.h"
#include "mc_absorb.h"
#include "mc_general.h"

// #define DEBUG_VANISH

// ******************************************************************
// *                                                                *
// *                        my_vanish  class                        *
// *                                                                *
// ******************************************************************

class my_vanish : public Old_MCLib::vanishing_chain {
  hypersparse_matrix* initial;
  mc_general* TT_proc;
  mc_absorb* V_proc;
  hypersparse_matrix* TV_rates;

  // stuff for eliminating vanishings
  Old_MCLib::Markov_chain::finish_options fopts;
  Old_MCLib::Markov_chain::renumbering mcrenumb;
  double* vtime;
  long vt_alloc;
public:
  my_vanish(bool disc, long nt, long nv);
  virtual ~my_vanish();

  virtual long addTangible();
  virtual long addVanishing();

  virtual bool addInitialTangible(long handle, double weight);
  virtual bool addInitialVanishing(long handle, double weight);

  virtual bool addTTedge(long from, long to, double v);
  virtual bool addTVedge(long from, long to, double v);
  virtual bool addVTedge(long from, long to, double v);
  virtual bool addVVedge(long from, long to, double v);

  virtual void eliminateVanishing(const LS_Options &opt);

  virtual void getInitialVector(LS_Vector &init);
  virtual Old_MCLib::Markov_chain* grabTTandClear();
};



// ******************************************************************
// *                           Front  end                           *
// ******************************************************************

Old_MCLib::vanishing_chain* 
Old_MCLib::startVanishingChain(bool disc, long nt, long nv)
{
  return new my_vanish(disc, nt, nv);
}

// ******************************************************************
// *                       my_vanish  methods                       *
// ******************************************************************

my_vanish::my_vanish(bool disc, long nt, long nv)
 : vanishing_chain(disc, nt, nv)
{
  initial = new hypersparse_matrix;
  TT_proc = new mc_general(disc, nt, 0);
  V_proc = new mc_absorb(false, nv, nt);
  TV_rates = new hypersparse_matrix;
  fopts.Verify_Absorbing = true;
  fopts.Store_By_Rows = false;
  fopts.Will_Clear = true;
  vtime = 0;
  vt_alloc = 0;
}

my_vanish::~my_vanish()
{
  delete initial;
  delete TT_proc;
  delete V_proc;
  delete TV_rates;
  free(vtime);
}

long my_vanish::addTangible()
{
  long handle = TT_proc->addState();
  long h2 = V_proc->addAbsorbing();
  if (-handle-1 != h2) throw Old_MCLib::error(Old_MCLib::error::Miscellaneous);
  num_tangible++;
  return handle;
}

long my_vanish::addVanishing()
{
  long handle = V_proc->addState();
  num_vanishing++;
  return handle;
}

bool my_vanish::addInitialTangible(long handle, double weight)
{
  return initial->AddElement(0, handle, weight);
}

bool my_vanish::addInitialVanishing(long handle, double weight)
{
  return TV_rates->AddElement(-1, handle, weight);
}

bool my_vanish::addTTedge(long from, long to, double v)
{
  return TT_proc->addEdge(from, to, v);
}

bool my_vanish::addTVedge(long from, long to, double v)
{
  return TV_rates->AddElement(from, to, v);
}

bool my_vanish::addVTedge(long from, long to, double v)
{
  return V_proc->addEdge(from, -to-1, v);
}

bool my_vanish::addVVedge(long from, long to, double v)
{
  return V_proc->addEdge(from, to, v);
}


void my_vanish::eliminateVanishing(const LS_Options &opt)
{
  // 1. finalize V_proc (into "by cols")
  try {
    V_proc->finish(fopts, mcrenumb);
  }
  // 2. if not absorbing then throw "absorbing_loop" error
  catch (Old_MCLib::error e) {
    if (e.getCode() == Old_MCLib::error::Wrong_Type)
      throw Old_MCLib::error(Old_MCLib::error::Loop_Of_Vanishing);
    else
      throw e;
  }

#ifdef DEBUG_VANISH
  printf("Finished vanishing process\n");
  if (Markov_chain::Success == mcerr) {
    LS_Matrix qtt;
    V_proc->exportQtt(qtt);
    V_proc->setClass(qtt, 0);
    printf("\tvv matrix:\n"); 
    printf("\tstart: %ld\t", qtt.start);
    printf("\tstop: %ld\n", qtt.stop);
    printf("\tcolptr: [");
    for (long i=0; i<=qtt.stop; i++) {
      if (i) printf(", ");
      printf("%ld", qtt.rowptr[i]);
    }
    printf("]\n");
    printf("\trowindex: [");
    for (long i=0; i<qtt.rowptr[qtt.stop]; i++) {
      if (i) printf(", ");
      printf("%ld", qtt.colindex[i]);
    }
    printf("]\n");
    printf("\tvalue: [");
    for (long i=0; i<qtt.rowptr[qtt.stop]; i++) {
      if (i) printf(", ");
      printf("%lf", qtt.f_value[i]);
    }
    printf("]\n");
    printf("\t1/diag: [");
    for (long i=0; i<qtt.stop; i++) {
      if (i) printf(", ");
      printf("%lf", qtt.f_one_over_diag[i]);
    }
    printf("]\n");
  }
#endif

  // Enlarge solution vector, if necessary
  if (vt_alloc < num_vanishing) {
    vt_alloc = num_vanishing;
    vtime = (double*) realloc(vtime, vt_alloc * sizeof(double));
    if (0==vtime) throw Old_MCLib::error(Old_MCLib::error::Out_Of_Memory);
  }

  // 3. for each non-empty row of rv do
  TV_rates->ConvertToStatic(false, false);
  for (long rh=0; rh<TV_rates->NumRows(); rh++) {
    // 3.1. build initial probability = scaled row of rv
    LS_Vector inittmp;
    long row;
    TV_rates->ExportRow(rh, row, &inittmp);
#ifdef DEBUG_VANISH
    if (row < 0) printf("Examining initial distribution: [");
    else         printf("Examining from tangible %ld: [", row);
    for (long i=0; i<inittmp.size; i++) {
      if (i) printf(", ");
      if (inittmp.index) printf("%ld:", inittmp.index[i]);
      if (inittmp.d_value) printf("%lf", inittmp.d_value[i]);
      else                 printf("%f", inittmp.f_value[i]);
    }
    printf("]\n");
#endif

    // 3.2. compute time per vanishing of v_proc using initial prob.
    LS_Output out;
    V_proc->computeTTA(inittmp, vtime, opt, out);
    if (out.status != LS_Success) {
      throw Old_MCLib::error(Old_MCLib::error::Miscellaneous);
    }

#ifdef DEBUG_VANISH
    printf("Got vanishing rate*times: [%lf", vtime[0]);
    for (long i=1; i<num_vanishing; i++) printf(", %lf", vtime[i]);
    printf("]\n");
#endif

    // 3.3. multiply to obtain t->v->t rates...
    if (row < 0) {
      // ... and add "virtual edges" to initial distribution
      V_proc->AbsorbingRatesToRow(vtime, row, initial);
    } else {
      // ... and add edges to tt_proc
      V_proc->AbsorbingRatesToMCRow(vtime, row, TT_proc);
    }
  } // for rh

  // 4. clear V_proc, TV_rates, others
  V_proc->clearKeepAbsorbing(); 
  TV_rates->Clear();
  num_vanishing = 0;
}


void my_vanish::getInitialVector(LS_Vector &init)
{
  if (!initial->IsStatic()) initial->ConvertToStatic(true, false);
  initial->ExportRowCopy(0, init);
}

Old_MCLib::Markov_chain* my_vanish::grabTTandClear()
{
  Old_MCLib::Markov_chain* mc = TT_proc;
  TT_proc = 0;
  V_proc->clear();
  return mc;
}

#endif
