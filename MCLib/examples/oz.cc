
// $Id$

/*
    Really basic test file, more or less a sanity check
    that things are working correctly.
*/

#include "mclib.h"
#include "lslib.h"

#include <iostream>

using namespace std;

inline void showVector(double* p)
{
  cout << "[" << p[0] << ", " << p[1] << ", " << p[2] << "]";
}

int RunTests(bool storeByRows)
{
  using namespace MCLib;

  const int R = 0;
  const int N = 1;
  const int S = 2;

  cout << "Building OZ DTMC\n";

  Markov_chain* ozmc = startIrreducibleMC(true, 3, 8);

  ozmc->addEdge(R, R, 2);   // 1/2
  ozmc->addEdge(R, N, 1);   // 1/4
  ozmc->addEdge(R, S, 1);   // 1/4

  ozmc->addEdge(N, R, 1);   // 1/2
  ozmc->addEdge(N, S, 1);   // 1/2

  ozmc->addEdge(S, R, 1);   // 1/4
  ozmc->addEdge(S, N, 1);   // 1/4
  ozmc->addEdge(S, S, 2);   // 1/2

  cout << "Finishing OZ DTMC ";
  if (storeByRows)  cout << "(by rows)\n";
  else              cout << "(by columns)\n";

  Markov_chain::finish_options foo;
  Markov_chain::renumbering ren;
  foo.Store_By_Rows = storeByRows;
  ozmc->finish(foo, ren);

  if (ren.NoRenumbering()) {
    cout << "No renumbering required (as expected)\n";
  }

  if (ren.AbsorbRenumbering()) {
    cout << "Absorbing DTMC?  That's wrong...\n";
    return 1;
  }

  if (ren.GeneralRenumbering()) {
    cout << "General renumbering?  That's wrong...\n";
    return 1;
  }

  //
  // Ordinary transient analysis
  // 
  double p[3];
  p[R] = 0;
  p[N] = 1;
  p[S] = 0;

  Markov_chain::transopts to;
  to.kill_aux_vectors = false;

  cout << "\n";
  for (int t=0; t<10; t++) {
    cout << "Prob. vector for time " << t << ":  ";
    showVector(p);
    cout << "\n";
    ozmc->computeTransient(1, p, to); 
  }

  //
  // Reverse transient analysis
  //

  p[R] = 0;
  p[N] = 1;
  p[S] = 0;
  cout << "\n";
  for (int t=0; t<10; t++) {
    cout << "Reversed vector for time " << t << ":  ";
    showVector(p);
    cout << "\n";
    ozmc->reverseTransient(1, p, to); 
  }

  return 0;
}

void usage(const char* who)
{
  cout << "\nUsage: " << who << " [options]\n\n";
  cout << "    Switches:\n\n";
  cout << "\t-h\tThis help screen\n";
  cout << "\n";
  cout << "\t-c\tStore matrix `by columns' (default)\n";
  cout << "\t-r\tStore matrix `by rows'\n";
  cout << "\n";
}

int main(int argc, const char** argv)
{
  //
  // Process arguments
  //
  bool by_rows = false;
  for (int i=1; i<argc; i++) {
    if (argv[i][0] != '-' || argv[i][2] != 0) {
      usage(argv[0]);
      return 1;
    }
    switch (argv[i][1]) {
      case 'c': by_rows = false;
                continue;

      case 'r': by_rows = true;
                continue;

      case 'h': usage(argv[0]);
                return 0;

      default:  usage(argv[0]);
                return 1;
    }
  }

  //
  // Ok, now run everything
  // 
  try {
    return RunTests(by_rows);
  }
  catch (MCLib::error e) {
    cout << "Caught MCLib error: " << e.getString() << "\n";
    return 1;
  }
}
