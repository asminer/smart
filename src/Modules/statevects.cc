
#include "../Options/options.h"
#include "../ExprLib/startup.h"
#include "../ExprLib/exprman.h"
// #include "../ExprLib/unary.h"
// #include "../ExprLib/binary.h"
// #include "../ExprLib/assoc.h"
#include "../SymTabs/symtabs.h"
#include "../ExprLib/functions.h"
// #include "biginttype.h"
#include "../ExprLib/mod_vars.h"
// #include "../ExprLib/dd_front.h"

#include "../Formlsms/stoch_llm.h"

#include "statevects.h"
#include "expl_ssets.h"

// External
#include "../_IntSets/intset.h"
#include "../_LSLib/lslib.h"

// ******************************************************************
// *                                                                *
// *                    statevect_printer  class                    *
// *                                                                *
// ******************************************************************

class statevect_printer : public state_lldsm::state_visitor {
  OutputStream &out;
  int display_style;
  double* myvect;
  long nextz;
  bool comma;
public:
  statevect_printer(const hldsm* mdl, OutputStream &s, const statevect* d, 
    int style);
  ~statevect_printer();
  virtual bool canSkipIndex();
  virtual bool visit();
};

statevect_printer::statevect_printer(const hldsm* m, OutputStream &s, 
  const statevect* d, int style) : state_visitor(m), out(s)
{
  display_style = style;
  comma = false;
  nextz = 0;

  long ns = d->getParent()->getNumStates();
  myvect = new double[ns];
  d->ExportTo(myvect);
}

statevect_printer::~statevect_printer()
{
  delete[] myvect;
}

bool statevect_printer::canSkipIndex()
{
  if (statevect::FULL == display_style) return false;
  return 0==myvect[x.current_state_index];
}

bool statevect_printer::visit()
{
  if (comma)  out << ", ";
  else        comma = true;
  
  if (statevect::SINDEX==display_style) {
    out.Put(x.current_state_index); 
    out.Put(':');
  }
  if (statevect::SSTATE==display_style) {
    x.current_state->Print(out, 0);
    out.Put(':');
  }

  out << myvect[x.current_state_index];
  out.can_flush();  // otherwise, huge sets will overflow the buffer
  return false;
}

// ******************************************************************
// *                                                                *
// *                       statevect  methods                       *
// *                                                                *
// ******************************************************************

int statevect::display_style;

statevect::statevect(const stochastic_lldsm* p, const double* d, long N)
: shared_object()
{
  parent = p;
  // determine number of nonzeroes
  long nnz = 0;
  for (long i=0; i<N; i++) if (d[i]) nnz++;
  if (0==nnz) {
    // easy case
    indexes = 0;
    vect = 0;
    vectsize = 0;
    return;
  }

  // determine truncation point
  long tsize;
  for (tsize=N-1; tsize>=0; tsize--) {
    if (d[tsize]) break;
  }
  tsize++;

  //
  // Decide how to store
  //
  if (storeAsTruncated(tsize, nnz)) {
    //
    // Truncated
    //
    indexes = 0;
    vect = new double[tsize];
    memcpy(vect, d, tsize*sizeof(double));
    vectsize = tsize;
  } else {
    //
    // Sparse
    //
    indexes = new long[nnz];
    vect = new double[nnz];
    vectsize = nnz;
    long z = 0;
    for (long i=0; i<N; i++) if (d[i]) {
      indexes[z] = i;
      vect[z] = d[i];
      z++;
    }
  }
}

statevect::statevect(const stochastic_lldsm* p, long* I, double* D, long N)
: shared_object()
{
  parent = p;
  indexes = I;
  vect = D;
  vectsize = N;
}

