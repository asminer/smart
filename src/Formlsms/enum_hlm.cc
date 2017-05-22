
// #include "../ExprLib/mod_inst.h"

#include "../Streams/streams.h"
#include "../ExprLib/mod_vars.h"
#include "rss_enum.h"

#include "enum_hlm.h"

// ******************************************************************
// *                                                                *
// *                         llhldsm  class                         *
// *                                                                *
// ******************************************************************

class llhldsm : public hldsm {
  int index;
  const enum_reachset* RSS;
public:
  llhldsm(lldsm* mdl);
  virtual lldsm::model_type GetProcessType() const;
  virtual int NumStateVars() const { return 1; }
  virtual bool containsListVar() const { return false; }
  virtual void determineListVars(bool* ilv) const { ilv[0] = 0; }
  virtual void reindexStateVars(int &start);
  virtual int getNumEvents(bool show) const;
  virtual void showState(OutputStream &s, const shared_state* x) const;
};

llhldsm::llhldsm(lldsm* mdl) : hldsm(Enumerated)
{
  SetProcess(mdl);
  mdl->SetParent(this);
  index = 0;

  state_lldsm* sm = dynamic_cast <state_lldsm*> (mdl);
  RSS = sm ? dynamic_cast <const enum_reachset*> (sm->getRSS()) : 0;
}

lldsm::model_type llhldsm::GetProcessType() const
{
  DCASSERT(process);
  return process->Type();
}

void llhldsm::reindexStateVars(int &start)
{
  index = start;
  start++;
}

int llhldsm::getNumEvents(bool show) const
{
  return 0;
}

void llhldsm::showState(OutputStream &s, const shared_state* x) const
{
  DCASSERT(x);
  DCASSERT(RSS);
  shared_object* foo = RSS->getEnumeratedState(x->get(index));
  DCASSERT(foo);
  const model_enum_value* mev = smart_cast <const model_enum_value*> (foo);
  DCASSERT(mev);
  s.Put(mev->Name());
  Delete(foo);
}

// ******************************************************************
// *                                                                *
// *                           Front  end                           *
// *                                                                *
// ******************************************************************

hldsm* MakeEnumeratedModel(lldsm* mdl)
{
  return new llhldsm(mdl);
}

