
// $Id$

#ifndef STATE_LLM_H
#define STATE_LLM_H

#include "../ExprLib/mod_inst.h"

#ifndef INITIALIZERS_ONLY

class stateset;

// ******************************************************************
// *                                                                *
// *                       state_lldsm  class                       *
// *                                                                *
// ******************************************************************

/**   Class for models with finite discrete reachable state spaces.
      Mostly a base class for further functionality.

      TBD - designing this class still,
            bunch of stuff will need to be removed from lldsm.

*/
class state_lldsm : public lldsm {

public:
  enum display_order {
    DISCOVERY  = 0,
    LEXICAL    = 1,
    NATURAL    = 2
  };
  static const int num_display_orders = 3;

  inline static display_order stateDisplayOrder() {
    switch (int_display_order) {
      case 0 : return DISCOVERY;
      case 1 : return LEXICAL;
      case 2 : return NATURAL;
    }
    // Sane default
    return NATURAL;
  }

public:
    // class for visiting states.
    class state_visitor {
      protected:
        traverse_data x;
      public:
        state_visitor(const hldsm* m);
        virtual ~state_visitor();
        inline long&  index() { return x.current_state_index; }
        inline shared_state* state() { return x.current_state; }

        /** Return true iff we can skip state 
            with index x.current_state_index.
        */
        virtual bool canSkipIndex() { return false; }

        /** Visit state; return true iff we can stop now.
            State to be visited is x.current_state, and it has
            index x.current_state_index.
        */
        virtual bool visit() { return false; }
    };
    // ------------------------------------------------------------
    // end of inner class state_visitor

public:

    /**
        Reachable states.
        Abstract base class; different implementations provided
        by derived classes.
    */
    class reachset : public shared_object {
        const state_lldsm* parent;
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
  
        inline void setParent(const state_lldsm* p) {
          if (parent != p) {
            DCASSERT(0==parent);
            parent = p;
          }
        }
  
        inline const state_lldsm* getParent() const {
          return parent;
        }
  
        inline const hldsm* getGrandParent() const {
          return parent ? parent->GetParent() : 0;
        }
  
        virtual void getNumStates(result &ns) const;  // default: use a long
        virtual void getNumStates(long &ns) const = 0;
        virtual void showInternal(OutputStream &os) const = 0;
        virtual void showState(OutputStream &os, const shared_state* st) const = 0;
        virtual iterator& iteratorForOrder(state_lldsm::display_order ord) = 0;
        virtual iterator& easiestIterator() const = 0;
  
        /*
            TBD - add a reachgraph parameter, needed for the stateset.
  
            TBD - adjust the stateset class and  use a proper class hierarchy.
  
        */
        virtual stateset* getReachable() const = 0;
        virtual stateset* getPotential(expr* p) const = 0;
        virtual stateset* getInitialStates() const = 0;
  
        /**
          Show all the states, in the desired order.
            @param  os      Output stream to write to
            @param  ord     Display order to use.
            @param  st      Memory space for use to use for individual states
        */
        void showStates(OutputStream &os, state_lldsm::display_order ord, 
          shared_state* st);
  
        /**
          Visit all the states, in the desired order.
            @param  x     State visitor.
            @param  ord   Order to use
        */
        void visitStates(state_lldsm::state_visitor &x, 
          state_lldsm::display_order ord);

        /**
          Visit all the states, in any convenient order.
            @param  x             State visitor.
        */
        void visitStates(state_lldsm::state_visitor &x) const;

        // Shared object requirements
        virtual bool Print(OutputStream &s, int width) const;
        virtual bool Equals(const shared_object* o) const;
    };
    // ------------------------------------------------------------
    // end of inner class reachset


public:
  state_lldsm(model_type t);
  virtual ~state_lldsm();

  inline const reachset* getRSS() const {
    return RSS;
  }

  inline void setRSS(reachset* rss) {
    DCASSERT(0==RSS);
    RSS = rss;
  }

  /** Get the number of reachable states.
      This version is used to implement Smart function num_states.
      The default version provided here will only work if
      the number of states fits in a long.
        @param  count   Number of states is stored here,
                        as a "bigint" if that type exists and there are
                        a large number of states, otherwise as a long.
  */
  virtual void getNumStates(result& count) const;

  /** Get the number of reachable states.
      This must be provided in derived classes, the
      default behavior here is to print an error message.

        @return  The number of reachable states, if it fits in a long;
                -1, otherwise (on overflow).
  */
  virtual long getNumStates() const;

  /** Show the reachable states.
      This must be provided in derived classes, the
      default behavior here is to print an error message.
        @param  internal  If true, show internal details of state storage only.
                          If false, show a sane list of states, unless there
                          are too many to display.
  */
  virtual void showStates(bool internal) const;

  /// Check if ns exceeds option, if so, show "too many states" message.
  static bool tooManyStates(long ns, bool show);

  /** Visit all our states, explicitly, in a convenient order.
      This must be provided in derived classes, the
      default behavior here is to print an error message.
        @param  x   Specifies what we do when visiting each state.
                    In practice, will be a derived class.
  */
  virtual void visitStates(state_visitor &x) const;

#ifdef NEW_STATESETS

  /** Get the reachable states, as a stateset.
      Default behavior here is to print an error message and return null.
        @return   New stateset object for the reachable states,
                  or 0 on error.
  */
  virtual stateset* getReachable() const;

  /** Get the possible initial (time 0) states.
      Conceptually, this tells which elements in the vector 
      constructed by getInitialDistribution() will have 
      non-zero probability.  This must be provided in derived 
      classes, the default behavior here is to print an error 
      message and return null.
        @return   New stateset object for the initial states,
                  or 0 on error.
  */
  virtual stateset* getInitialStates() const;

  /** Get the set of states satisfying a constraint.
      Default behavior here is to print an error message and return null.
        @param  p   Logical condition for states to satisfy.
                    If 0, we quickly return a new empty set.
        @return   New stateset object for states satisfying p,
                  or 0 on error.
  */
  virtual stateset* getPotential(expr* p) const;

#else

  /** Get the reachable states, as a stateset.
      Default behavior here is to (quietly) set the result to null.
        @param  ss  Set of reachable states is stored here,
                    as a "stateset".
  */
  virtual void getReachable(result &ss) const;

  /** Get the possible initial (time 0) states.
      Conceptually, this tells which elements in the
      vector constructed by getInitialDistribution() 
      will have non-zero probability.
      This must be provided in derived classes, the
      default behavior here is to print an error message.
        @param  x   On input: ignored.
                    On output: an appropriate "stateset"
                    containing the set of states the model
                    could be in at time 0.
                    Will be a "null" result on error.
  */
  virtual void getInitialStates(result &x) const;

  /** Get the set of states satisfying a constraint.
      Default behavior here is to (quietly) set the result to null.
        @param  p   Logical condition for states to satisfy.
        @param  ss  Set of "potential" states satisfying p is stored here,
                    as a "stateset".
  */
  virtual void getPotential(expr* p, result &ss) const;

#endif

  private:
    reachset* RSS;

  private:  
    static const char* max_state_display_option;
    static long max_state_display;
    static int int_display_order;

    friend void InitializeStateLLM(exprman* em);

};

#endif  // INITIALIZERS_ONLY

// **************************************************************************
// *                                                                        *
// *                               Front  end                               *
// *                                                                        *
// **************************************************************************

void InitializeStateLLM(exprman* em);

#endif

