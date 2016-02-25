
// $Id$

#include "formalism.h"
#include "measures.h"

formalism::formalism(const char* n, const char* sd, const char* ld)
 : simple_type(n, sd, ld)
{
  funcs = 0;
  idents = 0;
}

formalism::~formalism()
{
  delete funcs;
  delete idents;
}

void formalism::addCommonFuncs(List <msr_func> &cfl)
{
  for (int i=0; i<cfl.Length(); i++) {
    msr_func* mf = cfl.Item(i);
    switch (mf->getEngClass()) {
      case msr_func::CTL:
        if (!includeCTL())  continue;
        break;

      case msr_func::Stochastic:
        if (!includeStochastic()) continue;
        break;

      case msr_func::CSL:
        if (!includeCTL())  continue;
        if (!includeStochastic()) continue;
        break;

      case msr_func::DCP:
        if (!includeDCP())  continue;
        break;

      default:
        break;
    }; // end switch
    DCASSERT(funcs);
    funcs->AddSymbol(mf);
  }
}

bool formalism::isAFormalism() const
{
  return true;
}

bool formalism::isLegalMeasureType(const type* mtype) const
{
  if (0==mtype)                   return 0;
  if (mtype->matches("void"))     return 1;
  if (mtype->matches("bool"))     return 1;
  if (mtype->matches("int"))      return 1;
  if (mtype->matches("real"))     return 1;
  if (mtype->matches("bigint"))   return 1;
  if (includeCTL()) {
    if (mtype->matches("stateset")) return 1;
  }
  if (includeStochastic()) {
    if (mtype->matches("ph int"))     return 1;
    if (mtype->matches("ph real"))    return 1;
    if (mtype->matches("statedist"))  return 1;
    if (mtype->matches("stateprobs")) return 1;
  }
  return 0;
}

bool formalism::includeCTL() const
{
  return false;
}

bool formalism::includeStochastic() const
{
  return false;
}

bool formalism::includeDCP() const
{
  return false;
}
