
// $Id$

#ifndef MEASURES_H
#define MEASURES_H

#include "variables.h"
//@Include: variables.h

/** @name measures.h 
    @type File
    @args \ 

   Implementation of measures.

 */

//@{


// ******************************************************************
// *                                                                *
// *                         measure  class                         *
// *                                                                *
// ******************************************************************


/** Measures, within models.
    Like deterministic constants, except we don't replace the return
    expression with the return value until told.
 */
class measure : public constfunc {
  engineinfo *eng;
  result value;
public:
  measure(const char *fn, int line, type t, char *n);
  virtual ~measure();
  virtual void Compute(int i, result &x);
  virtual void ShowHeader(OutputStream &s) const;
  virtual Engine_type GetEngine(engineinfo *e);
  virtual expr* SplitEngines(List <measure> *mlist);

  /// Call this after we are computed
  inline void SetValue(const result &v) {
    Delete(return_expr);
    value = v;
    state = CS_Computed;
  }
};





//@}

#endif

