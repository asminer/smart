
// $Id$

#include "dcp_msr.h"
#include "../ExprLib/measures.h"
#include "../ExprLib/engine.h"

// #define DEBUG_DCP


// *****************************************************************
// *                                                               *
// *                           DCP_engine                          *
// *                                                               *
// *****************************************************************

class dcp_engine : public msr_func {
  engtype* whicheng;
public:
  dcp_engine(const type* t, const char* name, engtype* w);
  virtual measure* buildMeasure(traverse_data &x, expr** pass, int np);
};

dcp_engine::dcp_engine(const type* t, const char* name, engtype* w)
 : msr_func(DCP, t, name, 2)
{
  whicheng = w;
}

measure* dcp_engine::buildMeasure(traverse_data &x, expr** pass, int np)
{
  if (0==whicheng)  return 0;
  measure* m = new measure(pass[1], whicheng, x.model, pass[1]);
  return m;
}


// ******************************************************************
// *                            maximize                            *
// ******************************************************************

class dcp_maximize : public dcp_engine {
public:
  dcp_maximize(engtype* w);
};

dcp_maximize::dcp_maximize(engtype* w)
 : dcp_engine(em->REAL, "maximize", w)
{
  SetFormal(1, em->REAL, "c");
  SetDocumentation("Finds variables that maximize the given expression.");
}

// ******************************************************************
// *                            minimize                            *
// ******************************************************************

class dcp_minimize : public dcp_engine {
public:
  dcp_minimize(engtype* w);
};

dcp_minimize::dcp_minimize(engtype* w)
 : dcp_engine(em->REAL, "minimize", w)
{
  SetFormal(1, em->REAL, "c");
  SetDocumentation("Finds variables that minimize the given expression.");
}

// ******************************************************************
// *                           satisfiable                          *
// ******************************************************************

class dcp_satisfiable : public dcp_engine {
public:
  dcp_satisfiable(engtype* w);
};

dcp_satisfiable::dcp_satisfiable(engtype *w)
 : dcp_engine(em->BOOL, "satisfiable", w)
{
  SetFormal(1, em->BOOL, "c");
  SetDocumentation("Finds variables that satisfy the given expression.");
}

// **************************************************************************
// *                                                                        *
// *                               Front  end                               *
// *                                                                        *
// **************************************************************************

void InitDCPMeasureFuncs(exprman* em, List <msr_func> *common)
{
  // Add engine types
  engtype* MaxExpr = MakeEngineType(em,
      "MaxExpr",
      "Algorithm to use to find the variable assignments to maximize an expression",
      engtype::Single
    );

  engtype* MinExpr = MakeEngineType(em,
      "MinExpr",
      "Algorithm to use to find the variable assignments to minimize an expression",
      engtype::Single
    );

  engtype* SatExpr = MakeEngineType(em,
      "SatExpr",
      "Algorithm to use to find variable assignments, if any, so that a given boolean expression evaluates to true",
      engtype::Single
    );

  // "state space" engines
  MakeEngineType(em, 
      "ExplicitDCSolve",
      "Algorithm used to build explicit list of variable assignments that satisfy model constraints",
      engtype::Single
  );

  MakeEngineType(em,
      "ImplicitDCSolve",
      "Algorithm used to build implicit list of variable assignments that satisfy model constraints",
      engtype::Single
  );

  // Add functions
  if (0==common) return;

  common->Append( new dcp_maximize(MaxExpr) );
  common->Append( new dcp_minimize(MinExpr) );
  common->Append( new dcp_satisfiable(SatExpr)  );
}

