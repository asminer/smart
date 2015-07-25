
// $Id$

/**   \file lslib.h
      Library for linear solvers.
*/

#ifndef LSLIB_H
#define LSLIB_H

/**
  Get the name and version info of the library.
  The string should not be modified or deleted.

  @return    Information string.
*/
const char*  LS_LibraryVersion();


///  Specific linear solution types.
enum LS_Method {
  /**  Power method.
       For Ax = 0 systems only.
  */
  LS_Power,

  ///  Jacobi method, one row at a time.
  LS_Row_Jacobi,

  ///  Jacobi method, using vector-matrix multiply.
  LS_Jacobi,

  ///  Gauss-Seidel method.
  LS_Gauss_Seidel

  // other, fancy methods?  block methods?
};


/// Error codes.
enum LS_Error {
  /// No errors.
  LS_Success = 0,

  /// Solution method doesn't work with given matrix format.
  LS_Wrong_Format,

  /** Solution method doesn't work with the type of equation.
      Allows us to use methods that work only for "Ax=0".
  */
  LS_Illegal_Method,

  /// Unable to allocate auxiliary vectors.
  LS_Out_Of_Memory,

  /// Convergence criteria was not achieved.
  LS_No_Convergence,

  /// Not yet implemented...
  LS_Not_Implemented
};


/**  Options to set for solvers.
*/
struct LS_Options {
  /// Debug; displays vector after each iteration.
  bool debug;
  /// Which solver to use.
  LS_Method method;
  /// Auxiliary vectors, are floats?  Otherwise doubles.
  bool float_vectors;
  /// Use relaxation or not
  bool use_relaxation;
  /// (Initial) relaxation parameter to use.
  double relaxation;
  /// Number of iterations before checking for convergence.
  long min_iters;
  /// Maximum number of iterations.
  long max_iters;
  /// Convergence, based on relative precision? (otherwise, absolute)
  bool use_relative;
  /// Desired precision
  double precision;
public:
  /// Constructor.  Allows us to set reasonable defaults.
  LS_Options() {
    debug = false;
    method = LS_Row_Jacobi;
    float_vectors = 0;
    use_relaxation = 0;
    relaxation = 1.0;
    min_iters = 10;
    max_iters = 10000;
    use_relative = 1;
    precision = 1e-6;
  }
};


/**  Outputs for solvers.
*/
struct LS_Output {
  /// Number of iterations required.
  long num_iters;
  /// Relaxation parameter used
  double relaxation;
  /// Precision achieved
  double precision;
  /// Any error information
  LS_Error status;
};


/**  Matrix data structure of floats.
     We use the "standard" row pointer, column index format.
     Negated diagonals are stored separately.
     Values should be either all floats or all doubles.
*/
struct LS_Matrix {
  /// If true, the matrix is stored by columns, rather than by rows.
  bool is_transposed;
  /// First row to use (normally 0)
  long start;
  /// One plus the last row to use (normally matrix size)
  long stop;
  /// list for each row.  Dimension is at least \a stop + 1.
  const long* rowptr;
  /// column indexes, dimension is at least \a rowptr[stop]
  const long* colindex;
  /// values, as floats
  const float* f_value;
  /// values, as doubles
  const double* d_value;
  /// negated reciprocals of diagonals, as floats
  const float* f_one_over_diag;
  /// negated reciprocals of diagonals, as doubles
  const double* d_one_over_diag;

  /// Computes y += x * this;
  inline void VectorMatrixMultiply(double* y, const double* x) const 
  {
    if (is_transposed)  VMM_transposed(y, x);
    else                VMM_regular(y, x);
  }

  /// Computes y += x * this;
  inline void VectorMatrixMultiply(double* y, const float* x) const
  {
    if (is_transposed)  VMM_transposed(y, x);
    else                VMM_regular(y, x);
  }

  /// Computes y += this * x;
  inline void MatrixVectorMultiply(double* y, const double* x) const
  {
    if (is_transposed)  VMM_regular(y, x);
    else                VMM_transposed(y, x);
  }

  /// Computes y += this * x;
  inline void MatrixVectorMultiply(double* y, const float* x) const
  {
    if (is_transposed)  VMM_regular(y, x);
    else                VMM_transposed(y, x);
  }

private:
  void VMM_transposed(double *y, const double* x) const; 
  void VMM_transposed(double *y, const float* x) const; 

  void VMM_regular(double *y, const double* x) const; 
  void VMM_regular(double *y, const float* x) const; 
};
  

/**  Vector data structure.
     The vector can be stored either as "truncated full" or "sparsely".
     Also, the values may be floats or doubles.
*/
struct LS_Vector {
  /// Size of the following arrays.
  long size;
  /** Indexes of nonzeroes (dimension is \a size).
      If NULL, then the vector is stored "in full".
      If non-null, then the indexes must be in order.
  */
  const long* index;
  /// Values of nonzeroes (dimension is \a size), as doubles.
  const double* d_value;
  /// Values of nonzeroes (dimension is \a size), as floats.
  const float* f_value;
};


