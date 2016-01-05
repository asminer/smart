
// $Id$

#include <stdlib.h> 
#include <stdio.h> 
#include <string.h>
#include <math.h>

#include "hyper.h"
#include "mcbase.h"

#include "intset.h" // Compact integer set library
#include "lslib.h"  // Linear Solver Library
#include "rng.h"    // RNG library

// #define DEBUG_PERIOD
// #define DEBUG_UNIF
// #define DEBUG_SSDETECT
// #define DEBUG_REDUC_STEADY
// #define DEBUG_DDIST_TTA
// #define DEBUG_CDIST_TTA
// #define DEBUG_STEP

// ******************************************************************
// *                       Macros and such                          *
// ******************************************************************

/// SWAP "macro".
template <class T> inline void SWAP(T &x, T &y) { T tmp=x; x=y; y=tmp; }

/// Standard MAX "macro".
template <class T> inline T MAX(T X,T Y) { return ((X>Y)?X:Y); }

/// Standard MIN "macro".
template <class T> inline T MIN(T X,T Y) { return ((X<Y)?X:Y); }

inline void ShowVector(double* p, long size)
{
  printf("[%f", p[0]);
  for (long s = 1; s<size; s++)
    printf(", %f", p[s]);
  printf("]");
}

#ifdef DEBUG_UNIF
inline void ShowUnifStep(int steps, double poiss, double* p, double* a, long size)
{
  printf("After %d steps, dtmc distribution is:\n\t", steps);
  ShowVector(p, size);
  printf("\n\tPoisson: %g\n", poiss);
  printf("accumulator so far:\n\t");
  ShowVector(a, size);
  printf("\n");
}
#endif


// ******************************************************************
// *                                                                *
// *                     Basic transient stuff                      *
// *                                                                *
// ******************************************************************

