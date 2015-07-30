
// $Id$

/**   \file lslib.h
      Library for linear solvers.
*/

#ifndef LSLIB_H
#define LSLIB_H

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


/**
    Matrix in compressed row storage format.
    Except that the diagonal elements are stored separately,
    as their negative reciprocals, in a vector.
    This is a template struct. 
*/
template <class REAL>
struct LS_CRS_Matrix {
  /// Starting row/col, normally 0
  long start;
  /// One plus the last row/col, normally the matrix size
  long stop;
  /// Size, as needed for solution vectors
  long size;

  /// Array of matrix values, dimension at least #nonzeroes
  const REAL* val;
  /// Array of column indexes, dimension at least #nonzeroes
  const long* col_ind;
  /// Array of row pointers, dimension at least stop+1.
  const long* row_ptr;

  /// negated reciprocals of diagonals, dimension at least stop+1.
  const REAL* one_over_diag;


public:
  
  inline long Start() const { return start; }
  inline long Stop() const { return stop; }
  inline long Size() const { return size; }

  /**
      Compute y += (this without diagonals) * x
  */
  template <class REAL2>
  inline void MatrixVectorMultiply(double *y, const REAL2* x) const {
      long a = row_ptr[start];
      for (long i=start; i<stop; i++) {
        for ( ; a < row_ptr[i+1]; a++) {
          y[i] += x[col_ind[a]] * val[a];
        }
      }
  }

  /**
      Compute y += x * (this without diagonals) 
  */
  template <class REAL2>
  inline void VectorMatrixMultiply(double *y, const REAL2* x) const {
      // TBD
      throw LS_Not_Implemented;
  }

  /**
      Compute x[i] *= -1 / diag[i], for all i
  */
  inline void DivideDiag(double* x) const {
      for (long i=start; i<stop; i++) {
        x[i] *= one_over_diag[i];
      }
  }

  /**
      Compute x[i] *= -a / diag[i], for all i
  */
  inline void DivideDiag(double* x, double a) const {
      for (long i=start; i<stop; i++) {
        x[i] *= a * one_over_diag[i];
      }
  }

  /**
      Compute sum += (row i of this matrix without diagonals) * x
  */
  template <class REAL2>
  inline void RowDotProduct(long i, const REAL2* x, double &sum) const {
      for (long a = row_ptr[i]; a < row_ptr[i+1]; a++) {
        sum += x[col_ind[a]] * val[a];
      }
  }

  /**
      Compute sum =  (sum + (row i without diagonals) * x) / -diag[i]
  */
  template <class REAL2>
  inline void SolveRow(long i, const REAL2* x, double &sum) const {
      RowDotProduct(i, x, sum);
      sum *= one_over_diag[i];
  }
};



/**
    Matrix in compressed row storage format, using floats.
*/
struct LS_CRS_Matrix_float : public LS_CRS_Matrix<float> { } ;

/**
    Matrix in compressed row storage format, using doubles.
*/
struct LS_CRS_Matrix_double : public LS_CRS_Matrix<double> { } ;



/**
    Matrix in compressed column storage format.
    Except that the diagonal elements are stored separately,
    as their negative reciprocals, in a vector.
    This is a template struct. 
*/
template <class REAL>
struct LS_CCS_Matrix {
  /// Starting row/col, normally 0
  long start;
  /// One plus the last row/col, normally the matrix size
  long stop;
  /// Size, as needed for solution vectors
  long size;

  /// Array of matrix values, dimension at least #nonzeroes
  const REAL* val;
  /// Array of row indexes, dimension at least #nonzeroes
  const long* row_ind;
  /// Array of col pointers, dimension at least stop+1.
  const long* col_ptr;

  /// negated reciprocals of diagonals, dimension at least stop+1.
  const REAL* one_over_diag;


public:

  inline long Start() const { return start; }
  inline long Stop() const { return stop; }
  inline long Size() const { return size; }
  
  /**
      Compute y += (this without diagonals) * x
  */
  template <class REAL2>
  inline void MatrixVectorMultiply(double *y, const REAL2* x) const {
      long a = col_ptr[start];
      for (long i=start; i<stop; i++) {
        for ( ; a < col_ptr[i+1]; a++) {
          y[row_ind[a]] += x[i] * val[a];
        }
      }
  }

  /**
      Compute y += x * (this without diagonals) 
  */
  template <class REAL2>
  inline void VectorMatrixMultiply(double *y, const REAL2* x) const {
      // TBD
      throw LS_Not_Implemented;
  }

  /**
      Compute x[i] *= -1 / diag[i], for all i
  */
  inline void DivideDiag(double* x) const {
      for (long i=start; i<stop; i++) {
        x[i] *= one_over_diag[i];
      }
  }

