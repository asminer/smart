
#ifndef RGR_ECTL_H
#define RGR_ECTL_H

#include "graph_llm.h"
#include "../_GraphLib/graphlib.h"

// external library
#include "../_IntSets/intset.h"

// #define DEBUG_ECTL

#ifdef DEBUG_ECTL
#include <stdio.h>
#endif

/**
    Abstract base class, for reachgraphs that use explicit CTL stuff.
*/
class ectl_reachgraph : public graph_lldsm::reachgraph {

  public:
    
    // ======================================================================
    //
    // Helper class: use for the critical "traverse" method
    //
    class CTL_traversal : public GraphLib::BF_with_queue {
      public:
        CTL_traversal(long NS);
        virtual ~CTL_traversal();

        virtual bool visit(long, long dest, const void*);

      public:
        inline long getSize() const { return size; }

        /**
          Should this be a "one step only" traversal?
            @param  os    If true, when visiting states,
                          we do not add them to the queue.
        */
        inline void setOneStep(bool os) {
          one_step = os;
        }

        //
        // Queue initialization methods
        //

        /**
          Add elements from the set p to the queue,
          and sets the obligations to 0 for those elements.
            @param  p     Set of elements to add to queue
        */
        void init_queue_from(const intset &p);

        /**
          Add elements NOT in the set p to the queue,
          and sets the obligations to 0 for those elements.
            @param  p     Set of elements NOT to add to queue
        */
        void init_queue_complement(const intset &p);

        //
        // Obligation counting methods
        //

        /// Anything with zero obligations, set to new value
        void reset_zero_obligations(int newval=-1);

        /// Set obligations to value for anything not in the set p.
        void restrict_paths(const intset &p, int value=-1);

        /// Set obligations to value for anything in the set p.
        void set_obligations(const intset &p, int value=1);

        /// Set all obligations to value.
        inline void fill_obligations(int value) { 
          DCASSERT(obligations);
          for (long i=0; i<size; i++) obligations[i] = value;
        }

        /// Increment obligations for state i
        inline void add_obligation(long i) { 
          DCASSERT(obligations);
          CHECK_RANGE(0, i, size);
          obligations[i]++;
        }

        /// Increment obligations for states in set p
        void add_obligations(const intset& p);

        /// Decrement obligations for state i
        inline void remove_obligation(long i) { 
          DCASSERT(obligations);
          CHECK_RANGE(0, i, size);
          obligations[i]--;
        }

        /// Get current number of unmet obligations for state i
        inline long num_obligations(long i) const { 
          DCASSERT(obligations);
          CHECK_RANGE(0, i, size);
          return obligations[i];
        }

        /// Set obligations for state i
        inline void set_obligations(long i, int value) {
          DCASSERT(obligations);
          CHECK_RANGE(0, i, size);
          obligations[i] = value;
        }


        /**
          If obligations[i] == 0, add i to set x.
          Do this for all i.
        */
        void get_met_obligations(intset &x) const;

#ifdef DEBUG_ECTL
        void dump(FILE* out) const;
#endif

      private:
        long* obligations;  
        long size;
        bool one_step;
    };
    //
    // End of CTL_traversal class
    //
    // ======================================================================


  public:
    ectl_reachgraph();

    virtual stateset* EX(bool revTime, const stateset* p, trace_data* td) override;
    virtual stateset* AX(bool revTime, const stateset* p) override;

    virtual stateset* EU(bool revTime, const stateset* p, const stateset* q, trace_data* td) override;
    virtual stateset* unfairAU(bool revTime, const stateset* p, const stateset* q) override;
    virtual stateset* fairAU(bool revTime, const stateset* p, const stateset* q) override;

    virtual stateset* unfairEG(bool revTime, const stateset* p, trace_data* td) override;
    virtual stateset* fairEG(bool revTime, const stateset* p) override;

    virtual stateset* AG(bool revTime, const stateset* p) override;

    virtual stateset* unfairAEF(bool revTime, const stateset* p, const stateset* q) override;

  protected:
    virtual ~ectl_reachgraph();
    virtual const char* getClassName() const override;

    // Build set of deadlocked states (no outgoing edges)
    // Default: builds emptyset
    virtual void getDeadlocked(intset &r) const;

    const intset& getInitial() const;

    /** Determine TSCCs satisfying a property (absorbing don't count).
        This is done "in place".  Should only be called for "fair" models;
        behavior is undefined if the model is not fair.
        Default behavior here is to empty p.
          @param  p   On input: property p.
                      On output: states are removed if
                      they are not in a TSCC, or if
                      not all states in the TSCC satisfy p.
    */
    virtual void getTSCCsSatisfying(intset &p) const;

    /**
      Count edges and set obligations in our traverse helper.
        @param  rt    Reverse time?  If true, count incoming edges;
                      otherwise, count outgoing edges.
        @param  CTL   CTL traversal.  On output, obligations will 
                      be set accordingly.
    */
    virtual void count_edges(bool rt, CTL_traversal &CTL) const = 0;
      
    /**
      Traverse graph, using the given traversal.
        @param  rt      Should we reverse time?  If so, we traverse the
                        outgoing edges, otherwise we traverse the
                        incoming edges.
        @param  T       Graph traversal to use.  
    */
    virtual void traverse(bool rt, GraphLib::BF_graph_traversal &T) const = 0; 


  protected:
    CTL_traversal* TH;

    // TBD - need a timer, for reporting
    
  private:
    inline void startTraverse(const char* who) {
      // TBD - start timer 
#ifdef DEBUG_ECTL
      fprintf(stderr, "Calling traverse for %s\n", who);
      TH->dump(stderr);
#endif
    }
    inline void stopTraverse(const char* who) {
      // TBD - stop timer and reporting here
      // TBD - CTL debug output
#ifdef DEBUG_ECTL
      fprintf(stderr, "Finished traverse for %s\n", who);
      TH->dump(stderr);
#endif
    }

    /// On input, p_answer is the property;
    /// on output, it is the answer.
    inline void fair_EG_helper(bool revTime, intset &p_answer, const char* CTLOP) {

      // Obligations to 1
      TH->fill_obligations(1);

      // Restrict to paths along p
      TH->restrict_paths(p_answer);

      if (revTime) {
        // Determine source states satisfying p
        intset tmp(p_answer);
        tmp *= getInitial();

        // Determine TSCCs satisfying p
        getTSCCsSatisfying(p_answer);

        // Add source states
        p_answer += tmp;
      } else {
        // Determine TSCCs satisfying p
        getTSCCsSatisfying(p_answer);
      }

      // Explore from p_answer
      TH->init_queue_from(p_answer);

      // Traverse!
      startTraverse(CTLOP);
      TH->setOneStep(false);
      traverse(revTime, *TH);
      stopTraverse(CTLOP);

      // build answer
      p_answer.removeAll();
      TH->get_met_obligations(p_answer);
    }

};

#endif

