
/**
  Implementation of Markov_chain class.
*/

#include "mclib.h"

// #define DEBUG_GROUPBYSOURCE
// #define DEBUG_ELIMINATE
// #define DEBUG_VANLOOP

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

void showVector(const char* name, double* v, long size)
{
  cout << name << ": [" << v[0];
  for (long i=1; i<size; i++) {
    cout << ", " << v[i];
  }
  cout << "]\n";
}

#endif

template <class TYPE>
inline void zeroArray(TYPE* A, long size)
{
  for (long i=0; i<size; i++) A[i] = 0;
}

inline double vectorTotal(double* v, long size)
{
  double total = 0;
  for (long i=0; i<size; i++) {
    total += v[i];
  }
  return total;
}

inline void scaleVector(double a, double* v, long size)
{
  for (long i=0; i<size; i++) {
    v[i] *= a;
  }
}


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

// ******************************************************************

MCLib::vanishing_chain::pairlist::~pairlist()
{
  free(pairarray);
}

// ******************************************************************

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

// ******************************************************************

MCLib::vanishing_chain::edgelist::~edgelist()
{
  free(edgearray);
}

// ******************************************************************

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
  cout << "  Grouping by source.  Original edges:\n\t";  
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
  cout << "  Finished grouping edges by source.  Edges:\n\t";  
  for (long i=0; i<=last_edge; i++) {
    cout << "(" << edgearray[i].from << ", " << edgearray[i].to;
    cout << ", " << edgearray[i].weight << ") ";
  }
  cout << "\n";
#endif
}

// ******************************************************************

void MCLib::vanishing_chain::edgelist::clear()
{
  last_edge = -1;
}

// ******************************************************************

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

// ******************************************************************

MCLib::vanishing_chain::~vanishing_chain()
{
  // tbd
}


// ******************************************************************


void MCLib::vanishing_chain::eliminateVanishing(const LS_Options &opt)
{
  // SUPER DUPER EASY CASE
  if (0==getNumVanishing()) return;


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
        if (!reachable.contains(dest)) {
          reachable.addElement(dest);
          queuePush(dest);
#ifdef DEBUG_VANLOOP
          cout << "      vanishing " << dest << " is ok\n";
#endif
        }
        return false;
      }

    private:
      intset &reachable;
  }; // class back_reachable
  // ======================================================================

  //
  // Determine row sums (and invert for one_over_diagonals)
  // for VV, VT graphs
  //
  DCASSERT(VV_graph.isByRows());
  double* VV_one_over_diag = new double[getNumVanishing()];
  zeroArray(VV_one_over_diag, getNumVanishing());
  for (long i=0; i<=VT_edges.last_edge; i++) {
    VV_one_over_diag[VT_edges.edgearray[i].from] += VT_edges.edgearray[i].weight;
  }
  VV_graph.addRowSums(VV_one_over_diag);
#ifdef DEBUG_ELIMINATE
  showVector("  VT,VV rowsums", VV_one_over_diag, getNumVanishing());
#endif
  // Now invert
  for (long i=0; i<getNumVanishing(); i++) {
    if (VV_one_over_diag[i]) {
      VV_one_over_diag[i] = 1.0 / VV_one_over_diag[i];
    }
  }

  //
  // Convert the VV_graph into a static matrix, by columns.
  //
  GraphLib::static_graph VV_bycols;
  GraphLib::static_graph VV_byrows;
  VV_graph.exportToStatic(VV_byrows, 0);
  VV_bycols.transposeFrom(VV_byrows);

  //
  // Make sure that every vanishing state can eventually reach
  // a tangible one.  First, scan VT_edges and add all vanishing states
  // to the explore queue.  Then do a breadth-first reverse search from
  // those states in the VV_graph, and make sure we can reach everything.
  //
#ifdef DEBUG_VANLOOP
  cout << "  Checking if vanishing states can all reach tangibles.\n";
#endif
  intset escapable(getNumVanishing());
  back_reachable foo(escapable);
  for (long i=0; i<=VT_edges.last_edge; i++) {
    foo.visit(0, VT_edges.edgearray[i].from, 0);
  }
#ifdef DEBUG_VANLOOP
  cout << "  Vanishing states with edges to tangible: ";
  showSet(escapable);
  cout << "\n";
#endif
  VV_bycols.traverse(foo);