statevect::statevect(const stochastic_lldsm* p, LS_Vector &V, const long* ren)
: shared_object()
{
  parent = p;

  DCASSERT(V.d_value || V.f_value);

  //
  // Determine number of nonzeroes, truncation point
  //
  long nnz = 0;
  long tsize = -1;
  for (long z=0; z<V.size; z++) {
    if (V.d_value) {
      if (V.d_value[z]) nnz++;
      else              continue;
    } else {
      if (V.f_value[z]) nnz++;
      else              continue;
    }
    long i;
    if (V.index)  i = V.index[z];
    else          i = z;
    if (ren)      i = ren[i];
    tsize = MAX(tsize, i);
  }
  tsize++;

  if (storeAsTruncated(tsize, nnz)) {
    //
    // Use Trunated Full
    //
    indexes = 0;
    vect = new double[tsize];
    vectsize = tsize;
    for (long i=0; i<tsize; i++) vect[i] = 0;

    if (V.index) {
      //
      // V is sparse - convert
      //
      for (long z=0; z<V.size; z++) {
        if (V.d_value) {
          if (0==V.d_value[z]) continue;
        } else {
          if (0==V.f_value[z]) continue;
        }
        long i = ren ? ren[V.index[z]] : V.index[z];
        CHECK_RANGE(0, i, tsize);
        if (V.d_value)  vect[i] = V.d_value[z];
        else            vect[i] = V.f_value[z];
      }

    } else {
      //
      // V is full - copy
      //
      if (V.d_value) {
        for (long i=0; i<V.size; i++) {
          long ii = ren ? ren[i] : i;
          CHECK_RANGE(0, i, tsize);
          vect[ii] = V.d_value[i];
        }
      } else {
        for (long i=0; i<V.size; i++) {
          long ii = ren ? ren[i] : i;
          CHECK_RANGE(0, i, tsize);
          vect[ii] = V.f_value[i];
        }
      }
    }

  } else {
    //
    // Use Sparse
    //
    indexes = new long[nnz];
    vect = new double[nnz];
    vectsize = nnz;
    
    if (V.index) {
      //
      // V is sparse - copy
      //
      long z = 0;
      for (long vz=0; vz<V.size; vz++) {
        long i = ren ? ren[V.index[vz]] : V.index[vz];
        indexes[z] = i;
        if (V.d_value)  vect[z] = V.d_value[vz];
        else            vect[z] = V.f_value[vz];
        if (vect[z]) z++;
      }
    } else {
      //
      // V is full - convert
      //
      long z = 0;
      if (V.d_value) {
        for (long i=0; i<V.size; i++) if (V.d_value[i]) {
          vect[z] = V.d_value[i];
          indexes[z] = ren ? ren[i] : i;
          z++;
        }
      } else {
        for (long i=0; i<V.size; i++) if (V.f_value[i]) {
          vect[z] = V.f_value[i];
          indexes[z] = ren ? ren[i] : i;
          z++;
        }
      }
    }
  }

  delete[] V.index;
  delete[] V.d_value;
  delete[] V.f_value;
}

statevect::~statevect()
{
  // printf("Destroying state vector\n");  
  delete[] indexes;
  delete[] vect;
}

bool statevect::ExportTo(LS_Vector &V) const
{
  V.size = vectsize;
  V.index = indexes;
  V.d_value = vect;
  V.f_value = 0;
  return true;
}

void statevect::ExportTo(double *d) const
{
  DCASSERT(parent);
  long num_states = parent->getNumStates();

  for (long i=0; i<num_states; i++) d[i] = 0;
  
  if (indexes) {
    for (long z=0; z<vectsize; z++) {
      CHECK_RANGE(0, indexes[z], num_states);
      d[indexes[z]] = vect[z];
    }
  } else {
    for (long i=0; i<vectsize; i++) {
      d[i] = vect[i];
    }
  }
}

void statevect::ExportTo(long* ind, double* value) const
{
  long z = 0;

  if (indexes) {
    for (long i=0; i<vectsize; i++) if (vect[i]) {
      value[z] = vect[i];
      ind[z] = indexes[i];
      z++;
    }
  } else {
    for (long i=0; i<vectsize; i++) if (vect[i]) {
      value[z] = vect[i];
      ind[z] = i;
      z++;
    }
  }
}

long statevect::countNNZs() const
{
  long nnzs = 0;
  // same regardless of sparse/full storage
  for (long i=0; i<vectsize; i++)
    if (vect[i])
      nnzs++;

  return nnzs;
}

bool statevect::Print(OutputStream &s, int width) const
{
  DCASSERT(parent);

  if (FULL==display_style)  s.Put('[');
  else                      s.Put('(');

  statevect_printer foo(parent->GetParent(), s, this, display_style);
  parent->visitStates(foo);

  if (FULL==display_style)  s.Put(']');
  else                      s.Put(')');

  return true;
}

