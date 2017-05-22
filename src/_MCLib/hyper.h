
#ifndef HYPER_H
#define HYPER_H

#include "../_GraphLib/graphlib.h"
#include "../_LSLib/lslib.h"
#include "../_IntSets/intset.h"

/** Hyper-sparse, by rows, matrix storage.
    Conceptually: list of non-empty rows,
    each non-empty row is a list of non-zero matrix elements.
*/
class hypersparse_matrix {
public:
  /// Pointer to list of rows (tail, circular list).
  long row_tailptr;

  /// Number of non-empty rows.
  long num_rows;

  /** Number of "allocated" rows.
      We allocate several at a time.
  */
  long rows_alloc;

  /// Number of edges.
  long num_edges;

  /** Number of "allocated" edges.
      We allocate several at a time.
  */
  long edges_alloc;

  /** Index of a given row.
      Array of dimension \a rows_alloc.
      Entry i tells the index of the ith non-empty row.
  */
  long* row_index;

  /** Pointer to arcs from a non-empty row.
      Array of dimension \a rows_alloc.
      We actually use \a num_rows+1 entries.
      (The extra pointer is unused in "dynamic" mode.)
      Entry i is an index into arrays \a column_index,
      \a value, and \a next, for entries in the ith non-empty row.
  */
  long* row_pointer;   

  /** Next non-empty row.
      Array of dimension \a rows_alloc.
      Allows us to add rows not in order,
      and still maintain a list of ordered rows
      for faster insertions.
  */
  long* next_row;

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

  /// Are we currently "static".
  bool is_static;
public:
  /// Constructor.
  hypersparse_matrix();

  /// Destructor.
  ~hypersparse_matrix();
  
  /** Are we currently in a "static" state.
      Can be made true by calling ConvertToStatic().
      Can be made false by calling ConvertToDynamic().

      @return  true if we are "static", false if we are "dynamic".
  */
  inline bool IsStatic() const { return is_static; }
  
  /** (Current) number of non-empty rows.
  */
  inline long NumRows() const { return num_rows; }

  /** (Current) number of edges in the graph.
  */
  inline long NumEdges() const { return num_edges; }

  /** Add a new element to the matrix.
      This will fail if the matrix is "static".

      @param  from    Node the edge starts in.
      @param  to      Node the edge ends at.
      @param  weight  Matrix element value
  
      @return  0      If the element was previously zero
                      (new edge in matrix);
               1      If the element was previously non-zero
                      (weights will be summed).
  */
  bool AddElement(long from, long to, float weight);

  /** Get the first non-empty row.
      The matrix must be "static".

      @param  index   Index of the first non-empty row
      @param  handle  Internal handle, for use with getNextRow().

      @return  true  Iff there is at least one non-empty row.
  */
  bool getFirstRow(long &index, long &handle);

  /** Get the next non-empty row.
      The matrix must be "static".

      @param  index   Index of the next non-empty row
      @param  handle  On input: previous non-empty row handle
                      On output: next non-empty row handle.

      @return  true  Iff we found another non-empty row.
  */
  bool getNextRow(long &index, long &handle);

  /** Get the sum of entries in a given row.
      The matrix must be "static".

      @param  h  Handle of the row of interest.
      @param  x  The sum is added to x.
  */
  inline void getRowSum(long h, double &x) {
    for (long z = row_pointer[h]; z<row_pointer[h+1]; z++)
      x += value[z];
  }

  /** Normalize rows according to a given vector of row sums.
      This must be used when we are representing a submatrix.

        @param  rowsums  Entries in row i are divided by rowsums[i].
  */
  void NormalizeRows(const double* rowsums);

  /** Visit nonzero elements in a given row.
      Will be efficient only if the matrix is stored "by rows".

      @param  i   Row to visit.

      @param  x   We call x(element) for each nonzero element 
                  in row \a i.  If this returns true, we stop 
                  the traversal.

      @return  true, iff we stopped the traversal early because of x.
  */
  bool traverseRow(long i, GraphLib::generic_graph::element_visitor &x);

  /** Visit nonzero elements in a given column.
      Will be efficient only if the matrix is stored "by columns".

      @param  i   Column to visit.

      @param  x   We call x(element) for each nonzero element 
                  in column \a i.  If this returns true, we 
                  stop the traversal.

      @return  true, iff we stopped the traversal early because of x.
  */
  bool traverseCol(long i, GraphLib::generic_graph::element_visitor &x);

