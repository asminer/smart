
// $Id$

#ifndef RGR_ECTL_H
#define RGR_ECTL_H

#include "graph_llm.h"

// external library
#include "intset.h"

// #define DEBUG_ECTL

#ifdef DEBUG_ECTL
#include <stdio.h>
#endif

/**
    Abstract base class, for reachgraphs that use explicit CTL stuff.
*/
class ectl_reachgraph : public graph_lldsm::reachgraph {

  protected:
    
    // ======================================================================
    //
    // Helper class: use for the critical "traverse" method
    //
    class traverse_helper {
      public:
        traverse_helper(long NS);
        ~traverse_helper();

        inline long getSize() const { return size; }

        // Explore Queue Methods

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

        inline void clear_queue() {
          queue_head = 0;
          queue_tail = 0;
        }
        inline bool queue_nonempty() const { 
          return queue_head < queue_tail;
        }
        inline long queue_pop() { 
          DCASSERT(queue);
          DCASSERT(queue_nonempty());
          CHECK_RANGE(0, queue_head, size);
          return queue[queue_head++];
        }
        inline void queue_push(long x) { 
          DCASSERT(queue);
          CHECK_RANGE(0, queue_tail, size);
          queue[queue_tail] = x;
          queue_tail++;
        } 

        // Obligation Count Methods

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
        long size;
        long* obligations;  
        long* queue;
        long queue_head;  // reading end of the queue; index of next slot to read
        long queue_tail;  // writing end of the queue; index of next slot to write
    };
    //
    // End of traverse_helper class
    //
    // ======================================================================



  public:
    ectl_reachgraph();

    virtual stateset* EX(bool revTime, const stateset* p);
    virtual stateset* AX(bool revTime, const stateset* p);

    virtual stateset* EU(bool revTime, const stateset* p, const stateset* q);
    virtual stateset* unfairAU(bool revTime, const stateset* p, const stateset* q);
    virtual stateset* fairAU(bool revTime, const stateset* p, const stateset* q);

    virtual stateset* unfairEG(bool revTime, const stateset* p);
    virtual stateset* fairEG(bool revTime, const stateset* p);

    virtual stateset* AG(bool revTime, const stateset* p);

    virtual stateset* unfairAEF(bool revTime, const stateset* p, const stateset* q);

  protected:
    virtual ~ectl_reachgraph();
    virtual const char* getClassName() const { return "ectl_reachgraph"; }

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

    //
    // Add whatever it takes for us to efficiently do
    // reverse time CTL.
    // Assume that forward time CTL is already set.
    //
    virtual void need_reverse_time() = 0;

    /**
      Count edges and set obligations in our traverse helper.
        @param  rt    Reverse time?  If true, count incoming edges;
                      otherwise, count outgoing edges.
        @param  TH    Traverse helper.  On output, obligations will 
                      be set accordingly.
    */
    virtual void count_edges(bool rt, traverse_helper &TH) const = 0;
      
    /**
      Traverse graph, using the traverse helper.
      Explore using the queue of states to explore.
      When we visit a state, decrement its obligations.
      When obligations become zero, add to the queue.
      Skip over any state whose obligations are already less or equal zero.
        @param  rt        Should we reverse time
        @param  one_step  Just go one step?  If so, we don't add 
                          anything new to the queue.
        @param  TH        Traverse helper, contains a queue and 
                          obligation counts for states
    */
    virtual void traverse(bool rt, bool one_step, traverse_helper &TH) 
      const = 0; 


  private:
    traverse_helper* TH;

    // TBD - need a timer
    
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
      traverse(revTime, false, *TH);
      stopTraverse(CTLOP);

      // build answer
      p_answer.removeAll();
      TH->get_met_obligations(p_answer);
    }

};

#endif

