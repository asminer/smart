
/**  \file graphlib.h
      Graph library.
      Allows construction of large, sparse graphs.
*/

#ifndef GRAPHLIB_H
#define GRAPHLIB_H

#include <stdlib.h>

class intset;  // defined in another library

namespace GraphLib {

  // ======================================================================
  // |                                                                    |
  // |                          Error  reporting                          |
  // |                                                                    |
  // ======================================================================

  /// Error codes
  class error {
  public:
    enum code {
      /// Not implemented yet.
      Not_Implemented,
      /// Bad state index
      Bad_Index,
      /// Insufficient memory (e.g., for adding state or edge)
      Out_Of_Memory,
      /// Operation finished requirement does not match graph's finished state.
      Finished_Mismatch,
      /// Misc. error
      Miscellaneous
    };
  public:
    error(code c) { errcode = c; }

    /** Obtain a human-readable error string for this error.
        For convenience.
          @return     An appropriate error string.  It
                      should not be modified or deleted.
    */
    const char*  getString() const;

    /// grab the code.
    inline code getCode() const { return errcode; }
  private:
    code errcode;
  };


  // ======================================================================
  // |                                                                    |
  // |                          timer_hook class                          |
  // |                                                                    |
  // ======================================================================
  /**
    Timer class.
    Use if you want to report how long critical computations take.
  */
  class timer_hook {
    public:
        timer_hook();
        virtual ~timer_hook();
        /** Will be called when a major computation starts.
            @param w  Short description of computation.
        */
        virtual void start(const char* w) = 0;
        /// Will be called when a major computation stops.
        virtual void stop() = 0;
  };

  // ======================================================================
  // |                                                                    |
  // |                      BF_graph_traversal class                      |
  // |                                                                    |
  // ======================================================================

  /**
        Class for arbitrary breadth-first graph traversals.

        Conceptually, this class maintains a queue of states
        to explore (hidden from the traversal), and defines
        what to do when a state is visited.
        In practice, the "queue" could be an actual queue,
        or something trivial like a single state.
  */
  class BF_graph_traversal {
      public:
          BF_graph_traversal();
          virtual ~BF_graph_traversal();

          /// Do we need to explore any more states?
          virtual bool hasStatesToExplore() = 0;

          /// Return the next state to explore.
          virtual long getNextToExplore() = 0;

          /**
              Visit a state.
                @param  src     The state causing the visit, because
                                of an outgoing/incoming edge.
                @param  dest    The state being visited.
                @param  wt      The weight (if any) on the edge
                                causing the visit.

                @return true,   If we should stop the traversal now.
          */
          virtual bool visit(long src, long dest, void* wt) = 0;
  };


  // ======================================================================
  // |                                                                    |
  // |                         static_graph class                         |
  // |                                                                    |
  // ======================================================================

  /**
    Really generic, static graph.
    Stored in compressed row or column sparse format,
    depending on whether we are "by rows" or not.
  */
  class static_graph {

    public:
      /// Default constructor: Build an empty graph.
      static_graph(); 
      ~static_graph();

      /** Fill this with a transposed copy of m.
          I.e., make this a copy of g, except
          it will be "by columns" if g is "by rows"
          and vice versa.
      */
      void transposeFrom(const static_graph &g);
      

      /// @return true if we can efficiently enumerate "by rows".
      inline bool isByRows() const { return is_by_rows; }

      /// @return true if we can efficiently enumerate "by columns".
      inline bool isByCols() const { return !is_by_rows; }

      /// @return The current number of graph nodes.
      inline long getNumNodes() const { return num_nodes; }

      /// @return The current number of graph edges.
      inline long getNumEdges() const { return num_edges; }

      /** Determine which rows are empty.
          If the graph is instead "by columns", then
          determine which columns are empty.
          In other words, if we're by rows,
          determine which states have no outgoing edges;
          otherwise, determine states with no incoming edges.
            @param  x   On input: ignored.
                        On output, set of nodes with no 
                        incoming/outgoing edges.
      */
      void emptyRows(intset &x) const;