#ifdef DEBUG_VANLOOP
  cout << "  Vanishing states with path to tangible: ";
  showSet(escapable);
  cout << "\n";
#endif
  escapable.complement();
#ifdef DEBUG_VANLOOP
  cout << "  Vanishing states with no escape path: ";
  showSet(escapable);
  cout << "\n";
#endif
  if (escapable.getSmallestAfter(-1) >= 0) {
    // There is a vanishing state we cannot escape.
    // Throw an error.
    throw error(error::Loop_Of_Vanishing);
  }
  //
  // Build matrix for VV
  //
  LS_CRS_Matrix_double Mvv;
  Mvv.start = 0;
  Mvv.stop = getNumVanishing();
  Mvv.size = getNumVanishing();
  Mvv.val = (const double*) VV_bycols.Labels();
  DCASSERT(VV_bycols.EdgeBytes() == sizeof(double));
  Mvv.col_ind = VV_bycols.ColumnIndex();
  Mvv.row_ptr = VV_bycols.RowPointer();
  Mvv.one_over_diag = VV_one_over_diag;


  //
  // Preprocess TV edges
  //
  TV_edges.groupBySource();


  LS_Output out;
  // Initial vector to use for linear system
  double* Vinit_vect = new double[getNumVanishing()];
  LS_Vector V0;
  V0.size = getNumVanishing();
  V0.index = 0;
  V0.d_value = Vinit_vect;
  V0.f_value = 0;
  zeroArray(Vinit_vect, getNumVanishing());
  // Solution vector
  double* n = new double[getNumVanishing()];

  //
  // Get ready for first batch of source states
  //
  long last_src = -1;
  if (TV_edges.last_edge>=0) {
    last_src = TV_edges.edgearray[0].from;
  }
  for (long i=0; ; i++) {
    long ifrom = (i<=TV_edges.last_edge) 
      ? TV_edges.edgearray[i].from
      : getNumTangible(); 
    if (ifrom != last_src) {
#ifdef DEBUG_ELIMINATE
      cout << "  Determining new edges from tangible " << last_src << "\n";
      showVector("    initial weights", Vinit_vect, getNumVanishing());
#endif
      //
      // That's it for the previous batch of source states.
      // Negate Vinit_vect
      //
      scaleVector(-1, Vinit_vect, getNumVanishing());
      //
      // Now, solve linear system   n * Mvv = -Vinit_vect
      // which will give the expected time spent in each
      // vanishing state.
      //
      zeroArray(n, getNumVanishing());
      Solve_Axb(Mvv, n, V0, opt, out);
#ifdef DEBUG_ELIMINATE
      showVector("    time per vanishing", n, getNumVanishing());
#endif
      //
      // Multiply n by VT edges.  Result is dimension #tangible,
      // and are the new edges we need to add.
      //
      for (long j=0; j<=VT_edges.last_edge; j++) {
        double new_wt 
          = n[VT_edges.edgearray[j].from] * VT_edges.edgearray[j].weight;
        if (0==new_wt) continue;
#ifdef DEBUG_ELIMINATE
        cout << "    adding TT edge " << last_src << " : ";
        cout << VT_edges.edgearray[j].to << " : " << new_wt;
        cout << "\n";
#endif
        addTTedge(last_src, VT_edges.edgearray[j].to, new_wt);
      } // for j

      //
      // Reset for next source state
      //
      zeroArray(Vinit_vect, getNumVanishing());
      last_src = TV_edges.edgearray[i].from;
    }
    if (i>TV_edges.last_edge) break;
    Vinit_vect[ TV_edges.edgearray[i].to ] += TV_edges.edgearray[i].weight;
  } // for i

  //
  // Convert vanishing initial vector to tangibles.
  //
  // Fill Vinit_vect from Vinit
  for (long j=0; j<=Vinit.last_pair; j++) {
    Vinit_vect[ Vinit.pairarray[j].index ] += Vinit.pairarray[j].weight;
  }
#ifdef DEBUG_ELIMINATE
  cout << "  Rebuilding initial probabilities\n";
  showVector("    initial vanishing", Vinit_vect, getNumVanishing());
#endif
  scaleVector(-1, Vinit_vect, getNumVanishing());
  //
  // Solve n * Mvv = -Vinit_vect
  //
  Solve_Axb(Mvv, n, V0, opt, out);
