
// $Id$

#ifndef RSS_H
#define RSS_H

#include "check_llm.h"

class checkable_lldsm;

/**
    Base class for reachable states.
    Used by several low level models.
*/
class reachset : public shared_object {
    const checkable_lldsm* parent;
  public:
     /**
        Abstract base class for different state orders.
     */
     class iterator {
       public:
        iterator();
        virtual ~iterator();

        /// Reset the iterator back to the beginning
        virtual void start() = 0;

        /// Increment the iterator
        virtual void operator++(int) = 0;

        /// Is the iterator still valid?
        virtual operator bool() const = 0;

        /// Return the index of the current state.
        virtual long index() const = 0;

        /// Copy the current state into st.
        virtual void copyState(shared_state* st) const = 0;
     };

  public:
    reachset();
    virtual ~reachset();

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

    virtual void getNumStates(result &ns) const;  // default: use a long
    virtual void getNumStates(long &ns) const = 0;
    virtual void showInternal(OutputStream &os) const = 0;
    virtual void showState(OutputStream &os, const shared_state* st) const = 0;
    virtual iterator& iteratorForOrder(int display_order) = 0;
    virtual iterator& easiestIterator() const = 0;

    /*
        TBD - these should move to the "process" class,
        and should return a pointer to a stateset.

        Longer term - 
        adjust the stateset class?  Should we use a proper class hierarchy?

    */
    virtual void getReachable(result &ss) const = 0;
    virtual void getPotential(expr* p, result &ss) const = 0;
    virtual void getInitialStates(result &x) const = 0;

    /**
      Show all the states, in the desired order.
        @param  os             Output stream to write to
        @param  display_order  Display order to use.  See class lldsm
        @param  st             Memory space for use to use for individual states
    */
    void showStates(OutputStream &os, int display_order, shared_state* st);

    /**
      Visit all the states, in the desired order.
        @param  x             State visitor.
        @param  visit_order   Order to use, same as display_order constants in lldsm.
    */
    void visitStates(lldsm::state_visitor &x, int visit_order);

    /**
      Visit all the states, in any convenient order.
        @param  x             State visitor.
    */
    void visitStates(lldsm::state_visitor &x) const;

    // Shared object requirements
    virtual bool Print(OutputStream &s, int width) const;
    virtual bool Equals(const shared_object* o) const;
};

#endif

