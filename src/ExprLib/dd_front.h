
// $Id$

#ifndef DD_FRONT_H
#define DD_FRONT_H

#include "exprman.h"

class shared_state;

// ******************************************************************
// *                                                                *
// *                        sv_encoder class                        *
// *                                                                *
// ******************************************************************

/** Abstract class for building decision diagrams.
    Allows for a generic "encoding" of model state variables
    into decision diagram variables, and construction of
    DDs without knowing library details.
    This should work with CUDD, our library, or pretty much anything.
*/
class sv_encoder : public shared_object {
public:
  enum error {
    /// The operation was successful.
    Success = 0,
    /// Bad parameter somewhere.
    Invalid_Edge,
#ifdef DEVELOPMENT_CODE
    /// Output parameter should not be modified.
    Shared_Output_Edge,
#endif
    /// Library returned an out of memory error.
    Out_Of_Memory,
    /// Operation failed because this encodes the empty set
    Empty_Set,
    /// Generic failure.
    Failed
  };
  /** Produce a "human-readable" name for an error.
        @param  e   The engine error.
        @return     A string constant.
  */
  static const char* getNameOfError(error e);
public:
  /// Simple constructor.
  sv_encoder();
protected:
  /// Virtual destructor.
  virtual ~sv_encoder();
public:
  // Required for shared_object:
  virtual bool Print(OutputStream &s, int width) const;
  virtual bool Equals(const shared_object *o) const;

  /** For debugging, display the current node information.
        @param  s   Output stream to write to
        @param  e   Edge pointer
  */
  virtual error dumpNode(DisplayStream &s, shared_object* e) const = 0;

  /** For debugging, display the current "forest".
      Might not be supported for all backends.
        @param  s   Output stream to write to.
  */
  virtual void dumpForest(DisplayStream &s) const = 0;

  /** Does the forest separate primed from unprimed vars?
      This affects numbering.
      If false, then primed and unprimed vars are mixed together.
      If there are NO primed variables, then 
        TBD: does it matter?
  */
  virtual bool arePrimedVarsSeparate() const = 0;

  /** Get the number of variables in the decision diagram forest.
      If arePrimedVarsSeparate() returns true, then this returns
      the number of primed variables, which is assumed to equal
      the number of unprimed variables.
  */
  virtual int getNumDDVars() const = 0;

  /** Build a blank "edge" for this encoder.
      Can be used later for any parameter that needs an edge.
        @param    e   If nonzero, the new edge will be copied from \a e.
        @return   An empty wrapper.
  */
  virtual shared_object* makeEdge(const shared_object* e) = 0;

  /** Is this a valid edge for this encoder?
        @param  e   An edge, should have been created by makeEdge().
        @return true  iff it is a valid edge
  */
  virtual bool isValidEdge(const shared_object* e) const = 0;

  /** Copy one edge to another.
        @param  src   Source edge.
        @param  dest  Destination edge. 
        @return       Appropriate error code.
  */
  virtual error copyEdge(const shared_object* src, shared_object* dest) const = 0;

  /** Build the "symbolic" representation for a given boolean constant.
        @param  t       Boolean constant
        @param  answer  An edge for storing the result, 
                        should have been created by makeEdge().
        @return         Appropriate error code.
  */
  virtual error buildSymbolicConst(bool t, shared_object* answer) = 0;

  /** Build the "symbolic" representation for a given boolean constant.
        @param  t       Integer constant
        @param  answer  An edge for storing the result, 
                        should have been created by makeEdge().
        @return         Appropriate error code.
  */
  virtual error buildSymbolicConst(long t, shared_object* answer) = 0;

  /** Build the "symbolic" representation for a given boolean constant.
        @param  t       Real constant
        @param  answer  An edge for storing the result, 
                        should have been created by makeEdge().
        @return         Appropriate error code.
  */
  virtual error buildSymbolicConst(double t, shared_object* answer) = 0;

