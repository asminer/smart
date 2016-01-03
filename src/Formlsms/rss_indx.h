
// $Id$

#ifndef RSS_INDX_H
#define RSS_INDX_H

#include "graph_llm.h"

// External libs
#include "lslib.h"

/**
    Special case - explicit reachability sets with indexes per states.

    This prevents us from copying a few methods.

*/
class indexed_reachset : public graph_lldsm::reachset {
  public:
    indexed_reachset();
    virtual ~indexed_reachset();

    virtual void getReachable(result &ss) const;
    virtual void getPotential(expr* p, result &ss) const;
    virtual void getInitialStates(result &x) const;

    void setInitial(LS_Vector &init);

  private:
    class pot_visit : public lldsm::state_visitor {
      expr* p;
      intset &pset;
      result tmp;
      bool ok;
    public:
      pot_visit(const hldsm* mdl, expr* _p, intset &ps);
      inline bool isOK() const { return ok; }
      virtual bool visit();
    };

  private:
    LS_Vector initial;
};

#endif

