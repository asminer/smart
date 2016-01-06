
// $Id:$

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
    state_lldsm(model_type t);

    virtual ~state_lldsm();

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


//
// Option stuff
//

public:
  static const int DISCOVERY  = 0;
  static const int LEXICAL    = 1;
  static const int NATURAL    = 2;
  static const int num_display_orders = 3;


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

    /** Return true iff we can skip state with index x.current_state_index.
    */
    virtual bool canSkipIndex() { return false; }

    /** Visit state; return true iff we can stop now.
        State to be visited is x.current_state, and it has
        index x.current_state_index.
    */
    virtual bool visit() { return false; }
  };

  public:

  // copy class reachset here from graph_llm.h


  private:
    // reachset* RSS;

  // options
  private:  
    static const char* max_state_display_option;
    static long max_state_display;
    static int display_order;

    friend void InitializeStateLLM(exprman* em);
};

#endif  // INITIALIZERS_ONLY

// **************************************************************************
// *                                                                        *
// *                               Front  end                               *
// *                                                                        *
// **************************************************************************

void InitializeStateLLM(exprman* em);

