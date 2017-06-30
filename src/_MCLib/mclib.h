
/** \file mclib.h
    Markov chain library.
    Allows construction and analysis of Markov chains
    (both discrete and continuous time).
*/

#ifndef MCLIB_H
#define MCLIB_H

#define DISABLE_OLD_INTERFACE

#include "../_GraphLib/graphlib.h"
#include "../_LSLib/lslib.h"
#include "../_RngLib/rng.h"
#include "../_IntSets/intset.h"
#include "../_Distros/distros.h"

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
      /// Illegal class value
      Bad_Class,
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

      /// Options and auxiliary vectors for transient analysis of DTMCs.
      struct DTMC_transient_options {
        /// Precision for detection of steady-state (use 0 to not check)
        double ssprec;
        /// Vector to hold result of vector-matrix multiply
        double* vm_result;
        /// Vector to accumulate sum, if necessary
        double* accumulator;
        /// Output: number of vector matrix multiplies required.
        long multiplications;

        /** 
          Constructor; sets reasonable defaults
        */
        DTMC_transient_options() {
          ssprec = 1e-10;
          vm_result = 0;
          accumulator = 0;
        }
        /** 
          Destructor; destroys auxiliary vectors.
          If you don't want this to happen, set the pointers to 0.
        */
        ~DTMC_transient_options() {
          delete[] vm_result;
          delete[] accumulator;
        }
      };

    public:

      /// Options and auxiliary vectors for transient analysis of CTMCs.
      struct CTMC_transient_options : public DTMC_transient_options {
        /** Uniformization constant to use (if possible).
            If not large enough, we will change the value.
        */
        double q;
        /// Precision for poisson distribution
        double epsilon;
        /// Output: right truncation point of poisson.
        long poisson_right;
        
        /** 
          Constructor; sets reasonable defaults
        */
        CTMC_transient_options() {
          q = 0;  // Use smallest possible.
          epsilon = 1e-20;
        }

      };

  public:

      /// Options and auxiliary vectors for TTA distributions of DTMCs.
      struct DTMC_distribution_options {
        /// Vector to hold result of vector-matrix multiply
        double* vm_result;
        /// Probability vector at various times.
        double* prob_vect;
        /// Input: Desired error.
        double epsilon;
        /// Output: Achieved precision.
        double precision;

        /** 
          Constructor; sets reasonable defaults
        */
        DTMC_distribution_options() {
          vm_result = 0;
          prob_vect = 0;
          epsilon = 1e-6;
          precision = 0;
          distro = 0;
          error_distro = 0;
          distprod = 0;
          max_size = 0;
          needs_error = false;
          needs_distprod = false;
        }

        /** 
          Destructor; destroys auxiliary vectors.
          If you don't want this to happen, set the pointers to 0.
        */
        ~DTMC_distribution_options() {
          delete[] vm_result;
          delete[] prob_vect;
          free(distro);
          free(error_distro);
        }

        /**
          Set up maximum size for distribution;
          will allocate that much temporary space.
          This is the preferred way to set the maximum size.
        */
        void setMaxSize(long ms);


        //
        // Yes, the following are public,
        // but are tricker to get right, 
        // so be careful or use the provided methods.
        //
        public: 
          /// Distribution, while we're building it
          double* distro;
          /// Error distribution, if we need it
          double* error_distro;
          /// Distribution times poisson, if we need it
          double* distprod;
          /// Maximum distribution size.
          long max_size;
          /// Do we use the error distribution
          bool needs_error;
          /// Do we use the distprod array.
          bool needs_distprod;
      };


  public:

      /// Options and auxiliary vectors for TTA distributions of CTMCs.
      struct CTMC_distribution_options : public DTMC_distribution_options {
        /// Uniformization constant to use (if possible)
        double q;
        /// Precision for poisson distributions
        double poisson_epsilon;

        /**
          Constructor; sets reasonable defaults
        */
        CTMC_distribution_options() {
          q=0;
          poisson_epsilon = 1e-20;
          needs_error = true;
          needs_distprod = true;
        }

      };

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
      ~Markov_chain();

    public:
      /// Is this a discrete-time chain?
      inline bool isDiscrete() const { return is_discrete; }

      /// Is this a continuous-time chain?
      inline bool isContinuous() const { return !is_discrete; }

      /// Number of states in the chain
      inline long getNumStates() const { return G_byrows_diag.getNumNodes(); }

      /** Number of edges in the chain.
          CTMC - takes O(1) time.
          DTMC - takes O(#states) time.
      */
      long getNumEdges() const;

      /**
          Number of edges to/from state s in the chain.
            @param  incoming    If true, we count incoming edges.
                                Otherwise, we count outgoing edges.

            @param  state       State we care about

            @return Number of edges to/from state.
      */
      long getNumEdgesFor(bool incoming, long state) const;

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
          Return true if the graphs use doubles.
          We might need to know this outside the class
          if we're doing a traversal.
      */
      inline bool edgesStoredAsDoubles() const {
        return double_graphs;
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


      /** Compute the distribution at time t, given the starting distribution.
          Must be a DTMC.
          Vectors are allocated so that x[s] is the probability for state s,
          for any legal state handle s.

          @param  t       Time.

          @param  p       On input: distribution at time 0.
                          On output: distribution at time t.

          @param  opts    Options and auxiliary vectors.
      */
      void computeTransient(int t, double* p, DTMC_transient_options &opts) 
      const;

      /** Compute the distribution at time t, given the starting distribution.
          Must be a CTMC.
          Vectors are allocated so that x[s] is the probability for state s,
          for any legal state handle s.

          @param  t       Time.

          @param  p       On input: distribution at time 0.
                          On output: distribution at time t.

          @param  opts    Options and auxiliary vectors.
      */
      void computeTransient(double t, double* p, CTMC_transient_options &opts) 
      const;



      /** Compute an expectation at time t, for all possible starting states.
          Must be a DTMC.
          Vectors are allocated so that x[s] is the expectation when the
          chain starts in state s, for any legal state handle s.
          Specifically, we determine

            x[s] = E [ f(state at time t) ], given we start in state s

          @param  t   Time

          @param  x   On input: function f() to compute expectation over.
                      On output: expected value for each starting state.
      */
      void reverseTransient(int t, double* x, DTMC_transient_options &opts)
      const;

      /** Compute an expectation at time t, for all possible starting states.
          Must be a CTMC.
          Vectors are allocated so that x[s] is the expectation when the
          chain starts in state s, for any legal state handle s.
          Specifically, we determine

            x[s] = E [ f(state at time t) ], given we start in state s

          @param  t   Time

          @param  x   On input: function f() to compute expectation over.
                      On output: expected value for each starting state.
      */
      void reverseTransient(double t, double* x, CTMC_transient_options &opts)
      const;



      /** Compute the accumulated time spent in every state, up
          to and including time t.
          Must be a DTMC.

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
      void accumulate(int t, const double* p0, double* n0t, 
        DTMC_transient_options &opts) const;


      /** Compute the accumulated time spent in every state, up
          to and including time t.
          Must be a CTMC.

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
      void accumulate(double t, const double* p0, double* n0t, 
        CTMC_transient_options &opts) const;

      

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



      /** Compute the (discrete) distribution for "time to reach class c".

            @param  opts    Input and output: Options.

            @param  p0      Initial distribution.

            @param  c       Class or absorbing state we wish to enter.
                            If positive, it is a class, as specified by the
                            static classifier given by method
                            getStateClassification().
                            If zero or negative, it is a state.
                            Negate the value to get the state number.

            @param  dist    Output: the distribution to the desired precision.

      */
      void computeDiscreteDistTTA(DTMC_distribution_options &opts, 
          const LS_Vector &p0, long c, discrete_pdf &dist) const;



      /** Compute the (continuous) distribution for "time to reach class c".

            @param  opts    Options

            @param  p0      Initial distribution.

            @param  c       Class or absorbing state we wish to enter.
                            If positive, it is a class, as specified by the
                            static classifier given by method
                            getStateClassification().
                            If zero or negative, it is a state.
                            Negate the value to get the state number.

            @param  dt      Time increment.  We compute the PDF at times
                            0, dt, 2*dt, 3*dt, ...

            @param  dist    Output: the distribution to the desired precision,
                            but discretized using dt.  dist.f(i) gives the
                            PDF for time point i*dt.

      */
      void computeContinuousDistTTA(CTMC_distribution_options &opts,
        const LS_Vector &p0, long c, double dt, discrete_pdf &dist) const;



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
          Self loop probabilities, as doubles.
          In a DTMC, selfloops[s] is the probability of remaining
          in state s in one step.
          Note that selfloops[s] + rowsums[s] = 1 always.
          Needed in case rowsums[s] is very close to 1
          for numerical stability.
          Used only if the graphs are stored with doubles.
          For a CTMC, this will be null.
      */
      double* selfloops_d;

      /**
          Self loop probabilities, as floats.
          In a DTMC, selfloops[s] is the probability of remaining
          in state s in one step.
          Note that selfloops[s] + rowsums[s] = 1 always.
          Needed in case rowsums[s] is very close to 1
          for numerical stability.
          Used only if the graphs are stored with floats.
          For a CTMC, this will be null.
      */
      float* selfloops_f;
      
      /**
          Row sums.
          For each state s,
          rowsums[s] gives the sum of outgoing edges from state s
          (discarding any self loops).
          Note that selfloops[s] + rowsums[s] = 1 for a DTMC.
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




  // ======================================================================
  // |                                                                    |
  // |                                                                    |
  // |                       vanishing_chain  class                       |
  // |                                                                    |
  // |                                                                    |
  // ======================================================================


  /**
    Class for a semi-Markov processes containing "tangible" states
    and "vanishing" states.  The time spent in a vanishing state
    is 0, while the time spent in a tangible state is either 
    1 (in the discrete case) or exponentially-distributed
    (in the continuous case).
    This class is used to eliminate the vanishing states and
    produce a Markov chain (as a graph) over the tangible states only.
  */
  class vanishing_chain {
    public:
      /** Constructor.
            @param  disc  Are we discrete?
            @param  nt    Initial number of tangible states
            @param  nv    Initial number of vanishing states
      */
      vanishing_chain(bool disc, long nt, long nv);


      /// Destructor.
      ~vanishing_chain();

      /// @return Total memory required to store the chain, in bytes.
      size_t getMemTotal() const;

      /// Is this a discrete-time chain?
      inline bool isDiscrete() const { return discrete; }
      /// Is this a continuous-time chain?
      inline bool isContinuous() const { return !discrete; }

      /// Add new tangible states
      inline void addTangibles(long nt) {
        TT_graph.addNodes(nt);
      }

      /// Add one new tangible state
      inline void addTangible() {
        TT_graph.addNode();
      }

      /// Add new vanishing states
      inline void addVanishings(long nv) {
        VV_graph.addNodes(nv);
      }

      /// Add one new vanishing state
      inline void addVanishing() {
        VV_graph.addNode();
      }

      /// Get number of tangible states
      inline long getNumTangible() const { 
        return TT_graph.getNumNodes();
      }

      /// Get number of vanishing states
      inline long getNumVanishing() const { 
        return VV_graph.getNumNodes();
      }


      /** Set an initial tangible state.
      
            @param  handle  Index of tangible state.
            @param  weight  Probability "weight" to give to this state.
      */
      inline void addInitialTangible(long handle, double weight) {
        CHECK_RANGE(0, handle, getNumTangible());
        Tinit.addItem(handle, weight);
      }

      /** Set an initial vanishing state.

          @param  handle  Index of vanishing state.
          @param  weight  Probability "weight" to give to this state.
      */
      inline void addInitialVanishing(long handle, double weight) {
        CHECK_RANGE(0, handle, getNumVanishing());
        Vinit.addItem(handle, weight);
      }


      /** Add an edge in the chain from tangible to tangible.

          @param  from  The "source" tangible state handle.
          @param  to    The "destination" tangible state handle.
          @param  v     Probability, for discrete; rate, for continuous.
      */
      inline void addTTedge(long from, long to, double v) {
        CHECK_RANGE(0, from, getNumTangible());
        CHECK_RANGE(0, to, getNumTangible());
        TT_graph.addEdge(from, to, v);
      }

      /** Add an edge in the chain from vanishing to vanishing.

          @param  from  The "source" vanishing state handle.
          @param  to    The "destination" vanishing state handle.
          @param  v     Rate of the edge.
      */
      inline void addVVedge(long from, long to, double v) {
        CHECK_RANGE(0, from, getNumVanishing());
        CHECK_RANGE(0, to, getNumVanishing());
        VV_graph.addEdge(from, to, v);
      }

      /** Add an edge in the chain from tangible to vanishing.

          @param  from  The "source" tangible state handle.
          @param  to    The "destination" vanishing state handle.
          @param  v     Probability, for discrete; rate, for continuous.
      */
      inline void addTVedge(long from, long to, double v) {
        CHECK_RANGE(0, from, getNumTangible());
        CHECK_RANGE(0, to, getNumVanishing());
        TV_edges.addEdge(from, to, v);
      }

      /** Add an edge in the chain from vanishing to tangible.

          @param  from  The "source" vanishing state handle.
          @param  to    The "destination" tangible state handle.
          @param  v     Rate of the edge.
      */
      inline void addVTedge(long from, long to, double v) {
        CHECK_RANGE(0, from, getNumVanishing());
        CHECK_RANGE(0, to, getNumTangible());
        VT_edges.addEdge(from, to, v);
      }


      /** Eliminate all vanishing states.
          Assumes that all edges have been added for all known vanishing
          states.  As appropriate, edges will be added between tangible
          states.  The current "batch" of vanishing states will be removed,
          while the tangibles will remain.

          @param  opt   Linear solver options to use during elimination.
      */
      void eliminateVanishing(const LS_Options &opt);


      /** 
          Build the initial vector, over tangible states only.
          Must be called after all vanishing states have been eliminated.

          @param  floats  Should we use float vectors?  Ohterwise it will
                          be doubles.

          @param  init    Input: ignored and colbbered (not freed).
                          On output, contains the (normalized)
                          initial probability vector as specified
                          via addInitialTangible() and addInitialVanishing().
      */
      void buildInitialVector(bool floats, LS_Vector &init) const;


      /** 
          Obtain the process over tangible states only.
    
          @return  The tangible process, as a graph.
      */
      inline GraphLib::dynamic_summable<double> & TT() {
        return TT_graph;
      }

    private:
      struct pair {
        long index;
        double weight;
      };
      struct pairlist {
        public:
          pairlist();
          ~pairlist();

          void addItem(long i, double w);
          void clear();
          size_t getMemTotal() const;
        public:
          pair* pairarray;
          long alloc_pairs;
          long last_pair;
      };
      struct edge {
        long from;
        long to;
        double weight;
      };
      struct edgelist {
        public:
          edgelist();
          ~edgelist();

          void addEdge(long from, long to, double wt); 
          void groupBySource();
          void clear();
          size_t getMemTotal() const;
        private:
          void swapedges(long i, long j);

        public:
          edge* edgearray;
          long alloc_edges;
          long last_edge;
      };
    private:
      GraphLib::dynamic_summable<double> TT_graph;
      GraphLib::dynamic_summable<double> VV_graph;

      pairlist Tinit;
      pairlist Vinit;

      edgelist TV_edges;
      edgelist VT_edges;

      bool discrete;

  };  // class vanishing_chain




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

};  // namespace MCLib


#endif // include guard