#ifdef DEBUG_ELIMINATE
  showVector("    time per vanishing", n, getNumVanishing());
#endif
  //
  // Multiply n by VT edges.  Result is dimension #tangible,
  // and are the new initial probabilities to add.
  //
  for (long j=0; j<=VT_edges.last_edge; j++) {
    double new_wt 
      = n[VT_edges.edgearray[j].from] * VT_edges.edgearray[j].weight;
    if (0==new_wt) continue;
#ifdef DEBUG_ELIMINATE
    cout << "    adding initial tangible ";
    cout << VT_edges.edgearray[j].to << " : " << new_wt << "\n";
#endif
    addInitialTangible(VT_edges.edgearray[j].to, new_wt);
  } // for j

  //
  // Cleanup
  //
  delete[] n;
  delete[] Vinit_vect;
  delete[] VV_one_over_diag;

  //
  // Clear out old stuff
  //
  VV_graph.clear();
  TV_edges.clear();
  VT_edges.clear();
  Vinit.clear();
}


// ******************************************************************


void MCLib::vanishing_chain::buildInitialVector(bool floats, LS_Vector &init) const
{
#ifdef DEBUG_ELIMINATE
  cout << "building initial vector\n";
#endif

  //
  // Overwrite any existing vector
  //
  init.size = 0;
  init.index = 0;
  init.d_value = 0;
  init.f_value = 0;

  //
  // First, build a full vector for the initial distribution
  //
  double* initial = new double[getNumTangible()];
  zeroArray(initial, getNumTangible());
  for (long j=0; j<=Tinit.last_pair; j++) {
    initial[ Tinit.pairarray[j].index ] += Tinit.pairarray[j].weight;
  } // for j
  //
  // Normalize the initial distribution
  //
  double total = vectorTotal(initial, getNumTangible());
  if (total) scaleVector(1.0/total, initial, getNumTangible());

#ifdef DEBUG_ELIMINATE
  showVector("  initial distribution", initial, getNumTangible());
#endif

  //
  // Now, determine if we are better off compacting the initial vector
  // into a sparse or truncated full format.
  //
  long last_nz = -1;
  long nnz = 0;
  for (long i=0; i<getNumTangible(); i++) {
    if (0==initial[i]) continue;
    nnz++;
    last_nz = i;
  }
  last_nz++;

  if (0==nnz) {
    // Leave init zeroed out
    delete[] initial;
    return;
  }

#ifdef DEBUG_ELIMINATE
  cout << "  nnz: " << nnz << "\n";
  cout << "  last nonzero: " << last_nz << "\n";
#endif

  //
  // Get storage space for both schemes
  //
  const size_t real_bytes = floats ? sizeof(float) : sizeof(double);
  const size_t index_bytes = sizeof(long);
  const size_t trunc_bytes = last_nz * real_bytes;
  const size_t sparse_bytes = nnz * (real_bytes + index_bytes);

  if (sparse_bytes < trunc_bytes) {
    //
    // Convert initial to sparse
    //
#ifdef DEBUG_ELIMINATE
    cout << "  using sparse " << (floats ? "float" : "double") << " vector\n";
#endif
    init.size = nnz;
    long* index = new long[nnz];
    init.index = index;
    long z = 0;
    if (floats) {
      float* fval = new float[nnz]; 
      init.f_value = fval;
      for (long i=0; i<last_nz; i++) {
        if (initial[i]) {
          index[z] = i;
          fval[z] = initial[i];
          z++;
        }
      }
    } else {
      double* dval = new double[nnz];
      init.d_value = dval;
      for (long i=0; i<last_nz; i++) {
        if (initial[i]) {
          index[z] = i;
          dval[z] = initial[i];
          z++;
        }
      }
    }
  } else {
    //
    // Convert initial to truncated full
    //
#ifdef DEBUG_ELIMINATE
    cout << "  using truncated " << (floats ? "float" : "double") << " vector\n";
#endif
    init.size = last_nz;
    if (floats) {
      float* fval = new float[last_nz];
      init.f_value = fval;
      for (long i=0; i<last_nz; i++) {
        fval[i] = initial[i];
      }
    } else {
      double* dval = new double[last_nz];
      init.d_value = dval;
      for (long i=0; i<last_nz; i++) {
        dval[i] = initial[i];
      }
    }
  }


  //
  // Cleanup
  //
  delete[] initial;
}

