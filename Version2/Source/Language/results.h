
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
#include "shared.h"

//@{



/// Special cases for a result
enum compute_special {
  /// An ordinary value
  CS_Normal = 0,
  /// Plus or minus infinity
  CS_Infinity,
  /// Null value
  CS_Null,
  /// Same as null, but due to an error
  CS_Error,
  /// Unknown value
  CS_Unknown
};

/** Strings.
    The only reason we use this is so we can "share" strings
    without copying them.
*/
class shared_string : public shared_object {
public:
  char* string;
  shared_string() { string = NULL; }
  shared_string(char* s) { string = s; }
  virtual~ shared_string() { delete[] string; }
  inline void show(OutputStream &s) { s.Put(string); }
};

// ******************************************************************
// *                                                                *
// *                          result class                          *
// *                                                                *
// ******************************************************************

/**   The structure used by expressions to represent values.
 
      Completely redesigned for version 2:
      No more derived classes or virtual functions for the 
      "core" types (ints, reals, booleans).

      Conventions:

      Whenever an error occurs, the value should be set to
      null by setting null to true.
      (A value can be null when no error occurs.)

      For +- infinity, set infinity to true.
      Then, set the sign appropriately for ivalue.

*/  

struct result {
  public:
  /// Are we a special value, or not
  compute_special special;

  public:

  // No type!  can be determined from expression that computes us, if needed

  union {
    /// Used by boolean type
    bool   bvalue;
    /// Used by integer type
    int    ivalue;
    // Should we add bigint here?
    /// Used by real and expo type
    double rvalue;
    /// Used by strings
    shared_string* svalue;
    /// Everything else
    shared_object* other; 
  };

  /// Is this a normal value?
  inline bool isNormal() const { return CS_Normal == special; }

  /// Is this an unknown value?
  inline bool isUnknown() const { return CS_Unknown == special; }
  inline void setUnknown() { special = CS_Unknown; }

  /// Are we infinite.  Sign is determined from the value.
  inline bool isInfinity() const { return CS_Infinity == special; }
  inline void setInfinity() { special = CS_Infinity; }

  /// Was there an error?
  inline bool isError() const { return CS_Error == special; }
  inline void setError() { special = CS_Error; }

  /// Are we a null value?  
  inline bool isNull() const { return CS_Null == special; }
  inline void setNull() { special = CS_Null; ivalue = 0; }

  inline void Clear() {
    special = CS_Normal;
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
*/
void DeleteResult(type t, result &x);

/** Check equality of two results.
    Used primarily by models to check passed parameters.
*/
bool Equals(type t, const result &x, const result &y);

/** Copy two results.
    Shares whenever possible.
*/
inline void CopyResult(type t, result &dest, const result &src)
{
  dest = src;
  switch (t) {
    case STRING:
	Share(dest.svalue);
  }
}

//@}

#endif

