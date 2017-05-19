
// $Id$

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


template <class MATRIX>
void show_matrix(const MATRIX& m)
{
  printf("is_transposed: %s\n", m.is_transposed ? "true" : "false");
  printf("num_rows: %ld\n", m.num_rows);
  show_array("rowptr", m.rowptr, m.num_rows+1);
  long ne = m.rowptr ? m.rowptr[m.num_rows] : 0;
  show_array("colindex", m.colindex, ne);
  show_array("value", (const long*) m.value, ne);
}

bool match(const generic_graph::const_matrix &A, const generic_graph::matrix &B)
{
  if (A.is_transposed != B.is_transposed) return false;
  if (A.num_rows != B.num_rows) return false;
  if (memcmp(A.rowptr, B.rowptr, (A.num_rows+1) * sizeof(long))) return false;
  long ne = A.rowptr ? A.rowptr[A.num_rows] : 0;
  if (memcmp(A.colindex, B.colindex, ne * sizeof(long))) return false;
  if (A.edge_size != B.edge_size) return false;
  if (memcmp(A.value, B.value, ne * A.edge_size)) return false;

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
  merged_weighted_digraph <long> G(true);
  merged_weighted_digraph <long> H(true);
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

  // Finish graphs
  merged_weighted_digraph <long>::finish_options fo;
  fo.Store_By_Rows = true;
  G.finish(fo);
  fo.Store_By_Rows = false;
  H.finish(fo);

  // Export graphs
  merged_weighted_digraph <long>::const_matrix cG, cH;
  G.exportFinished(cG);
  H.exportFinished(cH);

  // Build transposes
  merged_weighted_digraph <long>::matrix Gt, Ht;
  Gt.transposeFrom(cG);
  Ht.transposeFrom(cH);

  // Compare Ht and cG, should match
  if (!match(cG, Ht)) {
    printf("cG and Ht DO NOT match\n");
    printf("cG:\n");
    show_matrix(cG);
    printf("Ht:\n");
    show_matrix(Ht);
    return false;
  }

  // Compare Gt and cH, should match
  if (!match(cH, Gt)) {
    printf("Gt and cH DO NOT match\n");
    printf("Gt:\n");
    show_matrix(Gt);
    printf("cH:\n");
    show_matrix(cH);
    return false;
  }
  
  // 
  // Cleanup
  //
  Gt.destroy();
  Ht.destroy();
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