      /**
          Run graph traversal t.
          If the graph is by rows, then we traverse outgoing edges;
          otherwise, we traverse incoming edges.

            @param  t   Traversal, which determines the
                        (possibly changing) list of states to explore,
                        and how to visit edges.

            @return true, iff a call to t.visit returned true
                          and we stopped traversal early.
      */
      bool traverse(BF_graph_traversal &t) const;

      /// Total memory required for graph storage, in bytes.
      size_t getMemTotal() const;

    private:
      void allocate(long nodes, long edges);

    private:
      /// Starting point of each row.  Dimension #nodes+1.
      long* row_pointer;

      /// Column index of each edge.  Dimension #edges.
      long* column_index;

      /// Label of each edge.  Size is #edges * (bytes per edgeval)
      unsigned char* label;

      /// Total number of nodes.
      long num_nodes;

      /// Total number of edges.
      long num_edges;

      /// Number of bytes for each edge label (can be 0).
      unsigned char edge_bytes;

      /** Do we have efficient row access?  Otherwise it's columns.
          Data structure names above assume we are stored by rows.
      */
      bool is_by_rows;

      
      friend class dynamic_graph;
  };

  // ======================================================================
  // |                                                                    |
  // |                        dynamic_graph  class                        |
  // |                                                                    |
  // ======================================================================

  /**
    Really generic, dynamic graph.
    Edges are handled using "memcpy" and the like.
    Some useful front-end wrappers (below) are 
    derived from this class.
  */
  class dynamic_graph {
    public:
      /**
        Constructor.
          @param  es    Number of bytes for edge weights (0 if none).
          @param  ksl   Keep self loops?  If not, they are discarded.
          @param  md    Merge duplicate edges?
      */
      dynamic_graph(unsigned char es, bool ksl, bool md); 
      virtual ~dynamic_graph();

      /// @return true if we can efficiently enumerate "by rows".
      inline bool isByRows() const { return is_by_rows; }

      /// @return true if we can efficiently enumerate "by columns".
      inline bool isByCols() const { return !is_by_rows; }

      /// @return The current number of graph nodes.
      inline long getNumNodes() const { return num_nodes; }

      /// @return The current number of graph edges.
      inline long getNumEdges() const { return num_edges; }


      /// Add several new nodes to the graph.
      void addNodes(long count);

      /// Add a new node to the graph.
      inline void addNode() { addNodes(1); }

      /** Add a new edge to the graph.
            @return true  iff this is a duplicate edge.
      */
      bool addEdge(long from, long to, const void* wt);

      /** Batch removal of edges.
            @param  t   We do a breadth-first traversal,
                        and for each edge, if t(edge) returns true,
                        then the edge will be removed from the graph.
      */
      void removeEdges(BF_graph_traversal &t);


      /** Renumber graph nodes.
            @param  newid   Old node number i is numbered newid[i].
      */
      void renumber(const long* newid);

      /** Reverse direction of all edges.
          Or, equivalently, flip storage between
          "store by outgoing edges" and "store by incoming edges".
            @param  sw  Place to report timing; nothing reported if 0.
      */
      void transpose(timer_hook* sw);

      /** Compute the terminal sccs.
          Useful for Markov chain state classification, or
          CTL model checking with "fairness".
          Currently, this is much faster if the graph is stored "by rows".
  
            @param  sw      Where to report timing information (nowhere if 0).
  
            @param  cons    If true, conserve memory, at a cost of 
                            (usually, slightly) increased CPU time.

            @param  sccmap  An array of dimension #nodes (at least).
                            ON OUTPUT:
                            sccmap[k] is 0 if node k is "transient",
                            between 1 and #classes if k is "recurrent".

            @param  aux     Auxiliary array, will be overwritten.
                            Dimension is #nodes (at least).

            @return   The number of terminal sccs (recurrent classes). 
      */
      long computeTSCCs(timer_hook* sw, bool cons, long* sccmap, long* aux) const;
  

      /**
          Export to a static graph.
          The static graph will be stored by rows iff this
          graph is stored by rows.
            @param  g   Place to store the static graph
                        (will be clobbered).

            @param  sw  Where to report timing information (nowhere if 0).
      */
      void exportToStatic(static_graph &g, timer_hook *sw);


