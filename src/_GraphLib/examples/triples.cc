
/*
    Random testing for graph library
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "graphlib.h"
#include "rng.h"

using namespace GraphLib;

void usage(const char* who)
{
  printf("Usage: %s [-n replications] [-r rows] [-t triples] [-s seed]\n\n", who);

  // TBD - more

}

inline int grabIntArg(const char* arg)
{
  if (0==arg) throw 1;
  int X = atoi(arg);
  if (X<1) throw 1;
  return X;
}

inline int equilikely(rng_stream *rngs, int a, int b)
{
  return int(a+ rngs->Uniform32() * (b-a+1));
}

void show_array(const char* name, const long* A, long n)
{
  printf("%s: ", name);
  if (0==A) {
    printf("(null pointer)\n");
    return;
  }
  printf("[");
  if (n>0) {
    printf("%ld", A[0]);
    for (long i=1; i<n; i++) {
      printf(", %ld", A[i]);
    }
  }
  printf("]\n");
}


void show_matrix(const static_graph &M)
{
  printf("isByRows: %s\n", M.isByRows() ? "true" : "false");
  printf("#nodes: %ld\n", M.getNumNodes());
  printf("#edges: %ld\n", M.getNumEdges());
  show_array("RowPointer", M.RowPointer(), M.getNumNodes()+1);
  show_array("ColumnIndex", M.ColumnIndex(), M.getNumEdges());
  show_array("Labels", (const long*) M.Labels(), M.getNumEdges());
}

bool match(const GraphLib::static_graph &A, const GraphLib::static_graph &B)
{
  if (A.isByRows() != B.isByRows()) return false;
  if (A.getNumNodes() != B.getNumNodes()) return false;
  if (A.getNumEdges() != B.getNumEdges()) return false;
  if (A.EdgeBytes() != B.EdgeBytes()) return false;
  // compare arrays
  if (memcmp(A.RowPointer(), B.RowPointer(), 
    (A.getNumNodes()+1) * sizeof(long))) return false;
  if (memcmp(A.ColumnIndex(), B.ColumnIndex(), 
    A.getNumEdges() * sizeof(long))) return false;
  if (memcmp(A.Labels(), B.Labels(), 
    A.getNumEdges() * A.EdgeBytes())) return false;

  return true;
}

bool runTest(rng_stream* rngs, int rows, int triples)
{
  //
  // Build two weighted digraphs with integer edge types.
  // Add the same random edges to each.
  // Finish both, transpose one.
  // Export matrices.
  // Build transposes.
  // Cross check equivalent matrices.
  //

  // Initialize two digraphs
  dynamic_summable <long> G(true, true);
  dynamic_summable <long> H(true, true);
  G.addNodes(rows);
  H.addNodes(rows);

  // Add the same random edges to both
  for (int e=1; e<=triples; e++) {
    int i = equilikely(rngs, 0, rows-1);
    int j = equilikely(rngs, 0, rows-1);
  //  printf("(%d, %d, %d)\n", i, j, e);
    G.addEdge(i, j, e);
    H.addEdge(i, j, e);
  }

  H.transpose(0);

  //
  // Export graphs and build transposes
  //
  static_graph Gr, Hr, Gc, Hc;
  G.exportToStatic(Gr, 0);
  Gc.transposeFrom(Gr);
  H.exportToStatic(Hc, 0);
  Hr.transposeFrom(Hc);

  //
  // Compare 
  //
  bool showall = false;
  if (!match(Gr, Hr)) {
    printf("Gr and Hr DO NOT match\n");
    showall = true;
  }
  if (!match(Gc, Hc)) {
    printf("Gc and Hc DO NOT match\n");
    showall = true;
  }

  if (showall) {
    printf("Gr:\n");
    show_matrix(Gr);
    printf("Hr:\n");
    show_matrix(Hr);
    printf("Gc:\n");
    show_matrix(Gc);
    printf("Hc:\n");
    show_matrix(Hc);
    return false;
  }

  //
  // Everything matches
  //
  
  return true;
}

int main(int argc, const char** argv)
{
  int rows = 10;
  int triples = 20;
  int seed = 12345;
  int N = 10;

  try {
    for (int i=1; i<argc; i++) {
      if ('-' != argv[i][0])  throw 1;
      if (0 != argv[i][2])    throw 1;
      switch (argv[i][1]) {
        case 'n': 
                  N = grabIntArg(argv[i+1]);
                  i++;
                  continue;

        case 'r': 
                  rows = grabIntArg(argv[i+1]);
                  i++;
                  continue;

        case 's': 
                  seed = grabIntArg(argv[i+1]);
                  i++;
                  continue;

        case 't': 
                  triples = grabIntArg(argv[i+1]);
                  i++;
                  continue;

        default:  
                  throw 1;
      }
    }
  }
  catch (int code) {
    usage(argv[0]);
    return code;
  }

  // Done processing arguments
  // Initialize RNG stream

  printf("Testing %s\n", Version());
  printf("Generating %d random triples, in square matrix of dimension %d\n",
    triples, rows
  );
  printf("Initial seed %d\n", seed);

  rng_manager* RNGman = RNG_MakeStreamManager();
  rng_stream* rngs = RNGman->NewStreamFromSeed(seed);

  for (int i=0; i<N; i++) {
    if (!runTest(rngs, rows, triples)) break;
  }

  printf("%d tests passed\n", N);

  delete rngs;
  delete RNGman;

  return 0;
}