namespace MCLib {

template <class MATRIX>
void forwStep(
  const MATRIX &Qtt, const hypersparse_matrix* Qta,
  double q, double* p, double* aux, bool normalize) 
{
#ifdef DEBUG_STEP
    printf("Forward step start  [%lf", p[0]);
    for (long i=1; i<Qtt.size; i++) {
      printf(", %lf", p[i]);
    }
    printf("]\n");
#endif
    // vector-matrix multiply
    for (long s=Qtt.size-1; s>=0; s--) aux[s] = 0.0;
    Qtt.MatrixVectorMultiply(aux, p);   // Qtt is transposed
    if (Qta) Qta->VectorMatrixMultiply(aux, p);

    // adjust for diagonals
    for (long s=Qtt.stop-1; s>=0; s--) {
      double d = q - 1.0/Qtt.one_over_diag[s];
      aux[s] += d*p[s];
    }

    // finally, adjust for absorbing states
    for (long s=Qtt.stop; s<Qtt.size; s++) {
      aux[s] += q*p[s];
    }

    if (normalize) {
      // normalize (also handles dividing by q)
      double total = 0.0;
      for (long s=Qtt.size-1; s>=0; s--)   total += aux[s];
      for (long s=Qtt.size-1; s>=0; s--)  aux[s] /= total;
    } else {
      // divide by q
      for (long s=Qtt.size-1; s>=0; s--)  aux[s] /= q;
    }

#ifdef DEBUG_STEP
    printf("Forward step finish [%lf", aux[0]);
    for (long i=1; i<Qtt.size; i++) {
      printf(", %lf", aux[i]);
    }
    printf("]\n");
#endif
}


template <class MATRIX>
void backStep(
  const MATRIX &Qtt, const hypersparse_matrix* Qta,
  double q, double* p, double* aux, bool) 
{
#ifdef DEBUG_STEP
    printf("Backward step start  [%lf", p[0]);
    for (long i=1; i<Qtt.size; i++) {
      printf(", %lf", p[i]);
    }
    printf("]\n");
#endif
    // matrix-vector multiply
    for (long s=Qtt.size-1; s>=0; s--) aux[s] = 0.0;
    Qtt.VectorMatrixMultiply(aux, p); // Qtt is transposed
#ifdef DEBUG_STEP
    printf("Backward step 1      [%lf", aux[0]);
    for (long i=1; i<Qtt.size; i++) {
      printf(", %lf", aux[i]);
    }
    printf("]\n");
#endif
    if (Qta) Qta->MatrixVectorMultiply(aux, p);
#ifdef DEBUG_STEP
    printf("Backward step 2      [%lf", aux[0]);
    for (long i=1; i<Qtt.size; i++) {
      printf(", %lf", aux[i]);
    }
    printf("]\n");
#endif

    // adjust for diagonals
    for (long s=Qtt.stop-1; s>=0; s--) {
      double d = q - 1.0/Qtt.one_over_diag[s];
      aux[s] += d*p[s];
    }

    // finally, adjust for absorbing states
    for (long s=Qtt.stop; s<Qtt.size; s++) {
      aux[s] += q*p[s];
    }

    // divide by q
    if (q != 1) for (long s=Qtt.size-1; s>=0; s--)  aux[s] /= q;

#ifdef DEBUG_STEP
    printf("Backward step finish [%lf", aux[0]);
    for (long i=1; i<Qtt.size; i++) {
      printf(", %lf", aux[i]);
    }
    printf("]\n");
#endif
}

template <class MATRIX>
int stepGeneric(
  const MATRIX &Qtt, const hypersparse_matrix* Qta,
  int n, double q, double* p, double* aux, double delta,
  void (*step)(const MATRIX&, const hypersparse_matrix*, double, double*, double*, bool)
) 
{
  double* myp = p;
  int i;
  for (i=0; i<n; i++) {
    step(Qtt, Qta, q, myp, aux, true);
    SWAP(aux, myp);
    if (delta <= 0) continue;
    // check for convergence
    double maxdelta = 0.0;
    for (long s=Qtt.size-1; s>=0; s--) {
      double d = aux[s] - myp[s];
      if (d < 0)   d = -d;
      if (aux[s])  d /= aux[s];
      maxdelta = MAX(maxdelta, d);
    }
#ifdef DEBUG_SSDETECT
    printf("Old vector: ");
    ShowVector(aux, Qtt.size);
    printf("\nNew vector: ");
    ShowVector(myp, Qtt.size);
    printf("\nmax delta: %g\n", maxdelta);
#endif
    if (maxdelta < delta) break;
  } // for n
  if (p != myp) memcpy(p, myp, Qtt.size * sizeof(double));
 
  return i;
}

template <class MATRIX>
void genericTransientDisc(
  const MATRIX &Qtt, const hypersparse_matrix* Qta,
  int t, double* p, Markov_chain::transopts &opts,
  void (*step)(const MATRIX&, const hypersparse_matrix*, double, double*, double*, bool)
) 
{
  if (0==p) throw MCLib::error(MCLib::error::Null_Vector);
  if (t<0)  throw MCLib::error(MCLib::error::Bad_Time);

  if (0==opts.vm_result) {
    opts.vm_result = (double*) malloc(Qtt.size * sizeof(double));
  }

  if (0==opts.vm_result) {
    throw MCLib::error(MCLib::error::Out_Of_Memory);
  } 

  opts.Steps = stepGeneric(Qtt, Qta, t, 1.0, p, opts.vm_result, opts.ssprec, step);

  if (opts.kill_aux_vectors) {
    free(opts.vm_result);
    free(opts.accumulator);
    opts.vm_result = 0;
    opts.accumulator = 0;
  }
  opts.Left = 0;
  opts.Right = 0;
}

template <class MATRIX>
void genericTransientCont(
  const MATRIX &Qtt, const hypersparse_matrix* Qta,
  double t, double* p, Markov_chain::transopts &opts,
  void (*step)(const MATRIX&, const hypersparse_matrix*, double, double*, double*, bool)
) 
{
  if (0==p) throw MCLib::error(MCLib::error::Null_Vector);
  if (t<0)  throw MCLib::error(MCLib::error::Bad_Time);

#ifdef DEBUG_UNIF
  printf("Starting uniformization, q=%f, t=%f\n", opts.q, t);
#endif
  int L;
  int R;
  double* poisson = MCLib::computePoissonPDF(opts.q*t, opts.epsilon, L, R);
#ifdef DEBUG_UNIF
  printf("Computed poisson with epsilon=%e; got left=%d, right=%d\n", opts.epsilon, L, R);
#endif

  if (0==opts.vm_result)
    opts.vm_result = (double*) malloc(Qtt.size * sizeof(double));

  if (0==opts.accumulator)
    opts.accumulator = (double*) malloc(Qtt.size * sizeof(double));

  if (0==opts.vm_result || 0==opts.accumulator) {
    free(poisson);
    throw MCLib::error(MCLib::error::Out_Of_Memory);
  } else {
      opts.Steps = stepGeneric(Qtt, Qta, L, opts.q, p, opts.vm_result, opts.ssprec, step);
      if (opts.Steps == L) {
        // steady state not reached, and not an error; continue.
        for (long s=Qtt.size-1; s>=0; s--) {
          opts.accumulator[s] = p[s] * poisson[0];
        } // for s
#ifdef DEBUG_UNIF
        ShowUnifStep(L, poisson[0], p, opts.accumulator, Qtt.size);
#endif
        int i;
        for (i=1; i<=R-L; i++) {
          if (0==stepGeneric(Qtt, Qta, 1, opts.q, p, opts.vm_result, opts.ssprec, step)) 
            break;
          opts.Steps++;
          for (long s=Qtt.size-1; s>=0; s--) {
            opts.accumulator[s] += p[s] * poisson[i];
          } // for s
#ifdef DEBUG_UNIF
          ShowUnifStep(L+i, poisson[i], p, opts.accumulator, Qtt.size);
#endif
        } // for i 
        // If steady state was reached, add the possion tail
        double tail = 0.0;
        int j = R-L;
        while (i <= j) {
          if (poisson[i] < poisson[j]) {
            tail += poisson[i];
            i++;
          } else {
            tail += poisson[j];
            j--;
          }
        }
        if (tail) {
          for (long s=Qtt.size-1; s>=0; s--) {
            opts.accumulator[s] += p[s] * tail;
          } // for s
        }
      } else {
        // DTMC gets to steady state before L;
        // final vector should equal the DTMC probability vector
        for (long s=Qtt.size-1; s>=0; s--) {
          opts.accumulator[s] = p[s];
        }
      }
  }
  memcpy(p, opts.accumulator, Qtt.size * sizeof(double));

  if (opts.kill_aux_vectors) {
    free(opts.vm_result);
    free(opts.accumulator);
    opts.vm_result = 0;
    opts.accumulator = 0;
  }
  opts.Left = L;
  opts.Right = R;
  free(poisson);
}


template <class MATRIX>
void accumulateDisc(
  const MATRIX &Qtt, const hypersparse_matrix* Qta,
  double t, const double* p0, double* n0t, Markov_chain::transopts &opts)
{
  if (p0) memcpy(opts.accumulator, p0, Qtt.size * sizeof(double));
  else    memcpy(opts.accumulator, n0t, Qtt.size * sizeof(double));
  for (long s=Qtt.size-1; s>=0; s--) n0t[s] = 0.0;

  for (; t>=1.0; t-=1.0) {
    for (long s=0; s<Qtt.size; s++) n0t[s] += opts.accumulator[s];
    forwStep(Qtt, Qta, 1.0, opts.accumulator, opts.vm_result, true);
    SWAP(opts.accumulator, opts.vm_result);
    opts.Steps++;
  }
  for (long s=0; s<Qtt.size; s++) n0t[s] += t*opts.accumulator[s];
}


template <class MATRIX>
void accumulateCont(
  const MATRIX &Qtt, const hypersparse_matrix* Qta,
  double t, const double* p0, double* n0t, Markov_chain::transopts &opts)
{
#ifdef DEBUG_UNIF
  printf("Starting uniformization, q=%f, t=%f\n", opts.q, t);
#endif
  int L;
  int R;
  double* poisson = MCLib::computePoissonPDF(opts.q*t, opts.epsilon, L, R);
#ifdef DEBUG_UNIF
  printf("Computed poisson with epsilon=%e; got left=%d, right=%d\n", opts.epsilon, L, R);
#endif
  if ((0==poisson) && (R-L>=0)) {
    throw MCLib::error(MCLib::error::Out_Of_Memory);
  }
  // convert from prob(Y=L+i) to prob(Y>L+i)
  for (int y=0; y<R-L; y++) poisson[y] = poisson[y+1];
  poisson[R-L] = 0.0;
  for (int y=R-L-1; y>=0; y--) poisson[y] += poisson[y+1];

  if (p0) memcpy(opts.accumulator, p0, Qtt.size * sizeof(double));
  else    memcpy(opts.accumulator, n0t, Qtt.size * sizeof(double));
  for (long s=0; s<Qtt.size; s++) n0t[s] = 0;
  // before L: prob(Y>i) is effectively 1
  double adj = 1.0 / opts.q;
  for (int i=0; i<L; i++) {
    opts.Steps++;
    for (long s=0; s<Qtt.size; s++) n0t[s] += adj * opts.accumulator[s];
    forwStep(Qtt, Qta, opts.q, opts.accumulator, opts.vm_result, true);
    SWAP(opts.accumulator, opts.vm_result);
  } // for i
  for (int i=0; ; i++) {
    opts.Steps++;
    adj = poisson[i] / opts.q;
    for (long s=0; s<Qtt.size; s++) n0t[s] += adj * opts.accumulator[s];
    if (i>=R-L) break;
    forwStep(Qtt, Qta, opts.q, opts.accumulator, opts.vm_result, true);
    SWAP(opts.accumulator, opts.vm_result);
  } // for i
  
  // cleanup
  free(poisson);
}
  

// ******************************************************************
// *                                                                *
// *                     For period computation                     *
// *                                                                *
// ******************************************************************

/// Euclid's gcd algorithm
inline long GCD(long a, long b)
{
  long r = a % b;
  while (r > 0) {
    a = b;
    b = r;
    r = a % b;
  }
  return b;
}


/*
  Algorithm adapted from discussions in chapter 7 of Stewart.  Idea:
    (1) Select a starting state (for us, state 0)
    (2) For each state, determine its distance from the start.
    (3) If there is an edge from i to j, with
            distance(i) >= distance(j),
        then add  
            distance(i) - distance(j) + 1
        to a set of integers.
    (4) The GCD of the resulting set gives the period.  
        Since GCD is associative, we don't need to construct
        the set of integers, just remember the GCD "so far".

  Returns -2 for out of memory error.
*/
long FindPeriod(const GraphLib::generic_graph::matrix &Q, long Qstart, long Qstop)
{
  if (Qstart == Qstop)  return 0;
#ifdef DEBUG_PERIOD
  printf("Computing period for MC\n");
  printf("Computing distances from state 0\n");
#endif
  long* dist = (long*) malloc((Qstop - Qstart) * sizeof(long));
  if (0==dist)  return -2;
  long* queue = (long*) malloc((Qstop - Qstart) * sizeof(long));
  if (0==queue) {
    free(dist);
    return -2;
  }
  long i, a;
  for (i=Qstop - Qstart - 1; i; i--) dist[i] = -1;
  dist[0] = 0;
  queue[0] = Qstart;
  long head = 1;
  long tail = 0;
  while (tail < head) {
    long i = queue[tail];
    tail++;
    for (a=Q.rowptr[i]; a<Q.rowptr[i+1]; a++) {
      long j = Q.colindex[a];
      long jmst = j - Qstart;
      if (dist[jmst]>=0) continue;
      dist[jmst] = dist[i-Qstart]+1;
      queue[head] = j;
      head++;
    } // for a
  } // while more to explore
  free(queue);
#ifdef DEBUG_PERIOD
  printf("Distances: [%d", dist[0]);
  for (i=1; i<Qstop-Qstart; i++) printf(", %d", dist[i]);
  printf("]\n");
#endif
  long period = 0;
  a = Q.rowptr[Qstart];
  for (i=Qstart; i<Qstop; i++) for (; a<Q.rowptr[i+1]; a++) {
    long imst = i-Qstart;
    long j = Q.colindex[a] - Qstart;
    if (dist[imst] < dist[j]) continue;
    long d = dist[imst] - dist[j] + 1;
#ifdef DEBUG_PERIOD
    printf("\tadding to GCD set: %d\n", d);
#endif
    if (period)  period = GCD(period, d);
    else  period = d;
    if (1==period) break;  // no point to continue, will be 1
  } // for a
  free(dist);
#ifdef DEBUG_PERIOD
  printf("Got period: %d\n", period);
#endif
  return period;
}


} // namespace


