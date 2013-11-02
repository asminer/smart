
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
  my_LS_Matrix(LS_Matrix p) : LS_Abstract_Matrix(p.stop) {
    P = p;
  }
  virtual ~my_LS_Matrix() { }
  virtual bool IsTransposed() const  { return P.is_transposed; }
  virtual long SolveRow(long r, const double *x, double& answer) const;
  virtual long SolveRow(long r, const float *x, double& answer) const;

  virtual void NoDiag_MultByRows(const float* x, double* y) const;
  virtual void NoDiag_MultByRows(const double* x, double* y) const;
  virtual void NoDiag_MultByCols(const float* x, double* y) const;
  virtual void NoDiag_MultByCols(const double* x, double* y) const;

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

long my_LS_Matrix::SolveRow(long r, const double* x, double& ans) const
{
  MySolveRow(r, x, ans);
  return r+1;
}

long my_LS_Matrix::SolveRow(long r, const float* x, double& ans) const
{
  MySolveRow(r, x, ans);
  return r+1;
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

void my_LS_Matrix::DivideDiag(double* x, double scalar) const
{
  for (int i=0; i<size; i++) {
    x[i] *= P.f_one_over_diag[i] * scalar;
  }
}

/* End of methods */

sparse_matrix* off_diags;
int size;
// float* diag;
float* one_over_diag;
LS_Matrix A;
my_LS_Matrix *AA;
bool by_cols;

void ReadInput(istream &s)
{
  int numarcs;
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
    d /= value;
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
    cerr << "\tp: Power method\n";
    cerr << "\ta: use abstract matrix\n";
    cerr << "\tc: store matrix by columns\n";
    cerr << "\ts: show solution vector\n";
    cerr << "\tw x: sets relaxation parameter (default 1.0)\n";
    cerr << "\te epsilon: sets precision\n";
    cerr << "\tn iters: sets maximum number of iterations (default 10000)\n";
    cerr << "\td iters: show precision achieved after this many iterations\n";
    return 0;
}

int main(int argc, char** argv)
{
  LS_Options opts;
  char* name = argv[0];
  // process command line
  int ch;
  by_cols = false;
  bool use_abstract = false;
  bool show_solution = false;
  int deltaiters = 1000000;
  int totaliters = 10000;
  for (;;) {
    ch = getopt(argc, argv, "?rjgpacsw:e:n:d:");
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

      case 'p':
          opts.method = LS_Power;
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
              totaliters = n;
            }
          }
          break;

      case 'd':
          if (optarg) {
            int n = atoi(optarg);
            if (n>0) {
              deltaiters = n;
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
  cerr << "Got matrix:\n";
  DumpInput(cerr);
#endif
  if (opts.relaxation != 1.0) {
    opts.use_relaxation = 1;
    cerr << "Using relaxation parameter " << opts.relaxation << "\n";
  }
  if (use_abstract)  cerr << "Calling abstract solver library with ";
  else      cerr << "Calling explicit solver library with ";
  switch (opts.method) {
    case LS_Gauss_Seidel:
        cerr << "Gauss-Seidel\n";
        break;
    case LS_Row_Jacobi: 
        cerr << "row Jacobi\n";
        break;
    case LS_Jacobi: 
        cerr << "with Jacobi\n";
        break;
    case LS_Power: 
        cerr << "with Power method\n";
        break;
    default:
        cerr << "Unknown solver\n";
        return 1;
  }
  // opts.debug = 1;
  // opts.float_vectors = 1;
  // opts.use_relative = true;

  double* x = new double[size];
  for (int i=0; i<size; i++) x[i] = 1.0 / size;

  LS_Output out;

  timer watch;
  watch.Start();
  int iters_so_far = 0;
  opts.max_iters = deltaiters;
  while (totaliters > 0) {
    if (totaliters < deltaiters) opts.max_iters = totaliters;
    if (opts.min_iters > opts.max_iters) opts.min_iters = opts.max_iters;
    totaliters -= deltaiters;
    if (use_abstract)   Solve_AxZero(AA, x, opts, out);
    else                Solve_AxZero(A, x, opts, out);
    iters_so_far += out.num_iters;
    if (out.status == LS_Success) break;
    iters_so_far--;
    cerr << "After " << iters_so_far << " iterations, achieved ";
    cerr << out.precision << " precision\n";
  }
  watch.Stop();
  cerr << watch.User_Seconds() << " seconds\n";

  switch(out.status) {
    case LS_Success:
        cerr << "Took " << iters_so_far << " iterations\n";
        cerr << "Achieved " << out.precision << " precision\n";
        break;

    case LS_Wrong_Format:
        cerr << "Error, matrix wrong format\n";
        return 0;

    case LS_No_Convergence:
        cerr << "Did not converge.\n";
        cerr << "Achieved " << out.precision << " precision\n";
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

