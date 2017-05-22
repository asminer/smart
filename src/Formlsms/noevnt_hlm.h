
#ifndef NOEVENT_HLM_H
#define NOEVENT_HLM_H

#include "../ExprLib/mod_inst.h"

// ******************************************************************
// *                                                                *
// *                      no_event_model class                      *
// *                                                                *
// ******************************************************************

class model_statevar;
class expr;

/** High-level models that have no events.
    Currently this is used for optimization problems.
*/
class no_event_model : public hldsm {
private:
  traverse_data *x;   // for SatisfiesConstraintsAt and ShowCurrentState.
  result foo;         // also.
protected:
  model_statevar** varlist;
  int num_vars;
  expr*** Constraints_By_Bottom;   // list per variable, can be empty
public:
  no_event_model(model_statevar** vl, int nvs, expr** cl, int ncs);
protected:
  virtual ~no_event_model();
public:
  virtual lldsm::model_type GetProcessType() const;
  virtual int NumStateVars() const;
  virtual bool containsListVar() const;
  virtual void determineListVars(bool *) const;
  virtual bool Print(OutputStream &, int) const;
  inline int NumVars() const { return num_vars; }
  inline model_statevar* GetVar(int i) {
    DCASSERT(varlist);
    CHECK_RANGE(0, i, num_vars);
    return varlist[i];
  }
  bool SatisfiesConstraintsAt(int i);
  void SetState(int* indexes);
  void ShowCurrentState(OutputStream &s) const;
  virtual void reindexStateVars(int &start);
  virtual void showState(OutputStream &s, const shared_state* x) const;

  inline expr** GetConstraintsAtLevel(int i) {
    DCASSERT(Constraints_By_Bottom);
    return Constraints_By_Bottom[i];
  }
protected:
  void Preprocess(expr** cl, int ncs);
};


#endif