// ******************************************************************
// *                                                                *
// *                     row_normalizer methods                     *
// *                                                                *
// ******************************************************************

row_normalizer::row_normalizer(const double* rs)
 : GraphLib::generic_graph::element_visitor()
{
  rowsums = rs;
}

bool row_normalizer::visit(long from, long to, void* wt) 
{
  float* v = (float*) wt;
  if (rowsums[from]) v[0] /= rowsums[from];
  return false;
}

// ******************************************************************
// *                                                                *
// *                        mc_base  methods                        *
// *                                                                *
// ******************************************************************

mc_base::mc_base(bool disc, long gs, long ge) : Markov_chain(disc)
{
  stop_index = 0;
  period = 0;

  // allocate row sums if necessary
  if (disc) {
    long pages = (gs / MAX_NODE_ADD);
    if (pages < 1) {
        rowsums_alloc = 4;
        while (rowsums_alloc < gs) rowsums_alloc *= 2;
    } else {
        rowsums_alloc = (1+pages) * MAX_NODE_ADD;
    }
    rowsums = (double*) malloc(rowsums_alloc * sizeof(double));
    for (long i=0; i<gs; i++) rowsums[i] = 0.0;
  } else {
    rowsums = 0;
    rowsums_alloc = 0;
  }

  // start graphs
  g = new GraphLib::merged_weighted_digraph <float> (false);
  g->addNodes(gs);
  h = 0;

  // Diagonal stuff
  oneoverd = 0;
  ood_size = 0;
  maxdiag = 0.0;

  // rawQ
  rawQ.rowptr = 0;
  rawQ.colindex = 0;
  rawQ.value = 0;
}

mc_base::~mc_base()
{
  free(rowsums);
  free(oneoverd);
  free(stop_index);
  free(period);
  delete g;
  delete h;

  // Don't delete rawQ those were const copies
}

long mc_base::getNumArcs() const
{
  long arcs = g ? g->getNumEdges() : 0;
  arcs += h ? h->NumEdges() : 0;
  return arcs;
}

bool mc_base::isEfficientByRows() const
{
  return g->isByRows();
}

void mc_base::transpose()
{
  try {
    g->transpose(0);
  }
  catch (GraphLib::error e) {
    throw MCLib::error(e);
  }
}

void mc_base::traverseFrom(long i, GraphLib::generic_graph::element_visitor &x)
{
  if (g->traverseFrom(i, x)) return;
  if (h) h->traverseRow(i, x);
  if (!isDiscrete()) return;
  if (isAbsorbingState(i)) {
    float f = 1.0;
    x.visit(i, i, &f);
  } else if (oneoverd[i] > 1) {
    float f = 1.0 - 1.0/oneoverd[i];
    x.visit(i, i, &f);
  }
}

void mc_base::traverseTo(long i, GraphLib::generic_graph::element_visitor &x)
{
  if (h) if (h->traverseCol(i, x)) return;
  g->traverseTo(i, x);
  if (!isDiscrete()) return;
  if (isAbsorbingState(i)) {
    float f = 1.0;
    x.visit(i, i, &f);
  } else if (oneoverd[i] > 1) {
    float f = 1.0 - 1.0/oneoverd[i];
    x.visit(i, i, &f);
  }
}

void mc_base::traverseEdges(GraphLib::generic_graph::element_visitor &x)
{
  if (g->traverseAll(x)) return;
  if (h) h->traverseAll(x);
  if (!isDiscrete()) return;
  float f;
  long i;
  long fa = getFirstAbsorbing();
  for (i=0; i<fa; i++) if (oneoverd[i] > 1) {
    f = 1.0 - 1.0/oneoverd[i];
    x.visit(i, i, &f);
  }
  long ns = getNumStates();
  f = 1.0;
  for ( ; i<ns; i++) if (x.visit(i, i, &f)) return;
}

bool mc_base::getForward(const intset& x, intset &y) const
{
  bool ans = g->getForward(x, y);
  bool hans = h ? h->getForward(x, y) : false;
  ans |= hans;
  if (!isDiscrete()) return ans;
  for (long i=getFirstTransient(); i<getFirstAbsorbing(); i++) {
    if (oneoverd[i] <= 1) continue;
    if (!x.contains(i)) continue;
    ans |= !y.testAndAdd(i);
  }
  for (long i=getFirstAbsorbing(); i<getNumStates(); i++) {
    if (!x.contains(i)) continue;
    ans |= !y.testAndAdd(i);
  }
  return ans;
}

bool mc_base::getBackward(const intset& x, intset &y) const
{
  bool ans = g->getBackward(x, y);
  bool hans = h ? h->getBackward(x, y) : false;
  ans |= hans;
  if (!isDiscrete()) return ans;
  for (long i=getFirstTransient(); i<getFirstAbsorbing(); i++) {
    if (oneoverd[i] <= 1) continue;
    if (!x.contains(i)) continue;
    ans |= !y.testAndAdd(i);
  }
  for (long i=getFirstAbsorbing(); i<getNumStates(); i++) {
    if (!x.contains(i)) continue;
    ans |= !y.testAndAdd(i);
  }
  return ans;
}


long mc_base::getFirstTransient() const
{
  return 0;
}

long mc_base::getNumTransient() const
{
  if (stop_index)  return stop_index[0];
  return 0;
}

long mc_base::getFirstAbsorbing() const
{
  if (stop_index)  return stop_index[num_classes];
  return -1;
}

long mc_base::getNumAbsorbing() const
{
  if (stop_index)  return num_states - stop_index[num_classes];
  return 0;
}

long mc_base::getFirstRecurrent(long c) const
{
  if (c<1)      return 0;
  if (finished) return stop_index[c-1];
  return -1;
}

long mc_base::getRecurrentSize(long c) const
{
  if (!finished)  return 0;
  if (c>0)        return stop_index[c] - stop_index[c-1];
  if (0==c)       return stop_index[0];
  return 0;
}

