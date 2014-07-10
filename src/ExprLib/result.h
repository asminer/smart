
// $Id$

#ifndef RESULT_H
#define RESULT_H

#include "../include/shared.h"

class type;
class option;
class io_environ;
class option_manager;
class engine_manager;
class symbol;

//#define OLD_INTERFACE

// ******************************************************************
// *                                                                *
// *                          result class                          *
// *                                                                *
// ******************************************************************


/**   The structure used by expressions to represent values.
 
      Conventions:

      Whenever an error occurs, the value should be set to
      null by calling setNull().
      (A value can be null when no error occurs.)

*/  
class result {
protected:
  /// Are we a special case, or not?
  enum {
    /// An ordinary value
    Normal = 0,
    /// Plus or minus infinity
    Infinity,
    /// Null value
    Null,
    /// Unknown value
    Unknown,
    /// Variable out of bounds; used for next-state computations
    Out_Of_Bounds
  } special;

  /// Used by boolean and integer type
  long   ivalue;
  /// Used by real and expo type
  double rvalue;
  /// Used by Out_Of_Bounds
  const symbol* var;
  /// Everything else
  shared_object* other; 
  
public:
  /// Default, empty constructor.
  result();
  /// Make an ordinary integer result.
  result(long a);
  /// Make an ordinary real result.
  result(double a);
  /// Make an ordinary pointer result.
  result(shared_object* a);
  /// Copy constructor.
  result(const result& x);
  /// Destructor.
  ~result();
  /// An explicit constructor, if you ever use malloc.
  void constructFrom(const result& x);
  /// Assignment operator.
  void operator= (const result& x);

  /// Is this a normal value?
  inline bool isNormal() const { return Normal == special; }

  /// Is this an unknown value?
  inline bool isUnknown() const { return Unknown == special; }
  inline void setUnknown() { 
    Delete(other);
    other = 0;
    special = Unknown; 
  }

  /// Was there a bounds violation?
  inline bool isOutOfBounds() const { return Out_Of_Bounds == special; }
  inline void setOutOfBounds(const symbol* who, long badval) {
    Delete(other);
    var = who;
    ivalue = badval;
    special = Out_Of_Bounds;
  }
  inline const symbol* getOutOfBounds() const {
    DCASSERT(Out_Of_Bounds == special);
    return var;
  }

  /// Are we infinite.  Sign is determined from the value.
  inline bool isInfinity() const { return Infinity == special; }
  inline void setInfinity(int sign) { 
    Delete(other);
    other = 0;
    DCASSERT(sign);
    ivalue = (sign<0) ? -1 : 1;
    special = Infinity; 
  }
  inline int signInfinity() const { 
    DCASSERT(isInfinity()); 
    return ivalue; 
  }

  /// Are we a null value?  
  inline bool isNull() const { return Null == special; }
  inline void setNull() { 
    Delete(other);
    other = 0;
    special = Null; 
  }

  inline void setBool(bool v) {
    Delete(other);
    other = 0;
    special = Normal;
    ivalue = v;
  }
  inline bool getBool() const {
    DCASSERT(Normal == special);
    return ivalue;
  }
  
  inline void setInt(long v) {
    Delete(other);
    other = 0;
    special = Normal;
    ivalue = v;
  }
  inline long getInt() const {
    DCASSERT(Normal == special || Out_Of_Bounds == special);
    return ivalue;
  }

  inline void setReal(double v) {
    Delete(other);
    other = 0;
    special = Normal;
    rvalue = v;
  }
  inline double getReal() const {
    DCASSERT(Normal == special);
    return rvalue;
  }

  inline void setPtr(shared_object* v) {
    Delete(other);
    other = v;
    special = Normal;
  }
  inline shared_object* getPtr() const {
    DCASSERT(Normal == special || Null == special);
    return other;
  }
  inline void deletePtr() {
    Delete(other);
    other = 0;
  }

  inline void setConfidence(double mid, shared_object* ci) {
    Delete(other);
    special = Normal;
    rvalue = mid;
    other = ci;
  }
};


/**
    Sort an array of integer results
    (infinity and finite results only)
    from smallest to largest.
    The array is modified.
      @param  A   Array of results.
      @param  n   Size of array A.
*/
void sortIntegers(result* A, int n);

/**
    Sort an array of real results
    (infinity and finite results only)
    from smallest to largest.
    The array is modified.
      @param  A   Array of results.
      @param  n   Size of array A.
*/
void sortReals(result* A, int n);

#endif

