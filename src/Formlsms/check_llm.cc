
// $Id$

#include "check_llm.h"
#include "../Streams/streams.h"
#include "../Options/options.h"
#include "../ExprLib/exprman.h"

int checkable_lldsm::graph_display_style;
bool checkable_lldsm::display_graph_node_names;
long checkable_lldsm::max_arc_display = 100000000;
const char* MAX_ARC_DISPLAY_OPTION = "MaxArcDisplay";

// ******************************************************************
// *                                                                *
// *                    checkable_lldsm  methods                    *
// *                                                                *
// ******************************************************************

checkable_lldsm::checkable_lldsm(model_type t) : lldsm(t)
{
  RSS = 0;
  RGR = 0;
}

checkable_lldsm::~checkable_lldsm()
{
  Delete(RSS);
  Delete(RGR);
}

bool checkable_lldsm::tooManyArcs(long na, bool show)
{
  if (na>=0) {
    if ((0==max_arc_display) || (na <= max_arc_display)) return false;
    if (!show) return true;
    em->cout() << "Too many arcs; to display, increase option ";
    em->cout() << MAX_ARC_DISPLAY_OPTION << ".\n";
  } else {
    if (!show) return true;
    em->cout() << "Too many arcs.\n";
  }
  em->cout().flush();
  return true;
}

void checkable_lldsm::getNumArcs(result& x) const
{
  x.setInt(getNumArcs());
  if (x.getInt() < 0) {
    x.setNull();
  } 
}

long checkable_lldsm::getNumArcs() const
{
  return bailOut(__FILE__, __LINE__, "Can't count arcs");
}

void checkable_lldsm::showArcs(bool internal) const
{
  bailOut(__FILE__, __LINE__, "Can't dispaly arcs (or process)");
}

void checkable_lldsm::showInitial() const
{
  bailOut(__FILE__, __LINE__, "Can't show initial state(s)");
}

void checkable_lldsm::countPaths(const intset&, const intset&, result& c)
{
  c.setNull();
  bailOut(__FILE__, __LINE__, "Can't count paths");
}

bool checkable_lldsm::requireByRows(const named_msg*)
{
  return false;
}

bool checkable_lldsm::requireByCols(const named_msg*)
{
  return false;
}

long checkable_lldsm::getOutgoingEdges(long from, ObjectList <int> *e) const
{
  bailOut(__FILE__, __LINE__, "Can't get outgoing edges");
  return -1;
}

long checkable_lldsm::getIncomingEdges(long from, ObjectList <int> *e) const
{
  bailOut(__FILE__, __LINE__, "Can't get incoming edges");
  return -1;
}

bool checkable_lldsm::getOutgoingCounts(long* a) const
{
  bailOut(__FILE__, __LINE__, "Can't get outgoing edge counts");
  return false;
}

bool checkable_lldsm::dumpDot(OutputStream &s) const
{
  return false;
}

void checkable_lldsm::getReachable(result &ss) const 
{
  bailOut(__FILE__, __LINE__, "Can't get reachable states");
  ss.setNull();
}

void checkable_lldsm::getInitialStates(result &x) const
{
  bailOut(__FILE__, __LINE__, "Can't get initial states");
  x.setNull();
}

void checkable_lldsm::getAbsorbingStates(result &x) const
{
  bailOut(__FILE__, __LINE__, "Can't get absorbing states");
  x.setNull();
}

void checkable_lldsm::getDeadlockedStates(result &x) const
{
  bailOut(__FILE__, __LINE__, "Can't get deadlocked states");
  x.setNull();
}

void checkable_lldsm::getPotential(expr* p, result &ss) const 
{
  bailOut(__FILE__, __LINE__, "Can't get potential states");
  ss.setNull();
}

bool checkable_lldsm::isFairModel() const
{
  return false;
}

void checkable_lldsm::getTSCCsSatisfying(stateset &p) const
{
  DCASSERT(isFairModel());
  bailOut(__FILE__, __LINE__, "Can't get TSCCs satisfying p");
}

void checkable_lldsm::findDeadlockedStates(stateset &) const
{
  bailOut(__FILE__, __LINE__, "Can't find deadlocked states");
}

bool checkable_lldsm::forward(const intset &p, intset &r) const
{
  bailOut(__FILE__, __LINE__, "Can't compute forward set");
  return false;
}

bool checkable_lldsm::backward(const intset &p, intset &r) const
{
  bailOut(__FILE__, __LINE__, "Can't compute backward set");
  return false;
}

bool checkable_lldsm::isAbsorbing(long st) const
{
  bailOut(__FILE__, __LINE__, "Can't check absorbing");
  return false;
}

bool checkable_lldsm::isDeadlocked(long st) const
{
  bailOut(__FILE__, __LINE__, "Can't check deadlocked");
  return false;
}


