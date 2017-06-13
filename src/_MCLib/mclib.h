
/** \file mclib.h
    Markov chain library.
    Allows construction and analysis of Markov chains
    (both discrete and continuous time).
*/

#ifndef MCLIB_H
#define MCLIB_H

#include "../_GraphLib/graphlib.h"
#include "../_LSLib/lslib.h"
#include "../_RngLib/rng.h"
#include "../_IntSets/intset.h"

namespace MCLib {

  // ======================================================================
  // |                                                                    |
  // |                            error  class                            |
  // |                                                                    |
  // ======================================================================

  /** 
      Error codes.
      Implemented in error.cc 
      (for consistency; there's not much to implement)
  */
  class error {
  public:
    enum code {
      /// Not implemented yet.
      Not_Implemented,
      /// Null vector.
      Null_Vector,
      /// Operation and type of chain are incompatible.
      Wrong_Type,
      /// Insufficient memory (e.g., for adding state or edge)
      Out_Of_Memory,
      /// Bad time for transient analysis.
      Bad_Time,
      /// Error in linear solver (for steady-state)
      Bad_Linear,
      /// There is an "absorbing vanishing loop"
      Loop_Of_Vanishing,
      /// Internal library error
      Internal,
      /// Misc. error
      Miscellaneous
    };

  public:
    error(code c);

    /** Obtain a human-readable error string for this error.
        For convenience.
          @return     An appropriate error string.  It
                      should not be modified or deleted.
    */
    const char*  getString() const;

    /// Grab the error code
    inline code getCode() const { return errcode; }
  private:
    code errcode;
  };  // class error


  // ======================================================================
  // |                                                                    |
  // |                                                                    |
  // |                         Markov_chain class                         |
  // |                                                                    |
  // |                                                                    |
  // ======================================================================

  /**
    Markov chain class.

    Built from a graph.  Most methods are for analysis.
    We store the graphs twice, one by rows and once by columns.

    Implemented in markov_chain.cc
  */
  class Markov_chain {
    public:
      /**
          Constructor.
          Fill this Markov chain based on the given graph
          and known state classification (see GraphLib for
          ways to determine this).

            @param  discrete    True iff this is a discrete time
                                Markov chain; otherwise it is a
                                continuous time Markov chain.

            @param  G           Graph to build from. 
                                Edge weights should be floats or doubles,
                                and indicate probabilities (DTMCs)
                                or rates (CTMCs).
                                Nodes should be numbered consistently
                                with TSCCinfo.

            @param  TSCCinfo    State classification, such that
                                class 0 contains all transient states,
                                class 1 contains all absorbing states,
                                and remaining classes correspond
                                to recurrent classes.

            @param  sw  Where to report timing information (nowhere if 0).
      */
      Markov_chain(bool discrete, GraphLib::dynamic_summable<double> &G, 
        const GraphLib::static_classifier &TSCCinfo,
        GraphLib::timer_hook *sw);  


      /**
          Constructor.
          Fill this Markov chain based on the given graph
          and known state classification (see GraphLib for
          ways to determine this).

            @param  discrete    True iff this is a discrete time
                                Markov chain; otherwise it is a
                                continuous time Markov chain.

            @param  G           Graph to build from. 
                                Edge weights should be floats or doubles,
                                and indicate probabilities (DTMCs)
                                or rates (CTMCs).
                                Nodes should be numbered consistently
                                with TSCCinfo.

            @param  TSCCinfo    State classification, such that
                                class 0 contains all transient states,
                                class 1 contains all absorbing states,
                                and remaining classes correspond
                                to recurrent classes.

            @param  sw  Where to report timing information (nowhere if 0).
      */
      Markov_chain(bool discrete, GraphLib::dynamic_summable<float> &G, 
        const GraphLib::static_classifier &TSCCinfo,
        GraphLib::timer_hook *sw);  



      /// Destructor.
      virtual ~Markov_chain();

      // TBD ^ ^ ^ ^ do we have any other virtual methods?

    public:
      /// Is this a discrete-time chain?
      inline bool isDiscrete() const { return is_discrete; }

      /// Is this a continuous-time chain?
      inline bool isContinuous() const { return !is_discrete; }

      /// Number of states in the chain
      inline long getNumStates() const { return G_byrows_diag.getNumNodes(); }

      /// @return Total memory required to store the chain, in bytes.
      size_t getMemTotal() const;

      /** 
            Get state classification information for the chain.
            Will be a copy of the one passed to the constructor.

            @return State classification information for the chain.
      */
      const GraphLib::static_classifier& getStateClassification() const {
        return stateClass;
      }

      /** 
            Get the Markov chain's nniformization constant.

            @return   The largest row sum.
                      For CTMCs, this is the smallest constant that may
                      be used for uniformization.
      */
      inline double getUniformizationConst() const {
        return uniformization_const;
      }


      /**
          Run graph traversal t, on outgoing edges.

            @param  t   Traversal, which determines the
                        (possibly changing) list of nodes to explore,
                        and how to visit edges.

            @return true, iff a call to t.visit returned true
                          and we stopped traversal early.
      */
      bool traverseOutgoing(GraphLib::BF_graph_traversal &t) const;

