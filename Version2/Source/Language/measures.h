
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
protected:
  engineinfo *eng;
  result value;
  expr* split_return;
  /// For mixed measures, the (non-mixed) measures we depend on.
  measure** dependencies;
  /// Length of dependencies array.
  int num_dependencies;
public:
  measure(const char *fn, int line, type t, char *n);
  virtual ~measure();
  virtual void ClearCache() { } // no cache for measures
  virtual void Compute(int i, result &x);
  virtual void ShowHeader(OutputStream &s) const;
  virtual Engine_type GetEngine(engineinfo *e);
  virtual expr* GetRewardExpr();
  virtual expr* SplitEngines(List <measure> *mlist);

  /** Preprocess the measure.
      If the engine type is mixed, the measure is split into
      an expression that depends on non-mixed measures,
      and these become the dependency list.
      Dependencies are also added to the parameter GlobalList
  */
  void ComputeDependencies(List <measure> *GlobalList);

  /** Number of other measures we depend on.
      Will be 0 unless we are "mixed".
  */
  inline int NumDependencies() const { return num_dependencies; }

  /// Get dependency i.
  inline measure* GetDependency(int i) const {
    CHECK_RANGE(0, i, num_dependencies);
    DCASSERT(dependencies);
    return dependencies[i];
  }

  /// Call this after we are computed
  inline void SetValue(const result &v) {
    Delete(return_expr);
    value = v;
    state = CS_Computed;
  }
};





//@}

#endif

