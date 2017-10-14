
/** \file expl_ssets.h

    Module for statesets, implemented explicity with a bitvector.

*/

#include "statesets.h"

#ifndef EXPL_SSETS_H
#define EXPL_SSETS_H

class intset;

// ******************************************************************
// *                                                                *
// *                      expl_stateset  class                      *
// *                                                                *
// ******************************************************************

class expl_stateset : public stateset {
  public:
    expl_stateset(const state_lldsm* p, intset* e);
  protected:
    virtual ~expl_stateset();

  public:
    virtual stateset* DeepCopy() const;
    virtual bool Complement();
    virtual bool Union(const expr* c, const char* op, const stateset* x);
    virtual bool Intersect(const expr* c, const char* op, const stateset* x);
    virtual bool Plus(const expr* c, const char* op, const stateset* x);

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

