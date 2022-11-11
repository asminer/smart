
#include "stoch_llm.h"
#include "../Streams/streams.h"
#include "../Options/options.h"
#include "../ExprLib/startup.h"
#include "../ExprLib/exprman.h"
#include "../ExprLib/mod_vars.h"
#include "../Modules/biginttype.h"

// ******************************************************************
// *                                                                *
// *                    stochastic_lldsm methods                    *
// *                                                                *
// ******************************************************************

stochastic_lldsm::stochastic_lldsm(model_type t) : graph_lldsm(t)
{
  accept_prob = -1;
  trap_prob = -1;
  mtta = -2;
  vtta = -2;

  PROC = 0;
}

stochastic_lldsm::~stochastic_lldsm()
{
  Delete(PROC);
}

void stochastic_lldsm::showProc(bool internal) const
{
  DCASSERT(PROC);
  if (internal) {
    PROC->showInternal(em->cout());
  } else {
    reachgraph::show_options opts;
    opts.ORDER = graph_lldsm::stateDisplayOrder();
    opts.STYLE = graph_lldsm::graphDisplayStyle();
    opts.NODE_NAMES = graph_lldsm::displayGraphNodeNames();
    opts.RG_ONLY = false;
    shared_state* st = new shared_state(parent);
    DCASSERT(RSS);
    PROC->showProc(em->cout(), opts, RSS, st);
    Delete(st);
  }
}

bool stochastic_lldsm::isFairModel() const
{
  return true;
}

void stochastic_lldsm::showClasses() const
{
  DCASSERT(PROC);
  shared_state* st = new shared_state(parent);
  DCASSERT(RSS);
  PROC->showClasses(em->cout(), RSS, st);
  Delete(st);
}

/*
void stochastic_lldsm::getClass(long cl, intset &) const
{
  bailOut(__FILE__, __LINE__, "Can't find class of states");
}
*/

/*
bool stochastic_lldsm::isTransient(long st) const
{
  bailOut(__FILE__, __LINE__, "Can't check transient");
  return false;
}
*/

/*
statedist* stochastic_lldsm::getInitialDistribution() const
{
  bailOut(__FILE__, __LINE__, "Can't get initial distribution");
  return 0;
}
*/

// ******************************************************************
// *                                                                *
// *               stochastic_lldsm::process  methods               *
// *                                                                *
// ******************************************************************

exprman* stochastic_lldsm::process::em = 0;

stochastic_lldsm::process::process()
{
  parent = 0;
}

stochastic_lldsm::process::~process()
{
  // DON'T delete parent
}

void stochastic_lldsm::process::attachToParent(stochastic_lldsm* p, LS_Vector &, state_lldsm::reachset*)
{
  DCASSERT(p);
  DCASSERT(0==parent);
  parent = p;
}

long stochastic_lldsm::process::getTrapState() const
{
  return -1;
}

void stochastic_lldsm::process::getNumClasses(result &na) const
{
  long lna;
  getNumClasses(lna);
  if (lna>=0) {
    na.setPtr(new bigint(lna));
  } else {
    na.setNull();
  }
}

long stochastic_lldsm::process::getOutgoingWeights(long from, long* to, double* w, long n) const
{
  parent->bailOut(__FILE__, __LINE__, "Can't get outgoing weights");
  return 0;
}

bool stochastic_lldsm::process::computeTransient(double t, double*, double*, double*) const
{
  parent->bailOut(__FILE__, __LINE__, "Can't compute transient");
  return false;
}

bool stochastic_lldsm::process::computeAccumulated(double t, const double*, double*, double*, double*) const
{
  parent->bailOut(__FILE__, __LINE__, "Can't compute accumulated");
  return false;
}

bool stochastic_lldsm::process::computeSteadyState(double* probs) const
{
  parent->bailOut(__FILE__, __LINE__, "Can't compute steady-state");
  return false;
}

bool stochastic_lldsm::process::computeTimeInStates(const double*, double*) const
{
  parent->bailOut(__FILE__, __LINE__, "Can't compute time in states");
  return false;
}

bool stochastic_lldsm::process::computeClassProbs(const double*, double*) const
{
  parent->bailOut(__FILE__, __LINE__, "Can't compute class probabilities");
  return false;
}

bool stochastic_lldsm::process
::randomTTA(rng_stream &, long &, const stateset*, long, long &)
{
  parent->bailOut(__FILE__, __LINE__, "Can't simulate discrete-time random walk");
  return false;
}

bool stochastic_lldsm::process
::randomTTA(rng_stream &, long &, const stateset*, double, double &)
{
  parent->bailOut(__FILE__, __LINE__, "Can't simulate continuous-time random walk");
  return false;
}




long stochastic_lldsm::process::getAcceptingState() const
{
  return -1;
}

bool stochastic_lldsm::process::computeDiscreteTTA(double, long,
  discrete_pdf &) const
{
  parent->bailOut(__FILE__, __LINE__, "Can't compute discrete TTA");
  return false;
}

bool stochastic_lldsm::process::computeContinuousTTA(double, double, long,
  discrete_pdf &) const
{
  parent->bailOut(__FILE__, __LINE__, "Can't compute continuous TTA");
  return false;
}

bool stochastic_lldsm::process::reachesAccept(double*) const
{
  parent->bailOut(__FILE__, __LINE__, "Can't compute stateprobs for reaching acceptance");
  return false;
}

bool stochastic_lldsm::process::reachesAcceptBy(double, double*) const
{
  parent->bailOut(__FILE__, __LINE__, "Can't compute stateprobs for reaching acceptance by fixed time");
  return false;
}

bool stochastic_lldsm::process::Print(OutputStream &s, int width) const
{
  // Required for shared object, but will we ever call it?
  s << "stochastic process (why is it printing?)";
  return true;
}

bool stochastic_lldsm::process::Equals(const shared_object* o) const
{
  return (this == o);
}

void stochastic_lldsm::process::showError(const char* s)
{
  if (em->startError()) {
    em->causedBy(0);
    em->cerr() << s;
    em->stopIO();
  }
}


// ******************************************************************
// *                                                                *
// *                                                                *
// *                         Initialization                         *
// *                                                                *
// *                                                                *
// ******************************************************************

class init_stochllm : public initializer {
  public:
    init_stochllm();
    virtual bool execute();
};
init_stochllm the_stochllm_initializer;

init_stochllm::init_stochllm() : initializer("init_stochllm")
{
  usesResource("em");
}

bool init_stochllm::execute()
{
  if (0==em) return false;

  stochastic_lldsm::process::em = em;

  // TBD - any options go here
  return true;
}

