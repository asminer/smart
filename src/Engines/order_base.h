
#ifndef ORDER_BASE_H
#define ORDER_BASE_H

#include "../ExprLib/engine.h"

// **************************************************************************
// *                                                                        *
// *                         static_varorder  class                         *
// *                                                                        *
// **************************************************************************

/// underlying static variable ordering engine base class.
class static_varorder : public subengine {
  friend class init_static_varorder;
protected:
  static reporting_msg report;
  static debugging_msg debug;
public:
  static_varorder();
  virtual ~static_varorder();

  // Any useful helpers?
};

#endif
