
/**  \file graphlib.h
      Graph library.
      Allows construction of large, sparse graphs.
*/

#ifndef GRAPHLIB_H
#define GRAPHLIB_H

#include "../include/defines.h"

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
      /// Bad node index
      Bad_Index,
      /// Insufficient memory (e.g., for adding node or edge)
      Out_Of_Memory,
      /// Operation required by rows and graph is by columns, or vice versa
      Format_Mismatch,
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

        Conceptually, this class maintains a queue (hidden from the traversal)
        of nodes whose outgoing edges should be explored, and defines what to
        do when an edge is visited.  In practice, the "queue" could be an
        actual queue, or something trivial like a single node.

  */
  class BF_graph_traversal {
      public:
          BF_graph_traversal();
          virtual ~BF_graph_traversal();

          /// Do we need to explore any more nodes?
          virtual bool hasNodesToExplore() = 0;

          /// Return the next node to explore.
          virtual long getNextToExplore() = 0;

          /**
              Visit an edge.
                @param  src     The node causing the visit, because
                                of an outgoing/incoming edge.
                @param  dest    The node being visited.
                @param  wt      The weight (if any) on the edge
                                causing the visit.

                @return true,   If we should stop the traversal now.
          */
          virtual bool visit(long src, long dest, const void* wt) = 0;
  };


  // ======================================================================
  // |                                                                    |
  // |                        BF_with_queue  class                        |
  // |                                                                    |
  // ======================================================================

  /**
        Breadth first traversals that use a queue.
  */
  class BF_with_queue : public BF_graph_traversal {
      public:
          BF_with_queue(long max_queue);
          virtual ~BF_with_queue();

          inline bool queueEmpty() const {
            return queue_head == queue_tail;
          }
          inline long queuePop() {
            return queue[queue_head++];
          }
          inline void queuePush(long v) {
            if (queue_tail >= queue_alloc) {
              // we've overflowed
              throw error(error::Out_Of_Memory);
            }
            queue[queue_tail++] = v;
          }
          inline void queueReset() {
            queue_head = queue_tail = 0;
          }

          /**
              Reasonable default: checks if queue is empty.
          */
          virtual bool hasNodesToExplore();
          /**
              Reasonable default: pops value off the queue.
          */
          virtual long getNextToExplore();
      private:
          long* queue;
          long queue_head;
          long queue_tail;
          long queue_alloc;
  };
  // ======================================================================
  // |                                                                    |
  // |                       node_renumberer  class                       |
  // |                                                                    |
  // ======================================================================

  /**
      Abstract class for node renumbering in a graph.
  */
  class node_renumberer {
    public:
      node_renumberer();
      virtual ~node_renumberer();

      /// Give the new node number for node s.
      virtual long new_number(long s) const = 0;

      /// Does the renumbering scheme change the number ofat least one node?
      virtual bool changes_something() const = 0;
  };

  // ======================================================================
  // |                                                                    |
  // |                     nochange_renumberer  class                     |
  // |                                                                    |
  // ======================================================================

  /**
      Node renumbering when we don't need to renumber.
  */
  class nochange_renumberer : public node_renumberer {
    public:
      nochange_renumberer();
      virtual ~nochange_renumberer();

      virtual long new_number(long s) const;
      virtual bool changes_something() const;
  };

  // ======================================================================
  // |                                                                    |
  // |                       array_renumberer class                       |
  // |                                                                    |
  // ======================================================================

  /**
      Node renumbering using an array.
      The easiest and most common case, so we provide it for convenience.
  */
  class array_renumberer : public node_renumberer {
    public:
      array_renumberer(long* nn, long length);

      // Will destroy the array using delete[].
      virtual ~array_renumberer();

      // Will simply return newnumber[s].
      virtual long new_number(long s) const;

      virtual bool changes_something() const;
    protected:
      long* newnumber;
      long length;
      bool not_identity;
  };

  // ======================================================================
  // |                                                                    |
  // |                      static_classifier  class                      |
  // |                                                                    |
  // ======================================================================

  /**
      Helper class for classes of contiguous nodes in a graph.
      
      Classes must be numbered contiguously 0, 1, ..., NC-1
      but a class can have size zero (i.e., no nodes assigned to it).

      Nodes in each class must be numbered contiguously.
      Since the first state must be numbered 0, it sufficies to specify
      the number of classes and the size of each class.

  */
  class static_classifier {
    public:
      /// Empty constructor.
      static_classifier();

      /// Copy constructor.
      static_classifier(const static_classifier &SC);

      /// Destructor.
      ~static_classifier();

      inline long getNumClasses() const { return num_classes; }

      inline long sizeOfClass(long c) const {
        CHECK_RANGE(0, c, num_classes);
        return class_start[c+1] - class_start[c];
      }

      inline long firstNodeOfClass(long c) const {
        CHECK_RANGE(0, c, num_classes);
        return class_start[c];
      }

      inline long lastNodeOfClass(long c) const {
        CHECK_RANGE(0, c, num_classes);
        return class_start[c+1]-1;
      }

      inline bool isNodeInClass(long n, long c) const {
        CHECK_RANGE(0, c, num_classes);
        return (n >= class_start[c]) && (n<class_start[c+1]);
      }

      long classOfNode(long s) const;

      /// Total memory required for this classifier, in bytes.
      size_t getMemTotal() const;

    private:
      /**
        Rebuild the classifier.
        Called by abstract_classifier when exporting.
          @param  nc      Number of classes.
          @param  sizes   Array of dimension nc+1 where
                            sizes[i] is the size of class i, for 0 <= i < nc,
                            and sizes[nc] is ignored.
                          (The array is reused in the class, and the extra
                          element is required.)
      */
      void rebuild(long nc, long* sizes);

      /**
        Replace the classifier.
        Called by abstract_classifier when exporting.
          @param  nc      Number of classes.
          @param  starts  Array of dimension nc+1 where
                            starts[i] is the index of the 
                            first node in class i.
      */
      void replace(long nc, long* starts);

      /// Replace the classifier with an existing one.
      void replace(const static_classifier &SC);

      friend class abstract_classifier;

    private:
      long num_classes;
      long* class_start;
  };


  // ======================================================================
  // |                                                                    |
  // |                     abstract_classifier  class                     |
  // |                                                                    |
  // ======================================================================

  /**
    Abstract class for defining classes of nodes in a graph.
    Typically used for SCCs and TSCCs, such as for classifying
    states in a Markov chain.
    
    For this classifer, nodes within a class do not need to be
    numbered contiguously, and we can generate a renumberer
    if we need to renumber the nodes so that they are contiguous.
    For example, if we have the following classes for nodes:
      0:3 1:1 2:0 3:1 4:1 5:1 6:2 7:2 8:0 9:1
    Then we might reorder the nodes by class
      2:0 8:0 1:1 3:1 4:1 5:1 9:1 6:2 7:2 0:3
    which would give a renumbering
      0->9, 1->2, 2->0, 3->3, 4->4, 5->5, 6->7, 8->1, 9->6

    This is an abstract base class.
    The idea is that different classification methods could use
    different derived classes.

    Classes must be numbered 0, 1, ..., NC-1
    but a class can have size zero (i.e., no nodes assigned to it).
  */
  class abstract_classifier {
    public:
      abstract_classifier(long ns, long nc);
      virtual ~abstract_classifier();

      inline long getNumClasses() const { return num_classes; }
      inline long getNumNodes() const { return num_nodes; }

      /// For a given node s, return its class.
      virtual long classOfNode(long s) const = 0;

      /**
          Build a renumbering scheme and static classifier.
          Must be provided by derived classes.

            @param  C     On output: static classifier based on this one.
                          The static classifier will work only if nodes are
                          renumbered according to the node_renumberer
                          returned by this method.

            @return       A node_renumberer that will cause classes to
                          be contiguous.  Specifically, new node numbers
                          will be such that nodes belonging to class 0
                          appear first, followed by nodes belonging to 
                          class 1, then class 2, and so on.
                          If 0, then no renumbering is necessary.
      */
      virtual node_renumberer* buildRenumbererAndStatic(static_classifier &C) const = 0;

    protected:
      /* 
          Helpers, will be needed by derived classes when exporting.
      */

      inline void rebuild_classifier(static_classifier &C, long nc, 
        long* sizes) const
      {
        C.rebuild(nc, sizes);
      }

      inline void replace_classifier(static_classifier &C, long nc,
        long* starts) const
      {
        C.replace(nc, starts);
      }

    private:
      long num_nodes;
      long num_classes;
  };


  // ======================================================================
  // |                                                                    |
  // |                      general_classifier class                      |
  // |                                                                    |
  // ======================================================================

  /**
      General classifier using an array.
  */
  class general_classifier : public abstract_classifier {
    public:
      /**
          Build a classifier.
            @param  C   Array of dimension number of nodes, where
                        C[n] gives the class of node n.
                        The destructor will destroy this array using delete[].

            @param  ns  Number of nodes.

            @param  nc  Number of classes.  Array C must contain
                        values in the range 0, 1, ..., nc-1.
      */
      general_classifier(long* C, long ns, long nc);
      virtual ~general_classifier();

      // Required interface:

      virtual long classOfNode(long s) const;
      virtual node_renumberer* buildRenumbererAndStatic(static_classifier &C) const;

    private:
      long* class_of_node;
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

      /// @return The number of graph nodes.
      inline long getNumNodes() const { return num_nodes; }

      /// @return The total number of graph edges.
      inline long getNumEdges() const { return num_edges; }

      /// @return The number of graph edges for node s.
      inline long getNumEdgesFor(long s) const {
        return row_pointer[s+1] - row_pointer[s];
      }

      /** Determine which rows are empty.
          If the graph is instead "by columns", then
          determine which columns are empty.
          In other words, if we're by rows,
          determine which nodes have no outgoing edges;
          otherwise, determine nodes with no incoming edges.
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
                        (possibly changing) list of nodes to explore,
                        and how to visit edges.

            @return true, iff a call to t.visit returned true
                          and we stopped traversal early.
      */
      bool traverse(BF_graph_traversal &t) const;

      /// Total memory required for graph storage, in bytes.
      size_t getMemTotal() const;

    public:
      // Read-only access to internal storage

      inline const long* RowPointer() const { return row_pointer; }
      inline const long* ColumnIndex() const { return column_index; }
      inline const void* Labels() const { return label; } 
      inline unsigned char EdgeBytes() const { return edge_bytes; }

      inline long RowPointer(long s) const {
        CHECK_RANGE(0, s, num_nodes+1); 
        return row_pointer[s];
      }
      inline long ColumnIndex(long e) const {
        CHECK_RANGE(0, e, num_edges); 
        return column_index[e];
      }
      inline const void* Label(long e) const {
        CHECK_RANGE(0, e, num_edges); 
        return label + e*edge_bytes;
      }

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


      /**
          Renumber nodes in the graph.
            @param  r   Node renumbering scheme.  Node i in the graph will be
                        renumbered to r.new_number(i).
      */
      void renumberNodes(const node_renumberer &r);


      /** Reverse direction of all edges.
          Or, equivalently, flip storage between
          "store by outgoing edges" and "store by incoming edges".
            @param  sw  Place to report timing; nothing reported if 0.
      */
      void transpose(timer_hook* sw);


      /** Compute the terminal sccs.
          Useful for Markov chain state classification, or
          CTL model checking with "fairness".
          Currently, this assumes the graph is stored "by rows".

          Implementation is in sccs.cc
  
            @param  nonterminal   Index to use for nonterminal SCCs
                                  (we will merge them all together),
                                  or -1 if nonterminal SCCs should be
                                  kept separated.

            @param  sinks         Index to use for single sink state SCCs
                                  (we will merge them all together),
                                  or -1 if each sink state should be its
                                  own SCC.

            @param  cons          If true, conserve memory, at a cost of 
                                  (usually, slightly) increased CPU time.

            @param  sw            Where to report timing information 
                                  (nowhere if 0).
  

            @return   A classification for each node.  SCCs will be 
                      numbered "densely" from 0 to a maximum number, with 
                      nonterminals grouped together (if specified) and sinks
                      grouped together (if specified).  If we ask it but
                      there are none, then those classes will be empty.  If 
                      we specify a large index for nonterminal or sinks, 
                      and there are not enough SCCs, then there will be
                      empty classes in between.  (For example, if we
                      specify "nonterminal = 50" and "sinks = 35", 
                      and there are no other SCCs, then classes 0..34
                      and 36..49 will be empty.)
      */
      abstract_classifier* determineSCCs(long nonterminal, long sinks, 
        bool cons, timer_hook* sw) const;
  

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


      /**
          Export to two static graphs, based on node classification.
          Graphs will have the same nodes but differ in the edges:
          all edges within a class will be copied into the first graph, 
          and the others will be copied into the second graph.
          The static graphs will be stored by rows iff this graph
          is stored by rows.
            @param  c       State classifier.

            @param  ksl     Should we keep any self loops, i.e., should
                            edges from a state s to itself be copied into
                            g_diag?  If not, then those edges are discarded.

            @param  g_diag  On output, will contain a copy of this graph
                            but only with edges that are between two nodes
                            belonging to the same class.
                            If the graph incidence matrix is viewed as a
                            block-structured matrix (with blocks 
                            corresponding to classes), then this will be 
                            a block diagonal matrix.


            @param  g_off   On output, will contain a copy of this graph
                            but only with edges that are between two nodes
                            belonging to different classes.
                            If the graph incidence matrix is viewed as a
                            block-structured matrix (with blocks 
                            corresponding to classes), then this matrix
                            will have zeroes on the block diagonals.

            @param  sw  Where to report timing information (nowhere if 0).
      */
      void splitAndExport(const static_classifier &c, bool ksl,
        static_graph &g_diag, static_graph &g_off, timer_hook *sw);


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
                        (possibly changing) list of nodes to explore,
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


    protected:
      // Read-only access to internal storage
      inline long RowPointer(long s) const {
        CHECK_RANGE(0, s, num_nodes+1); 
        return row_pointer[s];
      }
      inline long ColumnIndex(long e) const {
        CHECK_RANGE(0, e, num_edges); 
        return column_index[e];
      }
      inline long Next(long e) const {
        CHECK_RANGE(0, e, num_edges); 
        return next[e];
      }
      inline const void* Label(long e) const {
        CHECK_RANGE(0, e, num_edges); 
        return label + e*edge_size;
      }
      inline void* WriteLabel(long e) {
        CHECK_RANGE(0, e, num_edges); 
        return label + e*edge_size;
      }

    private:
      // Return true if the edge was added; false if it was a duplicate.
      bool AddToMergedCircularList(long &list, long ptr);
      void AddToUnmergedCircularList(long &list, long ptr);

      long Defragment(long);


    private:
      // Details for SCC computation, see sccs.h and sccs.cc
      class scc_traversal;
      // friend class dynamic_graph::scc_traversal;

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

  // ======================================================================
  // |                                                                    |
  // |                   dynamic_weighted_digraph class                   |
  // |                                                                    |
  // ======================================================================

  /**
      Directed graphs with summable edge weights.
  */
  template <class TYPE>
  class dynamic_summable : public dynamic_graph {
  public:
    dynamic_summable(bool ksl, bool merge)
    : dynamic_graph(sizeof(TYPE), ksl, merge) { };

    inline bool addEdge(long from, long to, const TYPE &val) {
      return dynamic_graph::addEdge(from, to, &val);
    }

    inline TYPE Value(long e) const {
      return * (static_cast <const TYPE*>(Label(e)));
    }
    inline TYPE& WriteValue(long e) {
      return * (static_cast <TYPE*>(WriteLabel(e)));
    }

    /**
        Add row sums to array sums.
          @param  sums    Array of dimension at least num_nodes.
                          We do NOT zero out the array first,
                          so take care that this array is initialized
                          before calling this method.
    */
    template <class ACC>
    inline void addRowSums(ACC* sums) const {
      if (isByRows()) {
        for (long s=0; s<getNumNodes(); s++) {
          long ptr = RowPointer(s);
          if (ptr<0) continue;
          // loop over edges
          do {
            sums[s] += Value(ptr);
            ptr = Next(ptr);
          } while (ptr != RowPointer(s));
        } // for s
      } else {
        for (long s=0; s<getNumNodes(); s++) {
          long ptr = RowPointer(s);
          if (ptr<0) continue;
          // loop over edges
          do {
            sums[ ColumnIndex(ptr) ] += Value(ptr);
            ptr = Next(ptr);
          } while (ptr != RowPointer(s));
        } // for s
      }
    };


    /**
        Divide values based on rows.
        Specifically, for each edge (i, j, value),
        divide value by A[i] unless it is zero.
          @param  A     Array of dimension at least num_nodes,
                        Divide all edges from state i by A[i],
                        unless A[i] is zero.
    */
    template <class ACC>
    inline void divideRows(const ACC* A) {
      if (isByRows()) {
        for (long s=0; s<getNumNodes(); s++) {
          if (0==A[s]) continue;  // no point looping over edges
          long ptr = RowPointer(s);
          if (ptr<0) continue;
          // loop over edges
          do {
            WriteValue(ptr) /= A[s];
            ptr = Next(ptr);
          } while (ptr != RowPointer(s));
        } // for s
      } else {
        for (long s=0; s<getNumNodes(); s++) {
          long ptr = RowPointer(s);
          if (ptr<0) continue;
          // loop over edges
          do {
            if (A[ColumnIndex(ptr)]) {
              WriteValue(ptr) /= A[ColumnIndex(ptr)];
            }
            ptr = Next(ptr);
          } while (ptr != RowPointer(s));
        } // for s
      }
    };

    protected:
      virtual void merge_edges(void* ev, const void* nv) const {
        TYPE* tev = (TYPE*) ev;
        TYPE* tnv = (TYPE*) nv;
        *tev += *tnv;
      }

  };  // dynamic_weighted_digraph



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
