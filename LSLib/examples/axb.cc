
// $Id$

#include "lslib.h"
#include "timers.h"

#include "matrix.h"

#include <stdlib.h>
#include <iostream>
#include <iomanip>

using namespace std;

class my_LS_Matrix : public LS_Abstract_Matrix {
protected:
  LS_Matrix P;
public:
  my_LS_Matrix(LS_Matrix p) : LS_Abstract_Matrix(p.start, p.stop, p.stop) {
    P = p;
  }
  virtual ~my_LS_Matrix() { }
  virtual bool IsTransposed() const  { return P.is_transposed; }
  virtual void SolveRow(long r, const double *x, double& answer) const;
  virtual void SolveRow(long r, const float *x, double& answer) const;

  virtual void NoDiag_MultByRows(const float* x, double* y) const;
  virtual void NoDiag_MultByRows(const double* x, double* y) const;
  virtual void NoDiag_MultByCols(const float* x, double* y) const;
  virtual void NoDiag_MultByCols(const double* x, double* y) const;

  virtual void DivideDiag(double* x) const;
  virtual void DivideDiag(double* x, double scalar) const;

protected:
  template <class REAL>
  inline void MySolveRow(int r, const REAL* x, double& ans) const {
    const long* cstop = P.colindex + P.rowptr[r+1];
    const long* ci = P.colindex + P.rowptr[r];
    const float* v = P.f_value + P.rowptr[r];
    while (ci < cstop) {
      ans += x[ci[0]] * v[0];
      ci++;
      v++;
    }
    ans *= P.f_one_over_diag[r];
  }

  template <class REAL>
  inline void MyMultByRows(const REAL* x, double* y) const {
    const long* rp = P.rowptr;
    const long* rpstop = P.rowptr + P.stop;
    const long* ci = P.colindex + rp[0];
    const float* v = P.f_value + rp[0];
    // multiplication; matrix by rows
    while (rp < rpstop) {
        rp++;
        const long* cstop = P.colindex + rp[0];
        while (ci < cstop) {
          y[0] +=x[ci[0]] * v[0];
          ci++;
          v++;
        } // inner while
        y++;
    } // outer while
  }

  template <class REAL>
  inline void MyMultByCols(const REAL* x, double* y) const {
    const long* rp = P.rowptr;
    const long* rpstop = P.rowptr + P.stop;
    const long* ci = P.colindex + rp[0];
    const float* v = P.f_value + rp[0];
    // multiplication; matrix by columnns
    while (rp < rpstop) {
        rp++;
        const long* cstop = P.colindex + rp[0];
        while (ci < cstop) {
          y[ci[0]] +=x[0] * v[0];
          ci++;
          v++;
        } // inner while
        x++;
    } // outer while
  }
};

/* Methods */

void my_LS_Matrix::SolveRow(long r, const double* x, double& ans) const
{
  MySolveRow(r, x, ans);
}

void my_LS_Matrix::SolveRow(long r, const float* x, double& ans) const
{
  MySolveRow(r, x, ans);
}

void my_LS_Matrix::NoDiag_MultByRows(const float* x, double* y) const
{
  MyMultByRows(x, y);
}

void my_LS_Matrix::NoDiag_MultByRows(const double* x, double* y) const
{
  MyMultByRows(x, y);
}

void my_LS_Matrix::NoDiag_MultByCols(const float* x, double* y) const
{
  MyMultByCols(x, y);
}

void my_LS_Matrix::NoDiag_MultByCols(const double* x, double* y) const
{
  MyMultByCols(x, y);
}

void my_LS_Matrix::DivideDiag(double* x) const
{
  for (int i=0; i<size; i++) {
    x[i] *= P.f_one_over_diag[i];
  }
}

void my_LS_Matrix::DivideDiag(double* x, double scalar) const
{
  for (int i=0; i<size; i++) {
    x[i] *= P.f_one_over_diag[i] * scalar;
  }
}

/* End of methods */

sparse_matrix* off_diags;
int size;
float* one_over_diag;
LS_Matrix A;
my_LS_Matrix *AA;
LS_Vector b;
bool by_cols;
bool sparse_b;

void ReadSparseB(istream &s, int nnz)
{
  cerr << "Reading " << nnz << " non-zero elements of (sparse) b vector\n";
  long* bind = new long[nnz];
  float* bval = new float[nnz];
  for (int z=0; z<nnz; z++) {
    s >> bind[z];
    s >> bval[z];
  }
  b.size = nnz;
  b.index = bind;
  b.f_value = bval;
}

