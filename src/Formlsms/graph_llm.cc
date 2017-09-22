
#include "graph_llm.h"
#include "../Streams/streams.h"
#include "../Options/options.h"
#include "../ExprLib/startup.h"
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

// ******************************************************************
// *                                                                *
// *                graph_lldsm::reachgraph  statics                *
// *                                                                *
// ******************************************************************

named_msg graph_lldsm::reachgraph::ctl_report;
named_msg graph_lldsm::reachgraph::numpaths_report;
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
    reachgraph::show_options opts;
    opts.ORDER = stateDisplayOrder();
    opts.STYLE = graphDisplayStyle();
    opts.NODE_NAMES = displayGraphNodeNames();
    opts.RG_ONLY = true;
    shared_state* st = new shared_state(parent);
    DCASSERT(RSS);
    RGR->showArcs(em->cout(), opts, RSS, st);
    Delete(st);
  }
}

bool graph_lldsm::tooManyArcs(long na, OutputStream *os)
{
  if (na>=0) {
    if ((0==max_arc_display) || (na <= max_arc_display)) return false;
    if (os) {
      *os << "Too many arcs; to display, increase option ";
      *os << MAX_ARC_DISPLAY_OPTION << ".\n";
      os->flush();
    }
  } else {
    if (os) {
      *os << "Too many arcs.\n";
      os->flush();
    }
  }
  return true;
}

void graph_lldsm::dumpDot(OutputStream &s) const
{
  DCASSERT(RGR);
  shared_state* st = new shared_state(parent);
  DCASSERT(RSS);
  reachgraph::show_options opts;
  opts.ORDER = stateDisplayOrder();
  opts.STYLE = DOT;
  opts.NODE_NAMES = true;
  opts.RG_ONLY = true;
  RGR->showArcs(s, opts, RSS, st);
  Delete(st);
}

bool graph_lldsm::isFairModel() const
{
  return false;
}

bool graph_lldsm::isAbsorbing(long st) const
{
  bailOut(__FILE__, __LINE__, "Can't check absorbing");
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

void graph_lldsm::reachgraph::attachToParent(graph_lldsm* p, state_lldsm::reachset* rss)
{
  DCASSERT(p);
  DCASSERT(0==parent);
  parent = p;
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

stateset* graph_lldsm::reachgraph::EX(bool revTime, const stateset* p, trace_data* td)
{
  return notImplemented("EX");
}

stateset* graph_lldsm::reachgraph::AX(bool revTime, const stateset* p)
{
  return notImplemented("AX");
}

stateset* graph_lldsm::reachgraph
::EU(bool revTime, const stateset* p, const stateset* q, trace_data* td)
{
  return notImplemented("EU");
}

stateset* graph_lldsm::reachgraph
::unfairAU(bool revTime, const stateset* p, const stateset* q)
{
  return notImplemented("unfairAU");
}

stateset* graph_lldsm::reachgraph
::fairAU(bool revTime, const stateset* p, const stateset* q)
{
  return notImplemented("fairAU");
}

stateset* graph_lldsm::reachgraph::unfairEG(bool revTime, const stateset* p, trace_data* td)
{
  return notImplemented("unfairEG");
}

stateset* graph_lldsm::reachgraph::fairEG(bool revTime, const stateset* p)
{
  return notImplemented("fairEG");
}

stateset* graph_lldsm::reachgraph::AG(bool revTime, const stateset* p)
{
  return notImplemented("AG");
}

stateset* graph_lldsm::reachgraph
::unfairAEF(bool revTime, const stateset* p, const stateset* q)
{
  return notImplemented("unfairAEF");
}

void graph_lldsm::reachgraph::countPaths(const stateset*, const stateset*, result& c)
{
  notImplemented("`counting paths'");
  c.setNull();
}

void graph_lldsm::reachgraph
::traceEX(bool revTime, const stateset* p, const trace_data* td, List<stateset>* ans)
{
  notImplemented("traceEX");
}

void graph_lldsm::reachgraph
::traceEU(bool revTime, const stateset* p, const trace_data* td, List<stateset>* ans)
{
  notImplemented("traceEU");
}

void graph_lldsm::reachgraph
::traceEG(bool revTime, const stateset* p, const trace_data* td, List<stateset>* ans)
{
  notImplemented("traceEG");
}

trace_data* graph_lldsm::reachgraph
::makeTraceData() const
{
  notImplemented("makeTraceData");
  return 0;
}

stateset* graph_lldsm::reachgraph
::attachWeight(const stateset* p) const
{
  return notImplemented("attachWeight");
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

// ******************************************************************
// *                                                                *
// *                                                                *
// *                         Initialization                         *
// *                                                                *
// *                                                                *
// ******************************************************************

class init_graphllm : public initializer {
  public:
    init_graphllm();
    virtual bool execute();
};
init_graphllm the_graphllm_initializer;

init_graphllm::init_graphllm() : initializer("init_graphllm")
{
  usesResource("em");
}

bool init_graphllm::execute()
{
  if (0==em) return false;

  graph_lldsm::reachgraph::em = em;

  // ------------------------------------------------------------------
  option* report = em->findOption("Report");
  graph_lldsm::reachgraph::numpaths_report.Initialize(
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
  em->addOption(
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
  em->addOption(
    MakeRadioOption("GraphDisplayStyle",
      "Select the style to use when displaying a graph (e.g., using function show_arcs).  This does not affect the internal storage of the graph.",
      gds_list, graph_lldsm::num_graph_display_styles, graph_lldsm::graph_display_style
    )
  );

  // ------------------------------------------------------------------
  em->addOption(
    MakeBoolOption("DisplayGraphNodeNames",
      "When displaying a graph (e.g., using function show_arcs), should the nodes be referred to by \"name\" (the label of the node)?  Otherwise they are referred to by an index between 0 and the number of nodes-1.", 
      graph_lldsm::display_graph_node_names
    )
  );

  return true;
}


