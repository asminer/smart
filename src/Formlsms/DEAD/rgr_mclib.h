
// $Id$

#ifndef RGR_MCLIB_H
#define RGR_MCLIB_H

#include "rgr_ectl.h"
#include "rss_indx.h"

#include "mclib.h"

class mclib_reachgraph : public ectl_reachgraph {

  public:
    mclib_reachgraph(MCLib::Markov_chain* mc);

  protected:
    virtual ~mclib_reachgraph();
    virtual const char* getClassName() const { return "mclib_reachgraph"; }

  public:
    virtual void getNumArcs(long &na) const;
    virtual void showInternal(OutputStream &os) const;
    virtual void showArcs(OutputStream &os, state_lldsm::reachset* RSS, 
      state_lldsm::display_order ord, shared_state* st) const;

  protected:
    virtual bool forward(const intset& p, intset &r) const;
    virtual bool backward(const intset& p, intset &r) const;
    virtual void absorbing(intset &r) const;
    
  private:
    MCLib::Markov_chain* chain;

  private:
    class sparse_row_elems : public GraphLib::generic_graph::element_visitor {
      int alloc;
      const indexed_reachset::indexed_iterator &I;
      bool incoming;
      bool overflow;
    public:
      int last;
      long* index;
      double* value;
    public:
      sparse_row_elems(const indexed_reachset::indexed_iterator &i);
      virtual ~sparse_row_elems();

    protected:
      bool Enlarge(int ns);
    public:
      bool buildIncoming(MCLib::Markov_chain* chain, int i);
      bool buildOutgoing(MCLib::Markov_chain* chain, int i);

    // for element_visitor
      virtual bool visit(long from, long to, void*);    

    // for heapsort
      inline int Compare(long i, long j) const {
        CHECK_RANGE(0, i, last);
        CHECK_RANGE(0, j, last);
        return SIGN(index[i] - index[j]);
      }

      inline void Swap(long i, long j) {
        CHECK_RANGE(0, i, last);
        CHECK_RANGE(0, j, last);
        SWAP(index[i], index[j]);
        SWAP(value[i], value[j]);
      }
    };

};

#endif

