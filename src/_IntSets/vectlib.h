
/**  \file vectlib.h
  Library for large, sparse vectors.

  All vectors have two modes:
    "dynamic", in which elements may be modified;
    "static", in which elements are fixed, but may
        require less memory.

  Note: the library is NOT threadsafe, because
  there is a "global" stack.
*/

#ifndef VECTLIB_H
#define VECTLIB_H

/**
  Get the name and version info of the library.
  The string should not be modified or deleted.
  SV stands for "sparse vector".
  
  @return    Information string.
*/
const char*  SV_LibraryVersion();


/**  
  Static, sparse vector data structure.
  Used for "exporting" information from
  one of the dynamic classes below.
*/
struct SV_Vector {
  /// Dimension of the following arrays.
  long nonzeroes;
  /**  Indexes of nonzeroes.
  Dimension is \a nonzeroes.
  */
  const long* index;
  /**
  Integer values.
  Dimension is \a nonzeroes.
  Will be NULL for bit vectors or real vectors.
  */
  const long* i_value;
  /**
  Real values.
  Dimension is \a nonzeroes.
  Will be NULL for bit vectors or integer vectors.
  */
  const double* r_value;
};


// ******************************************************************
// *                    *
// *                    *
// *                        binary  vectors                         *
// *                    *
// *                    *
// ******************************************************************


/**  Abstract base class for traversing bitvectors.
*/
class bitvector_traverse {
public:
  bitvector_traverse();
  virtual ~bitvector_traverse();
  virtual void Visit(long index) = 0;
};

/**
  Abstract base class for sparse bitvectors.
  Alternatively, this can be viewed as a set of
  integers (the set of indices whose
  vector elements are non-zero).

  TODO: convert this to a "forest" of vectors?
*/

class sparse_bitvector {
protected:
  /// Number of "nonzero" vector elements.
  long num_elements;
  /// Are we static?
  bool is_static;
public:
  /// Constructor.
  sparse_bitvector();
  /// Destructor.
  virtual ~sparse_bitvector();

  inline long NumNonzeroes() const { return num_elements; }
  inline bool IsStatic() const { return is_static; }
  inline bool IsDynamic() const { return !is_static; }
  
  /**
  Determine the value of a specified vector element.
  @param  index  The index to check.
  @return  The vector element at the given index; either 0 or 1.
  */
  virtual bool GetElement(long index) = 0;

  /**
  Set the specified vector element to 1.
  @param  index  The index to set.
  @return  0  if the vector element was not previously set.
    1  if the vector element was previously set.
    -1  if the vector is static.
    -2  if an error occurred (i.e., not enough memory).
  */
  virtual int SetElement(long index) = 0;

  /**
  Set the specified vector element to 0.
  @param  index  The index to clear.
  @return  0  if the vector element was not previously set.
    1  if the vector element was previously set.
    -1  if the vector is static.
    -2  if an error occurred (i.e., not enough memory).
  */
  virtual int ClearElement(long index) = 0;

  /**
  Clear the entire vector.
  Sets all elements to 0, even if the vector is static.
  Memory is retained as appropriate.
  */
  virtual void ClearAll() = 0;

  /**
  Convert the vector to static mode.
  @param  tighten  Should we try to tighten memory usage or not?
      Normally should be true, unless we plan to
      change back to dynamic mode.
  @return  true iff the operation succeeded.
  */
  virtual bool ConvertToStatic(bool tighten) = 0;

  /**
  Convert the vector to dynamic mode.
  May fail if there is insufficient memory.
  @return  true iff the operation succeeded.
  */
  virtual bool ConvertToDynamic() = 0;

  /**
  Allows for arbitrary user-defined traversals.
  Works in both static and dynamic modes.
  */
  virtual void Traverse(bitvector_traverse *) = 0;

  /**
  Export the vector into a static format.
  The vector must be "static".
  @param  V  Data structure to hold the exported vector.
  @return  true iff the operation succeeded.
  */
  virtual bool ExportTo(SV_Vector* A) const = 0;
};

/**
  Make a sparse bitvector.
  @param  list2tree  Size at which the internal representation
        should change from a list to a tree.
        If negative, the default value will be used.
        
  @param  tree2list  Size at which the internal representation
        should change from a tree to a list.
        If negative, the default value will be used.

  @return  A new sparse bitvector, or NULL.
*/
sparse_bitvector*  SV_CreateSparseBitvector(int list2tree, int tree2list);



// ******************************************************************
// *                    *
// *                    *
// *                        integer vectors                         *
// *                    *
// *                    *
// ******************************************************************


/**  Abstract base class for traversing integer vectors.
*/
class intvector_traverse {
public:
  intvector_traverse();
  virtual ~intvector_traverse();
  virtual void Visit(long index, long value) = 0;
};

/**
  Abstract base class for sparse vectors of integers.
  Alternatively, this can be viewed as a multiset of
  integers.

  TODO: convert this to a "forest" of vectors?
*/

class sparse_intvector {
protected:
  /// Number of "nonzero" vector elements.
  long num_elements;
  /// Are we static?
  bool is_static;
public:
  /// Constructor.
  sparse_intvector();
  /// Destructor.
  virtual ~sparse_intvector();

  inline long NumNonzeroes() const { return num_elements; }
  inline bool IsStatic() const { return is_static; }
  inline bool IsDynamic() const { return !is_static; }
  