bool statevect::Equals(const shared_object *o) const
{
  const statevect* b = dynamic_cast <const statevect*> (o);
  if (0==b) return false;
  if (parent != b->parent) return false;  // TBD: may want to allow this
  
  DCASSERT(vectsize == b->vectsize);

  for (long i=0; i<vectsize; i++) {
    if (vect[i] != b->vect[i]) return false;
    // TBD - check within epsilon?
  }
  return true;
}

void statevect::greater_than(double v, intset* I) const
{
  if (0==I) return;
  if (v>=0) {
    //
    // skipped elements in vector - not set in I
    // 
    I->removeAll();

    // go through vector and see what to add
    if (indexes) {
      // 
      // sparse
      //
      for (long z=0; z<vectsize; z++) {
        if (vect[z] > v) 
          I->addElement(indexes[z]);
      }
    } else {
      //
      // full
      //
      for (long i=0; i<vectsize; i++) {
        if (vect[i] > v)
          I->addElement(i);
      }
    }
  } else {
    //
    // skipped elements in vector - are set in I
    //
    I->addAll();

    // go through vector and see what to remove
    if (indexes) {
      // 
      // sparse
      //
      for (long z=0; z<vectsize; z++) {
        if (vect[z] <= v) 
          I->removeElement(indexes[z]);
      }
    } else {
      //
      // full
      //
      for (long i=0; i<vectsize; i++) {
        if (vect[i] <= v)
          I->removeElement(i);
      }
    }
  }
}

void statevect::less_than(double v, intset* I) const
{
  if (0==I) return;
  if (v<=0) {
    //
    // skipped elements in vector - not set in I
    // 
    I->removeAll();

    // go through vector and see what to add
    if (indexes) {
      // 
      // sparse
      //
      for (long z=0; z<vectsize; z++) {
        if (vect[z] < v) 
          I->addElement(indexes[z]);
      }
    } else {
      //
      // full
      //
      for (long i=0; i<vectsize; i++) {
        if (vect[i] < v)
          I->addElement(i);
      }
    }
  } else {
    //
    // skipped elements in vector - are set in I
    //
    I->addAll();

    // go through vector and see what to remove
    if (indexes) {
      // 
      // sparse
      //
      for (long z=0; z<vectsize; z++) {
        if (vect[z] >= v) 
          I->removeElement(indexes[z]);
      }
    } else {
      //
      // full
      //
      for (long i=0; i<vectsize; i++) {
        if (vect[i] >= v)
          I->removeElement(i);
      }
    }
  }
}

void statevect::equals(double v, intset* I) const 
{
  if (0==I) return;
  //
  // TBD - need a precision for this
  //
  if (v) {
    //
    // skipped elements in vector - not set in I
    // 
    I->removeAll();

    // go through vector and see what to add
    if (indexes) {
      // 
      // sparse
      //
      for (long z=0; z<vectsize; z++) {
        if (vect[z] == v) 
          I->addElement(indexes[z]);
      }
    } else {
      //
      // full
      //
      for (long i=0; i<vectsize; i++) {
        if (vect[i] == v)
          I->addElement(i);
      }
    }
  } else {
    //
    // special case - equal to 0
    //
    I->addAll();

    // go through vector and see what to remove
    if (indexes) {
      // 
      // sparse
      //
      for (long z=0; z<vectsize; z++) {
        if (vect[z]) 
          I->removeElement(indexes[z]);
      }
    } else {
      //
      // full
      //
      for (long i=0; i<vectsize; i++) {
        if (vect[i])
          I->removeElement(i);
      }
    }
  }
}