      /**
          Export to a static graph and destroy this graph.
          The static graph will be stored by rows iff this
          graph is stored by rows.
            @param  g   Place to store the static graph
                        (will be clobbered).

            @param  sw  Where to report timing information (nowhere if 0).
      */
      void exportAndDestroy(static_graph &g, timer_hook *sw);


      /// Total memory required for graph storage, in bytes.
      size_t getMemTotal() const;


      /** Clear the current graph.

          This operation is similar to, but more efficient than,
          calling the destructor and then re-constructing an
          empty graph.  Memory may be retained.
          (The intent is to allow users to build and analyze
          several different small graphs without having to
          re-allocate memory each time.)
      */
      void clear();

      /**
          Run graph traversal t.
          If the graph is by rows, then we traverse outgoing edges;
          otherwise, we traverse incoming edges.

            @param  t   Traversal, which determines the
                        (possibly changing) list of states to explore,
                        and how to visit edges.

            @return true, iff a call to t.visit returned true
                          and we stopped traversal early.
      */
      bool traverse(BF_graph_traversal &t) const;

    protected:
      /**
          Define how to merge edge labels.
          Must be provided in derived classes.
            @param  ev    Edge label, will be modified in place.
            @param  nv    Edge label. will be "added to" ev,
                          where "added to" can be defined however.
      */
      virtual void merge_edges(void* ev, const void* nv) const = 0;

    private:
      // Return true if the edge was added; false if it was a duplicate.
      bool AddToMergedCircularList(long &list, long ptr);
      void AddToUnmergedCircularList(long &list, long ptr);

      long Defragment(long);


    // for SCC computation
    private:
      inline long getFirstEdgeFor(long s) const {
        return row_pointer[s];
      }
      inline void readEdgeInfo(long z, long &col, long &nxt) const {
        col = column_index[z];
        nxt = next[z];
      }

      friend class scc_data;


    private:
      long* row_pointer;
      long* column_index;
      long* next;
      unsigned char* label;
      long nodes_alloc;
      long edges_alloc;
      long num_nodes;
      long num_edges;
      unsigned char edge_size;
      bool keep_self_loops;
      bool merge_duplicates;
      bool is_by_rows;
  };


  // ======================================================================
  // |                                                                    |
  // |                       dynamic_digraph  class                       |
  // |                                                                    |
  // ======================================================================

  /// Directed graphs with unlabeled edges.
  class dynamic_digraph : public dynamic_graph {
  public:
    dynamic_digraph(bool keep_self);
  
    inline bool addEdge(long from, long to) { 
      return dynamic_graph::addEdge(from, to, 0); 
    }
  protected:
    virtual void merge_edges(void* ev, const void* nv) const;
  };


// ==========================================================================================================================================================================
// ==========================================================================================================================================================================
// ==========================================================================================================================================================================
// ==========================================================================================================================================================================
// ==========================================================================================================================================================================
// OLD INTERFACE BELOW, will eventually be discarded!
// ==========================================================================================================================================================================
// ==========================================================================================================================================================================
// ==========================================================================================================================================================================
// ==========================================================================================================================================================================
// ==========================================================================================================================================================================

#define ALLOW_OLD_GRAPH_INTERFACE
#ifdef  ALLOW_OLD_GRAPH_INTERFACE

  // ======================================================================
  // |                                                                    |
  // |                      Generic Graph  interface                      |
  // |                                                                    |
  // ======================================================================

  /**
    Really generic graph.
    Edges are handled using "memcpy" and the like.
    Some useful front-end wrappers (below) are 
    derived from this class.
  */
  class generic_graph {
  public:
    /// For exporting a finished graph.
    struct const_matrix {
        /// If true, the matrix is stored by columns, rather than by rows.
        bool is_transposed;
        /// Number of rows.
        long num_rows;
        /// List for each row.  Dimension is num_rows+1.
        const long* rowptr;
        /// Column indexes.
        const long* colindex;
        /// Edge values.
        const void* value;
        /// Size of each edge value
        int edge_size;
    };
    /// For transposing a finished graph.
    struct matrix {
        /// If true, the matrix is stored by columns, rather than by rows.
        bool is_transposed;
        /// Number of rows.
        long num_rows;
        /// List for each row.  Dimension is num_rows+1.
        long* rowptr;
        /// Column indexes.
        long* colindex;
        /// Edge values.
        void* value;
        /// Size of each edge value
        int edge_size;
      public:
        matrix();
        void destroy();
        
