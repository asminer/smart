
// $Id$

#include "lslib.h"
#include "timerlib.h"

#include "matrix.h"

#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <iomanip>
#include <fstream>

using namespace std;

template <class MATRIX>
class my_Matrix : public LS_Generic_Matrix {
protected:
  MATRIX &A;
public:
  my_Matrix(MATRIX &p) : LS_Generic_Matrix(p.start, p.stop, p.size), A(p) {
    one_over_diag = A.one_over_diag;
  }
  virtual ~my_Matrix() { }

  virtual void MatrixVectorMultiply(double *y, const float *x) const {
    A.MatrixVectorMultiply(y, x);
  }
  virtual void MatrixVectorMultiply(double *y, const double *x) const {
    A.MatrixVectorMultiply(y, x);
  }

  virtual void RowDotProduct(long i, const float* x, double &sum) const {
    A.RowDotProduct(i, x, sum);
  }

  virtual void RowDotProduct(long i, const double* x, double &sum) const {
    A.RowDotProduct(i, x, sum);
  }

};


sparse_matrix* off_diags;
int size;
float* one_over_diag;
LS_CRS_Matrix_float rA;
LS_CCS_Matrix_float cA;
LS_Generic_Matrix *AA;
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

  off_diags->ExportTo(rA);
  off_diags->ExportTo(cA);
  if (by_cols) {
    rA.one_over_diag = 0;
    cA.one_over_diag = one_over_diag;
    AA = new my_Matrix <LS_CCS_Matrix_float>(cA);
  } else {
    rA.one_over_diag = one_over_diag;
    cA.one_over_diag = 0;
    AA = new my_Matrix <LS_CRS_Matrix_float>(rA);
  }
}

/*
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
*/

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
  char* file = 0;
  ifstream* infile = 0;
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
  if (optind < argc) {
    file = argv[optind];
    optind++;
  }
  if (optind < argc) {
    return Usage(name);
  }

  cerr << LS_LibraryVersion() << "\n\n";
  cerr << "Reading diagonal-free, transposed Markov chain from ";
  if (file) {
    cerr << "`" << file << "'\n";
    infile = new ifstream(file);
    if (!(*infile)) {
      cerr << "Couldn't open file\n";
      return 1;
    }
    ReadInput(*infile);
  } else {
    cerr << "standard input\n";
    ReadInput(cin);
  }
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
  if (use_abstract)   Solve_Axb(*AA, x, b, opts, out);
  else if (by_cols)   Solve_Axb(cA, x, b, opts, out);
  else                Solve_Axb(rA, x, b, opts, out);
  cerr << watch.elapsed_seconds() << " seconds\n";

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
  double me = infile ? CompareVector(*infile, x) : CompareVector(cin, x);
  if (me<0) {
    cerr << "No solution vector in input\n";
  } else {
    cerr << "Maximum relative error: " << me << "\n";
  }

  return 0;
}

