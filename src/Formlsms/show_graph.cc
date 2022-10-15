
#include "show_graph.h"

// ******************************************************************
// *                                                                *
// *                   graphlib_displayer methods                   *
// *                                                                *
// ******************************************************************

graphlib_displayer::graphlib_displayer(OutputStream &_out, edge_type T, 
  const graph_lldsm::reachgraph::show_options &_opt,
  state_lldsm::reachset* _RSS, shared_state* _st)
  : BF_graph_traversal(), out(_out), opt(_opt), 
    I( dynamic_cast <indexed_reachset::indexed_iterator &> (_RSS->iteratorForOrder(_opt.ORDER)))
{
  Type = T;
  RSS = _RSS;
  st = _st;

  current_row.clear();

  row_displ = -1;
  row_gr = -1;
}

graphlib_displayer::~graphlib_displayer()
{
}

bool graphlib_displayer::hasNodesToExplore()
{
  if (row_displ >= 0) {

    std::set<pair>::const_iterator pos;
    for (pos=current_row.begin(); pos != current_row.end(); ++pos) {
      show_edge(out, row_displ, pos->dest, pos->label);
    }
    finish_row(out);
  }

  return I;
}

long graphlib_displayer::getNextToExplore()
{
  row_displ = I.getI();
  row_gr = I.index();
  current_row.clear();

  start_row(out, row_displ);

  I++;

  return row_gr;
}

bool graphlib_displayer::visit(long, long dest, const void* label)
{
  current_row.insert( pair(I.index2ord(dest), label) );
  return false;
}

void graphlib_displayer::header(OutputStream &os)
{
  if (graph_lldsm::TRIPLES == opt.STYLE) return;
  if (graph_lldsm::DOT == opt.STYLE) {
    os << "digraph ";
    if (NONE == Type) os << "fsm"; else os << "mc";
    os << " {\n";
    for (I.start(); I; I++) { 
      os << "\ts" << I.index();
      os << " [label=\"";
      showState(os, I.index());
      os << "\"];\n";
      os.flush();
    } // for i
    os << "\n";
    return;
  }
  if (NONE == Type) {
    os << "Reachability graph:\n";
    return;
  }
  os << "Markov chain:\n";
}

void graphlib_displayer::start_row(OutputStream &os, long row)
{
  switch (opt.STYLE) {
      case graph_lldsm::INCOMING:
          os << "To state ";
          showState(os, row);
          os << ":\n";
          return;
      
      case graph_lldsm::OUTGOING:
          os << "From state ";
          showState(os, row);
          os << ":\n";
          return;

      default:
          return;
  }
}

void graphlib_displayer::show_edge(OutputStream &os, long src, long dest, const void* label)
{
  switch (opt.STYLE) {
      case graph_lldsm::INCOMING:
          os << "\tFrom state ";
          showState(os, dest);
          if (Type != NONE) {
            os << " with weight ";
            showLabel(os, label);
          }
          os << "\n";
          return;

      case graph_lldsm::OUTGOING:
          os << "\tTo state ";
          showState(os, dest);
          if (Type != NONE) {
            os << " with weight ";
            showLabel(os, label);
          }
          os << "\n";
          return;

      case graph_lldsm::TRIPLES:
          os << "\t" << src << " " << dest;
          if (Type != NONE) {
            showLabel(os, label);
          }
          os << "\n";
          return;

      case graph_lldsm::DOT:
          os << "\ts" << src << " -> s" << dest << ";\n";
          // TBD - label?
          return;

  };
}

void graphlib_displayer::finish_row(OutputStream &os)
{
  os.flush();
}

void graphlib_displayer::footer(OutputStream &os)
{
  switch (opt.STYLE) {
      case graph_lldsm::DOT:
          os << "}\n";
          break;

      default:
          break;
  }
  os.flush();
}

void graphlib_displayer::showState(OutputStream &os, long s)
{
  if (opt.NODE_NAMES) {
    I.copyState(st, s);
    RSS->showState(os, st);
  } else {
    os << s;
  }
}

void graphlib_displayer::showLabel(OutputStream &os, const void* label) const
{
  if (NONE == Type) return;
  if (0==label) {
    os << "(null)";
    return;
  }
  switch (Type) {
    case FLOAT: {
        const float* fl = (const float*) label;
        os << *fl;
        return;
    };
    case DOUBLE: {
        const double* dl = (const double*) label;
        os << *dl;
        return;
    };
    default:
        DCASSERT(0);
        return;
  };
}