  /**
      Compute x[i] *= -a / diag[i], for all i
  */
  inline void DivideDiag(double* x, double a) const {
      for (long i=start; i<stop; i++) {
        x[i] *= a * one_over_diag[i];
      }
  }

  template <class REAL2>
  inline void SolveRow(long i, const REAL2* x, double &sum) const {
    throw LS_Wrong_Format;
  }
};



/**
    Matrix in compressed column storage format, using floats.
*/
struct LS_CCS_Matrix_float : public LS_CCS_Matrix<float> { } ;

/**
    Matrix in compressed column storage format, using doubles.
*/
struct LS_CCS_Matrix_double : public LS_CCS_Matrix<double> { } ;



/**
    Interface for user-defined matrices.
    To use, derive a class from this one and provide
    the required virtual functions.
*/
class LS_Generic_Matrix {
  long start;
  long stop;
  long size;
public:
  LS_Generic_Matrix(long start, long stop, long size);
  virtual ~LS_Generic_Matrix();

  inline long Start() const { return start; }
  inline long Stop() const { return stop; }
  inline long Size() const { return size; }

  /**
      Show information for debugging.
      Used when the debugging option is turned on.
  */
  virtual void ShowDebugInfo() const;

  /**
      Compute y += (this without diagonals) * x.
      Must be overridded in derived class.
  */
  virtual void MatrixVectorMultiply(double *y, const float* x) const  = 0;

  /**
      Compute y += (this without diagonals) * x.
      Must be overridded in derived class.
  */
  virtual void MatrixVectorMultiply(double *y, const double* x) const = 0;


  /**
      Compute x[i] *= -1 / diag[i], for all i
      Must be overridded in derived class.
  */
  virtual void DivideDiag(double* x) const = 0;

  /**
      Compute x[i] *= -a / diag[i], for all i
      Must be overridded in derived class.
  */
  virtual void DivideDiag(double* x, double a) const = 0;


  /**
      Compute sum =  (sum + (row i without diagonals) * x) / -diag[i]
      If we cannot, then throw LS_Wrong_Format (default behavior).
  */
  virtual void SolveRow(long i, const float* x, double &sum) const;
  
  /**
      Compute sum =  (sum + (row i without diagonals) * x) / -diag[i]
      If we cannot, then throw LS_Wrong_Format (default behavior).
  */
  virtual void SolveRow(long i, const double* x, double &sum) const;
};



// ******************************************************************
// *                                                                *
// *                                                                *
// *                       frontend functions                       *
// *                                                                *
// *                                                                *
// ******************************************************************

/**
  Get the name and version info of the library.
  The string should not be modified or deleted.

  @return    Information string.
*/
const char*  LS_LibraryVersion();


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
void Solve_AxZero(const LS_CRS_Matrix_float &A,   double* x, const LS_Options &opts, LS_Output &out);
void Solve_AxZero(const LS_CRS_Matrix_double &A,  double* x, const LS_Options &opts, LS_Output &out);
void Solve_AxZero(const LS_CCS_Matrix_float &A,   double* x, const LS_Options &opts, LS_Output &out);
void Solve_AxZero(const LS_CCS_Matrix_double &A,  double* x, const LS_Options &opts, LS_Output &out);
void Solve_AxZero(const LS_Generic_Matrix &A,     double* x, const LS_Options &opts, LS_Output &out);



/**  Solve the linear system Ax = b for x.
     Vector b may be either sparse or truncated full.
        @param  A     The matrix A.
        @param  x     The solution vector x.
                      On input: the initial "guess" solution to use.
                      On output: the solution, if the method converged.
        @param  b     Sparse, rhs vector.
        @param  opts  Solver options.
        @param  out   Status of solution.
*/
void Solve_Axb(const LS_CRS_Matrix_float &A,  double* x, const LS_Vector &b, const LS_Options &opts, LS_Output &out);
void Solve_Axb(const LS_CRS_Matrix_double &A, double* x, const LS_Vector &b, const LS_Options &opts, LS_Output &out);
void Solve_Axb(const LS_CCS_Matrix_float &A,  double* x, const LS_Vector &b, const LS_Options &opts, LS_Output &out);
void Solve_Axb(const LS_CCS_Matrix_double &A, double* x, const LS_Vector &b, const LS_Options &opts, LS_Output &out);
void Solve_Axb(const LS_Generic_Matrix &A,    double* x, const LS_Vector &b, const LS_Options &opts, LS_Output &out);



/* ======================================================================

    OLD INTERFACE BELOW SOON TO BE TRASHED!

====================================================================== */

#if 0

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
  inline long Size() const { return size; }

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

  /** Compute x[i] *= 1 / diag[i].
      Default behavior: throws LS_Not_Implemented.
  */
  virtual void DivideDiag(double* x) const = 0;

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

#endif  // if1 or if0 to cut rest of this crap

#endif
