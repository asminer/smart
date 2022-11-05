
#ifndef STATE_LLM_H
#define STATE_LLM_H

#include "../ExprLib/mod_inst.h"
#include<vector>

#include "../Modules/statesets.h" // for now
#include "../_StateLib/lchild_rsiblingt.h"

class stateset;

namespace StateLib {
  class state_db;
};

// ******************************************************************
// *                                                                *
// *                       state_lldsm  class                       *
// *                                                                *
// ******************************************************************

/**   Class for models with finite discrete reachable state spaces.
      Mostly a base class for further functionality.

      This class provides methods for the reachability set,
      and an inner class for different reachability set implementations.

*/
class state_lldsm : public lldsm {

public:
  enum display_order {
    DISCOVERY  = 0,
    LEXICAL    = 1,
    NATURAL    = 2
  };
  static const unsigned num_display_orders = 3;

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
      protected:
        static exprman* em;
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
      protected:
        virtual ~reachset();
        virtual const char* getClassName() const = 0;

      protected:
        // Hook - called when the RSS is set for a state model.
        virtual void attachToParent(state_lldsm* p);

      public:
        inline const state_lldsm* getParent() const {
          return parent;
        }

        inline const hldsm* getGrandParent() const {
          return parent ? parent->GetParent() : 0;
        }

        /**
            A bit of a hack, needed for explicit state generation.
            Return the state database for generating the underlying process.
            Default behavior here is to return null.
        */
        virtual StateLib::state_db* getStateDatabase() const;

        /** Get the number of reachable states.
            This version is used to implement Smart function num_states.
            The default version provided here will only work if
            the number of states fits in a long.
              @param  count   Number of states is stored here as a bigint.
        */
        virtual void getNumStates(result &ns) const;

        /** Get the number of reachable states.
              @return  The number of reachable states, if it fits in a long;
                      -1, otherwise (on overflow).
        */
        virtual void getNumStates(long &ns) const = 0;

        /** Get the upper bound of the given set of places among reachable states.
         @param  count   Upper bound of states is stored here as a bigint.
         */
        virtual void getBounds(result &ns, std::vector<int> set_of_places) const;

        virtual void getBounds(long &ns, std::vector<int> set_of_places) const = 0;

        /** Show the internal representation of the reachable states.
              @param  os    Output stream to write to
        */
        virtual void showInternal(OutputStream &os) const = 0;

        /** Show the given state
              @param  os    Output stream to write to
              @param  st    State to display
        */
        virtual void showState(OutputStream &os, const shared_state* st) const = 0;

        /// Build an iterator for a desired order.
        virtual iterator& iteratorForOrder(state_lldsm::display_order ord) = 0;

        /// Build an easy iterator, for when the order is irrelevant.
        virtual iterator& easiestIterator() const = 0;

        /// Build and return a stateset for the reachable states
        virtual stateset* getReachable() const = 0;

        /** Build a stateset for states satisfying a constraint.
              @param  p   Logical condition for states to satisfy.
                          If 0, we quickly return a new empty set.
              @return   New stateset object for states satisfying p,
                        or 0 on error.
        */
        virtual stateset* getPotential(expr* p) const = 0;

        /// Build and return a stateset for the initial states
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
            @param  x     State visitor, specifies what to do for each state.
            @param  ord   Order to use
        */
        void visitStates(state_lldsm::state_visitor &x,
          state_lldsm::display_order ord);

        /**
          Visit all the states, in any convenient order.
            @param  x     State visitor, specifies what to do for each state.
        */
        void visitStates(state_lldsm::state_visitor &x) const;

        // Shared object requirements
        virtual bool Print(OutputStream &s, int width) const;
        virtual bool Equals(const shared_object* o) const;

        friend class init_statellm;
        friend class state_lldsm;   // overkill
    };
    // ------------------------------------------------------------
    // end of inner class reachset


public:
  state_lldsm(model_type t);
protected:
  virtual ~state_lldsm();
  virtual const char* getClassName() const;

public:

  inline const reachset* getRSS() const {
    return RSS;
  }

  inline void setRSS(reachset* rss) {
    DCASSERT(0==RSS);
    RSS = rss;
    if (RSS) RSS->attachToParent(this);
  }

  inline reachset* useRSS() {
    return RSS;
  }
  inline const lchild_rsiblingt* getCG() const {
    return CG;
  }

  inline void setCG(lchild_rsiblingt* cg) {
    //DCASSERT(0==RSS);
    CG = cg;
  //  if (RSS) RSS->attachToParent(this);
  }

  inline lchild_rsiblingt* useCG() {
    return CG;
  }

  inline void getNumStates(result& count) const {
    if (RSS)  RSS->getNumStates(count);
    else      count.setNull();
  }
  inline void getNumStatesCOV(result& count) const {
	  int result=CG->getNumState(CG,0);
	  //("\n\n$$$$$ %d",result);
    printf("\n\n$$$$$$2 %d",result);

       count.setInt(result);
    }
  inline long getNumStatesCOV() const {
     DCASSERT(CG);
     //long ns;
    int res= CG->getNumState(CG,0);
    printf("\n\n$$$$$$111 %d",res);

     return res;
   }
  inline long getNumStates() const {
    DCASSERT(RSS);
    long ns;
    RSS->getNumStates(ns);
    return ns;
  }

  inline void getBounds(result& count, std::vector<int> set_of_places) const {
    if (RSS)  RSS->getBounds(count, set_of_places);
    else      count.setNull();
  }
   void showStatesCOV(bool internal)const;
  /** Show the reachable states.
        @param  internal    If true, show the internal representation.
  */
  void showStates(bool internal) const;

  /** Compare number of states with option.
        @param  ns    Number of states
        @param  os    If not null, display "too many states" message as appropriate.
        @return true  iff the number of states exceeds the option.
  */
  static bool tooManyStates(long ns, OutputStream *os);

  /// Check if reachable states has too many states
  inline bool tooManyStates(OutputStream *os) const {
    return tooManyStates(getNumStates(), os);
  }

  /** Visit all our states, explicitly, in a convenient order.
        @param  x   Specifies what we do when visiting each state.
                    In practice, will be a derived class.
  */
  inline void visitStates(state_visitor &x) const {
    DCASSERT(RSS);
    RSS->visitStates(x);
  }

  inline stateset* getReachable() const {
    return RSS ? RSS->getReachable() : 0;
  }

  inline stateset* getInitialStates() const {
    return RSS ? RSS->getInitialStates() : 0;
  }

  inline stateset* getPotential(expr* p) const {
    return RSS ? RSS->getPotential(p) : 0;
  }

  protected:
    reachset* RSS;
    lchild_rsiblingt* CG;

  private:
    static const char* max_state_display_option;
    static long max_state_display;
    static unsigned int_display_order;

    friend class init_statellm;

};

#endif