  /**
  Determine the value of a specified vector element.
  @param  index  The index to check.
  @return  The vector element at the given index.
  */
  virtual long GetElement(long index) = 0;

  /**
  Change the specified vector element.
  @param  index  The index of the element to increment.
  @param  delta  Amount to change the element.
  @return  0  if the vector element was previously zero.
    1  if the vector element was previously not zero.
    -1  if the vector is static.
    -2  if an error occurred (i.e., not enough memory).
  */
  virtual int ChangeElement(long index, long delta) = 0;

  /**
  Set the specified vector element to a specific value.
  @param  index  The index to set.
  @return  0  if the vector element was previously zero.
    1  if the vector element was previously not zero.
    -1  if the vector is static.
    -2  if an error occurred (i.e., not enough memory).
  */
  virtual int SetElement(long index, long value) = 0;

  /**
  Clear the entire vector.
  Sets all elements to 0, even if the vector is static.
  Memory is retained as appropriate.
  */
  virtual void ClearAll() = 0;

  /**
  Convert the vector to static mode.
  @param  tighten  Should we try to tighten memory usage or not?
      Normally should be true, unless we plan to
      change back to dynamic mode.
  @return  true iff the operation succeeded.
  */
  virtual bool ConvertToStatic(bool tighten) = 0;

  /**
  Convert the vector to dynamic mode.
  May fail if there is insufficient memory.
  @return  true iff the operation succeeded.
  */
  virtual bool ConvertToDynamic() = 0;

  /**
  Allows for arbitrary user-defined traversals.
  Works in both static and dynamic modes.
  */
  virtual void Traverse(intvector_traverse *) = 0;

  /**
  Export the vector into a static format.
  The vector must be "static".
  @param  V  Data structure to hold the exported vector.
  @return  true iff the operation succeeded.
  */
  virtual bool ExportTo(SV_Vector* A) const = 0;
};

/**
  Make a sparse intvector.
  @param  list2tree  Size at which the internal representation
        should change from a list to a tree.
        If negative, the default value will be used.
        
  @param  tree2list  Size at which the internal representation
        should change from a tree to a list.
        If negative, the default value will be used.

  @return  A new sparse intvector, or NULL.
*/
sparse_intvector*  SV_CreateSparseIntvector(int list2tree, int tree2list);


// ******************************************************************
// *                    *
// *                    *
// *                         real  vectors                          *
// *                    *
// *                    *
// ******************************************************************


/**  Abstract base class for traversing real vectors.
*/
class realvector_traverse {
public:
  realvector_traverse();
  virtual ~realvector_traverse();
  virtual void Visit(long index, double value) = 0;
};

/**
  Abstract base class for sparse vectors of reals.

  TODO: convert this to a "forest" of vectors?
*/

class sparse_realvector {
protected:
  /// Number of "nonzero" vector elements.
  long num_elements;
  /// Are we static?
  bool is_static;
public:
  /// Constructor.
  sparse_realvector();
  /// Destructor.
  virtual ~sparse_realvector();

  inline long NumNonzeroes() const { return num_elements; }
  inline bool IsStatic() const { return is_static; }
  inline bool IsDynamic() const { return !is_static; }
  
  /**
  Determine the value of a specified vector element.
  @param  index  The index to check.
  @return  The vector element at the given index.
  */
  virtual double GetElement(long index) = 0;

  /**
  Change the specified vector element.
  @param  index  The index of the element to increment.
  @param  delta  Amount to change the element.
  @return  0  if the vector element was previously zero.
    1  if the vector element was previously not zero.
    -1  if the vector is static.
    -2  if an error occurred (i.e., not enough memory).
  */
  virtual int ChangeElement(long index, double delta) = 0;

  /**
  Set the specified vector element to a specific value.
  @param  index  The index to set.
  @return  0  if the vector element was previously zero.
    1  if the vector element was previously not zero.
    -1  if the vector is static.
    -2  if an error occurred (i.e., not enough memory).
  */
  virtual int SetElement(long index, double value) = 0;

  /**
  Clear the entire vector.
  Sets all elements to 0, even if the vector is static.
  Memory is retained as appropriate.
  */
  virtual void ClearAll() = 0;

  /**
  Convert the vector to static mode.
  @param  tighten  Should we try to tighten memory usage or not?
      Normally should be true, unless we plan to
      change back to dynamic mode.
  @return  true iff the operation succeeded.
  */
  virtual bool ConvertToStatic(bool tighten) = 0;

  /**
  Convert the vector to dynamic mode.
  May fail if there is insufficient memory.
  @return  true iff the operation succeeded.
  */
  virtual bool ConvertToDynamic() = 0;

  /**
  Allows for arbitrary user-defined traversals.
  Works in both static and dynamic modes.
  */
  virtual void Traverse(realvector_traverse *) = 0;

  /**
  Export the vector into a static format.
  The vector must be "static".
  @param  V  Data structure to hold the exported vector.
  @return  true iff the operation succeeded.
  */
  virtual bool ExportTo(SV_Vector* A) const = 0;
};

/**
  Make a sparse realvector.
  @param  list2tree  Size at which the internal representation
        should change from a list to a tree.
        If negative, the default value will be used.
        
  @param  tree2list  Size at which the internal representation
        should change from a tree to a list.
        If negative, the default value will be used.

  @return  A new sparse realvector, or NULL.
*/
sparse_realvector*  SV_CreateSparseRealvector(int list2tree, int tree2list);

#endif