// ******************************************************************
// *                                                                *
// *               checkable_lldsm::reachset  methods               *
// *                                                                *
// ******************************************************************

checkable_lldsm::reachset::reachset()
{
  parent = 0;
}

checkable_lldsm::reachset::~reachset()
{
}

void checkable_lldsm::reachset::getNumStates(result &ns) const
{
  long lns;
  getNumStates(lns);
  if (lns>=0) {
    ns.setInt(lns);
  } else {
    ns.setNull();
  }
}

void checkable_lldsm::reachset
::showStates(OutputStream &os, int display_order, shared_state* st)
{
  DCASSERT(parent);

  long num_states;
  getNumStates(num_states);

  if (num_states<=0) return;
  if (parent->tooManyStates(num_states, true)) return;

  iterator& I = iteratorForOrder(display_order);

  long i = 0;
  for (I.start(); I; I++, i++) {
    os << "State " << i << ": ";
    I.copyState(st);
    showState(os, st);
    os << "\n";
    os.flush();
  }
}

void checkable_lldsm::reachset
::visitStates(lldsm::state_visitor &v, int visit_order)
{
  DCASSERT(v.state());

  iterator& I = iteratorForOrder(visit_order);

  for (I.start(); I; I++) {
    v.index() = I.index();
    if (v.canSkipIndex()) continue;
    I.copyState(v.state());
    if (v.visit()) return;
  }
}

void checkable_lldsm::reachset::visitStates(lldsm::state_visitor &v) const
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

bool checkable_lldsm::reachset::Print(OutputStream &s, int width) const
{
  // Required for shared object, but will we ever call it?
  s << "reachset (why is it printing?)";
  return true;
}

bool checkable_lldsm::reachset::Equals(const shared_object* o) const
{
  return (this == o);
}


checkable_lldsm::reachset::iterator::iterator()
{
}

checkable_lldsm::reachset::iterator::~iterator()
{
}


// ******************************************************************
// *                                                                *
// *              checkable_lldsm::reachgraph  methods              *
// *                                                                *
// ******************************************************************

checkable_lldsm::reachgraph::reachgraph()
{
}

checkable_lldsm::reachgraph::~reachgraph()
{
}

bool checkable_lldsm::reachgraph::Print(OutputStream &s, int width) const
{
  // Required for shared object, but will we ever call it?
  s << "reachgraph (why is it printing?)";
  return true;
}

bool checkable_lldsm::reachgraph::Equals(const shared_object* o) const
{
  return (this == o);
}

// **************************************************************************
// *                                                                        *
// *                               Front  end                               *
// *                                                                        *
// **************************************************************************


void InitializeCheckableLLM(exprman* om)
{
  if (0==om) return;

  // ------------------------------------------------------------------
  om->addOption(
    MakeIntOption(
      MAX_ARC_DISPLAY_OPTION,
      "The maximum number of arcs to display for a model.  If 0, the graph will be displayed whenever possible, regardless of the number of arcs.",
      checkable_lldsm::max_arc_display,
      0, 1000000000
    )
  );

  // ------------------------------------------------------------------
  radio_button** gds_list = new radio_button*[checkable_lldsm::num_graph_display_styles];
  gds_list[checkable_lldsm::DOT] = new radio_button(
      "DOT", 
      "Graphs are displayed in a format compatible with the graph visualization tool \"dot\".",
      checkable_lldsm::DOT
  );
  gds_list[checkable_lldsm::INCOMING] = new radio_button(
      "INCOMING", 
      "Graphs are displayed by listing the incoming edges for each node.",
      checkable_lldsm::INCOMING
  );
  gds_list[checkable_lldsm::OUTGOING] = new radio_button(
      "OUTGOING", 
      "Graphs are displayed by listing the outgoing edges for each node.",
      checkable_lldsm::OUTGOING
  );
  gds_list[checkable_lldsm::TRIPLES] = new radio_button(
      "TRIPLES", 
      "Graphs are displayed by listing edges as triples FROM TO INFO, where INFO is any edge information (e.g., the rate).",
      checkable_lldsm::TRIPLES
  );
  checkable_lldsm::graph_display_style = checkable_lldsm::OUTGOING;
  om->addOption(
    MakeRadioOption("GraphDisplayStyle",
      "Select the style to use when displaying a graph (e.g., using function show_arcs).  This does not affect the internal storage of the graph.",
      gds_list, checkable_lldsm::num_graph_display_styles, checkable_lldsm::graph_display_style
    )
  );

  // ------------------------------------------------------------------
  om->addOption(
    MakeBoolOption("DisplayGraphNodeNames",
      "When displaying a graph (e.g., using function show_arcs), should the nodes be referred to by \"name\" (the label of the node)?  Otherwise they are referred to by an index between 0 and the number of nodes-1.", 
      checkable_lldsm::display_graph_node_names
    )
  );
}


