
// $Id$

#ifndef RSS_MEDDLY_H
#define RSS_MEDDLY_H

#include "graph_llm.h"
#include "../Modules/glue_meddly.h"

class meddly_reachset : public checkable_lldsm::reachset {
  public:
    meddly_reachset();
    virtual ~meddly_reachset();

    virtual void getNumStates(long &ns) const;
    virtual void getNumStates(result &ns) const;  
    virtual void showInternal(OutputStream &os) const;
    virtual void showState(OutputStream &os, const shared_state* st) const;
    virtual iterator& iteratorForOrder(int display_order);
    virtual iterator& easiestIterator() const;

    virtual void getReachable(result &ss) const;
/*
    // TBD THESE!

    virtual void getPotential(expr* p, result &ss) const;
    virtual void getInitialStates(result &x) const;
*/

  private:
    class lexical_iter : public reachset::iterator {
      public:
        lexical_iter(const meddly_encoder &w, shared_ddedge &s);
        virtual ~lexical_iter();

        virtual void start();
        virtual void operator++(int);
        virtual operator bool() const;
        virtual long index() const;
        virtual void copyState(shared_state* st) const;

      private:
        shared_ddedge &states;
        const meddly_encoder &wrapper;
        MEDDLY::enumerator* iter;
        long i;
    };

  private:
    shared_domain* vars;

    meddly_encoder* mdd_wrap;
    shared_ddedge* initial;
    shared_ddedge* states;

    reachset::iterator* natorder;

// Do we need these here?
// meddly_encoder* index_wrap;
// shared_ddedge* state_indexes;
};

#endif

