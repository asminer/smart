
/*
    Really basic test file, more or less a sanity check
    that things are working correctly.
    This one checks ergodic DTMCs.
*/

#include "mclib.h"
#include "lslib.h"

#include <iostream>

using namespace std;
using namespace MCLib;

inline void showVector(double* p)
{
  cout << "[" << p[0] << ", " << p[1] << ", " << p[2] << "]";
}

void forwardTransient(const Markov_chain &ozmc, int start)
{
  static Markov_chain::DTMC_transient_options to;
  double p[3];
  p[0] = p[1] = p[2] = 0;
  p[start] = 1;

  for (int t=0; t<10; t++) {
    cout << "    Prob. vector for time " << t << ":  ";
    showVector(p);
    cout << "\n";
    ozmc.computeTransient(1, p, to); 
  }
}

int RunTests()
{
  const int R = 0;
  const int N = 1;
  const int S = 2;

  cout << "Building OZ DTMC\n";

  GraphLib::dynamic_summable<double> G(true, true);
  G.addNodes(3);

  G.addEdge(R, R, 2);   // 1/2
  G.addEdge(R, N, 1);   // 1/4
  G.addEdge(R, S, 1);   // 1/4

  G.addEdge(N, R, 1);   // 1/2
  G.addEdge(N, S, 1);   // 1/2

  G.addEdge(S, R, 1);   // 1/4
  G.addEdge(S, N, 1);   // 1/4
  G.addEdge(S, S, 2);   // 1/2

  cout << "Finishing OZ DTMC\n";

  GraphLib::abstract_classifier* ac = G.determineSCCs(0, 1, true, 0);
  GraphLib::static_classifier C;
  GraphLib::node_renumberer* ren = ac->buildRenumbererAndStatic(C);
  DCASSERT(ren);
  if (ren->changes_something()) {
    cout << "Non-identity node renumbering?\n";
    return 1;
  }
  delete ren;
  Markov_chain ozmc(true, G, C, 0);


  //
  // Ordinary transient analysis
  // 

  // from "rain"

  cout << "\nStarting from state \"rain\":\n";
  forwardTransient(ozmc, R);

  // from "nice"

  cout << "\nStarting from state \"nice\":\n";
  forwardTransient(ozmc, N);

  // from "snow"

  cout << "\nStarting from state \"snow\":\n";
  forwardTransient(ozmc, S);

  //
  // Reverse transient analysis
  //

  static Markov_chain::DTMC_transient_options to;
  double p[3];
  p[R] = 0;
  p[N] = 1;
  p[S] = 0;
  cout << "\nReverse analysis, reaching state \"nice\"\n";
  for (int t=0; t<10; t++) {
    cout << "    vector for time " << t << ":  ";
    showVector(p);
    cout << "\n";
    ozmc.reverseTransient(1, p, to); 
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