        /// Fill this with a copy of m.
        void copyFrom(const const_matrix &m);

        /// Fill this with a transposed copy of m.
        void transposeFrom(const const_matrix &m);

      private:
        void alloc(long nr, long ne);
    };
    /// For traversing the graph.
    class element_visitor {
    public:
        element_visitor();
        virtual ~element_visitor();
        virtual bool visit(long from, long to, void* wt) = 0;
    };
    /// Options for finishing graphs.
    struct finish_options {
        /** If true, the matrix (or matrices) used to store the
            finished graph will be sparse "by rows", instead of
            the usual sparse "by columns".
        */
        bool Store_By_Rows;

        /** This indicates that the library should plan for
            the user to eventually call "Clear" and build
            another graph, or call "unfinish".
            If true, the library is less vigorous in freeing
            memory, since it will need to be re-allocated
            when Clear() is called.
        */
        bool Will_Clear;

        /** If 0 (the default), no timing information will be reported.
            Otherwise, timing information is sent here.
        */
        timer_hook* report;

        /** Constructor.
            Will initialize all settings to reasonable defaults.
        */
        finish_options() {
          Store_By_Rows = true;
          Will_Clear = false;
          report = 0;
        };
    };

  private:
    int edge_size;
    bool keep_self_loops;
    bool merge_duplicates;
    bool finished;
    bool is_by_rows;
    long nodes_alloc;
    long edges_alloc;
  protected:
    long* row_pointer;
    long* column_index;
    long* next;
    char* label;
  protected:
    long num_nodes;
    long num_edges;

  public:
    generic_graph(int es, bool ksl, bool md); 
    virtual ~generic_graph();

    /// Is the graph "finished", or can we still add states and arcs?
    inline bool isFinished() const { return finished; }

    /// @return true if we can efficiently enumerate "by rows".
    inline bool isByRows() const { return is_by_rows; }

    /// @return true if we can efficiently enumerate "by columns".
    inline bool isByCols() const { return !is_by_rows; }

    /// @return The current number of graph nodes.
    inline long getNumNodes() const { return num_nodes; }

    /// @return The current number of graph edges.
    inline long getNumEdges() const { return num_edges; }

    /// Add several new nodes to the graph.
    void addNodes(long count);

    /// Add a new node to the graph.
    inline void addNode() { addNodes(1); }

    /** Add a new edge to the graph.
          @return true  iff this is a duplicate edge.
    */
    bool addEdge(long from, long to, const void* wt);

    /** Check if two graph states have the same outgoing edges.
        Graphs may be finished or unfinished.
        Both graphs must be "by rows".
        Requires that equal edge labels have the same memory
        footprint (i.e., we can use memcmp to compare them).
        Should be used only for graphs that merge duplicate edges.
          @param  from    Source state in this graph.
          @param  g2      Other graph to check.
          @param  from2   Source state in other graph.
          @return true iff
                  for every edge (from, to, label) in this graph,
                  there is an edge (from2, to, label) in g2.
    */
    bool haveSameEdges(long from, const generic_graph &g2, long from2) const;

    /** Visit edges from a given state.
        Will be efficient only if the graph is stored "by rows".
          @param  i   Source state.
          @param  x   We call x(edge) for each edge from state \a i.
                      If this returns true, we stop the traversal.
          @return  true, iff we stopped the traversal early because of x.
    */
    bool traverseFrom(long i, element_visitor &x) const;

    /** Visit edges to a given state.
        Will be efficient only if the graph is stored "by columns".
          @param  i   Target state.
          @param  x   We call x(edge) for each edge to state \a i.
                      If this returns true, we stop the traversal.
          @return  true, iff we stopped the traversal early because of x.
    */
    bool traverseTo(long i, element_visitor &x) const;