  /** Build the "symbolic" representation for f(sv),
      where sv is a state variable (possibly "primed"), and
      f is an expression depending on sv.
        @param  sv      A model state variable
        @param  primed  Do we want the primed or unprimed version?
        @param  f       Expression to evaluate on sv;
                        if 0, we assume the identity function.
        @param  answer  An edge for storing the result, 
                        should have been created by makeEdge().
        @return         Appropriate error code.
  */
  virtual error buildSymbolicSV(const symbol* sv, bool primed, 
                                expr* f, shared_object* answer) = 0;

  /** Convert a state to a "minterm" (path in DD).
      Depends on choice of "variable encoding".
        @param  s     A model state.
        @param  mt    An array of dimension (#levels+1),
                      minterm will be written here.
        @return       Appropriate error code.
  */
  virtual error state2minterm(const shared_state* s, int* mt) const = 0;

  /** Convert a "minterm" (path in DD) to a state.
      Inverse operation of \a state2minterm().
      Used to convert from state representation to DD representation,
      which of course depends on the choice of "encoding".
        @param  mt    Minterm.  Array of dimension (#levels+1)
                      of (unprimed) variable assignments.
        @param  s     Output: corresponding state.
                      Must be allocated already.
        @return       Appropriate error code.
  */
  virtual error minterm2state(const int* mt, shared_state *s) const = 0;


  /** Find the "first" element in a set.
        @param  set   The set we care about.
        @return       The first minterm in the set,
                      or 0 on error or if this is the empty set.
  */
  virtual const int* firstMinterm(shared_object* set) const = 0;

  /** Find the "next" element in a set.
      \a firstMinterm() must be called before calling this method.
      Behavior is undefined if the set has been changed since calling \a firstMinterm().
        @param  set   The set we care about.
        @return       The first minterm in the set,
                      or 0 on error or if there are no more minterms.
  */
  virtual const int* nextMinterm(shared_object* set) const = 0;

  /** Convert a set of "minterms" to a set, encoded as a DD.
        @param  mts     Minterms to add.  Array of 
                        arrays of dimension (#levels+1)
                        of (unprimed) variable assignments.
        @param  n       Number of minterms.
        @param  ans     Set of minterms encoded as a DD.
        @return         Appropriate error code.
  */
  virtual error createMinterms(const int* const* mts, int n, shared_object* ans) = 0;


  /** Convert a set of "minterms" to a set of edges, encoded as a DD.
        @param  from    From states.  Array of 
                        arrays of dimension (#levels+1)
                        of unprimed variable assignments.
        @param  to      To states. Array of
                        arrays of dimension (#levels+1)
                        of primed variable assignments.
        @param  n       Number of minterms.
        @param  ans     Set of edges encoded as a DD.
        @return         Appropriate error code.
  */
  virtual error createMinterms(const int* const* from, const int* const* to, int n, shared_object* ans) = 0;


  /** Build a unary operation on a DD node.
        @param  op    Unary operation requested.
        @param  opnd  Operand, as a wrapper around a DD node.
        @param  ans   An edge for storing the result,
                      should have been created by makeEdge().
                      Can be the same pointer as \a opnd.
        @return       Appropriate error code.
  */
  virtual error buildUnary(exprman::unary_opcode op, 
                            const shared_object* opnd, shared_object* ans) = 0;

  /** Build a binary operation on DD nodes.
        @param  left  Left operand.
        @param  op    Binary operation requested.
        @param  right Right operand.
        @param  ans   An edge for storing the result,
                      should have been created by makeEdge().
                      Can be the same pointer as \a left or \a right.
        @return       Appropriate error code.
  */
  virtual error buildBinary(const shared_object* left, 
                            exprman::binary_opcode op, 
                            const shared_object* right,
                            shared_object* ans) = 0;

  /** Build an associative operation on two DD nodes.
        @param  left  Left operand.
        @param  flip  Should the operator be inverted.
        @param  op    Associative operator.
        @param  right Right operand.
        @param  ans   An edge for storing the result,
                      should have been created by makeEdge().
                      Can be the same pointer as \a left or \a right.
        @return       Appropriate error code.
  */
  virtual error buildAssoc(const shared_object* left, 
                            bool flip, exprman::assoc_opcode op, 
                            const shared_object* right,
                            shared_object* ans) = 0;


