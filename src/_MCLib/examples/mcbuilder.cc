
/*
    Common code for lots of the test programs.
*/

/*
    Tests for discrete "tta" distributions.
*/

#include <iostream>

#include "mcbuilder.h"

using namespace GraphLib;
using namespace std;
using namespace MCLib;

// =======================================================================

my_timer::my_timer(bool activ) 
{
  active = activ;
}

my_timer::~my_timer() 
{
}

void my_timer::start(const char* w) 
{
  if (active) {
    cout << "    " << w << " ...";
    cout.flush();
  }
}

void my_timer::stop() 
{
  if (active) {
    cout << "\n";
    cout.flush();
  }
}

// =======================================================================

Markov_chain* build_double(const bool discrete, const edge graph[], 
  const long num_nodes, bool verbose)
{
  my_timer T(verbose);

  //
  // Build graph from list of edges
  //
  dynamic_summable<double>* G = new dynamic_summable<double>(true, true);
  G->addNodes(num_nodes);

  for (long i=0; graph[i].from >= 0; i++) {
    G->addEdge(graph[i].from, graph[i].to, graph[i].rate);
    if (verbose) {
      cout << "\t" << graph[i].from << " -> " << graph[i].to;
      cout << " rate " << graph[i].rate << "\n";
    }
  }

  //
  // Construct Markov chain
  //
  abstract_classifier* SCCs = G->determineSCCs(0, 1, true, &T);
  static_classifier C;
  node_renumberer* R = SCCs->buildRenumbererAndStatic(C);

  if (verbose) {
    if (0==R) {
      cout << "Null renumbering:\n";
    } else {
      cout << "Got renumbering:\n";
      for (long i=0; i<num_nodes; i++) {
        cout << "    " << i << " -> " << R->new_number(i) << "\n";
      }
    }
  
    cout << "Got static classifier:\n";
    cout << "    #classes: " << C.getNumClasses() << "\n";
    for (long c=0; c<C.getNumClasses(); c++) {
      cout << "    class " << c << ": ";
      if (C.firstNodeOfClass(c) <= C.lastNodeOfClass(c)) {
        cout << " [" << C.firstNodeOfClass(c) << ".." << C.lastNodeOfClass(c) << "]";
      } else {
        cout << " []";
      }
      cout << " size " << C.sizeOfClass(c) << "\n";
    }
  }

  Markov_chain* MC = new Markov_chain(discrete, *G, C, &T);
  delete R;
  delete SCCs;
  delete G;
  return MC;
}

// =======================================================================

Markov_chain* build_float(const bool discrete, const edge graph[], 
  const long num_nodes, bool verbose)
{
  my_timer T(verbose);

  //
  // Build graph from list of edges
  //
  dynamic_summable<float>* G = new dynamic_summable<float>(true, true);
  G->addNodes(num_nodes);

  for (long i=0; graph[i].from >= 0; i++) {
    G->addEdge(graph[i].from, graph[i].to, graph[i].rate);
    if (verbose) {
      cout << "\t" << graph[i].from << " -> " << graph[i].to;
      cout << " rate " << graph[i].rate << "\n";
    }
  }

  //
  // Construct Markov chain
  //
  abstract_classifier* SCCs = G->determineSCCs(0, 1, true, &T);
  static_classifier C;
  node_renumberer* R = SCCs->buildRenumbererAndStatic(C);

  if (verbose) {
    if (0==R) {
      cout << "Null renumbering:\n";
    } else {
      cout << "Got renumbering:\n";
      for (long i=0; i<num_nodes; i++) {
        cout << "    " << i << " -> " << R->new_number(i) << "\n";
      }
    }
    
    cout << "Got static classifier:\n";
    cout << "    #classes: " << C.getNumClasses() << "\n";
    for (long c=0; c<C.getNumClasses(); c++) {
      cout << "    class " << c << ": ";
      if (C.firstNodeOfClass(c) <= C.lastNodeOfClass(c)) {
        cout << " [" << C.firstNodeOfClass(c) << ".." << C.lastNodeOfClass(c) << "]";
      } else {
        cout << " []";
      }
      cout << " size " << C.sizeOfClass(c) << "\n";
    }
  }

  Markov_chain* MC = new Markov_chain(discrete, *G, C, &T);
  delete R;
  delete SCCs;
  delete G;
  return MC;
}

// =======================================================================
