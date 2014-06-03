
// $Id$

#include "lslib.h"

#include <iostream>
#include <fstream>

using namespace std;

// #define DEBUG_IO

// #define DEBUG_VMMULT

int lineno;

int Usage(const char* name)
{
  cerr << "Usage: " << name << " [file]\n\n";
  cerr << "Reads a discrete phase type random variable from a file (or stdin)\n";
  cerr << "(an initial vector and a matrix), and computes its distribution\n";
  cerr << "(the pdf) using a series of vector-matrix multiplications and\n";
  cerr << "displaying the last element.\n\n";
  cerr << "File format (whitespace ignored, # to ignore rest of line):\n";
  cerr << "\tMatrix dimension\n";
  cerr << "\t[v, v, v, v] # Initial vector\n";
  cerr << "\tNumber of matrix edges\n";
  cerr << "\tRow r:\n";
  cerr << "\t\tcol : value\n";
  cerr << "\t\tcol : value\n";
  cerr << "\tRow r:\n";
  cerr << "\t...\n";
  cerr << "\tEnd\n";
  cerr << "\t(rows should be in order)\n\n";
  return 0;
}

void ignoreToEol(istream &s)
{
  for (;;) {
    char c = s.get();
    if ('\n' == c) {
      lineno++;
      return;
    }
    if (!s) return;
  }
}

char skipUntil(istream &s, const char* expected, const char* error)
{
  for (;;) {
    char c = s.get();
    if (' ' == c) continue;
    if ('\t' == c) continue;
    if ('\n' == c) {
      lineno++;
      continue;
    }
    if ('\r' == c) continue;
    if ('#' == c) {
      ignoreToEol(s);
      continue;
    }
    for (int i=0; expected[i]; i++) {
      if (expected[i] == c) {
        s.unget();
        return c;
      }
    }
    // Bad character
    throw error;
  }
}

long readInt(istream &s)
{
  skipUntil(s, "0123456789", "Expected integer");
  long N;
  s >> N;
  return N;
}

double readReal(istream &s)
{
  skipUntil(s, "0123456789", "Expected real");
  double D;
  s >> D;
  return D;
}

void readLbrak(istream &s)
{
  skipUntil(s, "[", "Expecting `['");
  s.get();
}

void readRbrak(istream &s)
{
  skipUntil(s, "]", "Expecting `]'");
  s.get();
}

void readComma(istream &s)
{
  skipUntil(s, ",", "Expecting `,`");
  s.get();
}

void readColon(istream &s)
{
  skipUntil(s, ":", "Expecting `:`");
  s.get();
}

void readCol(istream &s)
{
  skipUntil(s, "C", "Expecting `Col'");
  s.get();
  if (s.get() != 'o') throw "Expecting `Col'";
  if (s.get() != 'l') throw "Expecting `Col'";
#ifdef DEBUG_IO
  cerr << "Consumed `Col'\n";
#endif
}

void readEnd(istream &s)
{
  skipUntil(s, "E", "Expecting `End'");
  s.get();
  if (s.get() != 'n') throw "Expecting `End'";
  if (s.get() != 'd') throw "Expecting `End'";
}

template <typename T>
inline void showVector(ostream &s, const char* name, const T *v, long n)
{
  s << name << ": [" << v[0];
  for (int i=1; i<n; i++) s << ", " << v[i];
  s << "]\n";
}

void parseInput(istream &s, LS_Matrix &A, double* &initial)
{
  lineno = 1;
#ifdef DEBUG_IO
  cerr << "Reading matrix dimension\n";
#endif
  long N = readInt(s);
#ifdef DEBUG_IO
  cerr << "Reading initial vector of size " << N << "\n";
#endif
  initial = new double[N];
  readLbrak(s);
  initial[0] = readReal(s);
  for (long i=1; i<N; i++) {
    readComma(s);
    initial[i] = readReal(s);
  }
  readRbrak(s);
  //
  // Normalize initial vector
  //
  double total = 0.0;
  for (long i=0; i<N; i++) {
    total += initial[i];
  }
  if (total) for (long i=0; i<N; i++) {
    initial[i] /= total;
  }
#ifdef DEBUG_IO
  cerr << "Got initial vector\n";
  cerr << "Reading #matrix edges\n";
#endif
  long E = readInt(s);
#ifdef DEBUG_IO
  cerr << "Reading " << E << " edges\n";
#endif
  //
  // Set up A
  //
  A.is_transposed = false;
  A.start = 0;
  A.stop = N;
  long* rp = new long[N+1];
  long* ci = new long[E];
  float* fv = new float[E];
  A.rowptr = rp;
  A.colindex = ci;
  A.f_value = fv;
  A.d_value = 0;
  A.f_one_over_diag = 0;
  A.d_one_over_diag = 0;
  // 
  // Read the actual elements
  //
  long colindex = -1;
  for (long e=0; e<E; ) {
   
    char next = skipUntil(s, "C0123456789", "Expecting `Col', or row integer");
    if ('C' == next) {
      // New row!
      readCol(s);
      long c = readInt(s);
      readColon(s);
      if (c < colindex) throw "Columns not in order";
      if (c < 0 || c>=N) throw "Column out of range";
      while (colindex < c) {
        ++colindex;
        rp[colindex] = e;
      }
#ifdef DEBUG_IO
      cerr << "  new col " << colindex << "\n";
#endif
      continue;
    }
    // Edge
    long r = readInt(s);
    readColon(s);
    double v = readReal(s);

    if (r<0 || r>=N) throw "Row out of range";
#ifdef DEBUG_IO
    cerr << "  edge " << colindex << ", " << r << ", " << v << "\n";
#endif
    ci[e] = r;
    fv[e] = v;
    ++e;
  } // for e
  readEnd(s);
  // And, get the final row pointers set
  while (colindex<N) {
    ++colindex;
    rp[colindex] = E;
  }
#ifdef DEBUG_IO
  cerr << "Done reading matrix\n";
  showVector(cerr, "rp", rp, N+1);
  showVector(cerr, "ci", ci, E);
  showVector(cerr, "fv", fv, E);
#endif
}

inline void clearVector(double* x, int n)
{
  for (int i=0; i<n; i++) x[i] = 0;
}

int main(int argc, char** argv)
{
  if (argc > 2) return 1+Usage(argv[0]);

  LS_Matrix A;
  double* x;

  try {
    if (2 == argc) {
      ifstream inf(argv[1]);
      if (!inf) {
        cerr << "Couldn't open file " << argv[1] << "\n";
        return 2;
      }
      parseInput(inf, A, x);
    } else {
      parseInput(cin, A, x);
    }
  }
  catch (const char* err) {
    cerr << "Error near line " << lineno << ": " << err << "\n";
    return 1;
  }

  double* y = new double[A.stop];

  cout << "#Time Prob\n";
  for (int t=0; ; t++) {
    cout << t << "  " << x[A.stop-1] << "\n";
    //
    // determine what's left in x
    //
    double acc = 0.0;
    for (long i=0; i<A.stop-1; i++) acc += x[i];
    if (acc < 1e-6) break;

    //
    // keep going
    //
    clearVector(y, A.stop);
    A.VectorMatrixMultiply(y, x);
#ifdef DEBUG_VMMULT
    cerr << "Got y vector [" << y[0];
    for (int i=1; i<A.stop; i++) cerr << ", " << y[i];
    cerr << "]\n";
#endif
    double* tmp = x;
    x = y;
    y = tmp;
  }

  return 0;
}