// experimental: what is the overhead of a generic interface?

class LS_Abstract_Matrix {
  long start;
  long stop;
protected:
  long size;
public:
  LS_Abstract_Matrix(long start, long stop, long s);
  virtual ~LS_Abstract_Matrix();

  inline long Start() const { return start; }
  inline long Stop() const { return stop; }
  inline long GetSize() const { return size; }

  virtual bool IsTransposed() const = 0;

  /** Basic operation for (most) solvers.
      Computes:
        answer += x * row r of matrix;
        answer /= diagonal[r];

      @param   r      Current row number
      @param   x      Vector to multiply by
      @param   answer Answer goes here (mind the input value)
  */
  virtual void SolveRow(long r, const double* x, double& answer) const = 0;

  /** Basic operation for (most) solvers.
      Computes:
        answer += x * row r of matrix;
        answer /= diagonal[r];

      @param   r        Current row number
      @param   x        Vector to multiply by
      @param   answer   Answer goes here (mind the input value)

      @return  The next row (for cases where this is not in order)
  */
  virtual void SolveRow(long r, const float* x, double& answer) const = 0;


  // for jacobi

  /** y = (this without diagonals) * x 
  */
  // virtual void MV_NoDiag_Mult(const float* x, double* y) const = 0;

  /** y = (this without diagonals) * x 
  */
  // virtual void MV_NoDiag_Mult(const double* x, double* y) const = 0;


  /** y = (this without diagonals) * x
      Called when IsTransposed() returns false.
      Default behavior: throws LS_Not_Implemented.
  */
  virtual void NoDiag_MultByRows(const float* x, double* y) const = 0;

  /** y = (this without diagonals) * x
      Called when IsTransposed() returns false.
      Default behavior: throws LS_Not_Implemented.
  */
  virtual void NoDiag_MultByRows(const double* x, double* y) const = 0;

  /** y = (this without diagonals) * x
      Called when IsTransposed() returns true.
      Default behavior: throws LS_Not_Implemented.
  */
  virtual void NoDiag_MultByCols(const float* x, double* y) const = 0;

  /** y = (this without diagonals) * x
      Called when IsTransposed() returns true.
      Default behavior: throws LS_Not_Implemented.
  */
  virtual void NoDiag_MultByCols(const double* x, double* y) const = 0;


  /*
      Compute y += (this without diagonals) * old
      Call this when we don't know how the matrix is stored
  */
  template <class REAL2>
  inline void Multiply(double *y, const REAL2* old) const {
    if (IsTransposed())   NoDiag_MultByCols(old, y);
    else                  NoDiag_MultByRows(old, y);
  }

  /** Compute x[i] *= scalar / diag[i].
      Default behavior: throws LS_Not_Implemented.
  */
  virtual void DivideDiag(double* x, double scalar) const = 0;
};


/**  Solve the linear system Ax = 0 for x.
     Since any scalar multiple of x is also a solution,
     we arbitrarily choose to normalize x so that its
     elements sum to 1.
        @param  A     The matrix A.
        @param  x     The solution vector x.
                      On input: the initial "guess" solution to use.
                      On output: the solution, if the method converged.
        @param  opts  Solver options.
        @param  out   Status of solution.
*/
void Solve_AxZero(const LS_Matrix &A, double* x, const LS_Options &opts, LS_Output &out);



/**  Solve the linear system Ax = 0 for x.
     Since any scalar multiple of x is also a solution,
     we arbitrarily choose to normalize x so that its
     elements sum to 1.
        @param  A     The matrix A.
        @param  x     The solution vector x.
                      On input: the initial "guess" solution to use.
                      On output: the solution, if the method converged.
        @param  opts  Solver options.
        @param  out   Status of solution.
*/
void Solve_AxZero(const LS_Abstract_Matrix &A, double* x, const LS_Options &opts, LS_Output &out);



/**  Solve the linear system Ax = b for x.
     In this version, vector b is sparse.
        @param  A     The matrix A.
        @param  x     The solution vector x.
                      On input: the initial "guess" solution to use.
                      On output: the solution, if the method converged.
        @param  b     Sparse, rhs vector.
        @param  opts  Solver options.
        @param  out   Status of solution.
*/
void Solve_Axb(const LS_Matrix &A, double* x, const LS_Vector &b, const LS_Options &opts, LS_Output &out);


/**  Solve the linear system Ax = b for x.
     In this version, vector b is sparse.
        @param  A     The matrix A.
        @param  x     The solution vector x.
                      On input: the initial "guess" solution to use.
                      On output: the solution, if the method converged.
        @param  b     Sparse, rhs vector.
        @param  opts  Solver options.
        @param  out   Status of solution.
*/
void Solve_Axb(const LS_Abstract_Matrix &A, double* x, const LS_Vector &b, const LS_Options &opts, LS_Output &out);


#endif