bool mc_base::isAbsorbingState(long s) const
{
  if (!finished)  return false;
  return s >= stop_index[num_classes];
}

bool mc_base::isTransientState(long s) const
{
  if (!finished)  return false;
  return s < stop_index[0];
}

long mc_base::getClassOfState(long s) const
{
  if (!finished)  return -1;  // unfinished, or error

  // Check transient
  if (s < stop_index[0]) 
  return 0;
  // Check absorbing
  if (s >= stop_index[num_classes])
  return stop_index[num_classes] - (s+1);

  // find c such that stop_index[c-1] <= s < stop_index[c]
  long high = num_classes+1;
  long low = 0;
  while (high>low) {
    long mid = (high+low)/2;
    if (s==stop_index[mid])   return 1+mid;
    if (s < stop_index[mid])  high = mid;
    else      low = mid+1;
  }
  return high;
}

bool mc_base::isStateInClass(long s, long c) const
{
  if (!finished)  return false;

  // Check absorbing
  if (s >= stop_index[num_classes])
  return stop_index[num_classes] - (s+1) == c;

  // bogus states
  if (s<0) return false;
  if (s>=num_states) return false;

  // All others: we must have
  // stop_index[c-1] <= s < stop_index[c]

  if (c<0) return false;
  if (c>num_classes) return false;
  if (s >= stop_index[c]) return false;
  if (0==c) return true;
  return (s >= stop_index[c-1]);
}

void mc_base::computePeriodOfClass(long c)
{
  if (!finished)  
    throw MCLib::error(MCLib::error::Finished_Mismatch);

  if ((c<0) || (c>num_classes))
    throw MCLib::error(MCLib::error::Bad_Class);

  if (Error_type == our_type)
    throw MCLib::error(MCLib::error::Miscellaneous);

  // compute away
  if (0==period) {
    period = (long*) malloc((num_classes+1)*sizeof(long));
    if (0==period) throw MCLib::error(MCLib::error::Out_Of_Memory);
    for (long i=1; i<= num_classes; i++) period[i] = -1;
    period[0] = 0;
  }
  if (period[c]<0) {
    period[c] = MCLib::FindPeriod(rawQ, c ? stop_index[c-1] : 0, stop_index[c]);
  }
}

long mc_base::getPeriodOfClass(long c) const
{
  if (period)  return period[c];
  throw MCLib::error(MCLib::error::Miscellaneous);
}

double mc_base::getUniformizationConst() const
{
  return maxdiag;
}

void mc_base::computeTransient(double t, double* p, transopts &opts) const
{
  if (isDiscrete()) {
    return computeTransient(int(t), p, opts);
  }
  opts.q = MAX(opts.q, getUniformizationConst());
  if (rawQ.is_transposed) { 
    LS_CRS_Matrix_float QT;
    exportQT(QT);
    useAllButAbsorbing(QT);
    MCLib::genericTransientCont(QT, h, t, p, opts, MCLib::forwStep);
  } else {
    LS_CCS_Matrix_float QT;
    exportQT(QT);
    useAllButAbsorbing(QT);
    MCLib::genericTransientCont(QT, h, t, p, opts, MCLib::forwStep);
  }
}

void mc_base::computeTransient(int t, double* p, transopts &opts) const
{
  if (isContinuous()) {
    return computeTransient(double(t), p, opts);
  }

  if (rawQ.is_transposed) { 
    LS_CRS_Matrix_float QT;
    exportQT(QT);
    useAllButAbsorbing(QT);
    MCLib::genericTransientDisc(QT, h, t, p, opts, MCLib::forwStep);
  } else {
    LS_CCS_Matrix_float QT;
    exportQT(QT);
    useAllButAbsorbing(QT);
    MCLib::genericTransientDisc(QT, h, t, p, opts, MCLib::forwStep);
  }
}

void mc_base::reverseTransient(double t, double* p, transopts &opts) const
{
  if (isDiscrete()) {
    return reverseTransient(int(t), p, opts);
  }
  opts.q = MAX(opts.q, getUniformizationConst());
  if (rawQ.is_transposed) { 
    LS_CRS_Matrix_float QT;
    exportQT(QT);
    useAllButAbsorbing(QT);
    MCLib::genericTransientCont(QT, h, t, p, opts, MCLib::backStep);
  } else {
    LS_CCS_Matrix_float QT;
    exportQT(QT);
    useAllButAbsorbing(QT);
    MCLib::genericTransientCont(QT, h, t, p, opts, MCLib::backStep);
  }
}

void mc_base::reverseTransient(int t, double* p, transopts &opts) const
{
  if (isContinuous()) {
    return reverseTransient(double(t), p, opts);
  }
  if (rawQ.is_transposed) { 
    LS_CRS_Matrix_float QT;
    exportQT(QT);
    useAllButAbsorbing(QT);
    MCLib::genericTransientDisc(QT, h, t, p, opts, MCLib::backStep);
  } else {
    LS_CCS_Matrix_float QT;
    exportQT(QT);
    useAllButAbsorbing(QT);
    MCLib::genericTransientDisc(QT, h, t, p, opts, MCLib::backStep);
  }
}

void mc_base
::accumulate(double t, const double* p0, double* n0t, transopts &opts) const
{
  if (0==n0t) throw MCLib::error(MCLib::error::Null_Vector);
  if (t<0)    throw MCLib::error(MCLib::error::Bad_Time);
  if (0==t) {
    for (long s=0; s<num_states; s++) n0t[s] = 0;
    return;
  }

  // allocate auxiliary vectors, if necessary
  if (0==opts.vm_result) {
    opts.vm_result = (double*) malloc(num_states * sizeof(double));
  }
  if (0==opts.accumulator) {
    opts.accumulator = (double*) malloc(num_states * sizeof(double));
  }

  if (0==opts.vm_result || 0==opts.accumulator)  {
    throw MCLib::error(MCLib::error::Out_Of_Memory);
  } 

  if (isDiscrete()) {
    accDTMC(t, p0, n0t, opts);
  } else {
    accCTMC(t, p0, n0t, opts);
  }

  // kill auxiliary vectors, if requested
  if (opts.kill_aux_vectors) {
    free(opts.vm_result);
    free(opts.accumulator);
    opts.vm_result = 0;
    opts.accumulator = 0;
  }
}


void 
mc_base::accDTMC(double t, const double* p0, double* n0t, transopts &opts) const
{
  if (rawQ.is_transposed) { 
    LS_CRS_Matrix_float QT;
    exportQT(QT);
    useAllButAbsorbing(QT);
    MCLib::accumulateDisc(QT, h, t, p0, n0t, opts);
  } else {
    LS_CCS_Matrix_float QT;
    exportQT(QT);
    useAllButAbsorbing(QT);
    MCLib::accumulateDisc(QT, h, t, p0, n0t, opts);
  }
}

void
mc_base::accCTMC(double t, const double* p0, double* n0t, transopts &opts) const
{
  opts.q = MAX(opts.q, getUniformizationConst());
  if (rawQ.is_transposed) { 
    LS_CRS_Matrix_float QT;
    exportQT(QT);
    useAllButAbsorbing(QT);
    MCLib::accumulateCont(QT, h, t, p0, n0t, opts);
  } else {
    LS_CCS_Matrix_float QT;
    exportQT(QT);
    useAllButAbsorbing(QT);
    MCLib::accumulateCont(QT, h, t, p0, n0t, opts);
  }
}


