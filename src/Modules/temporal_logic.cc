#include "../Options/options.h"
#include "../ExprLib/startup.h"
#include "../ExprLib/exprman.h"
#include "../ExprLib/unary.h"
#include "../ExprLib/binary.h"
#include "../ExprLib/assoc.h"
#include "../SymTabs/symtabs.h"
#include "../ExprLib/functions.h"

#include "../Formlsms/graph_llm.h"


#include "temporal_logic.h"

// ******************************************************************
// *                                                                *
// *                   state_formula_type  class                    *
// *                                                                *
// ******************************************************************

class state_formula_type : public simple_type {
public:
  state_formula_type();
};

// ******************************************************************
// *                  state_formula_type  methods                   *
// ******************************************************************

state_formula_type::state_formula_type() : simple_type("stateformula", "State properties", "Type used to express state properties, used for CTL model checking and other operations.")
{
  setPrintable();
}

// ******************************************************************
// *                                                                *
// *                   state_formula_type  class                    *
// *                                                                *
// ******************************************************************

class path_formula_type : public simple_type {
public:
  path_formula_type();
};

// ******************************************************************
// *                  state_formula_type  methods                   *
// ******************************************************************

path_formula_type::path_formula_type() : simple_type("pathformula", "Path properties", "Type used to express path properties, used for LTL model checking and other operations.")
{
  setPrintable();
}

// ******************************************************************
// *                                                                *
// *                                                                *
// *                         Initialization                         *
// *                                                                *
// *                                                                *
// ******************************************************************

class init_temporal : public initializer {
public:
  init_temporal();
  virtual bool execute();
};
init_temporal the_temporal_initializer;

init_temporal::init_temporal() : initializer("init_temporal")
{
  usesResource("em");
  buildsResource("temporaltype");
  buildsResource("types");
}

bool init_temporal::execute()
{
  if (0==em)  return false;

  em->registerType(  new path_formula_type   );
  em->registerType(  new state_formula_type  );
  em->setFundamentalTypes();

  return true;
}