double statevect::dot_product(const statevect* x) const
{
  if (0==x) return 0;

  double answer = 0;

  if (isSparse()) {
    //
    // We're sparse.
    // Transform x to truncated full (if necessary) and determine product
    // (we do this because sparse vectors do not require indexes to be sorted)
    //

    long truncsize;
    const double* trunc;
    double* temp = 0;

    if (x->isSparse()) {
      //
      // Build a small truncated-full vector to hold vector x
      // but don't bother to hold elements past our largest index
      //

      long mymax = 0;
      for (long z=0; z<vectsize; z++) {
        mymax = MAX(mymax, indexes[z]);
      }
      long xmax = 0;
      for (long z=0; z<x->vectsize; z++) {
        xmax = MAX(xmax, x->indexes[z]);
      }

      truncsize = 1+MIN(mymax, xmax);
      
      temp = new double[truncsize];
      for (long i=0; i<truncsize; i++) temp[i] = 0;
      for (long z=0; z<x->vectsize; z++) {
        if (x->indexes[z] >= truncsize) continue;
        temp[x->indexes[z]] = x->vect[z];
      }
      trunc = temp;
    } else {
      //
      // Already truncated full, use it
      //
      truncsize = x->vectsize;
      trunc = x->vect;
    }

    //
    // sparse - full
    //
    for (long z=0; z<vectsize; z++) {
      if (indexes[z] >= truncsize) continue;
      answer += vect[z] * trunc[indexes[z]];
    }

    delete[] temp;
  } else {
    if (x->isSparse()) {
      //
      // full - sparse
      //
      for (long z=0; z<x->vectsize; z++) {
        if (x->indexes[z] >= vectsize) continue;
        answer += x->vect[z] * vect[x->indexes[z]];
      }
    } else {
      //
      // full - full
      //
      long zstop = MIN(vectsize, x->vectsize);
      for (long z=0; z<zstop; z++) {
        answer += vect[z] * x->vect[z];
      }
    }
  }
  return answer;
}


void statevect::copyRestricted(const statevect* sv, const intset* e)
{
  DCASSERT(sv);

  long nnz = 0;
  long tsize = 0;

  if (sv->isSparse()) {
    for (long z=0; z<sv->size(); z++) {
      long i = sv->readSparseIndex(z);
      if (e && !e->contains(i)) continue;
      if (0==sv->readSparseValue(z)) continue;
      nnz++;
      tsize = MAX(tsize, 1+i);
    }
  } else {
    for (long i=0; i<sv->size(); i++) {
      if (0==sv->readFull(i)) continue;
      if (e && !e->contains(i)) continue;
      nnz++;
      tsize = MAX(tsize, 1+i);
    }
  }

  if (tsize * sizeof(double) <= nnz * (sizeof(double)+sizeof(long))) {
    //
    // Store as truncated
    //
    indexes = 0;
    vect = new double[tsize];
    vectsize = tsize;
    //
    if (sv->isSparse()) {
      //
      // Trunc <- sparse
      //
      for (long i=0; i<tsize; i++) vect[i] = 0;
      for (long z=0; z<nnz; z++) {
        long i = sv->readSparseIndex(z);
        if (e && !e->contains(i)) continue;
        vect[i] = sv->readSparseValue(z);
      }
    } else {
      //
      // Trunc <- trunc
      //
      for (long i=0; i<tsize; i++) {
        if (e && !e->contains(i))   vect[i] = 0;
        else                        vect[i] = sv->readFull(i);
      }
    }
  } else {
    //
    // Store as sparse
    //
    indexes = new long[nnz];
    vect = new double[nnz];
    vectsize = nnz;
    //
    if (sv->isSparse()) {
      //
      // Sparse <- sparse
      //
      long zp = 0;
      for (long z=0; z<sv->size(); z++) {
        long i = sv->readSparseIndex(z);
        if (e && !e->contains(i)) continue;
        if (0==sv->readSparseValue(z)) continue;
        indexes[zp] = i;
        vect[zp] = sv->readSparseValue(z);
        zp++;
      }
      DCASSERT(zp == nnz);
    } else {
      //
      // Sparse <- trunc
      //
      long zp = 0;
      for (long i=0; i<sv->size(); i++) {
        if (0==sv->readFull(i)) continue;
        if (e && !e->contains(i)) continue;
        indexes[zp] = i;
        vect[zp] = sv->readFull(i);
        zp++;
      }
      DCASSERT(zp == nnz);
    }
  }
}

// ******************************************************************
// *                                                                *
// *                       statedist  methods                       *
// *                                                                *
// ******************************************************************

statedist::statedist(const stochastic_lldsm *p, const double *d, long N)
 : statevect(p, d, N)
{
}

