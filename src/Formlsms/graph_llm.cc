
// $Id$

#include "graph_llm.h"
#include "../Streams/streams.h"
#include "../Options/options.h"
#include "../ExprLib/exprman.h"
#include "../ExprLib/mod_vars.h"
#include "../Modules/biginttype.h"

// ******************************************************************
// *                                                                *
// *                      graph_lldsm  statics                      *
// *                                                                *
// ******************************************************************

int graph_lldsm::graph_display_style;
bool graph_lldsm::display_graph_node_names;
long graph_lldsm::max_arc_display = 100000000;
const char* MAX_ARC_DISPLAY_OPTION = "MaxArcDisplay";
named_msg graph_lldsm::numpaths_report;

// ******************************************************************
// *                                                                *
// *                graph_lldsm::reachgraph  statics                *
// *                                                                *
// ******************************************************************

named_msg graph_lldsm::reachgraph::ctl_report;
exprman* graph_lldsm::reachgraph::em = 0;

// ******************************************************************
// *                                                                *
// *                      graph_lldsm  methods                      *
// *                                                                *
// ******************************************************************

graph_lldsm::graph_lldsm(model_type t) : state_lldsm(t)
{
  RGR = 0;
}

graph_lldsm::~graph_lldsm()
{
  Delete(RGR);
}

const char* graph_lldsm::getClassName() const
{
  return "graph_lldsm";
}

void graph_lldsm::showArcs(bool internal) const
{
  DCASSERT(RGR);
  if (internal) {
    RGR->showInternal(em->cout());
  } else {
    shared_state* st = new shared_state(parent);
    DCASSERT(RSS);
    RGR->showArcs(em->cout(), RSS, stateDisplayOrder(), st);
    Delete(st);
  }
}

bool graph_lldsm::tooManyArcs(long na, bool show)
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

void graph_lldsm::showInitial() const
{
  bailOut(__FILE__, __LINE__, "Can't show initial state(s)");
}

#ifdef NEW_STATESETS

void graph_lldsm::countPaths(const stateset*, const stateset*, result& c)
{
  c.setNull();
  bailOut(__FILE__, __LINE__, "Can't count paths");
}

#else

void graph_lldsm::countPaths(const intset&, const intset&, result& c)
{
  c.setNull();
  bailOut(__FILE__, __LINE__, "Can't count paths");
}

#endif

bool graph_lldsm::requireByRows(const named_msg*)
{
  return false;
}

bool graph_lldsm::requireByCols(const named_msg*)
{
  return false;
}

long graph_lldsm::getOutgoingEdges(long from, ObjectList <int> *e) const
{
  bailOut(__FILE__, __LINE__, "Can't get outgoing edges");
  return -1;
}

long graph_lldsm::getIncomingEdges(long from, ObjectList <int> *e) const
{
  bailOut(__FILE__, __LINE__, "Can't get incoming edges");
  return -1;
}

bool graph_lldsm::getOutgoingCounts(long* a) const
{
  bailOut(__FILE__, __LINE__, "Can't get outgoing edge counts");
  return false;
}

bool graph_lldsm::dumpDot(OutputStream &s) const
{
  return false;
}

#ifdef NEW_STATESETS

stateset* graph_lldsm::getAbsorbingStates() const
{
  bailOut(__FILE__, __LINE__, "Can't get absorbing states");
  return 0;
}

stateset* graph_lldsm::getDeadlockedStates() const
{
  bailOut(__FILE__, __LINE__, "Can't get deadlocked states");
  return 0;
}

#else

void graph_lldsm::getAbsorbingStates(result &x) const
{
  bailOut(__FILE__, __LINE__, "Can't get absorbing states");
  x.setNull();
}

void graph_lldsm::getDeadlockedStates(result &x) const
{
  bailOut(__FILE__, __LINE__, "Can't get deadlocked states");
  x.setNull();
}

#endif

bool graph_lldsm::isFairModel() const
{
  return false;
}

#ifdef NEW_STATESETS


#else

void graph_lldsm::getTSCCsSatisfying(stateset &p) const
{
  DCASSERT(isFairModel());
  bailOut(__FILE__, __LINE__, "Can't get TSCCs satisfying p");
}

void graph_lldsm::findDeadlockedStates(stateset &) const
{
  bailOut(__FILE__, __LINE__, "Can't find deadlocked states");
}

bool graph_lldsm::forward(const intset &p, intset &r) const
{
  bailOut(__FILE__, __LINE__, "Can't compute forward set");
  return false;
}

bool graph_lldsm::backward(const intset &p, intset &r) const
{
  bailOut(__FILE__, __LINE__, "Can't compute backward set");
  return false;
}

#endif

bool graph_lldsm::isAbsorbing(long st) const
{
  bailOut(__FILE__, __LINE__, "Can't check absorbing");
  return false;
}

bool graph_lldsm::isDeadlocked(long st) const
{
  bailOut(__FILE__, __LINE__, "Can't check deadlocked");
  return false;
}

// ******************************************************************
// *                                                                *
// *                graph_lldsm::reachgraph  methods                *
// *                                                                *
// ******************************************************************

graph_lldsm::reachgraph::reachgraph()
{
  parent = 0;
}

