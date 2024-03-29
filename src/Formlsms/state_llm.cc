
#include "state_llm.h"
#include "../Streams/streams.h"
#include "../Options/options.h"
#include "../Options/optman.h"
#include "../ExprLib/startup.h"
#include "../ExprLib/exprman.h"
#include "../ExprLib/mod_vars.h"
#include "../Modules/biginttype.h"

// ******************************************************************
// *                                                                *
// *                      state_lldsm  methods                      *
// *                                                                *
// ******************************************************************

unsigned state_lldsm::int_display_order;
long state_lldsm::max_state_display = 0;
const char* state_lldsm::max_state_display_option = "MaxStateDisplay";
exprman* state_lldsm::reachset::em = 0;

state_lldsm::state_lldsm(model_type t) : lldsm(t)
{
  RSS = 0;
}

state_lldsm::~state_lldsm()
{
  Delete(RSS);
}

const char* state_lldsm::getClassName() const
{
  return "state_lldsm";
}

void state_lldsm::showStates(bool internal) const
{
  DCASSERT(RSS);
  if (internal) {
    RSS->showInternal(em->cout());
  } else {
    long num_states;
    RSS->getNumStates(num_states);
    if (tooManyStates(num_states, &(em->cout()))) return;

    shared_state* st = new shared_state(parent);
    RSS->showStates(em->cout(), stateDisplayOrder(), st);
    Delete(st);
  }
}
void state_lldsm::showStatesCOV(bool internal) const
{
  DCASSERT(CG);
  if (internal) {
  //  RSS->showInternal(em->cout());
  } else {
//long num_states;
  int num_states= CG->getNumState(CG,0);
  //  RSS->getNumStates(num_states);
    if (tooManyStates(num_states, &(em->cout()))) return;

  //  shared_state* st = new shared_state(parent);
    CG->getNumState(CG,1);
  //  Delete(st);
  }
}
bool state_lldsm::tooManyStates(long ns, OutputStream* os)
{
  if (ns>=0) {
    if ((0==max_state_display) || (ns <= max_state_display)) return false;
    if (os) {
      *os << "Too many states; to display, increase option ";
      *os << max_state_display_option << ".\n";
      os->flush();
    }
  } else {
    if (os) {
      *os << "Too many states.\n";
      os->flush();
    }
  }
  return true;
}

// ******************************************************************
// *                                                                *
// *               state_lldsm::state_visitor methods               *
// *                                                                *
// ******************************************************************

state_lldsm::state_visitor::state_visitor(const hldsm* m)
 : x(traverse_data::Compute)
{
  DCASSERT(m);
  x.current_state = new shared_state(m);
}

state_lldsm::state_visitor::~state_visitor()
{
  Nullify(x.current_state);
}

// ******************************************************************
// *                                                                *
// *                 state_lldsm::reachset  methods                 *
// *                                                                *
// ******************************************************************

state_lldsm::reachset::reachset()
{
  parent = 0;
}

state_lldsm::reachset::~reachset()
{
}

void state_lldsm::reachset::attachToParent(state_lldsm* p)
{
  DCASSERT(p);
  DCASSERT(0==parent);
  parent = p;
}

StateLib::state_db* state_lldsm::reachset::getStateDatabase() const
{
  return 0;
}

void state_lldsm::reachset::getNumStates(result &ns) const
{
  long lns;
  getNumStates(lns);
  if (lns>=0) {
    ns.setPtr(new bigint(lns));
  } else {
    ns.setNull();
  }
}


void state_lldsm::reachset::getBounds(result &ns, std::vector<int> set_of_places) const
{
  long lns = 0;
  getBounds(lns, set_of_places);
  if (lns>=0) {
    ns.setPtr(new bigint(lns));
  } else {
    ns.setNull();
  }
}

void state_lldsm::reachset
::showStates(OutputStream &os, display_order ord, shared_state* st)
{
  iterator& I = iteratorForOrder(ord);

  long i = 0;
  for (I.start(); I; I++, i++) {
    os << "State " << i << ": ";
    I.copyState(st);
    showState(os, st);
    os << "\n";
    os.flush();
  }
}

void state_lldsm::reachset
::visitStates(state_lldsm::state_visitor &v, display_order ord)
{
  DCASSERT(v.state());

  iterator& I = iteratorForOrder(ord);

  for (I.start(); I; I++) {
    v.index() = I.index();
    if (v.canSkipIndex()) continue;
    I.copyState(v.state());
    if (v.visit()) return;
  }
}

void state_lldsm::reachset::visitStates(state_lldsm::state_visitor &v) const
{
  DCASSERT(v.state());

  iterator& I = easiestIterator();

  for (I.start(); I; I++) {
    v.index() = I.index();
    if (v.canSkipIndex()) continue;
    I.copyState(v.state());
    if (v.visit()) return;
  }
}

bool state_lldsm::reachset::Print(OutputStream &s, int width) const
{
  // Required for shared object, but will we ever call it?
  s << "reachset (why is it printing?)";
  return true;
}

bool state_lldsm::reachset::Equals(const shared_object* o) const
{
  return (this == o);
}


state_lldsm::reachset::iterator::iterator()
{
}

state_lldsm::reachset::iterator::~iterator()
{
}


// ******************************************************************
// *                                                                *
// *                                                                *
// *                         Initialization                         *
// *                                                                *
// *                                                                *
// ******************************************************************

class init_statellm : public initializer {
  public:
    init_statellm();
    virtual bool execute();
};
init_statellm the_statellm_initializer;

init_statellm::init_statellm() : initializer("init_statellm")
{
  usesResource("em");
}

bool init_statellm::execute()
{
  if (0==em) return false;

  state_lldsm::reachset::em = em;

  state_lldsm::max_state_display = 100000000;
  state_lldsm::int_display_order = state_lldsm::NATURAL;

  // set up options
  // ------------------------------------------------------------------

  if (em->OptMan()) {

    em->OptMan()->addIntOption(
      state_lldsm::max_state_display_option,
      "The maximum number of states to display for a model.  If 0, the states will be displayed whenever possible, regardless of number.",
      state_lldsm::max_state_display,
      0, 1000000000
    );

    // ------------------------------------------------------------------
    option* sdo = em->OptMan()->addRadioOption("StateDisplayOrder",
      "The order to use for displaying states in functions show_states and show_arcs. This does not affect the internal storage of the states, so the reordering is done as necessary only for display.",
      state_lldsm::num_display_orders, state_lldsm::int_display_order
    );

    sdo->addRadioButton(
      "DISCOVERY",
      "States are displayed in the order in which they are discovered (or defined), if possible.",
      state_lldsm::DISCOVERY
    );
    sdo->addRadioButton(
      "LEXICAL",
      "States are sorted by lexical order.",
      state_lldsm::LEXICAL
    );
    sdo->addRadioButton(
      "NATURAL",
      "States are displayed in the most natural order for the selected state space data structure.",
      state_lldsm::NATURAL
    );
  }

  return true;
}
