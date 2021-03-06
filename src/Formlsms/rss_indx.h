
#ifndef RSS_INDX_H
#define RSS_INDX_H

#include "state_llm.h"

// External libs
#include "../_LSLib/lslib.h"        // for LS_Vector
#include "../_GraphLib/graphlib.h"  // for intset
#include "../_IntSets/intset.h"     // for intset

/**
    Special case - explicit reachability sets with indexes per states.

    This prevents us from copying a few methods.

*/
class indexed_reachset : public state_lldsm::reachset {
  public:
    indexed_reachset();
    virtual ~indexed_reachset();

    virtual stateset* getReachable() const;
    virtual stateset* getPotential(expr* p) const;
    virtual stateset* getInitialStates() const;
    virtual void getBounds(long &ns, std::vector<int> set_of_places) const;

    void setInitial(const LS_Vector &init);
    inline void setInitial(const intset& init) {
      initial = init;
    }

    inline const intset& getInitial() const {
      return initial;
    }

    // Shrink the reachset to a more static structure.
    // Default does nothing.
    virtual void Finish();

    // Renumber the states, for example after classifying a Markov chain.
    // Default does nothing.
    //  @param  Ren   Node renumbering scheme.
    virtual void Renumber(const GraphLib::node_renumberer* Ren);

    

  public:
    class indexed_iterator : public reachset::iterator {
      public:
        indexed_iterator(long ns);
        virtual ~indexed_iterator();
        
        virtual void start();
        virtual void operator++(int);
        virtual operator bool() const;
        virtual long index() const;

        virtual void copyState(shared_state* st) const;
        virtual void copyState(shared_state* st, long ord) const = 0;

        inline long ord2index(long i) const {
          CHECK_RANGE(0, i, num_states);
          return map ? map[i] : i;
        }
        inline long index2ord(long i) const {
          CHECK_RANGE(0, i, num_states);
          return invmap ? invmap[i] : i;
        }
        inline long getI() const {
          return I;
        }
        inline long getIndex() const {
          return ord2index(I);
        }
      protected:
        void setMap(long* m);
      private:
        long num_states;
        long* map;
        long* invmap;
        long I;
    };

  private:
    class pot_visit : public state_lldsm::state_visitor {
      expr* p;
      intset &pset;
      result tmp;
      bool ok;
    public:
      pot_visit(const hldsm* mdl, expr* _p, intset &ps);
      inline bool isOK() const { return ok; }
      virtual bool visit();
    };

  private:
    intset initial;
};

#endif

