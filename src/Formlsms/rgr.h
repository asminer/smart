
// $Id:$

#ifndef RGR_H
#define RGR_H

#include "check_llm.h"

/**
    Base class for reachability graphs.
    Basically a Kripke structure, but without the labelling function.
    Used mainly by fsm models, and used as a base class for Markov chains.
*/
class reachgraph : public shared_object {
    const checkable_lldsm* parent;
  public:
    reachgraph();
    virtual ~reachgraph();

    inline void setParent(const checkable_lldsm* p) {
      if (parent != p) {
        DCASSERT(0==parent);
        parent = p;
      }
    }

    inline const checkable_lldsm* getParent() const {
      return parent;
    }

    inline const hldsm* getGrandParent() const {
      return parent ? parent->GetParent() : 0;
    }

    // What virtual functions here?
    // virutal void getNumArcs(result &na) const;  // default: use a long
    // virtual void getNumArcs(long &na) const = 0;
    // virtual void showInternal(OutputStream &os);
    
    // which of these will belong here?

    // checkable requirements
    // virtual bool isAbsorbing(long st) const;
    // virtual bool isDeadlocked(long st) const;

    // virtual void findDeadlockedStates(stateset &ss) const;
    // virtual bool forward(const intset &p, intset &r) const;
    // virtual bool backward(const intset &p, intset &r) const;

    // virtual bool dumpDot(OutputStream &s) const;

    /**
      Show all the edges, in the desired order.
        @param  os    Output stream to write to
        @param  dispo Display order to use.  See class lldsm
        @param  st    Memory space to use for individual states

      TBD - reachset is needed, should it be a parameter or is it 
      needed for so much stuff that the class keeps a pointer to it?

    */
    void showArcs(OutputStream *os, int dispo, shared_state* st);

    // TBD?
    // void visitArcs();  to visit in any order

    // Shared object requirements
    virtual bool Print(OutputStream &s, int width) const;
    virtual bool Equals(const shared_object* o) const;
};

#endif

