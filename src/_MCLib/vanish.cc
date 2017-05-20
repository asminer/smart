
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

class my_vanish : public MCLib::vanishing_chain {
  hypersparse_matrix* initial;
  mc_general* TT_proc;
  mc_absorb* V_proc;
  hypersparse_matrix* TV_rates;

  // stuff for eliminating vanishings
  MCLib::Markov_chain::finish_options fopts;
  MCLib::Markov_chain::renumbering mcrenumb;
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
  virtual MCLib::Markov_chain* grabTTandClear();
};



// ******************************************************************
// *                           Front  end                           *
// ******************************************************************

MCLib::vanishing_chain* 
MCLib::startVanishingChain(bool disc, long nt, long nv)
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
  if (-handle-1 != h2) throw MCLib::error(MCLib::error::Miscellaneous);
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
  catch (MCLib::error e) {
    if (e.getCode() == MCLib::error::Wrong_Type)
      throw MCLib::error(MCLib::error::Loop_Of_Vanishing);
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
    if (0==vtime) throw MCLib::error(MCLib::error::Out_Of_Memory);
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
      throw MCLib::error(MCLib::error::Miscellaneous);
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

MCLib::Markov_chain* my_vanish::grabTTandClear()
{
  MCLib::Markov_chain* mc = TT_proc;
  TT_proc = 0;
  V_proc->clear();
  return mc;
}

