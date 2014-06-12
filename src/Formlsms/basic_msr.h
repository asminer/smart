
// $Id$

#ifndef BASIC_MSR_H
#define BASIC_MSR_H

#include "../include/list.h"

class exprman;
class msr_func;

void InitBasicMeasureFuncs(exprman* em, List <msr_func> *common);

// Kind of a hack, sorry
#ifdef EXPERT_BASIC_MSR

// ******************************************************************
// *                                                                *
// *                      proc_noengine  class                      *
// *                                                                *
// ******************************************************************

/// Abstract class for custom engines that require the process.
class proc_noengine : public msr_noengine {
  static engtype* ProcGen;
  friend void InitBasicMeasureFuncs(exprman* em, List <msr_func> *common);
public:
  proc_noengine(eng_class ect, const type* t, const char* name, int np);

protected:
  lldsm* BuildProc(hldsm* hlm, bool states_only, const expr* err);
};

#endif

#endif
