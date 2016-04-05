
// $Id$

#include "order_base.h"

#include "../ExprLib/startup.h"
#include "../ExprLib/engine.h"
#include "../Formlsms/dsde_hlm.h"

#include "../Options/options.h"

// **************************************************************************
// *                                                                        *
// *                        static_varorder  methods                        *
// *                                                                        *
// **************************************************************************

named_msg static_varorder::report;
named_msg static_varorder::debug;

static_varorder::static_varorder()
 : subengine()
{
}

static_varorder::~static_varorder()
{
}

// **************************************************************************
// *                                                                        *
// *                          user_varorder  class                          *
// *                                                                        *
// **************************************************************************

class user_varorder : public static_varorder {
  public:
    user_varorder();
    virtual ~user_varorder();

    virtual bool AppliesToModelType(hldsm::model_type mt) const;
    virtual void RunEngine(hldsm* m, result &);
};
user_varorder the_user_varorder;

// **************************************************************************
// *                         user_varorder  methods                         *
// **************************************************************************

user_varorder::user_varorder()
{
}

user_varorder::~user_varorder()
{
}

bool user_varorder::AppliesToModelType(hldsm::model_type mt) const
{
  return (hldsm::Asynch_Events == mt);
}

void user_varorder::RunEngine(hldsm* hm, result &)
{
  DCASSERT(hm);
  DCASSERT(AppliesToModelType(hm->Type()));

  if (hm->hasPartInfo()) return;  // already done!

  dsde_hlm* dm = dynamic_cast <dsde_hlm*> (hm);
  DCASSERT(dm);

  if (debug.startReport()) {
    debug.report() << "using user-defined variable order\n";
    debug.stopIO();
  }
  dm->useDefaultVarOrder();
}

// ******************************************************************
// *                                                                *
// *                                                                *
// *                         Initialization                         *
// *                                                                *
// *                                                                *
// ******************************************************************

class init_static_varorder : public initializer {
  public:
    init_static_varorder();
    virtual bool execute();
};
init_static_varorder the_static_varorder_initializer;

init_static_varorder::init_static_varorder() : initializer("init_static_varorder")
{
  usesResource("em");
  buildsResource("varorders");
  buildsResource("engtypes");
}

bool init_static_varorder::execute()
{
  if (0==em)  return false;

  // Initialize options
  option* report = em->findOption("Report");
  option* debug = em->findOption("Debug");

  static_varorder::report.Initialize(report,
    "varorder",
    "When set, static variable ordering heuristic performance is reported.",
    false
  );

  static_varorder::debug.Initialize(debug,
    "varorder",
    "When set, static variable ordering heuristic details are displayed.",
    false
  );

  MakeEngineType(em,
      "VariableOrdering",
      "Algorithm to use to determine the (static) variable order for a high-level model",
      engtype::Model
  );
  
  RegisterEngine(em,
    "VariableOrdering",
    "USER_DEFINED",
    "Variable order is determined by calls to partition() in the model",
    &the_user_varorder
  );

  return true;
}

