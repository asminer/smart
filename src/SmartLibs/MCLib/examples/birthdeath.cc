

// $Id$

#include "mclib.h"
#include "lslib.h"

#include <iostream>

// #define DEBUG

const bool discrete = false;

const char* DTMC = (discrete) ? "DTMC" : "CTMC";

using namespace std;
using namespace MCLib;

#ifdef DEBUG
class show_entries : public generic_graph::element_visitor {
  bool show_target;
public:
  show_entries(bool st) : element_visitor() { show_target = st; }
  virtual bool visit(long from, long to, void* wt) {
    float* rate = (float*) wt;
    if (show_target)   cout << "\tto state " << to;
    else               cout << "\tfrom state " << from;
    cout << " value " << rate[0] << "\n";
    return false;
  }
};

void RowShow(Markov_chain* mc)
{
  show_entries foo(true);
  for (int i=0; i<mc->getNumStates(); i++) {
    cout << "From state " << i << ":\n";
    mc->traverseFrom(i, foo);
  }
}

void ColShow(Markov_chain* mc)
{
  show_entries foo(false);
  for (int i=0; i<mc->getNumStates(); i++) {
    cout << "To state " << i << ":\n";
    mc->traverseTo(i, foo);
  }
}
#endif

void DoEverything()
{
  int N;
  double lambda;
  double mu;
  cout << Version() << "\n\n";
  cout << "Birth death " << DTMC << " example.\nEnter number of states: ";
  cin >> N;
  cout << "\nEnter birth rate lambda: ";
  cin >> lambda;
  cout << "\nEnter death rate mu: ";
  cin >> mu;
  
  cout << "\nCreating " << DTMC << "...\n";
  Markov_chain* mymc = startIrreducibleMC(discrete, N, 2*(N-1));

  cout << "Adding edges...\n";
  for (int i=1; i<N; i++) {
    mymc->addEdge(i, i-1, mu);
    mymc->addEdge(i-1, i, lambda);
  }
  
#ifdef DEBUG
  cout << "Markov chain by rows:\n";
  RowShow(mymc);
  cout << "Markov chain by columns:\n";
  ColShow(mymc);
#endif

  cout << "Finishing " << DTMC << "...\n";
  Markov_chain::finish_options bar;
  Markov_chain::renumbering ren;
  mymc->finish(bar, ren);

  cout << "Finished chain:\n";
  if (mymc->isDiscrete())   cout << "\tdiscrete\n";
  else                      cout << "\tcontinuous\n";
  switch (mymc->getType()) {
      case Markov_chain::Irreducible:
          cout << "\tirreducible\n";
          break;
      case Markov_chain::Error_type:
          cout << "\terror\n";
          break;
      default:
          cout << "\tsome other type\n";
          break;
  } 
  cout << "\t#Recurrent classes: " << mymc->getNumClasses() << "\n";
  cout << "\tSize of recurrent class 1: " << mymc->getRecurrentSize(1) << "\n";
  mymc->computePeriodOfClass(1);
  cout << "\tPeriod of recurrent class 1: " << mymc->getPeriodOfClass(1) << "\n";
  cout << "\tStorage takes " << mymc->ReportMemTotal() << " bytes\n";

#ifdef DEBUG
  cout << "Markov chain by rows:\n";
  RowShow(mymc);
  cout << "Markov chain by columns:\n";
  ColShow(mymc);
#endif

  double* p = new double[mymc->getNumStates()];
  LS_Vector init;  // unused
  LS_Options opt;
  LS_Output out;
  opt.method = LS_Gauss_Seidel;
  // opt.debug = 1;
  // opt.relaxation = 1.1;
  // opt.use_relaxation = 1;
  opt.precision = 1e-7;
  
  cout << "Computing steady state: general\n";
  mymc->computeSteady(init, p, opt, out);  
  cout << "\t" << out.num_iters << " iterations\n";
  cout << "\t" << out.precision << " precision achieved\n";

  cout << "Computing steady state: theory\n";
  double* theory = new double[mymc->getNumStates()];
  if (lambda < mu) {
    theory[0] = 1.0;
    for (int i=1; i<mymc->getNumStates(); i++)
      theory[i] = theory[i-1] * (lambda/mu);
  } else {
    theory[mymc->getNumStates()-1] = 1.0;
    for (int i=mymc->getNumStates()-2; i>=0; i--)
      theory[i] = theory[i+1] * (mu/lambda);
  }
  double total = 0.0;
  for (int i=0; i<mymc->getNumStates(); i++) total += theory[i];
  for (int i=0; i<mymc->getNumStates(); i++) theory[i] /= total;

  // compare the two
  double maxrelerr = 0.0;
  int badspot = -1;
  for (int i=0; i<mymc->getNumStates(); i++) {
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
  delete mymc;
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