  /** Determine the cardinality of a DD node.
      Normally that means the number of minterms that lead
      to a non-default function value.
      If the DD node represents a set, then it gives the
      cardinality of the set.
        @param  x     Set to determine.
        @param  card  Output: set cardinality; will be negative on overflow.
        @return       Appropriate error code.
  */
  virtual error getCardinality(const shared_object* x, long &card) = 0;

  /** Determine the (approximate) cardinality of a DD node.
        @param  x     Set to determine.
        @param  card  Output: set cardinality; will be INF on overflow.
        @return       Appropriate error code.
  */
  virtual error getCardinality(const shared_object* x, double &card) = 0;

  /** Determine the cardinality of a DD node.
        @param  x     Set to determine.
        @param  card  Output: set cardinality, as a bigint.
        @return       Appropriate error code.
  */
  virtual error getCardinality(const shared_object* x, result &card) = 0;


  /** Determine if a DD node represents the empty set.
        @param  x       Set to determine
        @param  empty   Output: true iff set x is empty
        @return         Appropriate error code
  */
  virtual error isEmpty(const shared_object* x, bool &empty) = 0;

  /** Pre-image operator.
      For a given set of states x and edges E,
      return the set of states with an edge to a state in x.
        @param  x     Target states; within this forest.
        @param  E     Set of edges; must be "compatible" with this forest.
        @param  ans   Output: source states; within this forest.
        @return       Appropriate error code.
  */
  virtual error preImage(const shared_object* x, 
                          const shared_object* E, 
                          shared_object* ans) = 0;

  /** Post-image operator.
      For a given set of states x and edges E,
      return the set of states that can be reached by following
      an edge from a state in x.
        @param  x     Source states; within this forest.
        @param  E     Set of edges; must be "compatible" with this forest.
        @param  ans   Output: target states; within this forest.
        @return       Appropriate error code.
  */
  virtual error postImage(const shared_object* x, 
                          const shared_object* E, 
                          shared_object* ans) = 0;

  /** Backward reachability operator.
      Reflexive and transitive closure of the Pre-image operator.
        @param  x     Target states; within this forest.
        @param  E     Set of edges; must be "compatible" with this forest.
        @param  ans   Output: source states; within this forest.
        @return       Appropriate error code.
  */
  virtual error preImageStar(const shared_object* x, 
                              const shared_object* E, 
                              shared_object* ans) = 0;

  /** Forward reachability operator.
      Reflexive and transitive closure of the Post-image operator.
        @param  x     Source states; within this forest.
        @param  E     Set of edges; must be "compatible" with this forest.
        @param  ans   Output: target states; within this forest.
        @return       Appropriate error code.
  */
  virtual error postImageStar(const shared_object* x, 
                              const shared_object* E, 
                              shared_object* ans) = 0;

  /** Restrict first element in a relation.
        @param  E     Set of edges (e, e'); within this forest.
        @param  rows  Set of desired "rows"; must be "compatible" 
                      with this forest.
        @param  ans   Output: new set of edges equal to
                      E & (rows X all); within this forest.

        @return       Appropriate error code.
  */
  virtual error selectRows(const shared_object* E, 
                            const shared_object* rows,
                            shared_object* ans) = 0;

  /** Restrict second element in a relation.
        @param  E     Set of edges (e, e'); within this forest.
        @param  cols  Set of desired "columns"; must be "compatible" 
                      with this forest.
        @param  ans   Output: new set of edges equal to
                      E & (all X cols); within this forest.

        @return       Appropriate error code.
  */
  virtual error selectCols(const shared_object* E, 
                            const shared_object* cols,
                            shared_object* ans) = 0;


  /// Report stats
  virtual void reportStats(OutputStream &out) = 0;
};

#endif

