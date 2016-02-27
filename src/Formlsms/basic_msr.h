
// $Id$

#ifndef BASIC_MSR_H
#define BASIC_MSR_H

#include "../include/list.h"

// ******************************************************************
// *                                                                *
// *                      proc_noengine  class                      *
// *                                                                *
// ******************************************************************

/// Abstract class for custom engines that require the process.
class proc_noengine : public msr_noengine {
  static engtype* ProcGen;
  friend class init_basicmsrs;
public:
  proc_noengine(eng_class ect, const type* t, const char* name, int np);

protected:
  state_lldsm* BuildProc(hldsm* hlm, bool states_only, const expr* err);
};

#endif
