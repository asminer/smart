
/** \file expl_ssets.h

    Module for meddly tri statesets

*/
#include "statesets.h"
#include "meddly_ssets.h"
//#include "trissets.h"

#ifndef MEDDLY_TRI_SSETS_H
#define MEDDLY_TRI_SSETS_H

// These are all in glue_meddly.h
class shared_domain;
class shared_ddedge;
class meddly_encoder;

// ******************************************************************
// *                                                                *
// *                     meddly_tri_stateset  class                 *
// *                                                                *
// ******************************************************************

class meddly_tri_stateset: public stateset{
  public:
    meddly_tri_stateset(const state_lldsm* p, shared_domain*, meddly_encoder*, shared_ddedge*);
    meddly_tri_stateset(const state_lldsm* p, shared_domain* dom, meddly_encoder* enc, shared_ddedge* t, shared_ddedge* f);
    meddly_tri_stateset(const state_lldsm* p, meddly_stateset* trueset, meddly_stateset* falseset);
    meddly_tri_stateset(const meddly_tri_stateset* clone, shared_ddedge* set);
  protected:
    virtual ~meddly_tri_stateset();

  public:
    virtual meddly_tri_stateset* DeepCopy() const;
    virtual bool Complement();
    virtual bool Union(const expr* c, const char* op, const stateset* x);
    virtual bool Intersect(const expr* c, const char* op, const stateset* x);
    virtual bool Plus(const expr* c, const char* op, const stateset* x);

    inline const meddly_stateset* getMeddlyTrueSet() const {
        return trueset;
      };
      inline const meddly_stateset* getMeddlyFalseSet() const {
        return falseset;
      };
      inline const state_lldsm* getParent() const {
        return parent;
      };
      virtual meddly_stateset* computeMeddlyUnknownSet() const;

    virtual void getCardinality(long &card) const;
    virtual void getCardinality(result &x) const;

    virtual void getTrueCardinality(long &card) const;
    virtual void getTrueCardinality(result &x) const;
    virtual void getFalseCardinality(long &card) const;
    virtual void getFalseCardinality(result &x) const;
    virtual void getUnknownCardinality(long &card) const;
    virtual void getUnknownCardinality(result &x) const;

    virtual bool isEmpty() const;

    virtual shared_state* getSingleState() const;
    //virtual void Select();
    //virtual void Offset(int offset);

    virtual bool Print(OutputStream &s, int) const;
    virtual bool Equals(const shared_object *o) const;

  private:
    const state_lldsm* parent;  
    meddly_stateset *trueset, *falseset;
};

#endif