statedist::statedist(const stochastic_lldsm* p, long* I, double* D, long N)
 : statevect(p, I, D, N)
{
}

statedist::statedist(const stochastic_lldsm* p, LS_Vector &V, const long* ren)
 : statevect(p, V, ren)
{
}

double statedist::normalize()
{
  //
  // This works both for sparse and truncated full :^)
  //
  double total = 0.0;
  for (long i=0; i<vectsize; i++) total += vect[i];
  if (total <= 0.0) return total;
  if (1.0 == total) return total;
  for (long i=0; i<vectsize; i++) vect[i] /= total;
  return total;
}

// ******************************************************************
// *                                                                *
// *                       stateprobs methods                       *
// *                                                                *
// ******************************************************************

stateprobs::stateprobs(const stochastic_lldsm *p, const double *d, long N)
 : statevect(p, d, N)
{
}

stateprobs::stateprobs(const stochastic_lldsm* p, long* I, double* D, long N)
 : statevect(p, I, D, N)
{
}

stateprobs::stateprobs(const stochastic_lldsm* p, LS_Vector &V, const long* ren)
 : statevect(p, V, ren)
{
}

// ******************************************************************
// *                                                                *
// *                       statemsrs  methods                       *
// *                                                                *
// ******************************************************************

statemsrs::statemsrs(const stochastic_lldsm *p, const double *d, long N)
 : statevect(p, d, N)
{
}

statemsrs::statemsrs(const stochastic_lldsm* p, long* I, double* D, long N)
 : statevect(p, I, D, N)
{
}

statemsrs::statemsrs(const stochastic_lldsm* p, LS_Vector &V, const long* ren)
 : statevect(p, V, ren)
{
}

// ******************************************************************
// *                                                                *
// *                      statedist_type class                      *
// *                                                                *
// ******************************************************************

class statedist_type : public simple_type {
public:
  statedist_type();
};

statedist_type::statedist_type() : simple_type("statedist", "Distribution over states", "A probability distribution over a set of model states.  Elements will sum to one.")
{
  setPrintable();
}

// ******************************************************************
// *                                                                *
// *                     stateprobs_type  class                     *
// *                                                                *
// ******************************************************************

class stateprobs_type : public simple_type {
public:
  stateprobs_type();
};

stateprobs_type::stateprobs_type() : simple_type("stateprobs", "Probabilities for states", "A vector of probabilities over states, specifying a probability value for each state.  Note that this is not a distribution, but rather a special case of measures for each state, so probabilities may not sum to one.")
{
  setPrintable();
}

// ******************************************************************
// *                                                                *
// *                      statemsrs_type class                      *
// *                                                                *
// ******************************************************************

class statemsrs_type : public simple_type {
public:
  statemsrs_type();
};

statemsrs_type::statemsrs_type() : simple_type("statemsrs", "Measures for states", "A vector of real--valued measures over states, specifying a measure for each state.")
{
  setPrintable();
}

// ******************************************************************
// *                                                                *
// *                                                                *
// *                           Functions                            *
// *                                                                *
// *                                                                *
// ******************************************************************

// ******************************************************************
// *                          gt_si  class                          *
// ******************************************************************

class gt_si : public simple_internal {
public:
  gt_si();
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

gt_si::gt_si() : simple_internal(em->STATESET, "gt", 2)
{
  DCASSERT(em->STATESET);
  DCASSERT(em->STATEPROBS);
  SetFormal(0, em->STATEPROBS, "x");
  SetFormal(1, em->REAL, "v");
  SetDocumentation("Determine, the set of states whose x value is larger than v.");
}

void gt_si::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(2==np);
  DCASSERT(0==x.aggregate);

  SafeCompute(pass[0], x);
  if (!x.answer->isNormal()) return;

  stateprobs* p = smart_cast <stateprobs*>(Share(x.answer->getPtr()));
  DCASSERT(p);

  SafeCompute(pass[1], x);
  if (!x.answer->isNormal()) {
    Delete(p);
    return;
  }
  double v = x.answer->getReal();

  //
  // Ready to do actual computation
  //
  const stochastic_lldsm* llm = p->getParent();
  long ns = llm->getNumStates();
  intset* ans = new intset(ns);
  p->greater_than(v, ans);  

