
// $Id$

#ifndef RESULTS_H
#define RESULTS_H

/** @name results.h
    @type File
    @args \ 

  New, very fast, and simple way to represent results.
  Virtual-function free!

*/

#include "../Base/api.h"

//@{

/// Things that can go wrong when computing a result.
enum compute_error {
  /** No problems.  
      By forcing the value to zero we should be able to do things like
      "if (error) ..."
   */
  CE_Ok = 0,
  /// We haven't been computed yet
  CE_Uncomputed,
  /// We encountered overflow when casting to another result type
  CE_Overflow,
  /// Divide by zero
  CE_ZeroDivide,
  /// Undefined quantity such as infinity-infinity
  CE_Undefined,
  /// Tried to compute a rand
  CE_ComputeRand,
  /// Array range error
  CE_OutOfRange,
  /// There was a stack overflow
  CE_StackOverflow
};


// ******************************************************************
// *                                                                *
// *                          result class                          *
// *                                                                *
// ******************************************************************

/**   The structure used by expressions to represent values.
 
      Completely redesigned for version 2:
      No more derived classes or virtual functions.

      Conventions:

      Whenever an error occurs, the value should be set to
      null by setting null to true.
      (A value can be null when no error occurs.)

      For +- infinity, set infinity to true.
      Then, set the sign appropriately for ivalue.

      For pointer values, set the boolean flag "canfree" if the
      pointer needs to be freed when we're done
*/  

struct result {
  protected:
    unsigned char flags;  // Use the inline functions
  public:

  // No type!  can be determined from expression that computes us, if needed

  /// The first thing that went wrong while computing us.
  compute_error error;

  /// Is this an unknown value?
  inline bool isUnknown() const { return (flags & 1); }
  inline void setUnknown() { flags |= 1; }

  /// Are we infinite.  Sign is determined from the value.
  inline bool isInfinity() const { return (flags & 2); }
  inline void setInfinity() { flags |= 2; }

  /// Are we a null value?  
  inline bool isNull() const { return (flags & 4); }
  inline void setNull() { flags |= 4; }

  /// For pointers, should we free it?  (allows shallow copy of strings!)
  inline bool isFreeable() const { return (flags & 8); }
  inline void setFreeable() { flags |= 8; }
  inline void notFreeable() { flags &= 247; }

  union {
    /// Used by boolean type
    bool   bvalue;
    /// Used by integer type
    int    ivalue;
    // Should we add bigint here?
    /// Used by real and expo type
    double rvalue;
    /// Everything else
    void*  other; 
  };

  inline void Clear() {
    error = CE_Ok;
    flags = 0;
  }
};


// ******************************************************************
// *                                                                *
// *                  Global functions for output                   *
// *                                                                *
// ******************************************************************

/** Print a result.
    Basically a gigantic switch statement for the type.
    But that's ok, because I/O is slow anyway.
    Note that certain types cannot be printed.
 */
void PrintResult(OutputStream &s, type t, const result &x, int width=-1, int prec=-1);

/** Delete a result.
    I.e., free the pointer if necessary.
    Note: this is necessary for general things,
    like destroying passed parameters.
    For specifics, you should know how to delete it yourself
    (e.g., you know it is a string, so cast to a char* and call free)
*/
void DeleteResult(type t, result &x);

//@}

#endif

