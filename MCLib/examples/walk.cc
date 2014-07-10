

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

void VectorShow(double* x, int n)
{
  cout << "[" << x[0];
  for (int i=1; i<n; i++) cout << ", " << x[i];
  cout << "]";
}

void Compare(double* general, double* theory, int n)
{
  double maxrelerr = 0.0;
  int badspot = -1;
  for (int i=0; i<n; i++) {
    double delta = theory[i] - general[i];
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
    cout << "\tGeneral: " << general[badspot] << "\n";
  }
}


void DoEverything()
{
  int N;
  double lambda;
  double mu;
  cout << Version() << "\n\n";
  cout << "Random walk " << DTMC << " example.\nEnter distance to target (integer): ";
  cin >> N;
  cout << "\nEnter walk left rate: ";
  cin >> lambda;
  cout << "\nEnter walk right rate: ";
  cin >> mu;
  
  cout << "\nCreating " << DTMC << "...\n";
  Markov_chain* mymc = startAbsorbingMC(discrete, 2*N+1, 2);
  if (0==mymc) {
    cout << "Couldn't create absorbing chain\n";
    return;
  }

  cout << "Adding edges...\n";
  mymc->addEdge(0, -1, lambda);  // -1 is the left absorbing state
  mymc->addEdge(2*N, -2, mu);  // -2 is the right absorbing state
  for (int i=0; i<2*N; i++) {
    mymc->addEdge(i+1, i, lambda);
    mymc->addEdge(i, i+1, mu);
  }
  
#ifdef DEBUG
  cout << "Markov chain by rows:\n";
  RowShow(mymc);
  cout << "Markov chain by columns:\n";
  ColShow(mymc);
#endif

  cout << "Finishing " << DTMC << "...\n";
  Markov_chain::finish_options fopts;
  Markov_chain::renumbering ren;
  mymc->finish(fopts, ren);

  cout << "Finished chain:\n";
  if (mymc->isDiscrete())   cout << "\tdiscrete\n";
  else                      cout << "\tcontinuous\n";
  switch (mymc->getType()) {
    case Markov_chain::Absorbing:
        cout << "\tabsorbing\n";
        break;
    case Markov_chain::Error_type:
        cout << "\terror\n";
        break;
    default:
        cout << "\tsome other type\n";
        break;
  } 
  cout << "\t#Transient states: " << mymc->getNumTransient() << "\n";
  cout << "\t#Absorbing states: " << mymc->getNumAbsorbing() << "\n";

#ifdef DEBUG
  cout << "Markov chain by rows:\n";
  RowShow(mymc);
  cout << "Markov chain by columns:\n";
  ColShow(mymc);
#endif

  double* p = new double[mymc->getNumStates()];
  LS_Vector init; 
  long index = N;
  float f_value = 1.0;
  init.size = 1;
  init.index = &index;
  init.d_value = 0;
  init.f_value = &f_value;
  LS_Options opt;
  LS_Output out;
  opt.method = LS_Gauss_Seidel;
  // opt.relaxation = 1.1;
  // opt.use_relaxation = 1;
  opt.precision = 1e-8;
  
  cout << "Computing TTA and steady state: general\n";
  mymc->computeSteady(init, p, opt, out);  
  mymc->computeTTA(init, p, opt, out);  
  cout << "\t" << out.num_iters << " iterations\n";
  cout << "\t" << out.precision << " precision achieved\n";
  
#ifdef DEBUG
  cout << "Got general vector: ";
  VectorShow(p, 2*N+3);
  cout << "\n";
#endif

  cout << "Computing TTA and steady state: theory\n";
  double* lambdan = new double[N+2];
  double* mun = new double[N+2];
  double* Sn = new double[N+2];
  lambdan[0] = 1;
  mun[0] = 1;
  Sn[0] = 1;
  for (int i=1; i<N+2; i++) {
    lambdan[i] = lambda * lambdan[i-1];
    mun[i] = mu * mun[i-1];
    Sn[i] = lambda * Sn[i-1] + mun[i];
  }

  double* theory = new double[mymc->getNumStates()];
  double denom = lambdan[N+1] + mun[N+1];
  for (int i=0; i<N; i++) 
    theory[i] = lambdan[N-i] * Sn[i] / denom;
  theory[N] = Sn[N] / denom;
  for (int i=0; i<N; i++)
    theory[2*N-i] = mun[N-i] * Sn[i] / denom;
  theory[2*N+1] = lambdan[N+1] / denom;  // left absorbing prob
  theory[2*N+2] = mun[N+1] / denom;  // right absorbing prob

#ifdef DEBUG
  cout << "Got theoretical vector: ";
  VectorShow(theory, 2*N+3);
  cout << "\n";
#endif

  Compare(p, theory, mymc->getNumStates());
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