  //
  // Finalize & Cleanup 
  //
  x.answer->setPtr( new expl_stateset(llm, ans) );
  Delete(p);
}

// ******************************************************************
// *                          ge_si  class                          *
// ******************************************************************

class ge_si : public simple_internal {
public:
  ge_si();
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

ge_si::ge_si() : simple_internal(em->STATESET, "ge", 2)
{
  DCASSERT(em->STATESET);
  DCASSERT(em->STATEPROBS);
  SetFormal(0, em->STATEPROBS, "x");
  SetFormal(1, em->REAL, "v");
  SetDocumentation("Determine, the set of states whose x value is at least v.");
}

void ge_si::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(2==np);
  DCASSERT(0==x.aggregate);

  SafeCompute(pass[0], x);
  if (!x.answer->isNormal()) return;

  stateprobs* p = smart_cast <stateprobs*>(Share(x.answer->getPtr()));
  DCASSERT(p);

  SafeCompute(pass[1], x);
  if (!x.answer->isNormal()) {
    Delete(p);
    return;
  }
  double v = x.answer->getReal();

  //
  // Ready to do actual computation
  //
  const stochastic_lldsm* llm = p->getParent();
  long ns = llm->getNumStates();
  intset* ans = new intset(ns);
  p->less_than(v, ans);  
  ans->complement();

  //
  // Finalize & Cleanup 
  //
  x.answer->setPtr( new expl_stateset(llm, ans) );
  Delete(p);
}

// ******************************************************************
// *                          lt_si  class                          *
// ******************************************************************

class lt_si : public simple_internal {
public:
  lt_si();
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

lt_si::lt_si() : simple_internal(em->STATESET, "lt", 2)
{
  DCASSERT(em->STATESET);
  DCASSERT(em->STATEPROBS);
  SetFormal(0, em->STATEPROBS, "x");
  SetFormal(1, em->REAL, "v");
  SetDocumentation("Determine, the set of states whose x value is smaller than v.");
}

void lt_si::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(2==np);
  DCASSERT(0==x.aggregate);

  SafeCompute(pass[0], x);
  if (!x.answer->isNormal()) return;

  stateprobs* p = smart_cast <stateprobs*>(Share(x.answer->getPtr()));
  DCASSERT(p);

  SafeCompute(pass[1], x);
  if (!x.answer->isNormal()) {
    Delete(p);
    return;
  }
  double v = x.answer->getReal();

  //
  // Ready to do actual computation
  //
  const stochastic_lldsm* llm = p->getParent();
  long ns = llm->getNumStates();
  intset* ans = new intset(ns);
  p->less_than(v, ans);  

  //
  // Finalize & Cleanup 
  //
  x.answer->setPtr( new expl_stateset(llm, ans) );
  Delete(p);
}

// ******************************************************************
// *                          le_si  class                          *
// ******************************************************************

class le_si : public simple_internal {
public:
  le_si();
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

le_si::le_si() : simple_internal(em->STATESET, "le", 2)
{
  DCASSERT(em->STATESET);
  DCASSERT(em->STATEPROBS);
  SetFormal(0, em->STATEPROBS, "x");
  SetFormal(1, em->REAL, "v");
  SetDocumentation("Determine, the set of states whose x value is at most v.");
}

void le_si::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(2==np);
  DCASSERT(0==x.aggregate);

  SafeCompute(pass[0], x);
  if (!x.answer->isNormal()) return;

  stateprobs* p = smart_cast <stateprobs*>(Share(x.answer->getPtr()));
  DCASSERT(p);

  SafeCompute(pass[1], x);
  if (!x.answer->isNormal()) {
    Delete(p);
    return;
  }
  double v = x.answer->getReal();

  //
  // Ready to do actual computation
  //
  const stochastic_lldsm* llm = p->getParent();
  long ns = llm->getNumStates();
  intset* ans = new intset(ns);
  p->greater_than(v, ans);  
  ans->complement();

  //
  // Finalize & Cleanup 
  //
  x.answer->setPtr( new expl_stateset(llm, ans) );
  Delete(p);
}

// ******************************************************************
// *                       condition_si class                       *
// ******************************************************************

