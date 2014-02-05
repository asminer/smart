
// $Id$

#include "noevnt_hlm.h"
#include "../ExprLib/mod_vars.h"
#include "../Streams/streams.h"

// ******************************************************************
// *                                                                *
// *                     no_event_model methods                     *
// *                                                                *
// ******************************************************************

no_event_model::no_event_model(model_statevar** vl, int nvs, expr** cl, int ncs)
 : hldsm(No_Events)
{
  varlist = vl;
  num_vars = nvs;
  Constraints_By_Bottom = 0;
  Preprocess(cl, ncs);
  x = new traverse_data(traverse_data::Compute);
  x->answer = &foo;
}

no_event_model::~no_event_model()
{
  for (int v=0; v<num_vars; v++)
    Delete(varlist[v]);
  delete[] varlist;
  delete x;
}

lldsm::model_type no_event_model::GetProcessType() const
{
  return lldsm::RSS;
}

int no_event_model::NumStateVars() const
{
  return num_vars;
}

bool no_event_model::containsListVar() const
{
  return false;
}

void no_event_model::determineListVars(bool* ilv) const
{
  for (int v=0; v<num_vars; v++) ilv[v] = 0;
}

bool no_event_model::Print(OutputStream &s, int) const
{
  s << "no event model:\nVariables:\n\t";
  for (int i=0; i<num_vars; i++)
    s << varlist[i]->Name() << " ";
  for (int i=0; i<num_vars; i++) {
    s << "\nConstraints up to " << varlist[i]->Name() << ":\n";
    for (int j=0; Constraints_By_Bottom[i][j]; j++) {
      s << "\t";
      Constraints_By_Bottom[i][j]->Print(s, 0);
      s << "\n";
    }
  }
  return true;
}

void no_event_model::Preprocess(expr** cl, int ncs)
{
  List <expr> **clists = new List <expr>* [num_vars];
  for (int i=0; i<num_vars; i++)
    clists[i] = new List <expr>;
  List <symbol> symlist;
  for (int i=0; i<ncs; i++) {
    symlist.Clear();
    if (0==cl[i]->BuildSymbolList(traverse_data::GetSymbols, 0, &symlist)) {
      continue;
    }
    // TBD: if we have constraints with no vars, check them now!
    int max = 0;
    for (int j=symlist.Length()-1; j>=0; j--) {
      model_statevar* s = smart_cast <model_statevar*> (symlist.Item(j));
      DCASSERT(s);
      max = MAX(max, s->GetIndex());
    }
    CHECK_RANGE(0, max, num_vars);
    clists[max]->Append(cl[i]);
  }
  for (int i=0; i<num_vars; i++)
    clists[i]->Append(0);
  
  Constraints_By_Bottom = new expr**[num_vars];
  for (int i=0; i<num_vars; i++) {
    Constraints_By_Bottom[i] = clists[i]->CopyAndClear();
    delete clists[i];
  }
}

bool no_event_model::SatisfiesConstraintsAt(int i)
{
  CHECK_RANGE(0, i, num_vars);
  expr** clist = Constraints_By_Bottom[i];
  if (0==clist)  return true;
  for (int j=0; clist[j]; j++) {
    clist[j]->Compute(x[0]);
    DCASSERT(foo.isNormal());
    if (!foo.getBool())  return false;
  }
  return true;
}

void no_event_model::SetState(int* indexes)
{
  for (int i=0; i<num_vars; i++) {
    varlist[i]->SetToValueNumber(indexes[i]);
  }
}

void no_event_model::ShowCurrentState(OutputStream &s) const
{
  s.Put('[');
  for (int i=0; i<num_vars; i++) {
    if (i) s.Put(", ");
    varlist[i]->Compute(x[0]);
    s.Put(varlist[i]->Name());
    s.Put(" = ");
    DCASSERT(varlist[i]->Type());
    varlist[i]->Type()->print(s, foo);
  }
  s.Put(']'); 
}

void no_event_model::reindexStateVars(int &start)
{
  for (int i=0; i<num_vars; i++) {
    varlist[i]->SetIndex(start);
    start++;
  } // for
}

void no_event_model::showState(OutputStream &s, const shared_state* x) const
{
  DCASSERT(0);
}