      /**
          Run graph traversal t, on incoming edges.

            @param  t   Traversal, which determines the
                        (possibly changing) list of nodes to explore,
                        and how to visit edges.

            @return true, iff a call to t.visit returned true
                          and we stopped traversal early.
      */
      bool traverseIncoming(GraphLib::BF_graph_traversal &t) const;


      /** Compute the period for a given class.
          For CTMCs, gives the period of the embedded DTMC.

            @param  c   The recurrent class to check.

            @return     The period of class c.
                        If c does not refer to a recurrent class,
                        or to the group of absorbing states
                        (for example, if c is too large),
                        then we return zero.
      */
      long computePeriodOfClass(long c) const;

      // TBD - transient, forward time and backward time
      // TBD - accumulated transient, forward time

      /** Compute the time to absorption.
          Vectors are allocated so that x[s] is the total time for state s,
          for any legal state handle s.

          @param  p0    Initial distribution.

          @param  p     Answer stored here.
                        If s is transient, then 
                          p[s] = expected time spent in state s;
                        otherwise, 
                          p[s] is unchanged.

          @param  opt   Options for linear solver.

          @param  out   Linear solver status information as output.
      */
      void computeTTA(const LS_Vector &p0, double* p, const LS_Options &opt, 
        LS_Output &out) const;


      /** Compute the probability of starting in each recurent state.
          More formally, if run the Markov chain until it is not in a
          transient state, we will be in some recurrent state
          (either because we started there, or because it was the first
          one hit after leaving the transient ones).  This method
          determines, for each recurrent state, the probability of 
          hitting that one first.

          @param  p0    Initial distribution.

          @param  np    A vector of dimension number of states.
                        On output, np[i] gives the expected time spent
                        in state i, if state i is transient (we have to
                        compute this anyway).
                        Otherwise, i is recurrent, and np[i] gives the 
                        probability that i is the first recurrent state 
                        visited in the Markov chain.
                        Note that one can obtain the probability of reaching
                        a given recurrent class by summing np[i] over all
                        states i in the recurrent class.

          @param  opt   Options for linear solver.

          @param  out   Linear solver status information as output.
      */
      void computeFirstRecurrentProbs(const LS_Vector &p0, double* nc, 
        const LS_Options &opt, LS_Output &out) const;


      /** Compute the probability distribution at time infinity.
          For an ergodic chain, this is the steady-state distribution;
          otherwise, we depend on the initial distribution.
          Vectors are allocated so that x[s] is the probability for state s,
          for any legal state handle s.

          @param  p0    Initial distribution.

          @param  p     Answer stored here.
                        Note that if s is transient, then p[s] will be 0.
                        In any case, p[s] is the probability that the
                        chain is in state s, as time goes to infinity.

          @param  opt   Options for linear solver.

          @param  out   Linear solver status information as output.
      */
      void computeInfinityDistribution(const LS_Vector &p0, double* p, 
        const LS_Options &opt, LS_Output &out) const;


      // Methods that build TTA distributions here.
      // (TBD - design a nice distribution class, and use it for poisson.
      // unsure if it goes here or in its own "library".)


      /** Simulate a random walk through the chain.
          The random walk proceeds until we timeout,
          or we reach a state that is absorbing or in the set F.

          @param  rng     Random number stream
          @param  state   Input: the initial state; Output: the final state
          @param  F       Set of final states, or 0 for empty set
          @param  maxt    Maximum number of state changes to let the walk go.
          @param  q       Uniformization constant, if this is a CTMC.
                          Ignored completely if this is a DTMC.

          @return Number of state changes.
      */
      long randomWalk(rng_stream &rng, long &state, const intset* F,
                              long maxt, double q) const;

      /** Simulate a random walk through the chain.
          The chain must be continuous time.
          The random walk proceeds until we timeout,
          or we reach a state that is absorbing or in the set F.

          @param  rng     Random number stream
          @param  state   Input: the initial state; Output: the final state
          @param  F       Set of final states, or 0 for empty set
          @param  maxt    Maximum time to let the walk go.

          @return Time when the walk terminates.
      */
      double randomWalk(rng_stream &rng, long &state, 
                              const intset* F, double maxt) const;


    private:
      // Helper methods

      /**
          Things common to both constructors.
          Moved here to eliminate code duplication.
      */
      void finish_construction(GraphLib::dynamic_graph &G, 
        GraphLib::timer_hook *sw);

    private:
      /**
          Row sums.
          For each state s,
          rowsums[s] gives the sum of outgoing edges from state s
          (discarding any self loops).
          For a DTMC, 1-rowsum[s] gives the probability
          of remaining in state s in one step.
      */
      double* rowsums;

      /**
          Array of dimension number of states, stored as doubles.
          For each state s, one_over_rowsums_d[s] gives 1/rowsums[s], 
          unless rowsums[s] is zero, in which case 
          one_over_rowsums_d[s] is also zero.
          Used only if the graphs are stored with doubles.
      */
      double* one_over_rowsums_d;
      /** 
          Same as one_over_rowsums_d, but stored as floats.
          Used only if the graphs are stored with floats.
      */
      float * one_over_rowsums_f;