class condition_si : public simple_internal {
public:
  condition_si();
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

condition_si::condition_si() : simple_internal(em->STATEDIST, "condition", 2)
{
  DCASSERT(em->STATESET);
  DCASSERT(em->STATEDIST);
  SetFormal(0, em->STATEDIST, "p");
  SetFormal(1, em->STATESET, "e");
  SetDocumentation("Given a state distribution p, build a new distribution conditioned on the fact that a state belongs to the set e.  Will return null if the probability (according to p) of e is 0.");
}

void condition_si::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(2==np);
  DCASSERT(0==x.aggregate);

  SafeCompute(pass[0], x);
  if (!x.answer->isNormal()) return;

  statedist* p = smart_cast <statedist*>(Share(x.answer->getPtr()));
  DCASSERT(p);

  SafeCompute(pass[1], x);
  if (!x.answer->isNormal()) {
    Delete(p);
    return;
  }

  stateset* ss = smart_cast <stateset*>(Share(x.answer->getPtr()));
  DCASSERT(ss);

  if (p->getParent() != ss->getParent()) {
    if (em->startError()) {
      em->causedBy(this);
      em->cerr() << "State distribution and set parameters to condition()";
      em->newLine();
      em->cerr() << "are from different model instances";
      em->stopIO();
    }
    Delete(p);
    Delete(ss);
    x.answer->setNull();
    return;
  }

  expl_stateset* e = dynamic_cast <expl_stateset*>(ss);
  if (!e) {
  
    if (em->startError()) {
      em->causedBy(this);
      em->cerr() << "Sorry, condition() requires explicit statesets (for now)";
      em->stopIO();
    }
    Delete(p);
    Delete(ss);
    x.answer->setNull();
    return;
  }

  //
  // Ready to do actual computation
  //

  statedist* q = new statedist(p->getParent(), 0, 0, 0);
  const intset& eis = e->getExplicit();
  q->copyRestricted(p, &eis);

  double total = q->normalize();

  if (total <= 0) {
    // that's a null
    Delete(q);
    x.answer->setNull();
  } else if (total == 1) {
    // q is equal to p
    Delete(q);
    x.answer->setPtr(Share(p));
  } else {
    // generic case
    x.answer->setPtr(q);
  }

  //
  // Cleanup 
  //
  Delete(p);
  Delete(e);
}

// ******************************************************************
// *                         prob_si  class                         *
// ******************************************************************

class prob_si : public simple_internal {
public:
  prob_si();
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

prob_si::prob_si() : simple_internal(em->REAL, "prob", 2)
{
  DCASSERT(em->STATESET);
  DCASSERT(em->STATEDIST);
  SetFormal(0, em->STATEDIST, "p");
  SetFormal(1, em->STATESET, "e");
  SetDocumentation("Determine the probability (according to distribution p) of a state being in set e.");
}

void prob_si::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(2==np);
  DCASSERT(0==x.aggregate);

  SafeCompute(pass[0], x);
  if (!x.answer->isNormal()) return;

  statedist* p = smart_cast <statedist*>(Share(x.answer->getPtr()));
  DCASSERT(p);

  SafeCompute(pass[1], x);
  if (!x.answer->isNormal()) {
    Delete(p);
    return;
  }

  stateset* ss = smart_cast <stateset*>(Share(x.answer->getPtr()));
  DCASSERT(ss);

  if (p->getParent() != ss->getParent()) {
    if (em->startError()) {
      em->causedBy(this);
      em->cerr() << "State distribution and set parameters to prob()";
      em->newLine();
      em->cerr() << "are from different model instances";
      em->stopIO();
    }
    Delete(p);
    Delete(ss);
    x.answer->setNull();
    return;
  }

  expl_stateset* e = dynamic_cast <expl_stateset*>(ss);
  if (!e) {
    if (em->startError()) {
      em->causedBy(this);
      em->cerr() << "Sorry, prob() requires explicit statesets (for now)";
      em->stopIO();
    }
    Delete(p);
    Delete(ss);
    x.answer->setNull();
    return;
  }

