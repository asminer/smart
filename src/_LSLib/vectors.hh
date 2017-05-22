
#ifndef VECTORS_HH
#define VECTORS_HH

// Handy vector functions

inline void CopyToAuxOrSwap(float* xold, const double* x, long start, long stop)
{
  for (stop--; stop >= start; stop--) {
    xold[stop] = x[stop];
  }
}

inline void CopyToAuxOrSwap(double* &xold, double* &x, long, long)
{
  double* tmp = xold;
  xold = x;
  x = tmp;
}

// Internal vector classes

// ******************************************************************
// *                Internal sparse vector  template                *
// ******************************************************************

template <class REAL>
struct LS_Sparse_Vector {
  long size;
  const long* index;
  const REAL* value;
  // for convenience
  inline void Fill(const LS_Vector &v);
  inline void FillOthers(const LS_Vector &v) {
    size = v.size;
    index = v.index;
  }
  inline void FirstIndex(long &p, long start) const {
    for (p=0; p<size; p++) {
      if (index[p] >= start) return;
    }
  }
  inline double GetNegValue(long &p, long s) const {
    if (p<size) if (s == index[p]) {
      return -value[p++];
    }
    return 0;
  }
  inline void CopyNegativeToFull(double* x, long start, long stop) const {
    for (long i=0; i<size; i++) {
      if (index[i] >= stop) return;
      if (index[i] < start) continue;
      x[index[i]] = -value[i];
    }
  }
};

template<>
void LS_Sparse_Vector<double>::Fill(const LS_Vector &v)
{
  FillOthers(v);
  value = v.d_value;
}

template<>
void LS_Sparse_Vector<float>::Fill(const LS_Vector &v)
{
  FillOthers(v);
  value = v.f_value;
}

// ******************************************************************
// *            Internal truncated full vector  template            *
// ******************************************************************

template <class REAL>
struct LS_Full_Vector {
  long size;
  const REAL* value;
  // for convenience
  inline void Fill(const LS_Vector &v);
  inline void FillOthers(const LS_Vector &v) {
    size = v.size;
  }
  inline void FirstIndex(long &, long) const {
    // NO-OP
  }
  inline double GetNegValue(long &, long s) const {
    if (s < size) return -value[s];
    return 0;
  }
  inline void CopyNegativeToFull(double *x, long start, long stop) const {
    long istop = MIN(size, stop);
    for (long i=MAX(long(0), start); i<istop; i++) {
      x[i] = -value[i];
    }
  }
};

template<>
void LS_Full_Vector<double>::Fill(const LS_Vector &v)
{
  FillOthers(v);
  value = v.d_value;
}

template<>
void LS_Full_Vector<float>::Fill(const LS_Vector &v)
{
  FillOthers(v);
  value = v.f_value;
}

#endif