    /** Visit all graph edges.
          @param  x   We call x(edge) for every graph edge.
                      If this returns true, we stop the traversal.
          @return  true, iff we stopped the traversal early because of x.
    */
    bool traverseAll(element_visitor &x) const;

    /** Batch removal of edges.
        The graph must be unfinished.
          @param  x   For each edge, if x(edge) returns true,
                      then the edge will be removed from the graph.
    */
    void removeEdges(element_visitor &x);

    /** Renumber graph nodes.
        The graph must be "unfinished".
          @param  newid   Old node number i is numbered newid[i].
    */
    void renumber(const long* newid);

    /** Reverse direction of all edges.
        Or, equivalently, flip storage between
        "store by outgoing edges" and "store by incoming edges".
        The graph must be "unfinished".
          @param  sw  Place to report timing; nothing reported if 0.
    */
    void transpose(timer_hook* sw);

    /** Finish building the graph.
        The graph must be "unfinished".
        We may switch to a more compact, less dynamic, internal
        representation for the graph.  
        On return the graph will be "finished".
          @param  o  Options for creating the finished graph.
    */
    void finish(const finish_options &o);

    /** Convert a finished graph back to an unfinished one.
        The graph must be "finished".
        On return the graph will be "unfinished".
    */
    void unfinish();

    /** Find nodes with no outgoing edges.
        Faster if the graph is finished.
          @param  x   On output, set of nodes with no outgoing edges.
    */
    inline void noOutgoingEdges(intset &x) const {
      if (is_by_rows) rp_empty(x);
      else            ci_empty(x);
    }

    /** Find nodes with no incoming edges.
        Faster if the graph is finished.
          @param  x   On output, set of nodes with no incoming edges.
    */
    inline void noIncomingEdges(intset &x) const {
      if (is_by_rows) ci_empty(x);
      else            rp_empty(x);
    }

    /** Find states reached in one step.
        The graph must be finished.
          @param  x   Set of source states
          @param  y   If we can reach state j in one step
                      starting from state i in x, then
                      we add j to y.
          @return  true if the set y was changed; false otherwise.
    */
    inline bool getForward(const intset& x, intset &y) const {
      if (is_by_rows) return rowMult(x, y);
      else            return colMult(x, y);
    }

    /** Find states that can reach us in one step.
        The graph must be finished.
          @param  y   Set of target states
          @param  x   If we can reach state j in y in one step
                      starting from state i, then
                      we add i to x.
          @return  true if the set x was changed; false otherwise.
    */
    inline bool getBackward(const intset& y, intset &x) const {
      if (is_by_rows) return colMult(y, x);
      else            return rowMult(y, x);
    }


    /** Explore states reachable from a starting state.
        If the graph is transposed, explore states that can reach
        the given target state.
        The graph does not need to be finished.
          @param  s   Source or target state.
          @param  x   Input/output: 
                      Bitvector to use for explored states, i.e.,
                      x[s] is true if state s is explored.
          @param  q   Array to use for queue, during search.
                      Must have at least num_nodes elements.
          @return     Number of explored states.
    */
    inline long getReachable(long s, bool* x, long* q) const {
      if (finished) return getFinishedReachable(s, x, q);
      else          return getUnfinishedReachable(s, x, q);
    }

    /** Explore states reachable from a starting state.
        If the graph is transposed, explore states that can reach
        the given target state.
        The graph does not need to be finished.
          @param  s   Source or target state.
          @param  x   Input/output: 
                      Bitvector to use for explored states, i.e.,
                      x[s] is true if state s is explored.
          @param  q   Array to use for queue, during search.
                      Must have at least num_nodes elements.
          @return     Number of explored states.
    */
    inline long getReachable(long s, intset &x, long* q) const {
      if (finished) return getFinishedReachable(s, x, q);
      else          return getUnfinishedReachable(s, x, q);
    }

