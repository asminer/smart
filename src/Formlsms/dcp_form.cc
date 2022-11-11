
#include "dcp_form.h"
#include "../ExprLib/startup.h"
#include "../ExprLib/exprman.h"
#include "../ExprLib/formalism.h"
#include "../ExprLib/mod_def.h"
#include "../ExprLib/mod_vars.h"
#include "noevnt_hlm.h"

// #define DEBUG_DCP

// **************************************************************************
// *                                                                        *
// *                             dcp_def  class                             *
// *                                                                        *
// **************************************************************************

/** Support for the integer constraint "formalism".
    This is the front-end object that is managed by the parser.
*/
class dcp_def : public model_def {
  symbol* statelist;
  List <expr> *constraints;
  int num_vars;
public:
  dcp_def(const location &W, const type* t, char*n,
      formal_param **pl, int np);

  virtual ~dcp_def();

  // Required for models:
  virtual model_var* MakeModelVar(const symbol* wrap, shared_object* bnds);

  void AddConstraint(expr* c);

protected:
  virtual void InitModel();
  virtual void FinalizeModel(OutputStream& ds);
};

// ******************************************************************
// *                        dcp_def  methods                        *
// ******************************************************************

dcp_def::dcp_def(const location &W, const type* t, char*n,
  formal_param **pl, int np) : model_def(W, t, n, pl, np)
{
  statelist = 0;
  num_vars = 0;
}

dcp_def::~dcp_def()
{
  // need to traverse and kill statelist?
}

model_var* dcp_def::MakeModelVar(const symbol* wrap, shared_object* bnds)
{
  DCASSERT(wrap);
  model_statevar* s;
  const type* wt = wrap->Type();
  if (0==wt) {
    // should we make some noise here?
    DCASSERT(0);
    s = 0;
  } else if (wt->matches("bool")) {
    DCASSERT(0==bnds);
    s = new bool_statevar(wrap, current);
  } else if (wt->matches("int")) {
    DCASSERT(bnds);
    s = new int_statevar(wrap, current, bnds);
  } else {
    DCASSERT(0);
    s = 0;
  }
  if (s) {
    s->LinkTo(statelist);
    statelist = s;
  }
  s->SetIndex(num_vars);
  num_vars++;
  return s;
}

void dcp_def::AddConstraint(expr* c)
{
  DCASSERT(constraints);
#ifdef DEBUG_DCP
  em->cout() << "Adding constraint:\n\t";
  c->Print(em->cout(), 0);
  em->cout() << "\n";
  em->cout().flush();
#endif
  expr* newc = c->Substitute(0);
  newc->BuildExprList(traverse_data::GetProducts, 0, constraints);
  // TBD: delete newc?
}

void dcp_def::InitModel()
{
#ifdef DEBUG_DCP
  em->cout() << "Starting dcp_def...\n";
#endif
  constraints = new List <expr>;
}

void dcp_def::FinalizeModel(OutputStream &ds)
{
#ifdef DEBUG_DCP
  em->cout() << "Finalizing dcp_def...\n";
#endif

  model_statevar** varlist = new model_statevar*[num_vars];
  for (int i = num_vars-1; i>=0; i--) {
    varlist[i] = smart_cast <model_statevar*> (statelist);
    statelist = statelist->Next();
  }
  int num_c = constraints->Length();
  expr** clist = constraints->CopyAndClear();
  delete constraints;
  constraints = 0;

  no_event_model* m;
  m = new no_event_model(varlist, num_vars, clist, num_c);
  ConstructionSuccess(m);

#ifdef DEBUG_DCP
  em->cout() << "Got model:\n";
  m->Print(em->cout(), 0);
  em->cout().flush();
#endif
}

// **************************************************************************
// *                                                                        *
// *                             dcp_form class                             *
// *                                                                        *
// **************************************************************************

class dcp_form : public formalism {
public:
  dcp_form(const char* n, const char* sd, const char* ld);

  virtual model_def* makeNewModel(const location &W, char* name,
          symbol** formals, int np) const;

  virtual bool canDeclareType(const type* vartype) const;
  virtual bool canAssignType(const type* vartype) const;
  virtual bool isLegalMeasureType(const type* mtype) const;
  virtual bool includeDCP() const { return true; }
};