      /**
          Maximum row sum if we're a CTMC; otherwise, 1.
      */
      double uniformization_const;

      /// How states are classified (transient, recurrent class 1, etc.)
      GraphLib::static_classifier stateClass; 

      /// Edges between nodes in the same class, stored as a row graph
      GraphLib::static_graph  G_byrows_diag;

      /// Edges from transient states to recurrent, stored as a row graph
      GraphLib::static_graph  G_byrows_off;

      /// Edges between nodes in the same class, stored as a column graph
      GraphLib::static_graph  G_bycols_diag;

      /// Edges from transient states to recurrent, stored as a column graph
      GraphLib::static_graph  G_bycols_off;
      
      /// DTMC?
      bool is_discrete;

      /// Did we build from a graph of doubles?
      bool double_graphs;
  };  // class Markov_chain

};  // namespace MCLib

// ==========================================================================================================================================================================
// ==========================================================================================================================================================================
// ==========================================================================================================================================================================
// ==========================================================================================================================================================================
// ==========================================================================================================================================================================
// OLD INTERFACE BELOW, will eventually be discarded!
// ==========================================================================================================================================================================
// ==========================================================================================================================================================================
// ==========================================================================================================================================================================
// ==========================================================================================================================================================================
// ==========================================================================================================================================================================

#include "../_GraphLib/graphlib.h"  // we'll see how this goes

/// Defined in linear solver library: linear solver options.
struct LS_Options;

/// Defined in linear solver library: output from linear solvers.
struct LS_Output;

/// Defined in linear solver library: sparse vector.
struct LS_Vector;

/// Defined in RNG library: random number stream.
class rng_stream;

namespace Old_MCLib {

  // ======================================================================
  // |                                                                    |
  // |                          Error  reporting                          |
  // |                                                                    |
  // ======================================================================

  /// Error codes
  class error {
  public:
    enum code {
      /// Not implemented yet.
      Not_Implemented,
      /// Null vector.
      Null_Vector,
      /// Operation and type of chain are incompatible.
      Wrong_Type,
      /// Bad state index
      Bad_Index,
      /// Bad class index
      Bad_Class,
      /// Bad rate
      Bad_Rate,
      /// Insufficient memory (e.g., for adding state or edge)
      Out_Of_Memory,
      /// Bad time for transient analysis.
      Bad_Time,
      /// Operation finished requirement does not match chain's finished state.
      Finished_Mismatch,
      /// Linear solver incompatible with internal storage
      Wrong_Format,
      /// Error in linear solver (for steady-state)
      Bad_Linear,
      /// There is an "absorbing vanishing loop"
      Loop_Of_Vanishing,
      /// Internal library error
      Internal,
      /// Misc. error
      Miscellaneous
    };

  public:
    error(code c) { errcode = c; }
    error(const GraphLib::error &e) {
      switch (e.getCode()) {
        case GraphLib::error::Not_Implemented:   
                    errcode = Not_Implemented;    
                    break;
        case GraphLib::error::Bad_Index:         
                    errcode = Bad_Index;          
                    break;
        case GraphLib::error::Out_Of_Memory:     
                    errcode = Out_Of_Memory;      
                    break;
        case GraphLib::error::Finished_Mismatch: 
                    errcode = Finished_Mismatch;  
                    break;
        default:                          
                    errcode = Miscellaneous;
      }
    }

    /** Obtain a human-readable error string for this error.
        For convenience.
          @return     An appropriate error string.  It
                      should not be modified or deleted.
    */
    const char*  getString() const;

    /// Grab the error code
    inline code getCode() const { return errcode; }
  private:
    code errcode;
  };


  // ======================================================================
  // |                                                                    |
  // |                                                                    |
  // |                       Markov chain interface                       |
  // |                                                                    |
  // |                                                                    |
  // ======================================================================

  /**
    Abstract base class for Markov chains.

    A Markov chain can be in two phases: "finished" or "unfinished".
    Initially, a Markov chain is "unfinished", and we can add more
    states and edges (using appropriate methods).
    Once we have finished constructing the chain, we call the
    finish() method, and the Markov chain becomes "finished".
    Then, we can perform various types of analysis on the chain.
    The documentation for each method will specify if the
    method works on finished or unfinished chains (or both).
  */
  class Markov_chain {
  public:
    /// Types of Markov chains.
    enum type {
      /// A known irreducible chain.
      Irreducible,
      /// An absorbing chain, i.e., all states are transient or absorbing.
      Absorbing,
      /// A reducible chain (there is a recurrent class with 2 or more states).
      Reducible,
      /// Don't know the type yet.
      Unknown,
      /// An error occured when we tried to determine the type.
      Error_type
    };


    /// Options for finishing Markov chains.
    struct finish_options : public GraphLib::generic_graph::finish_options {

      /** For irreducible builders, should we verify
          that the chain is in fact irreducible?
      */
      bool Verify_Irred;

      /** For absorbing builders, should we verify
          that the chain is in fact absorbing?
          I.e., make sure all transient states are in fact transient.
      */
      bool Verify_Absorbing;

      /** For general chains, when determining TSCCs,
          should we optimize for less memory?  This may incur
          an increase in CPU time.
      */
      bool SCCs_Optimize_Memory;