void mc_base::computeSteady(const LS_Vector &p0, 
      double* p, 
      const LS_Options &opt, 
      LS_Output &out) const
{
  if (0==p) {
    throw MCLib::error(MCLib::error::Null_Vector);
  }
  if (0==p0.size) if (getNumClasses() + getNumAbsorbing() > 1) {
    throw MCLib::error(MCLib::error::Null_Vector);
  }
  switch (our_type) {
    case Irreducible:
        irredSteady(p, opt, out);
        break;

    case Absorbing:
    case Reducible:
        reducSteady(p0, p, opt, out);
        break;
 
    case Unknown:
        throw MCLib::error(MCLib::error::Finished_Mismatch);
  
    default:
        throw MCLib::error(MCLib::error::Miscellaneous);
  };
}

void mc_base::computeTTA(const LS_Vector &p0, 
      double* p, 
      const LS_Options &opt, 
      LS_Output &out) const
{
  if (0==p) {
    throw MCLib::error(MCLib::error::Null_Vector);
  }
  if (0==p0.size && getNumTransient() > 0) {
    throw MCLib::error(MCLib::error::Null_Vector);
  }
  switch (our_type) {
    case Irreducible:
        out.status = LS_Success;
        out.num_iters = 0;
        out.relaxation = 0;
        out.precision = 0;
        return;

    case Absorbing:
    case Reducible:
        reducTTA(p0, p, opt, out);
        break;
 
    case Unknown:
        throw MCLib::error(MCLib::error::Finished_Mismatch);
  
    default:
        throw MCLib::error(MCLib::error::Miscellaneous);
  };
}


void mc_base::computeClassProbs(const LS_Vector &p0, double* nc, 
                                const LS_Options &opt, LS_Output &out) const
{
  if (0==nc) {
    throw MCLib::error(MCLib::error::Null_Vector);
  }
  if (0==p0.size && stop_index[0] > 0) {
    throw MCLib::error(MCLib::error::Null_Vector);
  }
  // trivial case: irreducible chain, one class
  if (Irreducible == our_type) {
    for (long i=0; i<num_states; i++) nc[i] = 1.0;
    out.status = LS_Success;
    out.num_iters = 0;
    out.relaxation = 0;
    out.precision = 0;
    return;
  }
  if (Unknown == our_type) {
    throw MCLib::error(MCLib::error::Finished_Mismatch);
  }

  if (stop_index[0]>0) {  

    // compute n vector
    reducTTA(p0, nc, opt, out);

    // compute c vector
    for (long i=stop_index[0]; i<num_states; i++) nc[i] = 0;

    // Get absorption probabilities
    if (h) h->VectorMatrixMultiply(nc, nc);

  } else {
    
    // No transient states - don't bother!
    for (long i=0; i<num_states; i++) nc[i] = 0;
    out.status = LS_Success;

  }


  // Add initial absorbing probabilities
  if (p0.index) {
      // Sparse storage
      if (p0.d_value) {
        for (long z=0; z<p0.size; z++)
          if (p0.index[z] >= stop_index[0])
            nc[p0.index[z]] += p0.d_value[z];
      } else {
        for (long z=0; z<p0.size; z++)
          if (p0.index[z] >= stop_index[0])
            nc[p0.index[z]] += p0.f_value[z];
      }
  } else {
      // Full storage 
      if (p0.d_value) 
        for (long i=stop_index[0]; i<p0.size; i++)
          nc[i] += p0.d_value[i];
      else 
        for (long i=stop_index[0]; i<p0.size; i++)
          nc[i] += p0.f_value[i];
  }

  // collect probs for recurrent classes into their first states
  double total = 0.0;
  for (long c=1; c<=num_classes; c++) {
    long firstone = stop_index[c-1];
    for (long i=firstone+1; i<stop_index[c]; i++) nc[firstone] += nc[i];
    total += nc[firstone];
  } // for c
  for (long a=getFirstAbsorbing(); a<num_states; a++) total += nc[a];
  // normalize and fill
  for (long c=1; c<=num_classes; c++) {
    long firstone = stop_index[c-1];
    nc[firstone] /= total;
    for (long i=firstone+1; i<stop_index[c]; i++) nc[i] = nc[firstone];
  } // for c
  for (long a=getFirstAbsorbing(); a<num_states; a++) nc[a] /= total;
}

void
mc_base::internalDiscreteDistTTA(const LS_Vector &p0, extra_distopts &opts, int c) const
{
  if (!finished) {
    throw MCLib::error(MCLib::error::Finished_Mismatch);
  }
  if (0==c || c>num_classes || c<-getNumAbsorbing()) {
    throw MCLib::error(MCLib::error::Bad_Class);
  }

  /* Initialize vectors if necessary */
  if (0==opts.probvect) {
    opts.probvect = new double[num_states];
  }
  if (0==opts.vm_result) {
    opts.vm_result = new double[num_states];
  }
  if (0==opts.vm_result || 0==opts.probvect) {
    throw MCLib::error(MCLib::error::Out_Of_Memory);
  } 
  fillFullVector(opts.probvect, num_states, p0);


  /* Save the state indexes for this class */
  long start_class, stop_class;
  if (c>0) {
    // recurrent class
    start_class = getFirstRecurrent(c);
    stop_class = start_class + getRecurrentSize(c);
  } else {
    // absorbing state
    start_class = stop_index[num_classes]-1-c;
    stop_class = start_class+1;
  }

#ifdef DEBUG_DDIST_TTA
  printf("Starting DiscreteDistTTA computation\n");
#endif

  for (int i=0; ;) {
#ifdef DEBUG_DDIST_TTA
    printf("Time %d\n", i);
    printf("\tVector: ");
    ShowVector(opts.probvect, num_states);
    printf("\n");
#endif
    // get the probability that we just entered class c
    double dist = 0;
    for (int s=start_class; s<stop_class; s++) {
      dist += opts.probvect[s];
    }
    if (opts.fixed_dist) {
      opts.fixed_dist[i] = dist;
    } else {
      // Expand if necessary
      if (i>=opts.var_dist_size) {
        opts.var_dist_size += 256;
        opts.var_dist = (double*) realloc(opts.var_dist, opts.var_dist_size * sizeof(double));
        if (0==opts.var_dist) throw MCLib::error(MCLib::error::Out_Of_Memory);
      }
      opts.var_dist[i] = dist;
    }

    // Update "error" part of distribution, if necessary
    if (opts.need_error) {
      // Expand if we need to
      if (i>=opts.error_dist_size) {
        opts.error_dist_size += 256;
        opts.error_dist = (double*) realloc(opts.error_dist, opts.error_dist_size * sizeof(double));
        if (0==opts.error_dist) throw MCLib::error(MCLib::error::Out_Of_Memory);
      }
      // Determine error
      opts.error_dist[i] = 0.0;
      for (int s=0; s<stop_index[0]; s++) {
        opts.error_dist[i] += opts.probvect[s];
      }
#ifdef DEBUG_DDIST_TTA
      printf("\terror[%d]: %lf\n", i, opts.error_dist[i]);
#endif
    }

    i++; 

    // Check stopping criteria
    if (opts.fixed_dist) {
      if (i >= opts.fixed_dist_size) {
        /* Compute error */
        opts.epsilon = 0.0;
        for (int s=0; s<stop_index[0]; s++) {
          opts.epsilon += opts.probvect[s];
        }
#ifdef DEBUG_DDIST_TTA
        printf("Finished, error: %lf\n", opts.epsilon);
#endif
        break;
      }
    } else {
      /* Determine "error": total mass still in transient states */
      // Did we compute this already?
      double error;
      if (opts.need_error) {
        error = opts.error_dist[i-1];
      } else {
        // Compute as much of the error as we need
        error = 0;
        for (int s=0; s<stop_index[0]; s++) {
          error += opts.probvect[s];
          if (error >= opts.epsilon) break;
        }
      }
#ifdef DEBUG_DDIST_TTA
      printf("\tChecking error %lf < %lf\n", error, opts.epsilon);
#endif
      if (error < opts.epsilon) { // done!
        if (i < opts.var_dist_size) {
          // Shrink the distribution array
          opts.var_dist = (double*) realloc(opts.var_dist, i*sizeof(double));
          opts.var_dist_size = i;
        }
        if (i < opts.error_dist_size) {
          // Shrink the error array
          opts.error_dist = (double*) realloc(opts.error_dist, i*sizeof(double));
          opts.error_dist_size = i;
        }
        break; 
      }
    }

    // Build the next vector
    //  (a) zero the non-transient probs
    for (int s=stop_index[0]; s<num_states; s++) {
      opts.probvect[s] = 0.0;
    }
#ifdef DEBUG_DDIST_TTA
    printf("\tmodif : ");
    ShowVector(opts.probvect, num_states);
    printf("\n");
#endif
    //  (b) advance
    if (rawQ.is_transposed) {
      LS_CRS_Matrix_float QT;
      exportQT(QT);
      useAllButAbsorbing(QT);
      MCLib::forwStep(QT, h, opts.q, opts.probvect, opts.vm_result, false);
    } else {
      LS_CCS_Matrix_float QT;
      exportQT(QT);
      useAllButAbsorbing(QT);
      MCLib::forwStep(QT, h, opts.q, opts.probvect, opts.vm_result, false);
    }
    SWAP(opts.probvect, opts.vm_result);
  }

  /* Cleanup vectors as necessary */
  if (opts.kill_aux_vectors) {
    delete[] opts.probvect;
    delete[] opts.vm_result;
    opts.probvect = 0;
    opts.vm_result = 0;
  } 
}