// ******************************************************************
// *                        dcp_form methods                        *
// ******************************************************************

dcp_form::dcp_form(const char* n, const char* sd, const char* ld)
 : formalism(n, sd, ld)
{
}

model_def* dcp_form::makeNewModel(const location &W, char* name,
          symbol** formals, int np) const
{
  // TBD: check formals?
  return new dcp_def(W, this, name, (formal_param**) formals, np);
}

bool dcp_form::canDeclareType(const type* vartype) const
{
  if (0==vartype)                 return 0;
  if (vartype->matches("int"))    return 1;
  if (vartype->matches("bool"))   return 1;
  return 0;
}

bool dcp_form::canAssignType(const type* vartype) const
{
  return 0;
}

bool dcp_form::isLegalMeasureType(const type* mtype) const
{
  if (0==mtype)                 return 0;
  if (mtype->matches("int"))    return 1;
  if (mtype->matches("bool"))   return 1;
  if (mtype->matches("real"))   return 1;
  return 0;
}


// **************************************************************************
// *                                                                        *
// *                             DCP  Functions                             *
// *                                                                        *
// **************************************************************************

// ******************************************************************
// *                           constraint                           *
// ******************************************************************

class dcp_constraint : public model_internal {
public:
  dcp_constraint(const type* t);
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

dcp_constraint::dcp_constraint(const type* t)
 : model_internal(em->VOID, "constraint", 2)
{
  SetFormal(1, em->BOOL, "c");
  SetRepeat(1);
  SetDocumentation("Adds constraints to the model's state variables.");
}

void dcp_constraint::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(pass);
  DCASSERT(pass[0]);
  dcp_def* mdl = smart_cast<dcp_def*>(pass[0]);
  DCASSERT(mdl);

  if (x.stopExecution())  return;
  for (int i=1; i<np; i++) {
    if (0==pass[i])  continue;
    mdl->AddConstraint(Share(pass[i]));
  } // for i
}

// ******************************************************************
// *                             unique                             *
// ******************************************************************

class dcp_unique : public model_internal {
public:
  dcp_unique(const type* t);
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

dcp_unique::dcp_unique(const type* t)
 : model_internal(em->VOID, "unique", 2)
{
  SetFormal(1, em->INT, "v");
  SetRepeat(1);
  SetDocumentation("Adds constraints so that the passed values are all unique.");
}

void dcp_unique::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(pass);
  DCASSERT(pass[0]);
  dcp_def* mdl = smart_cast<dcp_def*>(pass[0]);
  DCASSERT(mdl);

  if (x.stopExecution())  return;

  for (int i=1; i<np; i++) {
    if (0==pass[i]) continue;
    for (int j=i+1; j<np; j++) {
      if (0==pass[j]) continue;

      expr* vineqvj = em->makeBinaryOp(
        x.parent ? x.parent->Where() : location::NOWHERE(),
        Share(pass[i]), exprman::bop_nequal, Share(pass[j])
      );
      DCASSERT(vineqvj);
      mdl->AddConstraint(vineqvj);
    } // for j
  } // for i
}

// ******************************************************************
// *                                                                *
// *                                                                *
// *                         Initialization                         *
// *                                                                *
// *                                                                *
// ******************************************************************

class init_dcps : public initializer {
  public:
    init_dcps();
    virtual bool execute();
};
init_dcps the_dcp_initializer;

init_dcps::init_dcps() : initializer("init_dcps")
{
  usesResource("em");
  usesResource("CML");
  buildsResource("formalisms");
}

bool init_dcps::execute()
{
  if (0==em) return false;

  // Set up and register formalism
  formalism* dcp = new dcp_form("dcp", "discrete constraint program", "foobar");
  if (! em->registerType(dcp)) {
    if (em->startInternal(__FILE__, __LINE__)) {
      em->causedBy(0);
      em->internal() << "Couldn't register dcp type";
      em->stopIO();
    }
    return false;
  }
  symbol_table* dcpsyms = MakeSymbolTable();
  dcpsyms->AddSymbol(  new dcp_constraint(dcp)  );
  dcpsyms->AddSymbol(  new dcp_unique(dcp)      );
  dcp->setFunctions(dcpsyms);
  dcp->addCommonFuncs(CML);

  return true;
}

