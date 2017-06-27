
/*
    Really basic test file, more or less a sanity check
    that things are working correctly.
    This one checks absorbing DTMCs.
*/

#include "mclib.h"
#include "lslib.h"

#include <iostream>

using namespace std;
using namespace MCLib;

inline void showVector(double* p)
{
  cout << "[" << p[0];
  for (int i=1; i<6; i++) cout << ", " << p[i];
  cout << "]";
}

void forwardTransient(const Markov_chain &mc, int start, int T)
{
  static Markov_chain::DTMC_transient_options to;
  double p[6];
  p[0] = p[1] = p[2] = p[3] = p[4] = p[5] = 0;
  p[start] = 1;

  for (int t=0; t<T; t++) {
    cout << "    time " << t << ":  ";
    showVector(p);
    cout << "\n";
    mc.computeTransient(1, p, to); 
  }
}

int RunTests()
{
  // probabilities
  const int p = 8;
  const int r = 1;
  const int q = 1;

  // states
  const int fr = 0;
  const int so = 1;
  const int jr = 2;
  const int sr = 3;
  const int grad = 4;
  const int fail = 5;

  cout << "Building University DTMC\n";

  GraphLib::dynamic_summable<double> G(true, true);
  G.addNodes(6);

  G.addEdge(fr, fr, r);
  G.addEdge(fr, so, p);
  G.addEdge(fr, fail, q);

  G.addEdge(so, so, r);
  G.addEdge(so, jr, p);
  G.addEdge(so, fail, q);

  G.addEdge(jr, jr, r);
  G.addEdge(jr, sr, p);
  G.addEdge(jr, fail, q);

  G.addEdge(sr, sr, r);
  G.addEdge(sr, grad, p);
  G.addEdge(sr, fail, q);
  
  cout << "Finishing University DTMC ";

  GraphLib::abstract_classifier* ac = G.determineSCCs(0, 1, true, 0);
  GraphLib::static_classifier C;
  GraphLib::node_renumberer* ren = ac->buildRenumbererAndStatic(C);
  DCASSERT(ren);
  if (ren->changes_something()) {
    cout << "Non-identity node renumbering?\n";
    return 1;
  }
  delete ren;
  Markov_chain univmc(true, G, C, 0);

  //
  // Ordinary transient analysis
  // 

  cout << "\nStarting from freshman year:\n";
  forwardTransient(univmc, fr, 10);

  cout << "\nStarting from sophomore year:\n";
  forwardTransient(univmc, so, 10);

  cout << "\nStarting from junior year:\n";
  forwardTransient(univmc, jr, 10);

  cout << "\nStarting from senior year:\n";
  forwardTransient(univmc, sr, 10);

  cout << "\nStarting from graduated:\n";
  forwardTransient(univmc, grad, 2);

  //
  // Reverse transient analysis
  //
  static Markov_chain::DTMC_transient_options to;
  double x[6];
  x[0] = x[1] = x[2] = x[3] = x[4] = x[5] = 0;
  x[grad] = 1;
  cout << "\nReverse analysis, for graduating students\n";
  for (int t=0; t<10; t++) {
    cout << "    vector for time " << t << ":  ";
    showVector(x);
    cout << "\n";
    univmc.reverseTransient(1, x, to); 
  }
  return 0;
}


int main()
{
  //
  // Ok, now run everything
  // 
  try {
    return RunTests();
  }
  catch (MCLib::error e) {
    cout << "Caught MCLib error: " << e.getString() << "\n";
    return 1;
  }
}