void ReadFullB(istream &s, int nnz)
{
  cerr << "Reading " << nnz << " non-zero elements of (truncated) b vector\n";
  int bsize = 8;
  int bp = 0;
  int btrunc = 0;
  float* bfull = (float*) malloc(bsize*sizeof(float));
  for (; bp<bsize; bp++) bfull[bp] = 0.0;
  for (int z=0; z<nnz; z++) {
    long index;
    s >> index;
    if (index > btrunc) btrunc = index;
    if (index >= bsize) {
      bsize = ((index/8)+1)*8;
      bfull = (float*) realloc(bfull, bsize*sizeof(float));
      if (NULL==bfull) {
        cerr << "Out of memory for b vector\n";
        exit(0);
      }
      for (; bp < bsize; bp++) bfull[bp] = 0.0;
    }
    s >> bfull[index];
  } // for z
  b.size = btrunc+1;
  b.index = NULL;
  b.f_value = bfull;
}

void ReadInput(istream &s)
{
  int numarcs;
  int bnnz;
  s >> bnnz;
  if (sparse_b) ReadSparseB(s, bnnz);
  else    ReadFullB(s, bnnz);
  s >> size;
  s >> numarcs;
  cerr << "Initializing " << size << " dimension ";
  if (by_cols) cerr << "(transposed) ";
  cerr << "linear system\n";
  off_diags = new sparse_matrix(size, numarcs, by_cols);
  one_over_diag = new float[size];
  for (int i=0; i<size; i++) one_over_diag[i] = 0.0;
  cerr << "Reading " << numarcs << " values\n";
  int lastr = 0;
  double rowsum = 0;
  for (int i=0; i<numarcs; i++) {
    int r, c;
    double v;
    s >> r;
    s >> c;
    s >> v;
    if (r != lastr) {
      if (rowsum)   one_over_diag[lastr] = 1.0 / rowsum;
      lastr = r;
      rowsum = 0;
    }
    if (r!=c) { 
      off_diags->AddElement(c, r, v);
      rowsum += v;
    }
  }
  if (rowsum)  one_over_diag[lastr] = 1.0 / rowsum;

  off_diags->ConvertToStatic();

  off_diags->ExportTo(A);
  A.f_one_over_diag = one_over_diag;
  A.d_one_over_diag = NULL;

  AA = new my_LS_Matrix(A);
}

void DumpInput(ostream &s)
{
  s << "Got b vector:\n";
  s << "[";
  if (b.index) {
    // sparse b vector
    for (int i=0; i<b.size; i++) {
      if (i) s << ", ";
      s << b.index[i] << ":" << b.f_value[i];
    }
  } else {
    // truncated full b vector
    s << b.f_value[0];
    for (int i=1; i<b.size; i++) s << ", " << b.f_value[i];
  }
  s << "]\nGot A matrix:\n";
  if (A.is_transposed) s << " Cols"; else s << " Rows";
  s << ": [" << A.rowptr[0];
  for (int i=1; i<=size; i++) s << ", " << A.rowptr[i];
  s << "]\n";
  int numarcs = A.rowptr[size];
  if (A.is_transposed) s << " Rows"; else s << " Cols";
  s << ": [" << A.colindex[0];
  for (int i=1; i<numarcs; i++) s << ", " << A.colindex[i];
  s << "]\n";
  s << "value: [" << A.f_value[0];
  for (int i=1; i<numarcs; i++) s << ", " << A.f_value[i];
  s << "]\n";
  s << "1/diags: [" << A.f_one_over_diag[0];
  for (int i=1; i<size; i++) s << ", " << A.f_one_over_diag[i];
  s << "]\n";
}

double CompareVector(istream &s, double* x)
{
  double maxerror = 0.0;
  for (int i=0; i<size; i++) {
    double value;
    s >> value;
    if (s.eof()) return -1.0;
    double d = x[i] - value;
    if (d<0) d = -d;
    if (value) d /= value;
    if (d > maxerror)  maxerror = d;
  }
  return maxerror;
}