  //
  // Ready to do actual computation
  //
  double prob = 0.0;
  const intset eis = e->getExplicit();  
  long i = -1;
  while ( (i=eis.getSmallestAfter(i)) >= 0) {
    prob += p->readFull(i);
  }
  Delete(p);
  Delete(e);
  x.answer->setReal(prob);
}


// ******************************************************************
// *                       expected_si  class                       *
// ******************************************************************

class expected_si : public simple_internal {
public:
  expected_si(const type* msrtype);
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

expected_si::expected_si(const type* msrtype)
: simple_internal(em->REAL, "expected", 2)
{
  DCASSERT(em->STATEDIST);
  DCASSERT(msrtype);
  SetFormal(0, msrtype, "x");
  SetFormal(1, em->STATEDIST, "p");
  SetDocumentation("Determine the expected value of x, according to distribution p.");
}

void expected_si::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(2==np);
  DCASSERT(0==x.aggregate);

  //
  // Compute first parameter - the measure vector
  //

  SafeCompute(pass[0], x);
  if (!x.answer->isNormal()) return;

  statevect* vx = smart_cast <statevect*>(Share(x.answer->getPtr()));
  DCASSERT(vx);

  //
  // Compute second parameter - the distribution vector
  //

  SafeCompute(pass[1], x);
  if (!x.answer->isNormal()) {
    Delete(vx);
    return;
  }

  statedist* vp = smart_cast <statedist*>(Share(x.answer->getPtr()));
  DCASSERT(vp);

  //
  // Make sure these are from the same model
  //

  if (vx->getParent() != vp->getParent()) {
    if (em->startError()) {
      em->causedBy(this);
      em->cerr() << "Measure and distribution are from different model instances";
      em->stopIO();
    }
    Delete(vx);
    Delete(vp);
    x.answer->setNull();
    return;
  }

  //
  // Ready to do actual computation
  //

  x.answer->setReal( vx->dot_product(vp) );
  Delete(vx);
  Delete(vp);
}



// ******************************************************************
// *                                                                *
// *                                                                *
// *                         Initialization                         *
// *                                                                *
// *                                                                *
// ******************************************************************

class init_statevects : public initializer {
  public:
    init_statevects();
    virtual bool execute();
};
init_statevects the_statevect_initializer;

init_statevects::init_statevects() : initializer("init_statevects")
{
  usesResource("em");
  usesResource("st");
  usesResource("statesettype");
  buildsResource("statevects");
  buildsResource("types");
}

bool init_statevects::execute()
{
  if (0==em)  return false;
  
  // Type registry
  // ------------------------------------------------------------------
  simple_type* t_statedist = new statedist_type;
  em->registerType(t_statedist);

  simple_type* t_stateprobs = new stateprobs_type;
  em->registerType(t_stateprobs);

  simple_type* t_statemsrs = new statemsrs_type;
  em->registerType(t_statemsrs);

  em->setFundamentalTypes();

  // Operators
  // ------------------------------------------------------------------

  // Options
  // ------------------------------------------------------------------
  radio_button** style = new radio_button*[3];
  style[statevect::FULL] = new radio_button(
      "FULL",
      "Vectors are displayed in (raw) full.",
      statevect::FULL
  );
  style[statevect::SINDEX] = new radio_button(
      "SPARSE_INDEX",
      "Vectors are displayed in sparse format, with state indexes.",
      statevect::SINDEX
  );
  style[statevect::SSTATE] = new radio_button(
      "SPARSE_STATE",
      "Vectors are displayed in sparse format, with states.",
      statevect::SSTATE
  );
  statevect::display_style = statevect::SINDEX; 
  em->addOption(
    MakeRadioOption("StatevectDisplayStyle",
      "Style to display a statedist, stateprobs, or statemsrs vector.",
      style, 3, statevect::display_style
    )
  );

  if (0==st) return false;

  // Functions
  // ------------------------------------------------------------------
  st->AddSymbol(  new gt_si                     );
  st->AddSymbol(  new ge_si                     );
  st->AddSymbol(  new lt_si                     );
  st->AddSymbol(  new le_si                     );

  st->AddSymbol(  new condition_si              );
  st->AddSymbol(  new prob_si                   );
  st->AddSymbol(  new expected_si(t_stateprobs) );
  st->AddSymbol(  new expected_si(t_statemsrs)  );

  return true;
}

