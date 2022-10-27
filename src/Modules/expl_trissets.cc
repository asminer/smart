
/** \file expl_ssets.h

    Module for set_statesets, implemented explicity with a bitvector.

*/

#include "set_statesets.h"

#ifndef EXPL_SSETS_H
#define EXPL_SSETS_H

class intset;

// ******************************************************************
// *                                                                *
// *                      expl_tri_set_stateset  class                      *
// *                                                                *
// ******************************************************************

class expl_tri_set_stateset : public set_stateset {
  public:
    expl_tri_set_stateset(const state_lldsm* p, intset* t, intset* f);
  protected:
    virtual ~expl_tri_set_stateset();

  public:
    virtual set_stateset* DeepCopy() const;
    virtual bool Complement();
    virtual bool Union(const expr* c, const char* op, const set_stateset* x);
    virtual bool Intersect(const expr* c, const char* op, const set_stateset* x);
    virtual bool Plus(const expr* c, const char* op, const set_stateset* x);

    virtual void getCardinality(long &card) const;
    virtual void getCardinality(result &x) const;

    virtual bool isEmpty() const;

    virtual bool Print(OutputStream &s, int) const;
    virtual bool Equals(const shared_object *o) const;

    inline const std::vector<stateset*>* getExplicit() const {
      DCASSERT(data);
      return &data;
    }1

    inline std::vector<stateset*>* changeExplicit() {
      DCASSERT(data);
      return &data;
    }

  private:
    int true_idx = 0, false_idx = 1;
};

#endif