int Usage(const char* name)
{
    cerr << "Usage: " << name << "\n\nSwitches:\n";
    cerr << "\t?: Print usage and exit\n";
    cerr << "\tr: Jacobi by rows\n";
    cerr << "\tj: Jacobi by vector-matrix multiply\n";
    cerr << "\tg: Gauss-Seidel (by rows)\n";
    cerr << "\ta: use abstract matrix\n";
    cerr << "\tc: store matrix by columns\n";
    cerr << "\tt: store vector as truncated full\n";
    cerr << "\ts: show solution vector\n";
    cerr << "\tw x: sets relaxation parameter (default 1.0)\n";
    cerr << "\te epsilon: sets precision\n";
    cerr << "\tn iters: sets maximum number of iterations (default 10000)\n";
    return 0;
}

int main(int argc, char** argv)
{
  LS_Options opts;
  char* name = argv[0];
  // process command line
  int ch;
  by_cols = false;
  sparse_b = true;
  bool use_abstract = false;
  bool show_solution = false;
  opts.max_iters = 10000;
  for (;;) {
    ch = getopt(argc, argv, "?rjgacstw:e:n:");
    if (ch<0) break;
    switch (ch) {
      case 'a':  
          use_abstract = true;
          break;

      case 'c':  
          by_cols = true;
          break;

      case 'r':
          opts.method = LS_Row_Jacobi;
          break;

      case 'j':
          opts.method = LS_Jacobi;
          break;

      case 'g':
          opts.method = LS_Gauss_Seidel;
          break;

      case 't':
          sparse_b = false;
          break;

      case 's':
          show_solution = true;
          break;

      case 'w':
          if (optarg) {
            double r = atof(optarg);
            if (r>0 && r<2) {
              opts.relaxation = r;
            }
          }
          break;

      case 'e':
          if (optarg) {
            double e = atof(optarg);
            if (e>0 && e<1) {
              opts.precision = e;
            }
          }
          break;

      case 'n':
          if (optarg) {
            int n = atoi(optarg);
            if (n>0) {
              opts.max_iters = n;
            }
          }
          break;

      default:
          return Usage(name);
    } // switch
  } // loop to process arguments

  cerr << LS_LibraryVersion() << "\n\n";
  cerr << "Reading diagonal-free, transposed Markov chain\n";
  ReadInput(cin);
#ifdef DEBUG
  DumpInput(cerr);
#endif
  if (opts.relaxation != 1.0) {
    opts.use_relaxation = 1;
    cerr << "Using relaxation parameter " << opts.relaxation << "\n";
  }
  if (use_abstract)   cerr << "Calling abstract solver library with ";
  else                cerr << "Calling explicit solver library with ";
  switch (opts.method) {
    case LS_Gauss_Seidel:
        cerr << "Gauss-Seidel\n";
        break;
    case LS_Row_Jacobi: 
        cerr << "row Jacobi\n";
        break;
    case LS_Jacobi: 
        cerr << "Jacobi\n";
        break;
    default:
        cerr << "Unknown solver\n";
        return 1;
  }
  // opts.debug = 1;
  // opts.float_vectors = 1;
  // opts.min_iters = 10;
  // opts.use_relative = true;

  double* x = new double[size];
  for (int i=0; i<size; i++) x[i] = 0;

  LS_Output out;

  timer watch;
  watch.Start();
  if (use_abstract)   Solve_Axb(*AA, x, b, opts, out);
  else                Solve_Axb(A, x, b, opts, out);
  watch.Stop();
  cerr << watch.User_Seconds() << " seconds\n";

  switch(out.status) {
    case LS_Success:
      cerr << "Took " << out.num_iters << " iterations\n";
      cerr << "Achieved " << out.precision << " precision\n";
      break;

    case LS_Wrong_Format:
      cerr << "Error, matrix wrong format\n";
      return 0;

    case LS_No_Convergence:
      cerr << "Did not converge.\nAchieved " << out.precision << " precision\n";
      break;

    case LS_Not_Implemented:
      cerr << "Error, not implemented\n";
      return 0;

    default:
      cerr << "Unknown return status.\n";
      return 0;
  }

  if (show_solution) {
    cout << setprecision(15);
    cout << "[" << x[0];
    for (int i=1; i<size; i++) cout << ", " << x[i];
    cout << "]\n";
  }

  cerr << "Comparing vector with input...\n";
  double me = CompareVector(cin, x);
  if (me<0) {
    cerr << "No solution vector in input\n";
  } else {
    cerr << "Maximum relative error: " << me << "\n";
  }

  return 0;
}