    /** Explore states reachable from a starting state.
        If the graph is transposed, explore states that can reach
        the given target state.
        The graph does not need to be finished.
          @param  s   Source or target state.
          @param  x   Input/output: 
                      Bitvector to use for explored states, i.e.,
                      x[s] is true if state s is explored.
          @return     Number of explored states, or -1 if not enough  memory.
    */
    inline long getReachable(long s, bool* x) const {
      long* q = (long*) malloc(num_nodes * sizeof(long));
      if (0==q) return -1;
      long ans = finished ? getFinishedReachable(s, x, q) 
                          : getUnfinishedReachable(s, x, q);
      free(q);
      return ans;
    }

    /** Explore states reachable from a starting state.
        If the graph is transposed, explore states that can reach
        the given target state.
        The graph does not need to be finished.
          @param  s   Source or target state.
          @param  x   Input/output: 
                      Bitvector to use for explored states, i.e.,
                      x[s] is true if state s is explored.
          @return     Number of explored states, or -1 if not enough  memory.
    */
    inline long getReachable(long s, intset &x) const {
      long* q = (long*) malloc(num_nodes * sizeof(long));
      if (0==q) return -1;
      long ans = finished ? getFinishedReachable(s, x, q) 
                          : getUnfinishedReachable(s, x, q);
      free(q);
      return ans;
    }


    /** Compute the terminal sccs.
        Useful for Markov chain state classification, or
        CTL model checking with "fairness".
        Currently, this is much faster if the graph is stored "by rows".
  
          @param  sw      Where to report timing information (nowhere if 0).
  
          @param  cons    If true, conserve memory, at a cost of 
                          (usually, slightly) increased CPU time.

          @param  sccmap  An array of dimension #nodes (at least).
                          ON OUTPUT:
                          sccmap[k] is 0 if node k is "transient",
                          between 1 and #classes if k is "recurrent".

          @param  aux     Auxiliary array, will be overwritten.
                          Dimension is #nodes (at least).

          @return   The number of terminal sccs (recurrent classes). 
    */
    long computeTSCCs(timer_hook* sw, bool cons, long* sccmap, long* aux) const;
  

    /// Export a finished graph.  @return true on success.
    bool exportFinished(const_matrix &m) const;

    /// Total memory required for graph storage, in bytes.
    long ReportMemTotal() const;

    /** Clear the current graph.
        The graph can be finished or unfinished.
        On return, the graph will be "unfinished".

        This operation is similar to, but more efficient than,
        calling the destructor and then re-constructing an
        empty graph.  Memory may be retained.
        (The intent is to allow users to build and analyze
        several different small graphs without having to
        re-allocate memory each time.)
    */
    void clear();

  public:
    // These functions not guaranteed to be supported in later versions
  
    inline long RowPtr(long i) const { return row_pointer[i]; }
    inline long ColIndx(long i) const { return column_index[i]; }
    inline long Next(long i) const { return next[i]; }

  protected:
    virtual void merge_edges(void* ev, const void* nv) = 0;
    bool AddToMergedCircularList(long &list, long ptr);
    void AddToUnmergedCircularList(long &list, long ptr);

    long Defragment(long);
    virtual void DefragSwap(long i, long j) = 0;

    bool rowMult(const intset& x, intset& y) const;
    bool colMult(const intset& x, intset& y) const;

    // check rp for empty edges
    void rp_empty(intset &x) const;

    // check ci for missing edges
    void ci_empty(intset &x) const;

    template <class T> inline static void SWAP(T &x, T &y) { 
      T tmp=x; 
      x=y; 
      y=tmp; 
    }

    long getFinishedReachable(long s, bool* x, long* q) const;
    long getUnfinishedReachable(long s, bool* x, long* q) const;

    long getFinishedReachable(long s, intset &x, long* q) const;
    long getUnfinishedReachable(long s, intset &x, long* q) const;
  };


  // ======================================================================
  // |                                                                    |
  // |                         Unweighted  graphs                         |
  // |                                                                    |
  // ======================================================================

  /// Directed graphs with unlabeled edges.
  class digraph : public generic_graph {
  public:
    digraph(bool ksl) : generic_graph(0, ksl, true) { };
  
