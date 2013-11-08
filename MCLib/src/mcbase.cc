
// $Id$

#include "hyper.h"
#include "mcbase.h"

#include "intset.h" // Compact integer set library
#include "lslib.h"  // Linear Solver Library
#include "rng.h"    // RNG library

#include <stdlib.h> 
#include <stdio.h> 
#include <string.h>
#include <math.h>

// #define DEBUG_PERIOD
// #define DEBUG_UNIF
// #define DEBUG_SSDETECT
// #define DEBUG_REDUC_STEADY


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
void ShowUnifStep(int steps, double poiss, double* p, double* a, long size)
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
long FindPeriod(const LS_Matrix &Q)
{
  if (Q.start == Q.stop)  return 0;
#ifdef DEBUG_PERIOD
  printf("Computing period for MC\n");
  printf("Computing distances from state 0\n");
#endif
  long* dist = (long*) malloc((Q.stop - Q.start) * sizeof(long));
  if (0==dist)  return -2;
  long* queue = (long*) malloc((Q.stop - Q.start) * sizeof(long));
  if (0==queue) {
    free(dist);
    return -2;
  }
  long i, a;
  for (i=Q.stop - Q.start - 1; i; i--) dist[i] = -1;
  dist[0] = 0;
  queue[0] = Q.start;
  long head = 1;
  long tail = 0;
  while (tail < head) {
    long i = queue[tail];
    tail++;
    for (a=Q.rowptr[i]; a<Q.rowptr[i+1]; a++) {
      long j = Q.colindex[a];
      long jmst = j - Q.start;
      if (dist[jmst]>=0) continue;
      dist[jmst] = dist[i-Q.start]+1;
      queue[head] = j;
      head++;
    } // for a
  } // while more to explore
  free(queue);
#ifdef DEBUG_PERIOD
  printf("Distances: [%d", dist[0]);
  for (i=1; i<Q.stop-Q.start; i++) printf(", %d", dist[i]);
  printf("]\n");
#endif
  long period = 0;
  a = Q.rowptr[Q.start];
  for (i=Q.start; i<Q.stop; i++) for (; a<Q.rowptr[i+1]; a++) {
    long imst = i-Q.start;
    long j = Q.colindex[a] - Q.start;
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
}

mc_base::~mc_base()
{
  free(rowsums);
  free(oneoverd);
  free(stop_index);
  free(period);
  delete g;
  delete h;
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
    LS_Matrix Qtt;
    exportQtt(Qtt);
    setClass(Qtt, c);
    period[c] = FindPeriod(Qtt);   // in utils.h
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
    computeTransient(int(t), p, opts);
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
  if ((0==poisson) && (R-L>=0)) {
    throw MCLib::error(MCLib::error::Out_Of_Memory);
  }

  if (0==opts.vm_result)
    opts.vm_result = (double*) malloc(num_states * sizeof(double));

  if (0==opts.accumulator)
    opts.accumulator = (double*) malloc(num_states * sizeof(double));

  if (0==opts.vm_result || 0==opts.accumulator) {
    free(poisson);
    throw MCLib::error(MCLib::error::Out_Of_Memory);
  } else {
      opts.Steps = stepForward(L, opts.q, p, opts.vm_result, opts.ssprec);
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
          if (0==stepForward(1, opts.q, p, opts.vm_result, opts.ssprec)) break;
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
      } // if successful
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

void mc_base::computeTransient(int t, double* p, transopts &opts) const
{
  if (isContinuous()) {
    computeTransient(double(t), p, opts);
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

  opts.Steps = stepForward(t, 0.0, p, opts.vm_result, opts.ssprec);

  if (opts.kill_aux_vectors) {
    free(opts.vm_result);
    free(opts.accumulator);
    opts.vm_result = 0;
    opts.accumulator = 0;
  }
  opts.Left = 0;
  opts.Right = 0;
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
  LS_Matrix Qtt;
  exportQtt(Qtt);
  Qtt.start = 0;
  Qtt.stop = stop_index[num_classes];
  if (p0) memcpy(opts.accumulator, p0, num_states * sizeof(double));
  else    memcpy(opts.accumulator, n0t, num_states * sizeof(double));
  for (long s=num_states-1; s>=0; s--) n0t[s] = 0.0;

  for (; t>=1.0; t-=1.0) {
    for (long s=0; s<num_states; s++) n0t[s] += opts.accumulator[s];
    oneStep(Qtt, 1.0, opts.accumulator, opts.vm_result);
    SWAP(opts.accumulator, opts.vm_result);
    opts.Steps++;
  }
  for (long s=0; s<num_states; s++) n0t[s] += t*opts.accumulator[s];
}

void
mc_base::accCTMC(double t, const double* p0, double* n0t, transopts &opts) const
{
  // Get poisson distribution
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
  if ((0==poisson) && (R-L>=0)) {
    throw MCLib::error(MCLib::error::Out_Of_Memory);
  }
  // convert from prob(Y=L+i) to prob(Y>L+i)
  for (int y=0; y<R-L; y++) poisson[y] = poisson[y+1];
  poisson[R-L] = 0.0;
  for (int y=R-L-1; y>=0; y--) poisson[y] += poisson[y+1];

  LS_Matrix Qtt;
  exportQtt(Qtt);
  Qtt.start = 0;
  Qtt.stop = stop_index[num_classes];
  if (p0) memcpy(opts.accumulator, p0, num_states * sizeof(double));
  else    memcpy(opts.accumulator, n0t, num_states * sizeof(double));
  for (long s=0; s<num_states; s++) n0t[s] = 0;
  // before L: prob(Y>i) is effectively 1
  double adj = 1.0 / opts.q;
  for (int i=0; i<L; i++) {
    opts.Steps++;
    for (long s=0; s<num_states; s++) n0t[s] += adj * opts.accumulator[s];
    oneStep(Qtt, opts.q, opts.accumulator, opts.vm_result);
    SWAP(opts.accumulator, opts.vm_result);
  } // for i
  for (int i=0; ; i++) {
    opts.Steps++;
    adj = poisson[i] / opts.q;
    for (long s=0; s<num_states; s++) n0t[s] += adj * opts.accumulator[s];
    if (i>=R-L) break;
    oneStep(Qtt, opts.q, opts.accumulator, opts.vm_result);
    SWAP(opts.accumulator, opts.vm_result);
  } // for i
  
  // cleanup
  free(poisson);
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
mc_base::computeDiscreteDistTTA(const LS_Vector &p0, distopts &opts, int c, 
            double epsilon, double* &dist, int &N) const
{
  if (!finished) {
    throw MCLib::error(MCLib::error::Finished_Mismatch);
  }
  if (0==c) throw MCLib::error(MCLib::error::Bad_Class);
  // TBD
  throw MCLib::error(MCLib::error::Not_Implemented);
}

double
mc_base::computeDiscreteDistTTA(const LS_Vector &p0, distopts &opts, int c, 
            double dist[], int N) const
{
  if (!finished) {
    throw MCLib::error(MCLib::error::Finished_Mismatch);
  }
  if (0==c) throw MCLib::error(MCLib::error::Bad_Class);
  // TBD
  throw MCLib::error(MCLib::error::Not_Implemented);
  return 0.0;
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


void mc_base::irredSteady(double* p, 
      const LS_Options &opt, 
      LS_Output &lo) const
{
  for (long i=num_states-1; i>=0; i--) 
  p[i] = 1;

  if (num_states > 1) {
    LS_Matrix Qtt;
    exportQtt(Qtt);
    setClass(Qtt, 1);
    Solve_AxZero(Qtt, p, opt, lo);
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

  LS_Matrix Qtt;
  exportQtt(Qtt);

  if (1==num_classes && 0==getNumAbsorbing()) {
    // we are absorbed into this class with probability 1
    if (stop_index)   p[stop_index[0]] = 1;
    else              p[0] = 1;
  } else {
    // need to figure out absorption probs for recurrent classes & absorbings
    setClass(Qtt, 0);
    Solve_Axb(Qtt, p, p0, opt, lo);

    if ((LS_Success != lo.status) && (LS_No_Convergence != lo.status)) return;

    // Adjust TTA solution
    for (long i=0; i < stop_index[0]; i++) p[i] = -p[i];

    // Get absorption proabilities
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
    for (i=stop_index[num_classes]; i<num_states; i++)  total += p[i];
    for (i=stop_index[num_classes]; i<num_states; i++)  p[i] /= total;
  } // if more than one recurrent class

  // Now, deal with each recurrent class
  for (long c=1; c<=num_classes; c++) {
    // determine the total probability of reaching this class
    double reachprob = 0.0; 
    for (long i=stop_index[c-1]; i<stop_index[c]; i++)
      reachprob += p[i];
#ifdef DEBUG_REDUC_STEADY
    printf("Reach class %d with prob %f\n", c, reachprob);
#endif
    if (0.0==reachprob) continue;

    // if nonzero, determine the stationary distribution for the class
    for (long i=stop_index[c-1]; i<stop_index[c]; i++) p[i] = 1;
   
    setClass(Qtt, c);
    Solve_AxZero(Qtt, p, opt, lo);  // Hmmmm.....
#ifdef DEBUG_REDUC_STEADY
    printf("%d iters, %f precision\n", lsout.num_iters, lsout.precision);
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

  LS_Matrix Qtt;
  exportQtt(Qtt);
  setClass(Qtt, 0);
  Solve_Axb(Qtt, p, p0, opt, lo);

  // Adjust TTA solution
  for (long i=0; i < stop_index[0]; i++) p[i] = -p[i];
}



int mc_base::stepForward(int n, double q, double* p, double* aux, double delta) const
{
  LS_Matrix Qtt;
  exportQtt(Qtt);
  Qtt.start = 0;
  Qtt.stop = stop_index[num_classes];

  if (isDiscrete()) q = 1.0;
  double* myp = p;
  int i;
  for (i=0; i<n; i++) {
    oneStep(Qtt, q, myp, aux);
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