void 
mc_base::computeDiscreteDistTTA(const LS_Vector &p0, distopts &opts, int c, 
            double epsilon, double* &dist, int &N) const
{
  extra_distopts eo(opts);
  eo.setVariable(epsilon);
  eo.q = 1;
  internalDiscreteDistTTA(p0, eo, c);
  dist = eo.var_dist;
  N = eo.var_dist_size;
}

double
mc_base::computeDiscreteDistTTA(const LS_Vector &p0, distopts &opts, int c, 
            double dist[], int N) const
{
  extra_distopts eo(opts);
  eo.setFixed(dist, N);
  eo.q = 1;
  internalDiscreteDistTTA(p0, eo, c);
  return eo.epsilon;
}


void 
mc_base::computeContinuousDistTTA(const LS_Vector &p0, distopts &opts, int c, 
            double dt, double epsilon, double* &dist, int &N) const
{
#ifdef DEBUG_CDIST_TTA
  printf("Inside computeContinuousDistTTA\n");
#endif
  // Initialize
  dist = 0;
  N = 0;

  /*
    Build discrete (uniformized) distribution
  */
  extra_distopts eo(opts);
  eo.setVariable(epsilon);
  eo.q = MAX(eo.q, getUniformizationConst());
  eo.need_error = true;
  internalDiscreteDistTTA(p0, eo, c);
#ifdef DEBUG_CDIST_TTA
  printf("Using q=%lf\n", eo.q);
  printf("Got discrete dist: ");
  ShowVector(eo.var_dist, eo.var_dist_size);
  printf("\nGot error dist: ");
  ShowVector(eo.error_dist, eo.error_dist_size);
  printf("\n");
#endif

  /*
    Loop over continuous times
  */
  for (int i=1; ; i++) {
    
    // Do we need to enlarge the distribution?
    if (i >= N) {
      N += 256;
      dist = (double*) realloc(dist, N*sizeof(double));
      if (0==dist) throw MCLib::error(MCLib::error::Out_Of_Memory);
    }

    // Determine poisson distribution
    double t = i*dt;
    double qt = t * eo.q;
#ifdef DEBUG_CDIST_TTA
    printf("TIME %lf\n", t);
#endif
    int L;
    int R;
    double* poisson = MCLib::computePoissonPDF(qt, epsilon, L, R);
    const double* poissML = poisson - L;
#ifdef DEBUG_CDIST_TTA
    printf("    Computed poisson with qt=%lf; got left=%d, right=%d\n", qt, L, R);
#endif

    // this data point is q * sum_n (poisson[n] * ddist[n+1])
    // DDist * poisson = this data point
    dist[i] = 0;
    for (int s=L; s<R; s++) {
      if (s+1>=eo.var_dist_size) break;
      dist[i] += poissML[s] * eo.var_dist[s+1];
    }
    dist[i] *= eo.q;

    // error * poisson = current error value
    double error = 0;
    for (int s=L; s<R; s++) {
      if (s>=eo.error_dist_size) break;
      error += eo.error_dist[s] * poissML[s];
#ifndef DEBUG_CDIST_TTA
      if (error > epsilon) break;
#endif
    }
#ifdef DEBUG_CDIST_TTA
    printf("    computed `error' %lf\n", error);
#endif
    
    // cleanup
    free(poisson);

    if (error >= epsilon) continue;

    // Shrink distribution and bail out
    dist = (double*) realloc(dist, (i+1)*sizeof(double));
    N=i+1;
    break;
  }

  if (dist) dist[0] = eo.var_dist[0];

  // cleanup
  free(eo.var_dist);
  free(eo.error_dist);
}

long
mc_base::randomWalk(rng_stream &rng, long &state, const intset* final,
                            long maxt, double q) const
{
  if (!finished) {
    throw MCLib::error(MCLib::error::Finished_Mismatch);
  }
  if (g->isByCols()) {
    throw MCLib::error(MCLib::error::Wrong_Format);
  }

  long elapsed;
  for (elapsed = 0; elapsed < maxt; elapsed++) {
    if (final && final->contains(state))  return elapsed;
    if (isAbsorbingState(state))          return elapsed;

    double u;
    if (isDiscrete())   u = rng.Uniform32();
    else                u = q * rng.Uniform32();
    double total = 0;
    if (g->findOutgoingEdge(state, state, total, u)) continue;
    if (h) h->findOutgoingEdge(state, state, total, u);
  }
  return elapsed;
}


double 
mc_base::randomWalk(rng_stream &rng, long &state, const intset* final,
                            double maxt) const
{
  if (isDiscrete()) {
    throw MCLib::error(MCLib::error::Wrong_Type);
  }
  if (!finished) {
    throw MCLib::error(MCLib::error::Finished_Mismatch);
  }
  if (g->isByCols()) {
    throw MCLib::error(MCLib::error::Wrong_Format);
  }

  double elapsed = 0;
  for (;;) {
    if (final && final->contains(state))  return elapsed;
    if (isAbsorbingState(state))          return elapsed;

    if (state >= ood_size) {
      throw MCLib::error(MCLib::error::Miscellaneous);
      // this shouldn't happen
    }

    // get time of next state change
    elapsed += -log(rng.Uniform32()) * oneoverd[state];
    if (elapsed >= maxt) {
      elapsed = maxt;
      return elapsed;
    }

    // get next state
    double u = rng.Uniform32() / oneoverd[state];
    double total = 0;
    if (g->findOutgoingEdge(state, state, total, u)) continue;
    if (h) h->findOutgoingEdge(state, state, total, u);
  } // infinite loop
}