      /** Use compact integer sets.
          Less memory required, but may increase time.
      */
      bool Use_Compact_Sets;

      /** Constructor.
          Will initialize all settings to reasonable defaults.
      */
      finish_options() : GraphLib::generic_graph::finish_options() {
        Store_By_Rows = false;
        Verify_Irred = true;
        Verify_Absorbing = true;
        SCCs_Optimize_Memory = true;
        Use_Compact_Sets = false;
      }
    };


    /** How to renumber states for a finished Markov chain.

        When a Markov chain is "finished", it is re-arranged
        into a more convenient internal representation
        which often allows for more efficient analysis.
        As part of this, it may be necessary to renumber
        the states, i.e., the state handle of the original
        chain during construction may differ from the state
        handle of the finished chain.
        Information about how to renumber is stored here.
    */
    class renumbering {
      protected:
        char type;
        long* newhandle;
      public:
        /** Constructor.
            Sets useful defaults.
        */
        renumbering();

        /** Destructor.
            Will destroy array \a newhandle.
        */
        ~renumbering();

        /** Clear.
            Equivalent to destructor plus constructor.
        */
        void clear();

        /** Are the state handles the same?
            The finishing algorithm tries its best to NOT
            renumber states.
            This method will return true if that is the case.
        */
        inline bool NoRenumbering() const { return 'N' == type; }

        /** Is a simple absorbing renumbering used?
            Normally this is true only for a Markov chain
            that was constructed as absorbing.
            During construction, 
            transient states are numbered 0..#transient-1 and
            absorbing states are numbered -1..-#absorbing.
            In the finished chain, the transient states
            have the same number, while the new number for
            an absorbing state is given by 
            #transient-(1+old_handle).
        */
        inline bool AbsorbRenumbering() const { return 'A' == type; }

        /** Is a general renumbering used?
            If so, the \a newhandle array gives the
            new number for an old state as
            newhandle[old_handle].
        */
        inline bool GeneralRenumbering() const { return 'G' == type; }


        inline const long* GetGeneral() const { return newhandle; }

        inline void setNoRenumber() { type = 'N'; }
        inline void setAbsorbRenumber() { type = 'A'; }
        inline void setGeneralRenumber(long* map) {
          type = 'G';
          newhandle = map;
        }
    };


    /// Options and such for transient analysis.
    struct transopts {
      /// Uniformization constant to use (if possible)
      double q;
      /// Precision for poisson distribution
      double epsilon;
      /// Precision for detection of steady-state
      double ssprec;
      /// Should the auxiliary vector(s) be destroyed?
      bool kill_aux_vectors;
      /// Vector to hold result of vector-matrix multiply
      double* vm_result;
      /// Vector to accumulate sum of poisson * distribution.
      double* accumulator;
      /// Output: left poisson truncation point
      int Left;
      /// Output: right poisson truncation point
      int Right;
      /// Output: number of vector-matrix multiplies required.
      int Steps;
      /// Constructor; sets reasonable defaults
      transopts() {
        q = 0.0;  
        epsilon = 1e-20;
        ssprec = 1e-10;
        kill_aux_vectors = true;
        vm_result = 0;
        accumulator = 0;
      }
    };


    /// Options and such for building TTA distributions.
    struct distopts {
      /// Uniformization constant to use (if possible)
      double q;
      /// Should the auxiliary vector(s) be destroyed?
      bool kill_aux_vectors;
      /// Vector to hold result of vector-matrix multiply
      double* vm_result;
      /// Probability vector
      double* probvect;
      /// Constructor - sets reasonable defaults
      distopts() {
        q = 0.0;
        kill_aux_vectors = true;
        vm_result = 0;
        probvect = 0;
      }
    };


  private:
    bool discrete;
  protected:
    bool finished;
    type our_type;
    long num_states;
    long num_classes;

  public:
    /// Basic constructor.
    Markov_chain(bool disc);
    /// Basic destructor.
    virtual ~Markov_chain();


    /// Is this a discrete-time chain?
    inline bool isDiscrete() const { return discrete; }
    /// Is this a continuous-time chain?
    inline bool isContinuous() const { return !discrete; }
    /// Is the chain "finished", or can we still add states and arcs?
    inline bool isFinished() const { return finished; }
    /// What is the type of the chain?
    inline type getType() const { return our_type; }

    /// @return The current number of states.
    inline long getNumStates() const { return num_states; }

    /// @return The current number of arcs (between states).
    virtual long getNumArcs() const = 0;

    /// @return Total memory required to store the chain
    virtual long ReportMemTotal() const = 0;

    /**  @return  The number of recurrent classes with size>1, 
                  or -1 if chain is unfinished.
    */
    inline long getNumClasses() const { return num_classes; }

    /** Add a new state.
        The chain must be "unfinished".
  
        @return  The handle of the state to use for edges.
    */
    virtual long addState() = 0;

    /** Add a state that we know is absorbing.
        The chain must be "unfinished".

        @return  The handle of the state to use for edges.
    */
    virtual long addAbsorbing() = 0;

