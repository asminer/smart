
/** \file expl_trissets.h

    Module for expl_tri_stateset, implemented explicity with a pair of bitvectors.

*/

#include "expl_ssets.h"
#include "trissets.h"

#ifndef EXPL_TRISSETS_H
#define EXPL_TRISSETS_H

// ******************************************************************
// *                                                                *
// *                      expl_tri_stateset  class                  *
// *                                                                *
// ******************************************************************

class expl_tri_stateset : public tri_stateset {
  public:
    expl_tri_stateset(const state_lldsm* p, intset* t, intset* f);
    expl_tri_stateset(const state_lldsm* p, expl_stateset* t, expl_stateset* f);
    expl_tri_stateset(const state_lldsm* p, stateset* t, stateset* f);
    expl_tri_stateset(const state_lldsm* p, const expl_stateset* t);
  protected:
    virtual ~expl_tri_stateset();

  public:
    virtual expl_tri_stateset* DeepCopy() const;
    virtual bool Complement();
    virtual bool Union(const expr* c, const char* op, const stateset* x);
    virtual bool Intersect(const expr* c, const char* op, const stateset* x);
    virtual bool Plus(const expr* c, const char* op, const stateset* x);

    inline const expl_stateset* getExplicitTrueSet() const {
      return trueset;
    };
    inline const expl_stateset* getExplicitFalseSet() const {
      return falseset;
    };
    virtual expl_stateset* computeExplicitUnknownSet() const;
    expl_stateset* computeUnknownSet() const override;

    virtual void getCardinality(long &card) const;
    virtual void getCardinality(result &x) const;

    virtual void getTrueCardinality(long &card) const;
    virtual void getTrueCardinality(result &x) const;
    virtual void getFalseCardinality(long &card) const;
    virtual void getFalseCardinality(result &x) const;
    virtual void getUnknownCardinality(long &card) const;
    virtual void getUnknownCardinality(result &x) const;
    

    virtual bool isEmpty() const;

    virtual bool isTrueEmpty() const;
    virtual bool isFalseEmpty() const;

    virtual bool Print(OutputStream &s, int) const;
    virtual bool Equals(const shared_object *o) const;

    inline const intset& getExplicit() const {
      DCASSERT(trueset);
      return trueset->getExplicit();
    }

    inline intset& changeExplicit() {
      DCASSERT(trueset);
      return trueset->changeExplicit();
    }

  private:
    expl_stateset *trueset, *falseset;
};

#endif