graph_lldsm::reachgraph::~reachgraph()
{
}

void graph_lldsm::reachgraph::Finish(state_lldsm::reachset*)
{
  // Default - do nothing
}

void graph_lldsm::reachgraph::getNumArcs(result &na) const
{
  long lna;
  getNumArcs(lna);
  if (lna>=0) {
    na.setPtr(new bigint(lna));
  } else {
    na.setNull();
  }
}

stateset* graph_lldsm::reachgraph::EX(bool revTime, const stateset* p) const
{
  return notImplemented("EX");
}

stateset* graph_lldsm::reachgraph
::EU(bool revTime, const stateset* p, const stateset* q) const
{
  return notImplemented("EU");
}

stateset* graph_lldsm::reachgraph::unfairEG(bool revTime, const stateset* p) const
{
  return notImplemented("unfairEG");
}

stateset* graph_lldsm::reachgraph::fairEG(bool revTime, const stateset* p) const
{
  return notImplemented("fairEG");
}

stateset* graph_lldsm::reachgraph
::unfairAEF(bool revTime, const stateset* p, const stateset* q) const
{
  return notImplemented("unfairAEF");
}

bool graph_lldsm::reachgraph::Print(OutputStream &s, int width) const
{
  // Required for shared object, but will we ever call it?
  s << "reachgraph (why is it printing?)";
  return true;
}

bool graph_lldsm::reachgraph::Equals(const shared_object* o) const
{
  return (this == o);
}

bool graph_lldsm::reachgraph::reportCTL()
{
  return ctl_report.isActive();
}

void graph_lldsm::reachgraph::reportIters(const char* who, long iters)
{
  if (!ctl_report.startReport()) return;
  ctl_report.report() << who << " required " << iters << " iterations\n";
  em->stopIO();
}

void graph_lldsm::reachgraph::showError(const char* s)
{
  if (em->startError()) {
    em->noCause();
    em->cerr() << s;
    em->stopIO();
  }
}

stateset* graph_lldsm::reachgraph::notImplemented(const char* op) const
{
  if (em->startError()) {
    em->noCause();
    em->cerr() << "Operation " << op << " not implemented in class " << getClassName();
    em->stopIO();
  }
  return 0;
}

stateset* graph_lldsm::reachgraph::incompatibleOperand(const char* op) const
{
  if (em->startError()) {
    em->noCause();
    em->cerr() << "Incompatible operand for " << op << " in class " << getClassName();
    em->stopIO();
  }
  return 0;
}

// **************************************************************************
// *                                                                        *
// *                               Front  end                               *
// *                                                                        *
// **************************************************************************


void InitializeGraphLLM(exprman* om)
{
  if (0==om) return;

  graph_lldsm::reachgraph::em = om;

  // ------------------------------------------------------------------
  option* report = om->findOption("Report");
  graph_lldsm::numpaths_report.Initialize(
    report,
    "num_paths",
    "When set, performance data for counting number of paths is displayed.",
    false
  );
  graph_lldsm::reachgraph::ctl_report.Initialize(report,
    "CTL_engines",
    "When set, CTL engine performance is reported.",
    false
  );

  // ------------------------------------------------------------------
  om->addOption(
    MakeIntOption(
      MAX_ARC_DISPLAY_OPTION,
      "The maximum number of arcs to display for a model.  If 0, the graph will be displayed whenever possible, regardless of the number of arcs.",
      graph_lldsm::max_arc_display,
      0, 1000000000
    )
  );

  // ------------------------------------------------------------------
  radio_button** gds_list = new radio_button*[graph_lldsm::num_graph_display_styles];
  gds_list[graph_lldsm::DOT] = new radio_button(
      "DOT", 
      "Graphs are displayed in a format compatible with the graph visualization tool \"dot\".",
      graph_lldsm::DOT
  );
  gds_list[graph_lldsm::INCOMING] = new radio_button(
      "INCOMING", 
      "Graphs are displayed by listing the incoming edges for each node.",
      graph_lldsm::INCOMING
  );
  gds_list[graph_lldsm::OUTGOING] = new radio_button(
      "OUTGOING", 
      "Graphs are displayed by listing the outgoing edges for each node.",
      graph_lldsm::OUTGOING
  );
  gds_list[graph_lldsm::TRIPLES] = new radio_button(
      "TRIPLES", 
      "Graphs are displayed by listing edges as triples FROM TO INFO, where INFO is any edge information (e.g., the rate).",
      graph_lldsm::TRIPLES
  );
  graph_lldsm::graph_display_style = graph_lldsm::OUTGOING;
  om->addOption(
    MakeRadioOption("GraphDisplayStyle",
      "Select the style to use when displaying a graph (e.g., using function show_arcs).  This does not affect the internal storage of the graph.",
      gds_list, graph_lldsm::num_graph_display_styles, graph_lldsm::graph_display_style
    )
  );

  // ------------------------------------------------------------------
  om->addOption(
    MakeBoolOption("DisplayGraphNodeNames",
      "When displaying a graph (e.g., using function show_arcs), should the nodes be referred to by \"name\" (the label of the node)?  Otherwise they are referred to by an index between 0 and the number of nodes-1.", 
      graph_lldsm::display_graph_node_names
    )
  );
}