    /** Add an edge in the chain.
        The chain must be "unfinished".

        @param  from  The "source" state handle.
        @param  to    The "destination" state handle.
        @param  v     Probability, for discrete; rate, for continuous.

        @return true iff this is a duplicate edge (rates are added)
    */
    virtual bool addEdge(long from, long to, double v) = 0;

    /** Finish building the chain.
        The chain must be "unfinished".
        We may switch to a more compact, less dynamic, internal
        representation for the chain.  Also, we will determine / verify
        the type of the chain.
        On return the chain will be "finished",
        and have an appropriate type.
        The type will be Error_type if an error occurred during finishing.
    
        @param  o   Options for renumbering and for creating
                    the finished Markov chain.

        @param  r   (Output) information about how states 
                    have been renumbered, if at all.
    */
    virtual void finish(const finish_options &o, renumbering &r) = 0;

    /// Is the chain stored so that row access is efficient?
    virtual bool isEfficientByRows() const = 0;
    /// Is the chain stored so that column access is efficient?
    inline bool isEfficientByCols() const { return !isEfficientByRows(); }

    /** Transpose a finished chain.
        Changes efficiency from row access to column access,
        or vice versa, keeping the chain finished.
    */
    virtual void transpose() = 0;


    /** Clear the current chain.
        The chain can be finished or unfinished.
        On return, the chain will be "unfinished".

        This operation is similar to, but more efficient than,
        calling the destructor and then re-constructing an
        empty chain.  Memory may be retained.
        (The intent is to allow users to build and solve 
        several different small chains without having to
        re-allocate memory each time.)
    */
    virtual void clear() = 0;


    /** Visit edges from a given state.
        Will be efficient only if the Markov chain is stored "by rows".
  
        @param  i   Source state.

        @param  x   We call x(edge) for each edge from state \a i.
                    If this returns true, we stop the traversal.
    */
    virtual void traverseFrom(long i, GraphLib::generic_graph::element_visitor &x) = 0;

    /** Visit edges to a given state.
        Will be efficient only if the Markov chain is stored "by columns".
  
        @param  i   Target state.

        @param  x   We call x(edge) for each edge to state \a i.
                    If this returns true, we stop the traversal.
    */
    virtual void traverseTo(long i, GraphLib::generic_graph::element_visitor &x) = 0;

    /** Visit all edges.

        @param  x   We call x(edge) for each edge.
                    If this returns true, we stop the traversal.
    */
    virtual void traverseEdges(GraphLib::generic_graph::element_visitor &x) = 0;

    /** Find states reached in one step.
        If this is a DTMC, then one step means "one unit of time".
        If this is a CTMC, then one step means "one change of state".
        The chain must be finished.

        @param  x   Set of source states

        @param  y   If we can reach state j in one step
                    starting from state i in x, then
                    we add j to y.

        @return  true if the set y was changed; false otherwise.
    */
    virtual bool getForward(const intset& x, intset &y) const = 0;

    /** Find states that can reach us in one step.
        If this is a DTMC, then one step means "one unit of time".
        If this is a CTMC, then one step means "one change of state".
        The chain must be finished.

        @param  y   Set of target states

        @param  x   If we can reach state j in y in one step
                    starting from state i, then
                    we add i to x.

        @return  true if the set x was changed; false otherwise.
    */
    virtual bool getBackward(const intset& y, intset &x) const = 0;


    ///  @return  Handle of the first transient state.
    virtual long getFirstTransient() const = 0;

    /// @return The current number of states known to be transient.
    virtual long getNumTransient() const = 0;

    /// @return Handle of the first absorbing state.
    virtual long getFirstAbsorbing() const = 0;

    /// @return The current number of states known to be absorbing.
    virtual long getNumAbsorbing() const = 0;

    /// @return Handle of the first recurrent state of class \a c.
    virtual long getFirstRecurrent(long c) const = 0;

    /// @return Number of states in recurrent class \a c.
    virtual long getRecurrentSize(long c) const = 0;

    /** Is the given state absorbing.
        If the chain is unfinished and we cannot answer,
        an error is thrown.

        @param  s  State handle.
  
        @return  true  if state s is absorbing; false otherwise.  
    */
    virtual bool isAbsorbingState(long s) const = 0;

    /** Is the given state transient.
        If the chain is unfinished and we cannot answer,
        an error is thrown.

        @param  s  State handle.

        @return  true  If state s is transient; false otherwise.
    */
    virtual bool isTransientState(long s) const = 0;

    /** Obtain the recurrent class number of the given state.
        If the chain is unfinished and we cannot answer,
        an error is thrown.

        @param  s  State handle.

        @return    0    If \a s is transient;
                  -n    If \a s is absorbing;
                  +c    If \a s is in recurrent class \a c.
                        Note \a c is at most \a NumClasses().
    */
    virtual long getClassOfState(long s) const = 0;

    /** Is the given state in the given class.
        If the chain is unfinished and we cannot answer,
        an error is thrown.

        @param  st  State handle

        @param  cl  Class number.
                0                    for transient,
                1 .. NumClasses()    for recurrent classes,
                -1 .. -NumAbsorbing() for absorbing states.
    */
    virtual bool isStateInClass(long st, long cl) const = 0;

