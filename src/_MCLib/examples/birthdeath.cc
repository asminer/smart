
#include "mclib.h"
#include "lslib.h"

#include <iostream>

const bool discrete = false;

const char* DTMC = (discrete) ? "DTMC" : "CTMC";

using namespace std;
using namespace MCLib;

void DoEverything()
{
  int N;
  double lambda;
  double mu;
  cout << "Birth death " << DTMC << " example.\nEnter number of states: ";
  cin >> N;
  cout << "\nEnter birth rate lambda: ";
  cin >> lambda;
  cout << "\nEnter death rate mu: ";
  cin >> mu;
  
  cout << "\nCreating " << DTMC << "...\n";
  GraphLib::dynamic_summable<double> G(false, true);
  G.addNodes(N);

  cout << "Adding edges...\n";
  for (int i=1; i<N; i++) {
    G.addEdge(i, i-1, mu);
    G.addEdge(i-1, i, lambda);
  }
  

  cout << "Finishing " << DTMC << "...\n";

  GraphLib::static_classifier C;

  GraphLib::abstract_classifier* ac = G.determineSCCs(0, 1, true, 0);
  if (0==ac) {
    cout << "Null classifier!\n";
    return;
  }
  GraphLib::node_renumberer* R = ac->buildRenumbererAndStatic(C);
  if (0==R) {
    cout << "Null renumberer!\n";
    return;
  }
  // Nodes shouldn't need to be renumbered
  if (R->changes_something()) {
    cout << "Non-identity node renumbering!\n";
    return;
  }

  delete R;
  delete ac;

  Markov_chain MC(discrete, G, C, 0);

  cout << "Finished chain:\n";
  cout << "\t#classes: " << C.getNumClasses() << "\n";
  for (long c=0; c<C.getNumClasses(); c++) {
    cout << "\tSize of class " << c << ": " << C.sizeOfClass(c) << "\n";
    if (0==C.sizeOfClass(c)) continue;
    cout << "\tPeriod of class " << c << ": " << MC.computePeriodOfClass(c) << "\n";
  }
  cout << "\tStorage takes " << MC.getMemTotal() << " bytes\n";

  double* p = new double[MC.getNumStates()];

  const long index[] = { 0 };
  const double prob[] = { 1.0 };
  LS_Vector init;  // unused
  init.size = 1;
  init.index = index;
  init.f_value = 0;
  init.d_value = prob;

  LS_Options opt;
  LS_Output out;
  opt.method = LS_Gauss_Seidel;
  // opt.debug = 1;
  // opt.relaxation = 1.1;
  // opt.use_relaxation = 1;
  opt.precision = 1e-7;
  
  cout << "Computing steady state: general\n";
  MC.computeInfinityDistribution(init, p, opt, out);  
  cout << "\t" << out.num_iters << " iterations\n";
  cout << "\t" << out.precision << " precision achieved\n";

  cout << "Computing steady state: theory\n";
  double* theory = new double[MC.getNumStates()];
  if (lambda < mu) {
    theory[0] = 1.0;
    for (int i=1; i<MC.getNumStates(); i++)
      theory[i] = theory[i-1] * (lambda/mu);
  } else {
    theory[MC.getNumStates()-1] = 1.0;
    for (int i=MC.getNumStates()-2; i>=0; i--)
      theory[i] = theory[i+1] * (mu/lambda);
  }
  double total = 0.0;
  for (int i=0; i<MC.getNumStates(); i++) total += theory[i];
  for (int i=0; i<MC.getNumStates(); i++) theory[i] /= total;

  // compare the two
  double maxrelerr = 0.0;
  int badspot = -1;
  for (int i=0; i<MC.getNumStates(); i++) {
    double delta = theory[i] - p[i];
    if (0.0 == delta) continue;
    delta /= theory[i];
    if (delta<0) delta = -delta;
    if (delta > maxrelerr) {
      maxrelerr = delta;
      badspot = i;
    }
  }
  if (badspot<0) cout << "Theory and general match perfectly\n";
  else {
    cout << "Worst relative difference:\n";
    cout << "\t" << maxrelerr << " error at element " << badspot << "\n";
    cout << "\tTheory:  " << theory[badspot] << "\n";
    cout << "\tGeneral: " << p[badspot] << "\n";
  }

  cout << "\nCleaning up\n";
  delete[] p;
  delete[] theory;
  cout << "Done\n";
}

int main()
{
  try {
    DoEverything();
  }
  catch (MCLib::error e) {
    cout << "Got MCLib error: " << e.getString() << "\n";
  }
  return 0;
}
