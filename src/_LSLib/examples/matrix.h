
// $Id$

#include "lslib.h"

class sparse_matrix {
private:
  /** Number of "allocated" nodes.
      We allocate several at a time.
  */
  long nodes_alloc;

  /** Pointer to arcs from a given node.
      Array of dimension \a nodes_alloc.
      We actually use \a num_nodes+1 entries.
      (The extra pointer is unused in "dynamic" mode.)
      Each entry is a pointer into arrays \a column_index
      and \a next, for entries in a given row.
  */
  long* row_pointer;   

  /** Number of "allocated" edges.
      We allocate several at a time.
  */
  long edges_alloc;

  /** Array of (target node of an edge).
      The array dimension is \a edges_alloc.
      Only \a num_edges of the entries are in use, though.
      Each entry corresponds to the "target node" of an arc.
  */
  long* column_index;

  /** Array of weights.
      The array dimension is \a edges_alloc.
      Only \a num_edges of the entries are in use, though.
      Each entry corresponds to the weight of the associated arc.
  */
  float* value;

  /** Array of next pointers.
      In static mode, this is always NULL.
      The array dimension is \a edges_alloc.
      Only \a num_edges of the entries are in use, though.
      The index of the next entry in the list, 
      with a negative value used for "null".
  */
  long* next;

protected:
  /// Are we currently stored "by rows".
  bool is_by_rows;
  /// Are we currently "static".
  bool is_static;
  /// Number of nodes.
  long num_nodes;
  /// Number of edges.
  long num_edges;
public:
  /// Constructor.
  sparse_matrix(long nodes, long edges, bool transpose);

  /// Destructor.
  ~sparse_matrix();
  
  /** Are we currently stored "by rows".
      Can be toggled (with some computation) by calling Transpose().
        @return  true if we are stored by rows, false otherwise.
  */
  inline bool IsByRows() const { return is_by_rows; }

  /** Are we currently in a "static" state.
      Can be made true by calling ConvertToStatic().
      Can be made false by calling ConvertToDynamic().
        @return  true if we are "static", false if we are "dynamic".
  */
  inline bool IsStatic() const { return is_static; }
  
  /** (Current) number of nodes in the graph.
  */
  inline long NumNodes() const { return num_nodes; }

  /** (Current) number of edges in the graph.
  */
  inline long NumEdges() const { return num_edges; }

  /** Add a new element to the matrix.
      This will fail if the matrix is "static".
        @param  from    Node the edge starts in.
        @param  to      Node the edge ends at.
        @param  weight  Weight, will be cast to float
  */
  void AddElement(long from, long to, double weight);

  /** Convert the graph to static storage.
      Quick success if the graph is already static.
  */
  void ConvertToStatic();

  /** Export the graph into the format expected for linear solvers.
      We must be by rows.
  */
  void ExportTo(LS_CRS_Matrix_float &A) const;

  /** Export the graph into the format expected for linear solvers.
      We must be by columns.
  */
  void ExportTo(LS_CCS_Matrix_float &A) const;

protected:
  /// Resize the row pointer array.  Returns true on success.
  bool ResizeNodes(long new_nodes);

  /// Resize the column index, next arrays.  Returns true on success.
  bool ResizeEdges(long new_edges);

  /// Add to circular list i, not in order.
  void AddToCircularList(long i, long ptr); 

  /** Converts the circular linked lists to null-terminated ones.
      Used right before conversion to static.
  */
  void CircularToTerminated();

  /** Swaps edges so that linked lists are in contiguous order in memory.
      Lists must be null-terminated, not circular.
      Used to convert from dynamic to static.
        @param  first_slot  Position to use for first node, usually 0.
                            But if the arrays are shared for example,
                            this might be something other than 0.
  */
  void Defragment(long first_slot);
};


