
#include "multstrm.h"

bigmatrix::bigmatrix(int n)
{
  N = n;
  row = new unsigned int*[32*N];  // one per bit
  int i;
  for (i=32*N-1; i>=0; i--) 
    row[i] = new unsigned int[N];
}

bigmatrix::~bigmatrix()
{
  int i;
  for (i=32*N-1; i>=0; i--) delete[] row[i];
  delete[] row;
}

void bigmatrix::Show(OutputStream &s)
{
  int i,j;
  for (i=0; i<32*N; i++) {
    if (i) if (i%32==0) {
      s << " ";
      for (j=0; j<N; j++) s << "-----------";
      s << " \n";
      s.flush();
    }
    s << "[";
    for (j=0; j<N; j++) {
      if (j) s.Put(":");
      s.PutHex(row[i][j]);
    }
    s << "]\n";
    s.flush();
  }
}