    inline bool addEdge(long from, long to) { 
      return generic_graph::addEdge(from, to, 0); 
    }
  protected:
    virtual void merge_edges(void* ev, const void* nv) { }
    virtual void DefragSwap(long i, long j);
  };


  // ======================================================================
  // |                                                                    |
  // |                          Weighted  graphs                          |
  // |                                                                    |
  // ======================================================================

  template <class TYPE>
  class weighted_digraph : public generic_graph {
  public:
    weighted_digraph(bool ksl, bool merge)
    : generic_graph(sizeof(TYPE), ksl, merge) { };

    inline bool addEdge(long from, long to, const TYPE &val) {
      return generic_graph::addEdge(from, to, &val);
    }

    template <class ACC>
    inline void getRowSum(long row, ACC& sum) const {
      if (!isByRows()) 
        throw error(error::Miscellaneous);
      if (row<0 || row>=num_nodes) 
        throw error(error::Bad_Index);
      TYPE* vlabel = (TYPE*) label;
      if (isFinished()) {
        for (long z=row_pointer[row]; z<row_pointer[row+1]; z++) 
          sum += vlabel[z];
      } else if (row_pointer[row]>=0) {
        long first = row_pointer[row];
        long ptr = first;
        do {
          sum += vlabel[ptr];
          ptr = next[ptr];
        } while (ptr != first);
      }
    }

    template <class AC>
    inline bool findOutgoingEdge(long r, long &c, AC &sum, const AC &u) const {
      TYPE* vlabel = (TYPE*) label;
      if (isFinished()) {
        // static storage
        for (long z=row_pointer[r]; z<row_pointer[r+1]; z++) {
          sum += vlabel[z];
          if (sum >= u) {
            c = column_index[z];
            return true;
          }
        } // for z
        return false;
      } else {
        // dynamic storage
        if (row_pointer[r]<0) return false;
        long first = row_pointer[r];
        long ptr = first;
        do {
          sum += vlabel[ptr];
          if (sum >= 0) {
            c = column_index[ptr];
            return true;
          }
          ptr = next[ptr];
        } while (ptr != first);
        return false;
      }
    }

  protected:
    virtual void DefragSwap(long i, long j) {
      // swap i and j, set up forward
      SWAP(column_index[i], column_index[j]);
      TYPE* li = (TYPE*) (label + i*sizeof(TYPE));
      TYPE* lj = (TYPE*) (label + j*sizeof(TYPE));
      SWAP(li[0], lj[0]);
      next[j] = next[i];
      next[i] = j;  // forwarding info
    }
  };


  /** Weighted, directed graphs with merged duplicate edges.  
      Edges can be any type, with operator "+=" used to accumulate
      duplicate edges.
  */
  template <class TYPE>
  class merged_weighted_digraph : public weighted_digraph <TYPE> {
  public:
    merged_weighted_digraph(bool ksl) 
    : weighted_digraph <TYPE> (ksl, true) { };
  
  protected:
    virtual void merge_edges(void* ev, const void* nv) {
      TYPE* tev = (TYPE*) ev;
      TYPE* tnv = (TYPE*) nv;
      *tev += *tnv;
    }
  };

  /** Weighted, directed graphs without merged duplicate edges.  
      Edges can be any type.  Duplicate edges are stored separately,
      i.e., it is possible to have multiple edges (presumably with
      different weights) between the same pair of states.
  */
  template <class TYPE>
  class unmerged_weighted_digraph : public weighted_digraph <TYPE> {
  public:
    unmerged_weighted_digraph(bool ksl) 
    : weighted_digraph <TYPE> (ksl, false) { };
  
  protected:
    // This should never be called.
    virtual void merge_edges(void* ev, const void* nv) { }
  };


#endif // ALLOW_OLD_GRAPH_INTERFACE

  // ======================================================================
  // |                                                                    |
  // |                        Front-end  interface                        |
  // |                                                                    |
  // ======================================================================

  /**
    Get the name and version info of the library.
    The string should not be modified or deleted.
  
    @return    Information string.
  */
  const char*  Version();

}; // namespace GraphLib

#endif
