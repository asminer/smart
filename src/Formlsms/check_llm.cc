
// $Id$

#include "check_llm.h"

// ******************************************************************
// *                                                                *
// *                    checkable_lldsm  methods                    *
// *                                                                *
// ******************************************************************

checkable_lldsm::checkable_lldsm(model_type t) : graph_lldsm(t)
{
}

void checkable_lldsm::getReachable(result &ss) const 
{
  bailOut(__FILE__, __LINE__, "Can't get reachable states");
  ss.setNull();
}

void checkable_lldsm::getInitialStates(result &x) const
{
  bailOut(__FILE__, __LINE__, "Can't get initial states");
  x.setNull();
}

void checkable_lldsm::getAbsorbingStates(result &x) const
{
  bailOut(__FILE__, __LINE__, "Can't get absorbing states");
  x.setNull();
}

void checkable_lldsm::getDeadlockedStates(result &x) const
{
  bailOut(__FILE__, __LINE__, "Can't get deadlocked states");
  x.setNull();
}

void checkable_lldsm::getPotential(expr* p, result &ss) const 
{
  bailOut(__FILE__, __LINE__, "Can't get potential states");
  ss.setNull();
}

bool checkable_lldsm::isFairModel() const
{
  return false;
}

void checkable_lldsm::getTSCCsSatisfying(stateset &p) const
{
  DCASSERT(isFairModel());
  bailOut(__FILE__, __LINE__, "Can't get TSCCs satisfying p");
}

void checkable_lldsm::findDeadlockedStates(stateset &) const
{
  bailOut(__FILE__, __LINE__, "Can't find deadlocked states");
}

bool checkable_lldsm::forward(const intset &p, intset &r) const
{
  bailOut(__FILE__, __LINE__, "Can't compute forward set");
  return false;
}

bool checkable_lldsm::backward(const intset &p, intset &r) const
{
  bailOut(__FILE__, __LINE__, "Can't compute backward set");
  return false;
}

bool checkable_lldsm::isAbsorbing(long st) const
{
  bailOut(__FILE__, __LINE__, "Can't check absorbing");
  return false;
}

bool checkable_lldsm::isDeadlocked(long st) const
{
  bailOut(__FILE__, __LINE__, "Can't check deadlocked");
  return false;
}