  /** Visit all nonzero elements.

      @param  x   We call x(element) for each nonzero element.
                  If this returns true, we stop the traversal.

      @return  true, iff we stopped the traversal early because of x.
  */
  bool traverseAll(GraphLib::generic_graph::element_visitor &x);

  /** Convert the graph to static storage.
      Quick success if the graph is already static.

      @param  normalize_rows  If true, normalize each non-empty
                              matrix row so that it sums to one.

      @param  tighten_memory  If true, be aggressive about
                              reclaiming unused memory.
  */
  void ConvertToStatic(bool normalize_rows, bool tighten_memory);

  /** Obtain non-empty row number i.
      The matrix must be static.

      @param  i     The non-empty row number to obtain.
                    We should have 0 <= i < NumRows().

      @param  row   Output: index of the ith non-empty row.

      @param  A     Output: filled with the ith non-empty row.
  */
  void ExportRow(long i, long& row, LS_Vector* A) const;

  /** Obtain non-empty row number i.
      The matrix must be static.

      @param  i   The non-empty row number to obtain.
                  We should have 0 <= i < NumRows().

      @param  A   Output: filled with the ith non-empty row.
  */
  void ExportRowCopy(long i, LS_Vector &A) const;

  /** Perform a quick reset, keeping memory.
      Allows us to build several different matrices using
      the same chunk of memory.
      Sets the state back to dynamic.
  */
  void Clear();

  /** Dump contents to standard output.
      Useful for debugging.
  */
  void Dump();

  /** Report total memory required.
  */
  long ReportMemTotal() const;


  /// Computes y += x * this;
  template <typename REAL>
  inline void VectorMatrixMultiply(double* y, const REAL* x) const {
      // assumes Static storage
      long z = row_pointer[0];
      for (long r = 0; r<num_rows; r++) {
        double xi = x[row_index[r]];
        for (; z<row_pointer[r+1]; z++) {
          y[column_index[z]] += value[z] * xi;
        } // for z
      } // for r
  }

  /// Computes y += this * x;
  template <typename REAL>
  inline void MatrixVectorMultiply(double* y, const REAL* x) const {
      // assumes Static storage
      long z = row_pointer[0];
      for (long r = 0; r<num_rows; r++) {
        for (; z<row_pointer[r+1]; z++) {
          y[row_index[r]] += value[z] * x[column_index[z]];
        } // for z
      } // for r
  }


  /// Computes y += x * this, in "reachability" sense.
  bool getForward(const intset& x, intset &y) const;

  /// Computes x += this * y, in "reachability" sense.
  bool getBackward(const intset& y, intset &x) const;


  /// For simulation: find next state by edge value
  void selectEdge(long &state, double acc, double u) const;

  /// For simulation, works only for static storage.
  inline bool findOutgoingEdge(long fr, long &to, double &sum, double u) const{
    long i = findRowIndex(fr);
    if (i<0) return false;
    for (long z = row_pointer[i]; z<row_pointer[i+1]; z++) {
      sum += value[z];
      if (sum >= u) {
        to = column_index[z];
        return true;
      }
    } // for z
    return false;
  }

protected:
  /** Find or make a new row ri.
      Resizes the row arrays as necessary.
      Returns the slot for ri in the row arrays, or -1 on error (no memory).
  */
  long GetRow(long ri);

  /// Resize the column index, next arrays.
  void ResizeEdges(long new_edges);

  /// Add to circular list 
  //  bool AddToCircularList(long& list, long ptr); 

  /** Converts the circular linked lists to null-terminated ones.
      Used right before conversion to static.
  */
  void CircularToTerminated();

  /** Find the index of a row.
      The matrix must be "static".
        @return   The index of row r, if row r is non-empty;
                  -1, if row r is empty.
  */
  inline long findRowIndex(long r) const {
    long low = 0;
    long high = num_rows;
    while (low < high) {
      long mid = (high+low)/2;
      if (r == row_index[mid])  return mid;
      if (r <  row_index[mid])  high = mid;
      else                      low = mid+1;
    };
    return -1;
  };

};

#endif
