
#ifndef SHOW_GRAPH_H
#define SHOW_GRAPH_H

#include "rss_indx.h"
#include "graph_llm.h"
#include "../Streams/streams.h"
#include "../_GraphLib/graphlib.h"

#include <set>

// ******************************************************************
// *                                                                *
// *                    graphlib_displayer class                    *
// *                                                                *
// ******************************************************************

/**
    Generic class for displaying graphs.
    Used to show Markov chains or FSMs in various styles,
    by plugging one of these into a graph traversal :^)
*/
class graphlib_displayer : public GraphLib::BF_graph_traversal {
  public:
    enum edge_type {
      NONE,
      FLOAT,
      DOUBLE
    };
  public:
    /**
        Constructor.
          @param  out   Output stream to display to
          @param  T     Type of edge labels
          @param  opt   Options for displaying.
          @param  RSS   Reachable states.
          @param  st    Space for state unpacking.
    */
    graphlib_displayer(OutputStream &out, edge_type T, 
      const graph_lldsm::reachgraph::show_options &opt,
      state_lldsm::reachset* RSS, shared_state* st);

    virtual ~graphlib_displayer();

    virtual bool hasNodesToExplore();
    virtual long getNextToExplore();
    virtual bool visit(long src, long dest, const void* label);

    inline void pre_traversal() {
      header(out);
      I.start();
    }
    inline void post_traversal() {
      footer(out);
    }

  protected:

    virtual void header(OutputStream &os);
    virtual void start_row(OutputStream &os, long row);
    virtual void show_edge(OutputStream &os, long src, long dest, const void* label);
    virtual void finish_row(OutputStream &os);
    virtual void footer(OutputStream &os);
    
    virtual void showState(OutputStream &os, long s);
    
  protected:
    //
    // Helpers for us and derived classes
    //

    void showLabel(OutputStream &os, const void* label) const;

  private:
    struct pair {
        long dest;
        const void* label;
      public:
        pair(long d, const void* l) {
          dest = d;
          label = l;
        }
    };

    friend bool operator< (const graphlib_displayer::pair &a, const graphlib_displayer::pair &b);


  private:
    OutputStream &out;
    edge_type Type;
    const graph_lldsm::reachgraph::show_options &opt;
    state_lldsm::reachset* RSS;
    shared_state* st;

    indexed_reachset::indexed_iterator &I;

    // current row that we display
    long row_displ;
    // current row that we select from the graph
    long row_gr;

    // set of edges from the current row
    std::set<pair> current_row;
};

inline bool operator< (const graphlib_displayer::pair &a, const graphlib_displayer::pair &b)
{
  return a.dest < b.dest;
}

// ******************************************************************
// *                                                                *
// *                           Front  end                           *
// *                                                                *
// ******************************************************************

// graphlib_displayer* buildDisplayer(graphlib_displayer::edge_type T, graph_llm::display_style thing);
// tbd - need more options

#endif
