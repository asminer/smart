
// $Id$

#ifndef RGR_EXPL_H
#define RGR_EXPL_H

#include "rgr_ectl.h"
#include "rss_indx.h"

#include "graphlib.h"

class expl_reachgraph : public ectl_reachgraph {

  public:
    expl_reachgraph(GraphLib::digraph* g);

  protected:
    virtual ~expl_reachgraph();
    virtual const char* getClassName() const { return "expl_reachgraph"; }
    virtual void Finish(state_lldsm::reachset*);

  public:
    virtual void getNumArcs(long &na) const;
    virtual void showInternal(OutputStream &os) const;
    virtual void showArcs(OutputStream &os, state_lldsm::reachset* RSS, 
      state_lldsm::display_order ord, shared_state* st) const;

    // Hold initial until we can give it to RSS.
    void setInitial(LS_Vector &init);

  protected:
    virtual bool forward(const intset& p, intset &r) const;
    virtual bool backward(const intset& p, intset &r) const;
    virtual void absorbing(intset &r) const;
    virtual void source(intset &r) const;
    
  private:
    GraphLib::digraph* edges;
    LS_Vector initial;    // hold until we can pass it to RSS

  private:
    class sparse_row_elems : public GraphLib::generic_graph::element_visitor {
      int alloc;
      const indexed_reachset::indexed_iterator &I;
      bool incoming;
      bool overflow;
    public:
      int last;
      long* index;
    public:
      sparse_row_elems(const indexed_reachset::indexed_iterator &i);
      virtual ~sparse_row_elems();

    protected:
      bool Enlarge(int ns);
    public:
      bool buildIncoming(GraphLib::digraph* g, int i);
      bool buildOutgoing(GraphLib::digraph* g, int i);

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
      }
    };

};

#endif

