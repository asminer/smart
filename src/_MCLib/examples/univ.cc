
/*
    Really basic test file, more or less a sanity check
    that things are working correctly.
    This one checks absorbing DTMCs.
*/

#include "mclib.h"
#include "lslib.h"

#include <iostream>

using namespace std;
using namespace Old_MCLib;

inline void showVector(double* p)
{
  cout << "[" << p[0];
  for (int i=1; i<6; i++) cout << ", " << p[i];
  cout << "]";
}

void forwardTransient(Markov_chain* mc, int start, int T)
{
  static Markov_chain::transopts to;
  double p[6];
  p[0] = p[1] = p[2] = p[3] = p[4] = p[5] = 0;
  p[start] = 1;

  for (int t=0; t<T; t++) {
    cout << "    time " << t << ":  ";
    showVector(p);
    cout << "\n";
    mc->computeTransient(1, p, to); 
  }
}

int RunTests(bool storeByRows)
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
  const int grad = -1;
  const int fail = -2;

  cout << "Building University DTMC\n";

  Markov_chain* univmc = startAbsorbingMC(true, 4, 2);

  univmc->addEdge(fr, fr, r);
  univmc->addEdge(fr, so, p);
  univmc->addEdge(fr, fail, q);

  univmc->addEdge(so, so, r);
  univmc->addEdge(so, jr, p);
  univmc->addEdge(so, fail, q);

  univmc->addEdge(jr, jr, r);
  univmc->addEdge(jr, sr, p);
  univmc->addEdge(jr, fail, q);

  univmc->addEdge(sr, sr, r);
  univmc->addEdge(sr, grad, p);
  univmc->addEdge(sr, fail, q);
  
  cout << "Finishing University DTMC ";
  if (storeByRows)  cout << "(by rows)\n";
  else              cout << "(by columns)\n";

  Markov_chain::finish_options foo;
  Markov_chain::renumbering ren;
  foo.Store_By_Rows = storeByRows;
  univmc->finish(foo, ren);

  if (ren.NoRenumbering()) {
    cout << "No renumbering?  That's wrong...\n";
    return 1;
  }

  if (ren.AbsorbRenumbering()) {
    cout << "Absorbing DTMC, as expected\n";
  }
  const int grad_ren = 4-(1+grad);
  //const int fail_ren = 4-(1+fail);

  if (ren.GeneralRenumbering()) {
    cout << "General renumbering?  That's wrong...\n";
    return 1;
  }

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
  forwardTransient(univmc, grad_ren, 2);

  //
  // Reverse transient analysis
  //
  static Markov_chain::transopts to;
  double x[6];
  x[0] = x[1] = x[2] = x[3] = x[4] = x[5] = 0;
  x[grad_ren] = 1;
  cout << "\nReverse analysis, for graduating students\n";
  for (int t=0; t<10; t++) {
    cout << "    vector for time " << t << ":  ";
    showVector(x);
    cout << "\n";
    univmc->reverseTransient(1, x, to); 
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
  catch (Old_MCLib::error e) {
    cout << "Caught MCLib error: " << e.getString() << "\n";
    return 1;
  }
}
