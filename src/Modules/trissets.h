
/** \file trissets.h

    Module for tri_stateset
 
*/

#include "statesets.h"

#ifndef TRISSETS_H
#define TRISSETS_H

class intset;

// ******************************************************************
// *                                                                *
// *                      tri_stateset  class                       *
// *                                                                *
// ******************************************************************

class tri_stateset : public stateset {
  public:
    tri_stateset(const state_lldsm* p, stateset* t, stateset* f);
    tri_stateset(const state_lldsm* p, stateset* t, stateset* f);
    tri_stateset(const state_lldsm* p, stateset* t);
    tri_stateset(const state_lldsm* p, const stateset* t);
  protected:
    virtual ~tri_stateset();

  public:
    virtual tri_stateset* DeepCopy() const = 0;
    virtual bool Complement();
    virtual bool Union(const expr* c, const char* op, const stateset* x);
    virtual bool Intersect(const expr* c, const char* op, const stateset* x);
    virtual bool Plus(const expr* c, const char* op, const stateset* x);

    inline const stateset* getTrueSet() const;
    inline const stateset* getFalseSet() const;
    virtual stateset* computeUnknownSet() const;

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
};

#endif