    /** Compute and remember the period for a given class.
        The chain must be "finished".
        For CTMCs, gives the period of the embedded DTMC.

        @param  c  The recurrent class to check.
    */
    virtual void computePeriodOfClass(long c) = 0;

    /** Recall the period for a given class.
        Must have been computed already via a call to computePeriodOfClass(),
        otherwise an error is thrown.

        @param  c  The recurrent class to check.

        @return p  The period of class c if c is a recurrent class;
                0  if c refers to the transient states;
    */
    virtual long getPeriodOfClass(long c) const = 0;

    /** Uniformization constant.
        The chain must be "finished".

        @return   The largest row sum (without diagonals).
                  For CTMCs, this is the smallest constant that may
                  be used for uniformization.
    */
    virtual double getUniformizationConst() const = 0;

    /** Compute the distribution at time t, given the starting distribution.
        The chain must be "finished".
        Vectors are allocated so that x[s] is the probability for state s,
        for any legal state handle s.

        @param  t     Time.

        @param  p     On input: distribution at time 0.
                      On output: distribution at time t.

        @param  opts  Options and such.
    */
    virtual void computeTransient(double t, double* p, transopts &opts) const = 0;

    /** Compute the distribution at time t, given the starting distribution.
        The chain must be "finished".
        Vectors are allocated so that x[s] is the probability for state s,
        for any legal state handle s.

        @param  t   Time.

        @param  p   On input: distribution at time 0.
                    On output: distribution at time t.
    */
    virtual void computeTransient(int t, double* p, transopts &opts) const = 0;

    /** Compute an expectation at time t, for all possible starting states.
        The chain must be "finished".
        Vectors are allocated so that x[s] is the expectation when the
        chain starts in state s, for any legal state handle s.
        Specifically, we determine

            x[s] = E [ x[state at time t] ], given we start in state s

        @param  t   Time

        @param  x   On input: function to compute expectation over.
                    On output: expected value for each starting state.
    */
    virtual void reverseTransient(double t, double* x, transopts &opts) const = 0;

    /** Compute an expectation at time t, for all possible starting states.
        The chain must be "finished".
        Vectors are allocated so that x[s] is the expectation when the
        chain starts in state s, for any legal state handle s.
        Specifically, we determine

            x[s] = E [ x[state at time t] ], given we start in state s

        @param  t   Time

        @param  x   On input: function to compute expectation over.
                    On output: expected value for each starting state.
    */
    virtual void reverseTransient(int t, double* x, transopts &opts) const = 0;

    /** Compute the accumulated time spent in every state, up
        to and including time t.
        The chain must be "finished".

        @param  t     Time.

        @param  p0    Distribution at time 0.
                      If this is a null pointer, then
                      the initial distribution will instead
                      be taken from the initial value of \a n0t.

        @param  n0t   On input: ignored if \a p0 is non-null;
                      otherwise, the distribution at time 0.
                      On output: accumulated time spent in each
                      state until time t.

        @param  opts  Options and such.
    */
    virtual void accumulate(double t, const double* p0, double* n0t, transopts &opts) const = 0;


    /** Compute the steady-state distribution.
        The chain must be "finished".
        Vectors are allocated so that x[s] is the probability for state s,
        for any legal state handle s.

        @param  p0    Initial distribution.

        @param  p     Answer stored here.
                      If s is transient, then p[s] = 0;
                      otherwise, p[s] = steady-state probability of s.

        @param  opt   Options for linear solver.

        @param  out   Linear solver status information as output.
    */
    virtual void computeSteady(const LS_Vector &p0, double* p, const LS_Options &opt, LS_Output &out) const = 0;

    /** Compute the time to absorption.
        The chain must be "finished".
        Vectors are allocated so that x[s] is the total time for state s,
        for any legal state handle s.

        @param  p0    Initial distribution.

        @param  p     Answer stored here.
                      If s is transient, then 
                        p[s] = expected time spent in state s;
                      otherwise, 
                        p[s] is unchanged.

        @param  opt   Options for linear solver.

        @param  out   Linear solver status information as output.
    */
    virtual void computeTTA(const LS_Vector &p0, double* p, const LS_Options &opt, LS_Output &out) const = 0;

    /** Compute the probability of hitting each recurrent class.
        The chain must be "finished".

        @param  p0    Initial distribution.

        @param  nc    A vector of dimension number of states.
                      On output, nc[i] gives the expected time spent
                      in state i, if state i is transient; otherwise,
                      nc[i] gives the probability that the Markov chain
                      ends up in the recurrent class containing state i.

        @param  opt   Options for linear solver.

        @param  out   Linear solver status information as output.
    */
    virtual void computeClassProbs(const LS_Vector &p0, double* nc, const LS_Options &opt, LS_Output &out) const = 0;


    /** Compute the (discrete) distribution for "time to reach class c".
        The chain must be "finished".

        @param  p0      Initial distribution.

        @param  opts    Options

        @param  c       Class we wish to enter.  See "getClassOfState".
                        If positive, this is a recurrent class.
                        If negative, this is an absorbing state.
                        Should never be zero, for transient states.

        @param  epsilon Build as much of the distribution as necessary,
                        but no more, such that the remaining probabilities
                        sum to less than epsilon.

        @param  dist    Output: malloc'd array of doubles to hold the
                        computed distribution.

        @param  N       Output: length of the \a dist array.

        @throw          Various errors: 
                          Out_Of_Memory if malloc fails.
                          Bad_Class if \a c is zero.
    */
    virtual void computeDiscreteDistTTA(const LS_Vector &p0, distopts &opts, int c, double epsilon, double* &dist, int &N) const = 0;

