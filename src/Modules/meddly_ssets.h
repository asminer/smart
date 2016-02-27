
// $Id$

/** \file expl_ssets.h

    Module for statesets, implemented explicity with a bitvector.

*/

#include "statesets.h"

#ifndef MEDDLY_SSETS_H
#define MEDDLY_SSETS_H

// These are all in glue_meddly.h
class shared_domain;
class shared_ddedge;
class meddly_encoder;

// ******************************************************************
// *                                                                *
// *                     meddly_stateset  class                     *
// *                                                                *
// ******************************************************************

class meddly_stateset : public stateset {
  public:
    meddly_stateset(const state_lldsm* p, shared_domain*, meddly_encoder*, shared_ddedge*);

    meddly_stateset(const meddly_stateset* clone, shared_ddedge* set);
  protected:
    virtual ~meddly_stateset();

  public:
    virtual stateset* DeepCopy() const;
    virtual bool Complement();
    virtual bool Union(const expr* c, const char* op, const stateset* x);
    virtual bool Intersect(const expr* c, const char* op, const stateset* x);

    virtual void getCardinality(long &card) const;
    virtual void getCardinality(result &x) const;

    virtual bool isEmpty() const;

    virtual bool Print(OutputStream &s, int) const;
    virtual bool Equals(const shared_object *o) const;

    inline const shared_ddedge* getStateDD() const {
      return states;
    }

  private:
    shared_domain* vars;
    meddly_encoder* mdd_wrap;
    shared_ddedge* states;
};

#endif