long mc_base::ReportMemTotal() const
{
    long mem = 0;
    if (g)            mem += g->ReportMemTotal();
    if (h)            mem += h->ReportMemTotal();
    if (stop_index)   mem += (1+num_classes) * sizeof(long);
    if (period)       mem += (1+num_classes) * sizeof(long);
    if (oneoverd)     mem += ood_size * sizeof(float);
    return mem;
}



void mc_base::irredSteady(double* p, 
      const LS_Options &opt, 
      LS_Output &lo) const
{
  for (long i=num_states-1; i>=0; i--) 
  p[i] = 1;

  if (num_states > 1) {
    if (rawQ.is_transposed) {
      LS_CRS_Matrix_float QT;
      exportQT(QT);
      useClass(QT, 1);
      Solve_AxZero(QT, p, opt, lo);
    } else {
      LS_CCS_Matrix_float QT;
      exportQT(QT);
      useClass(QT, 1);
      Solve_AxZero(QT, p, opt, lo);
    }
  }
}

void mc_base::reducSteady(const LS_Vector &p0, 
    double* p, 
    const LS_Options &opt, 
    LS_Output &lo) const
{
  for (long i=num_states-1; i>=0; i--) p[i] = 0;

  if (0==num_classes && 1==getNumAbsorbing()) {
    p[getFirstAbsorbing()] = 1;
    lo.status = LS_Success;
    lo.num_iters = 0;
    lo.relaxation = 0;
    lo.precision = 0;
    return;
  }

  if (1==num_classes && 0==getNumAbsorbing()) {
    // we are absorbed into this class with probability 1
    if (stop_index)   p[stop_index[0]] = 1;
    else              p[0] = 1;
  } else {
    //
    // Solve linear system to determine (negative of) 
    // expected number of visits to each transient state
    //
    if (rawQ.is_transposed) {
      LS_CRS_Matrix_float QT;
      exportQT(QT);
      useClass(QT, 0);
      Solve_Axb(QT, p, p0, opt, lo);
    } else {
      LS_CCS_Matrix_float QT;
      exportQT(QT);
      useClass(QT, 0);
      Solve_Axb(QT, p, p0, opt, lo);
    }

    if ((LS_Success != lo.status) && (LS_No_Convergence != lo.status)) return;

    // 
    // We actually solved for the negated solution; fix that
    //
    for (long i=0; i < stop_index[0]; i++) p[i] = -p[i];

    //
    // Visits to each state * rate to recurrent classes
    // gives the absorption probabilities for each
    // recurrent class / absorbing state
    //
    if (h) h->VectorMatrixMultiply(p, p);
  
    // Add initial absorbing probabilities
    if (p0.index) {
      // Sparse storage
      if (p0.d_value) {
        for (long z=0; z<p0.size; z++)
          if (p0.index[z] >= stop_index[0])
            p[p0.index[z]] += p0.d_value[z];
      } else {
        for (long z=0; z<p0.size; z++)
          if (p0.index[z] >= stop_index[0])
            p[p0.index[z]] += p0.f_value[z];
      }
    } else {
      // Full storage 
      if (p0.d_value) 
        for (long i=stop_index[0]; i<p0.size; i++)
          p[i] += p0.d_value[i];
      else 
        for (long i=stop_index[0]; i<p0.size; i++)
          p[i] += p0.f_value[i];
    }

    // normalize probs
    double total = 0.0;
    long i;
    for (i=stop_index[0]; i<num_states; i++)  total += p[i];
    for (i=stop_index[0]; i<num_states; i++)  p[i] /= total;
  } // if more than one recurrent class

  // Now, deal with each recurrent class
  for (long c=1; c<=num_classes; c++) {
    // determine the total probability of reaching this class
    double reachprob = 0.0; 
    for (long i=stop_index[c-1]; i<stop_index[c]; i++) {
      reachprob += p[i];
    }
#ifdef DEBUG_REDUC_STEADY
    printf("Reach class %ld with prob %f\n", c, reachprob);
#endif
    if (0.0==reachprob) continue;

    //
    // probability of hitting this class is non-zero.
    // determine stationary distribution for this class
    // by solving linear system
    for (long i=stop_index[c-1]; i<stop_index[c]; i++) p[i] = 1;
    if (rawQ.is_transposed) {
      LS_CRS_Matrix_float QT;
      exportQT(QT);
      useClass(QT, c);
      Solve_AxZero(QT, p, opt, lo);
    } else {
      LS_CCS_Matrix_float QT;
      exportQT(QT);
      useClass(QT, c);
      Solve_AxZero(QT, p, opt, lo);
    }
   
#ifdef DEBUG_REDUC_STEADY
    printf("%ld iters, %f precision\n", lo.num_iters, lo.precision);
#endif
    // scale by the probability
    for (long i=stop_index[c-1]; i<stop_index[c]; i++) p[i] *= reachprob;
  } // for c

  // zero out transient part
  for (long i=0; i<stop_index[0]; i++) p[i] = 0;
}


void mc_base::reducTTA(const LS_Vector &p0, 
    double* p, 
    const LS_Options &opt, 
    LS_Output &lo) const
{
  for (long i=stop_index[0]-1; i>=0; i--) p[i] = 0;

  //
  // Solve linear system to determine (negative of) 
  // expected number of visits to each transient state
  //
  if (rawQ.is_transposed) {
    LS_CRS_Matrix_float QT;
    exportQT(QT);
    useClass(QT, 0);
    Solve_Axb(QT, p, p0, opt, lo);
  } else {
    LS_CCS_Matrix_float QT;
    exportQT(QT);
    useClass(QT, 0);
    Solve_Axb(QT, p, p0, opt, lo);
  }

  // 
  // We actually solved for the negated solution; fix that
  //
  for (long i=0; i < stop_index[0]; i++) p[i] = -p[i];
}

//
// private, generic stuff
//

#if 0

void mc_base::genericTransient(double t, double* p, transopts &opts,
  void (mc_base::* step)(const LS_Matrix&, double, double*, double*, bool) const
) const
{
  if (isDiscrete()) {
    genericTransient(int(t), p, opts, step);
    return;
  }
  if (0==p) throw MCLib::error(MCLib::error::Null_Vector);
  if (t<0)  throw MCLib::error(MCLib::error::Bad_Time);
  opts.q = MAX(opts.q, getUniformizationConst());
#ifdef DEBUG_UNIF
  printf("Starting uniformization, q=%f, t=%f\n", opts.q, t);
#endif
  int L;
  int R;
  double* poisson = MCLib::computePoissonPDF(opts.q*t, opts.epsilon, L, R);
#ifdef DEBUG_UNIF
  printf("Computed poisson with epsilon=%e; got left=%d, right=%d\n", opts.epsilon, L, R);
#endif

  if (0==opts.vm_result)
    opts.vm_result = (double*) malloc(num_states * sizeof(double));

  if (0==opts.accumulator)
    opts.accumulator = (double*) malloc(num_states * sizeof(double));

  if (0==opts.vm_result || 0==opts.accumulator) {
    free(poisson);
    throw MCLib::error(MCLib::error::Out_Of_Memory);
  } else {
      opts.Steps = stepGeneric(L, opts.q, p, opts.vm_result, opts.ssprec, step);
      if (opts.Steps == L) {
        // steady state not reached, and not an error; continue.
        for (long s=num_states-1; s>=0; s--) {
          opts.accumulator[s] = p[s] * poisson[0];
        } // for s
#ifdef DEBUG_UNIF
        ShowUnifStep(L, poisson[0], p, opts.accumulator, num_states);
#endif
        int i;
        for (i=1; i<=R-L; i++) {
          if (0==stepGeneric(1, opts.q, p, opts.vm_result, opts.ssprec, step)) 
            break;
          opts.Steps++;
          for (long s=num_states-1; s>=0; s--) {
            opts.accumulator[s] += p[s] * poisson[i];
          } // for s
#ifdef DEBUG_UNIF
          ShowUnifStep(L+i, poisson[i], p, opts.accumulator, num_states);
#endif
        } // for i 
        // If steady state was reached, add the possion tail
        double tail = 0.0;
        int j = R-L;
        while (i <= j) {
          if (poisson[i] < poisson[j]) {
            tail += poisson[i];
            i++;
          } else {
            tail += poisson[j];
            j--;
          }
        }
        if (tail) {
          for (long s=num_states-1; s>=0; s--) {
            opts.accumulator[s] += p[s] * tail;
          } // for s
        }
      } else {
        // DTMC gets to steady state before L;
        // final vector should equal the DTMC probability vector
        for (long s=num_states-1; s>=0; s--) {
          opts.accumulator[s] = p[s];
        }
      }
  }
  memcpy(p, opts.accumulator, num_states * sizeof(double));

  if (opts.kill_aux_vectors) {
    free(opts.vm_result);
    free(opts.accumulator);
    opts.vm_result = 0;
    opts.accumulator = 0;
  }
  opts.Left = L;
  opts.Right = R;
  free(poisson);
}