    /** Compute the (discrete) distribution for "time to reach class c".
        The chain must be "finished".

        @param  p0      Initial distribution.

        @param  opts    Options

        @param  c       Class we wish to enter.  See "getClassOfState".
                        If positive, this is a recurrent class.
                        If negative, this is an absorbing state.
                        Should never be zero, for transient states.

        @param  dist    Fixed array of doubles to hold the
                        computed distribution.

        @param  N       Length of the \a dist array.

        @return         The "achieved precision", i.e., the sum of the
                        remaining probabilities.

        @throw          Various errors: 
                          Out_Of_Memory if malloc fails.
                          Bad_Class if \a c is zero.
    */
    virtual double computeDiscreteDistTTA(const LS_Vector &p0, distopts &opts, int c, double dist[], int N) const = 0;


    /** Compute the (continuous) distribution for "time to reach class c".
        The chain must be "finished".

        @param  p0      Initial distribution.

        @param  opts    Options

        @param  c       Class we wish to enter.  See "getClassOfState".
                        If positive, this is a recurrent class.
                        If negative, this is an absorbing state.
                        Should never be zero, for transient states.

        @param  dt      Time increment.  We compute the PDF at times
                        0, dt, 2*dt, 3*dt, ...

        @param  epsilon Build as much of the distribution as necessary,
                        but no more, such that the probability of remaining
                        in a transient state at the final time point
                        is less than epsilon.

        @param  dist    Output: malloc'd array of doubles to hold the
                        computed distribution.  dist[i] holds the PDF
                        for time point i*dt.

        @param  N       Output: length of the \a dist array.

        @throw          Various errors: 
                          Out_Of_Memory if malloc fails.
                          Bad_Class if \a c is zero.
    */
    virtual void computeContinuousDistTTA(const LS_Vector &p0, distopts &opts, int c, double dt, double epsilon, double* &dist, int &N) const = 0;

    /** Simulate a random walk through the chain.
        The chain must be finished and must be stored "by rows".
        The random walk proceeds until we reach a "final" state,
        or an absorbing state.

        @param  rng     Random number stream
        @param  state   Input: the initial state; Output: the final state
        @param  final   Set of final states, or 0 for empty set
        @param  maxt    Maximum number of state changes to let the walk go.
        @param  q       Uniformization constant, if this is a CTMC.
                        Ignored completely if this is a DTMC.

        @return Number of state changes.
    */
    virtual long randomWalk(rng_stream &rng, long &state, const intset* final,
                            long maxt, double q) const = 0;

    /** Simulate a random walk through the chain.
        The chain must be continuous time, finished, and stored "by rows".
        The random walk proceeds until we reach a "final" state,
        or an absorbing state.

        @param  rng     Random number stream
        @param  state   Input: the initial state; Output: the final state
        @param  final   Set of final states, or 0 for empty set
        @param  maxt    Maximum time to let the walk go.

        @return Time when the walk terminates.
    */
    virtual double randomWalk(rng_stream &rng, long &state, 
                              const intset* final, double maxt) const = 0;

  };



  // ======================================================================
  // |                                                                    |
  // |                                                                    |
  // |                     Vanishing chain  interface                     |
  // |                                                                    |
  // |                                                                    |
  // ======================================================================


  /**
    Abstract base class for Vanishing chains.

    These are semi-Markov processes containing "tangible" states
    and "vanishing" states.  The time spent in a vanishing state
    is 0, while the time spent in a tangible state is either 
    1 (in the discrete case) or exponentially-distributed
    (in the continuous case).
    This class is used to eliminate the vanishing states and
    produce a process over the tangible states only.
  */
  class vanishing_chain {
  protected:
    long num_tangible;
    long num_vanishing;

  private:
    bool discrete;

  public:
    /// Basic constructor.
    vanishing_chain(bool disc, long nt, long nv);
    /// Basic destructor.
    virtual ~vanishing_chain();

    /// Is this a discrete-time chain?
    inline bool isDiscrete() const { return discrete; }
    /// Is this a continuous-time chain?
    inline bool isContinuous() const { return !discrete; }

    inline long getNumTangible() const { return num_tangible; }
    inline long getNumVanishing() const { return num_vanishing; }

    /** Add a new tangible state.

        @return  The handle of the state to use for edges.
    */
    virtual long addTangible() = 0;

    /** Add a new vanishing state.

        @return  The handle of the state to use for edges.
    */
    virtual long addVanishing() = 0;

    /** Set an initial tangible state.
      
        @param  handle  Index of tangible state.
        @param  weight  Probability "weight" to give to this state.

        @return true  iff this state already has an initial weight
                      (the weights will be summed in this case).
    */
    virtual bool addInitialTangible(long handle, double weight) = 0;

