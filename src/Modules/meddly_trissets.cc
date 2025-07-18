// meddly_trissets.cc - Implementation for meddly_tri_stateset
#include "../ExprLib/exprman.h"
#include "../ExprLib/mod_vars.h"
#include "../Formlsms/graph_llm.h"

#include "meddly_trissets.h"
#include "meddly_ssets.h"
#include "glue_meddly.h"


meddly_tri_stateset::meddly_tri_stateset(
    const state_lldsm* p,
    shared_domain* dom,
    meddly_encoder* enc,
    shared_ddedge* set
):stateset(p)
{
    parent=p;
    trueset = new meddly_stateset(p, dom, enc, Share(set));
    falseset = new meddly_stateset(p, dom, enc,Share(set));
    falseset->Complement();
}
meddly_tri_stateset::meddly_tri_stateset(
    const state_lldsm* p,
    shared_domain* dom,
    meddly_encoder* enc,
    shared_ddedge* t,
    shared_ddedge* f
) : stateset(p)
{
    parent = p;
    trueset = new meddly_stateset(p, dom, enc, Share(t));
    falseset = new meddly_stateset(p, dom, enc, Share(f));
}
meddly_tri_stateset::meddly_tri_stateset(
    const state_lldsm* p,
    meddly_stateset* t,
    meddly_stateset* f
) : stateset(p)
{
    parent = p;
    trueset = Share(t);
    falseset = Share(f);
}
meddly_tri_stateset::meddly_tri_stateset(
    const meddly_tri_stateset* clone,
    shared_ddedge* set
):stateset(clone->getParent())
{
    DCASSERT(clone);
    parent = clone->getParent();
    trueset = Share(clone->trueset);
    falseset = Share(clone->falseset);
    
  //vars = Share(clone->trueset->vars);
  //mdd_wrap = Share(clone->mdd_wrap);
  //states = set;
}

meddly_tri_stateset::~meddly_tri_stateset()
{
    Delete(trueset);
    Delete(falseset);
}

meddly_tri_stateset* meddly_tri_stateset::DeepCopy() const {
    return new meddly_tri_stateset(getParent(),Share(this->trueset->getSharedD()),Share(this->trueset->getMeddlyEncoder()),Share(this->trueset->getStateDD()),Share(this->falseset->getStateDD()));
}

bool meddly_tri_stateset::Complement() {
    meddly_stateset* tmp = trueset;
    trueset = falseset;
    falseset = tmp;
    return true;
}


bool meddly_tri_stateset::Union(const expr* c, const char* op, const stateset* x) {
    const meddly_tri_stateset* mx = dynamic_cast<const meddly_tri_stateset*>(x);
    if (!mx) return false;
    trueset->Union(c, op, mx->trueset);
    falseset->Intersect(c, op, mx->falseset);
    return true;
}

bool meddly_tri_stateset::Intersect(const expr* c, const char* op, const stateset* x) {
    const meddly_tri_stateset* mx = dynamic_cast<const meddly_tri_stateset*>(x);
    if (!mx) {
        return false;
    }
    else{
    trueset->Intersect(c, op, mx->trueset);
    falseset->Intersect(c, op, mx->falseset);
    return true;
    }
}

bool meddly_tri_stateset::Plus(const expr* c, const char* op, const stateset* x) {
    return Intersect(c, op, x);
}

void meddly_tri_stateset::getCardinality(long &card) const {
    long tc, fc;
    trueset->getCardinality(tc);
    falseset->getCardinality(fc);
    card = tc + fc;
}

void meddly_tri_stateset::getCardinality(result &x) const {
    result a, b;
    trueset->getCardinality(a);
    falseset->getCardinality(b);
    //x = a + b;
}

void meddly_tri_stateset::getTrueCardinality(long &card) const {
    trueset->getCardinality(card);
}

void meddly_tri_stateset::getTrueCardinality(result &x) const {
    trueset->getCardinality(x);
}

void meddly_tri_stateset::getFalseCardinality(long &card) const {
    falseset->getCardinality(card);
}

void meddly_tri_stateset::getFalseCardinality(result &x) const {
    falseset->getCardinality(x);
}

void meddly_tri_stateset::getUnknownCardinality(long &card) const {
    meddly_stateset* u = computeMeddlyUnknownSet();
    u->getCardinality(card);
    Delete(u);
}

void meddly_tri_stateset::getUnknownCardinality(result &x) const {
    meddly_stateset* u = computeMeddlyUnknownSet();
    u->getCardinality(x);
    Delete(u);
}

bool meddly_tri_stateset::isEmpty() const {
    return trueset->isEmpty() && falseset->isEmpty();
}

shared_state* meddly_tri_stateset::getSingleState() const {
    return trueset->getSingleState();
}

bool meddly_tri_stateset::Print(OutputStream &s, int) const {
    s.Put("T{");
    trueset->Print(s, 0);
    s.Put("} F{");
    falseset->Print(s, 0);
    s.Put("}");
    return true;
}

bool meddly_tri_stateset::Equals(const shared_object *o) const {
    const meddly_tri_stateset* b = dynamic_cast<const meddly_tri_stateset*>(o);
    if (!b) return false;
    return trueset->Equals(b->trueset) && falseset->Equals(b->falseset);
}

meddly_stateset* meddly_tri_stateset::computeMeddlyUnknownSet() const {
    shared_ddedge* union_tf = new shared_ddedge(trueset->getMeddlyEncoder()->getForest());
    trueset->getMeddlyEncoder()->buildAssoc(trueset->getStateDD(), false, exprman::aop_or,
                                      falseset->getStateDD(), union_tf);
    shared_ddedge* unknown = new shared_ddedge(trueset->getMeddlyEncoder()->getForest());
    trueset->getMeddlyEncoder()->buildUnary(exprman::uop_not, union_tf, unknown);
    Delete(union_tf);
    return new meddly_stateset(trueset->getParent(), Share(trueset->getSharedD()),
                               Share(trueset->getMeddlyEncoder()), unknown);
}