void mc_base::genericTransient(int t, double* p, transopts &opts,
  void (mc_base::* step)(const LS_Matrix&, double, double*, double*, bool) const
) const
{
  if (isContinuous()) {
    genericTransient(double(t), p, opts, step);
    return;
  }
  if (0==p) throw MCLib::error(MCLib::error::Null_Vector);
  if (t<0)  throw MCLib::error(MCLib::error::Bad_Time);

  if (0==opts.vm_result) {
    opts.vm_result = (double*) malloc(num_states * sizeof(double));
  }

  if (0==opts.vm_result) {
    throw MCLib::error(MCLib::error::Out_Of_Memory);
  } 

  opts.Steps = stepGeneric(t, 0.0, p, opts.vm_result, opts.ssprec, step);

  if (opts.kill_aux_vectors) {
    free(opts.vm_result);
    free(opts.accumulator);
    opts.vm_result = 0;
    opts.accumulator = 0;
  }
  opts.Left = 0;
  opts.Right = 0;
}


int mc_base::stepGeneric(int n, double q, double* p, double* aux, double delta,
  void (mc_base::* step)(const LS_Matrix&, double, double*, double*, bool) const
) const
{
  LS_Matrix Qtt;
  exportQtt(Qtt);
  Qtt.start = 0;
  Qtt.stop = stop_index[num_classes];

  if (isDiscrete()) q = 1.0;
  double* myp = p;
  int i;
  for (i=0; i<n; i++) {
    (this->*step)(Qtt, q, myp, aux, true);
    SWAP(aux, myp);
    if (delta <= 0) continue;
    // check for convergence
    double maxdelta = 0.0;
    for (long s=num_states-1; s>=0; s--) {
      double d = aux[s] - myp[s];
      if (d < 0)   d = -d;
      if (aux[s])  d /= aux[s];
      maxdelta = MAX(maxdelta, d);
    }
#ifdef DEBUG_SSDETECT
    printf("Old vector: ");
    ShowVector(aux, num_states);
    printf("\nNew vector: ");
    ShowVector(myp, num_states);
    printf("\nmax delta: %g\n", maxdelta);
#endif
    if (maxdelta < delta) break;
  } // for n
  if (p != myp) memcpy(p, myp, num_states * sizeof(double));
 
  return i;
}



void mc_base::forwStep(const LS_Matrix &Qtt, double q, double* p, 
  double* aux, bool normalize) const 
{
#ifdef DEBUG_STEP
    printf("Forward step start  [%lf", p[0]);
    for (long i=1; i<num_states; i++) {
      printf(", %lf", p[i]);
    }
    printf("]\n");
#endif
    // vector-matrix multiply
    for (long s=num_states-1; s>=0; s--) aux[s] = 0.0;
    Qtt.VectorMatrixMultiply(aux, p);
    if (h) h->VectorMatrixMultiply(aux, p);

    // adjust for diagonals
    if (Qtt.d_one_over_diag) {
      for (long s=Qtt.stop-1; s>=0; s--) {
        double d = q - 1.0/Qtt.d_one_over_diag[s];
        aux[s] += d*p[s];
      }
    } else {
      for (long s=Qtt.stop-1; s>=0; s--) {
        double d = q - 1.0/Qtt.f_one_over_diag[s];
        aux[s] += d*p[s];
      }
    }

    // finally, adjust for absorbing states
    for (long s=Qtt.stop; s<num_states; s++) {
      aux[s] += q*p[s];
    }

    if (normalize) {
      // normalize (also handles dividing by q)
      double total = 0.0;
      for (long s=num_states-1; s>=0; s--)   total += aux[s];
      for (long s=num_states-1; s>=0; s--)  aux[s] /= total;
    } else {
      // divide by q
      for (long s=num_states-1; s>=0; s--)  aux[s] /= q;
    }

#ifdef DEBUG_STEP
    printf("Forward step finish [%lf", aux[0]);
    for (long i=1; i<num_states; i++) {
      printf(", %lf", aux[i]);
    }
    printf("]\n");
#endif
}


void mc_base::backStep(const LS_Matrix &Qtt, double q, double* p, 
  double* aux, bool) const 
{
#ifdef DEBUG_STEP
    printf("Backward step start  [%lf", p[0]);
    for (long i=1; i<num_states; i++) {
      printf(", %lf", p[i]);
    }
    printf("]\n");
#endif
    // matrix-vector multiply
    for (long s=num_states-1; s>=0; s--) aux[s] = 0.0;
    Qtt.MatrixVectorMultiply(aux, p);
#ifdef DEBUG_STEP
    printf("Backward step 1      [%lf", aux[0]);
    for (long i=1; i<num_states; i++) {
      printf(", %lf", aux[i]);
    }
    printf("]\n");
#endif
    if (h) h->MatrixVectorMultiply(aux, p);
#ifdef DEBUG_STEP
    printf("Backward step 2      [%lf", aux[0]);
    for (long i=1; i<num_states; i++) {
      printf(", %lf", aux[i]);
    }
    printf("]\n");
#endif

    // adjust for diagonals
    if (Qtt.d_one_over_diag) {
      for (long s=Qtt.stop-1; s>=0; s--) {
        double d = q - 1.0/Qtt.d_one_over_diag[s];
        aux[s] += d*p[s];
      }
    } else {
      for (long s=Qtt.stop-1; s>=0; s--) {
        double d = q - 1.0/Qtt.f_one_over_diag[s];
        aux[s] += d*p[s];
      }
    }

    // finally, adjust for absorbing states
    for (long s=Qtt.stop; s<num_states; s++) {
      aux[s] += q*p[s];
    }

    // divide by q
    if (q != 1) for (long s=num_states-1; s>=0; s--)  aux[s] /= q;

#ifdef DEBUG_STEP
    printf("Backward step finish [%lf", aux[0]);
    for (long i=1; i<num_states; i++) {
      printf(", %lf", aux[i]);
    }
    printf("]\n");
#endif
}

#endif
