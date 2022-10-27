
/** \file expl_ssets.h

    Module for set_statesets, implemented explicity with a bitvector.

*/

#include "set_statesets.h"

#ifndef EXPL_SSETS_H
#define EXPL_SSETS_H

class intset;

// ******************************************************************
// *                                                                *
// *                      expl_set_stateset  class                      *
// *                                                                *
// ******************************************************************

class expl_set_stateset : public set_stateset {
  public:
    expl_set_stateset(const state_lldsm* p, intset* e);
  protected:
    virtual ~expl_set_stateset();

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

    inline const intset& getExplicit() const {
      DCASSERT(data);
      return *data;
    }

    inline intset& changeExplicit() {
      DCASSERT(data);
      return *data;
    }
  private:
    intset* data;
};

#endif