    /** Set an initial vanishing state.

        @param  handle  Index of vanishing state.
        @param  weight  Probability "weight" to give to this state.

        @return true  iff this state already has an initial weight
                      (the weights will be summed in this case).
    */
    virtual bool addInitialVanishing(long handle, double weight) = 0;

    /** Add an edge in the chain from tangible to tangible.

        @param  from  The "source" tangible state handle.
        @param  to    The "destination" tangible state handle.
        @param  v     Probability, for discrete; rate, for continuous.
    */
    virtual bool addTTedge(long from, long to, double v) = 0;

    /** Add an edge in the chain from tangible to vanishing.

        @param  from  The "source" tangible state handle.
        @param  to    The "destination" vanishing state handle.
        @param  v     Probability, for discrete; rate, for continuous.
    */
    virtual bool addTVedge(long from, long to, double v) = 0;

    /** Add an edge in the chain from vanishing to tangible.

        @param  from  The "source" vanishing state handle.
        @param  to    The "destination" tangible state handle.
        @param  v     Rate of the edge.
    */
    virtual bool addVTedge(long from, long to, double v) = 0;

    /** Add an edge in the chain from vanishing to vanishing.

        @param  from  The "source" vanishing state handle.
        @param  to    The "destination" vanishing state handle.
        @param  v     Rate of the edge.
    */
    virtual bool addVVedge(long from, long to, double v) = 0;

    /** Eliminate all vanishing states.
        Assumes that all edges have been added for all known vanishing
        states.  As appropriate, edges will be added between tangible
        states.  The current "batch" of vanishing states will be removed,
        while the tangibles will remain.

        @param  opt  Linear solver options to use during elimination.
    */
    virtual void eliminateVanishing(const LS_Options &opt) = 0;

    /** Obtain the initial vector, over tangible states only.

        @param  init  On output, contains the (normalized)
                      initial probability vector as specified
                      via addInitialTangible() and addInitialVanishing().
    */
    virtual void getInitialVector(LS_Vector &init) = 0;

    /** Obtain the process over tangible states only.
        Effectively destroys the current process, so
        typically this is called right before destruction.
  
        @return  The tangible process, as a Markov chain.
    */
    virtual Markov_chain* grabTTandClear() = 0;
  };





  // ======================================================================
  // |                                                                    |
  // |                        Front-end  interface                        |
  // |                                                                    |
  // ======================================================================

  /**
    Get the name and version info of the library.
    The string should not be modified or deleted.
  
    @return    Information string.
  */
  const char*  Version();


  /**
    Compute the poisson PDF.
    The PDF is truncated, so that
    Prob{L <= X <= R} >= 1-epsilon.

    @param  lambda    Poisson parameter.
    @param  epsilon   Precision.
    @param  L         Output: left truncation point.
    @param  R         Output: right truncation point.

    @return   A newly allocated vector p of doubles,
              of dimension (R-L+1),
              where p[i] = Prob{X=L+i}
  */
  double*  computePoissonPDF(double lambda, double epsilon, int& L, int &R);


  /** Build a Markov chain that is known to be irreducible.
      When the chain is finished, we can verify that the chain
      is irreducible.

      @param  is_disc     If true, start a DTMC; otherwise, start a CTMC.
      @param  num_states  Initial number of states in the chain.
      @param  num_edges   Initial number of edges in the chain.

      @return  A new, appropriate Markov_chain.
  */
  Markov_chain* 
  startIrreducibleMC(bool is_disc, long num_states, long num_edges);


  /** Build a Markov chain that is known to be absorbing.
      When the chain is finished, we can verify that the chain
      is absorbing (i.e., all recurrent states are absorbing).
      Absorbing states must be added using method addAbsorbing().

      @param  is_disc         If true, start a DTMC; otherwise, start a CTMC.
      @param  num_transient   Initial number of transient states.
      @param  num_absorbing   Initial number of absorbing states.

      @return  A new, appropriate Markov_chain.
  */
  Markov_chain* 
  startAbsorbingMC(bool is_disc, long num_transient, long num_absorbing);

  /** Build a Markov chain of any type, not required to be known.
      When the chain is finished, we will determine if it is irreducible,
      absorbing, or reducible (has at least one recurrent class of more
      than one state, and is not irreducdible).
      Known absorbing states can be added using method addAbsorbing(),
      this will cause an error to occur if an outgoing edge is added
      from this state.
      Also, states of any kind may be added using method addState().
    
      @param  is_disc     If true, start a DTMC; otherwise, start a CTMC.
      @param  num_states  Initial number of states in the chain.
      @param  num_edges   Initial number of edges in the chain.

      @return  A new, appropriate Markov_chain.
  */
  Markov_chain* 
  startUnknownMC(bool is_disc, long num_states, long num_edges);


  /** Build a vanishing chain.
      New states and edges may be added.

      @param  is_disc   If true, the final process will be a DTMC,
                        otherwise it will be a CTMC.
      @param  num_tang  Initial number of tangible states.
      @param  num_van   Initial number of vanishing states.

      @return  A new vanishing chain.
  */
  vanishing_chain* 
  startVanishingChain(bool is_disc, long num_tang, long num_van);

}; // namespace Old_MCLib

#endif
