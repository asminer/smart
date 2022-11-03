
/** \file expl_trissets.h

    Module for set_statesets, implemented explicity with a pair of bitvectors.

*/

#include "expl_ssets.h"
#include "set_statesets.h"

#ifndef EXPL_TRISSETS_H
#define EXPL_TRISSETS_H

class intset;

// ******************************************************************
// *                                                                *
// *                      expl_tri_set_stateset  class                      *
// *                                                                *
// ******************************************************************

class expl_tri_set_stateset : public set_stateset {
  public:
    expl_tri_set_stateset(const state_lldsm* p, intset* t, intset* f);
    expl_tri_set_stateset(const state_lldsm* p, expl_stateset* t, expl_stateset* f);
  protected:
    virtual ~expl_tri_set_stateset();

  public:
    virtual set_stateset* DeepCopy() const;
    virtual bool Complement();
    virtual bool Union(const expr* c, const char* op, const set_stateset* x);
    virtual bool Intersect(const expr* c, const char* op, const set_stateset* x);
    virtual bool Plus(const expr* c, const char* op, const set_stateset* x);

    // virtual void getCardinality(long &card) const;
    // virtual void getCardinality(result &x) const;

    virtual void getTrueCardinality(long &card) const;
    virtual void getTrueCardinality(result &x) const;
    virtual void getFalseCardinality(long &card) const;
    virtual void getFalseCardinality(result &x) const;
    virtual void getUnknownCardinality(long &card) const;
    virtual void getUnknownCardinality(result &x) const;

    // virtual bool isEmpty() const;

    virtual bool Print(OutputStream &s, int) const;
    virtual bool Equals(const shared_object *o) const;

    // inline const intset& getExplicit() const {
    //   DCASSERT(data);
    //   return *data;
    // }

    // inline intset& changeExplicit() {
    //   DCASSERT(data);
    //   return *data;
    // }

  private:
    expl_stateset *trueset, *falseset;
    // or expl_stateset*?
};

#endif

