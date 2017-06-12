
/*
  Test graph renumbering
*/

#include "graphlib.h"

#include <iostream>

// #define VERBOSE

const int sizes[]     = { 10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 0 };
const int outgoing[]  = {  4,  5,  6,  7,  8,  9, 10, 11, 12,  13, 0 };

inline void SWAP(long &a, long &b) 
{
  long tmp = a;
  a = b;
  b = tmp;
}

/*
    Simple RNG
*/

double Random()
{
  const long MODULUS = 2147483647L;
  const long MULTIPLIER = 48271L;
  const long Q = MODULUS / MULTIPLIER;
  const long R = MODULUS % MULTIPLIER;

  static long seed = 123456789;

  long t = MULTIPLIER * (seed % Q) - R * (seed / Q);
  if (t > 0) {
    seed = t;
  } else {
    seed = t + MODULUS;
  }
  return ((double) seed / MODULUS);
}

int Equilikely(int a, int b)
{
  return (a + (int) ((b - a + 1) * Random()));
}

/*
    Traversal to display a graph
*/

class graph_display : public GraphLib::BF_graph_traversal {
  public:
    graph_display(long ns) {
      current = 0;
      num_states = ns;
    }
    
    virtual bool hasNodesToExplore() {
      return current < num_states;
    }

    virtual long getNextToExplore() {
      return current++;
    }

    virtual bool visit(long src, long dest, const void*) {
      std::cout << "    " << src << " -> " << dest << "\n";
      return false;
    }
    
    void restart() {
      current = 0;
    }

  private:
    long current;
    long num_states;
};

/*
    Traversal to copy and renumber a graph
*/

class graph_renumber : public GraphLib::BF_graph_traversal {
  public:
    graph_renumber(GraphLib::dynamic_digraph& _G, long* _r) : G(_G) {
      current = 0;
      renumber = _r;
    }

    virtual bool hasNodesToExplore() {
      return current < G.getNumNodes();
    }

    virtual long getNextToExplore() {
      return current++;
    }

    virtual bool visit(long src, long dest, const void*) {
      G.addEdge(renumber[src], renumber[dest]);
      return false;
    }

  private:
    GraphLib::dynamic_digraph& G;
    long* renumber;
    long current;
};

/*
    Helpers
*/

using namespace GraphLib;

void build_random_graph(static_graph &G, long size, long edges_per)
{
  dynamic_digraph dg(true);

  dg.addNodes(size);

  for (long src=0; src<size; src++) {
    for (long z=0; z<edges_per; z++) {
      long dest = Equilikely(0, size-1);
      dg.addEdge(src, dest);
    }
  }

  dg.exportToStatic(G, 0);
}

long* build_shuffle(long size)
{
  long* A = new long[size];
  for (long i=0; i<size; i++) {
    A[i] = i;
  }
  // shuffle
  for (long i=0; i<size; i++) {
    long j = Equilikely(i, size-1);
    if (i!=j) {
      SWAP(A[i], A[j]);
    }
  }
  return A;
}

long* build_inverse(long* A, long size)
{
  long* B = new long[size];
  for (long i=0; i<size; i++) {
    B[A[i]] = i;
  }
  return B;
}

void show_array(const char* name, const long* A, long size)
{
  using namespace std;
  cout << name << ": [" << A[0];
  for (long i=1; i<size; i++) cout << ", " << A[i];
  cout << "]\n";
}

void show_graph(const char* name, const static_graph &G)
{
  using namespace std;
  cout << "Internal storage for " << name << ":\n";
  show_array("     row pointer", G.RowPointer(), G.getNumNodes()+1);
  show_array("    column index", G.ColumnIndex(), G.getNumEdges());
}

int main()
{
  using namespace std;

  for (int i=0; sizes[i]; i++) {
    cout << "Testing graph of size " << sizes[i];
    cout << " with " << outgoing[i] << " outgoing edges per node\n";

    //
    // Generate a random graph
    //

    static_graph first;
    build_random_graph(first, sizes[i], outgoing[i]);
#ifdef VERBOSE    
    cout << "Built first graph:\n";
    graph_display d(sizes[i]);
    first.traverse(d);
#endif

    //
    // Generate a random renumbering scheme and its inverse
    //

    long* forwd = build_shuffle(sizes[i]);
    long* back = build_inverse(forwd, sizes[i]);
#ifdef VERBOSE
    show_array("Built  forward", forwd, sizes[i]);
    show_array("Built backward", back, sizes[i]);
#endif

    //
    // Build a second graph by adding renumbered edges of the first graph
    // 

    dynamic_digraph dsec(true);  
    dsec.addNodes(sizes[i]);
    graph_renumber gr(dsec, forwd);
    first.traverse(gr);
    delete[] forwd;
    forwd = 0;

#ifdef VERBOSE
    cout << "Built second graph:\n";
    d.restart();
    dsec.traverse(d);
#endif

    //
    // Use the inverse renumbering on the second graph
    //
    array_renumberer ar(back);  //ar now owns back
    dsec.renumberNodes(ar);
    static_graph second;
    dsec.exportToStatic(second, 0);

#ifdef VERBOSE
    cout << "Renumbered second graph (should match first):\n";
    d.restart();
    second.traverse(d);
#endif

    //
    // Now, compare the graphs
    //
    if (first.isByRows() != second.isByRows()) {
      cout << "By rows mismatch!\n";
      return 1;
    }
    if (first.getNumNodes() != second.getNumNodes()) {
      cout << "Number of rows mismatch!\n";
      return 1;
    }
    if (first.getNumEdges() != second.getNumEdges()) {
      cout << "Number of edges mismatch!\n";
      return 1;
    }
    if (memcmp(first.RowPointer(), second.RowPointer(), 
               sizeof(long) * (first.getNumNodes()+1))) 
    {
      cout << "Row pointer array mismatch\n";
      show_graph("first graph", first);
      show_graph("second graph", second);
      return 1;
    }
    if (memcmp(first.ColumnIndex(), second.ColumnIndex(), 
               sizeof(long) * first.getNumEdges())) 
    {
      cout << "Column index array mismatch\n";
      show_graph("first graph", first);
      show_graph("second graph", second);
      return 1;
    }

    cout << ". . . . . . test passed\n";
  }


  return 0;
}
