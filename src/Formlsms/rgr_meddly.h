
#ifndef RGR_MEDDLY_H
#define RGR_MEDDLY_H

#include "graph_llm.h"
#include "../ExprLib/engine.h"
#include "../Modules/glue_meddly.h"

#ifdef HAVE_MEDDLY_H

class meddly_reachset;

// ******************************************************************
// *                                                                *
// *                   meddly_monolithic_rg class                   *
// *                                                                *
// ******************************************************************

/**
    Class for monolithic next state functions.
*/
class meddly_monolithic_rg : public graph_lldsm::reachgraph {
  public:
    meddly_monolithic_rg(shared_domain* v, meddly_encoder* wrap);

  protected:
    virtual ~meddly_monolithic_rg();
    virtual const char* getClassName() const { return "meddly_monolithic_rg"; }
    virtual void attachToParent(graph_lldsm* p, state_lldsm::reachset* rss);

  private:
    void setEdges(shared_ddedge* nsf);

  public:
    inline void setPotential(shared_ddedge* nsf) {
      setEdges(nsf);
      uses_potential = true;
    }

    inline void setActual(shared_ddedge* nsf) {
      setEdges(nsf);
      uses_potential = false;
    }

    /// Indicate that when attachToParent is called, we'll convert potential to actual.
    void scheduleConversionToActual();

    //
    // Required
    //

    virtual void getNumArcs(result &na) const;
    virtual void getNumArcs(long &na) const;
    virtual void showInternal(OutputStream &os) const;
    virtual void showArcs(OutputStream &os, const show_options &opt, 
      state_lldsm::reachset* RSS, shared_state* st) const;

    //
    // CTL engines
    //

    virtual stateset* EX(bool revTime, const stateset* p);
    virtual stateset* EU(bool revTime, const stateset* p, const stateset* q);
    virtual stateset* unfairEG(bool revTime, const stateset* p);


  // 
  // Helpers
  //
  public:
    meddly_encoder* newMxdWrapper(const char* n, MEDDLY::forest::range_type t,
      MEDDLY::forest::edge_labeling ev) const;

    inline meddly_encoder& useMxdWrapper() {
      DCASSERT(mxd_wrap);
      return *mxd_wrap;
    }

    inline MEDDLY::forest* getMxdForest() const {
      DCASSERT(mxd_wrap);
      return mxd_wrap->getForest();
    }

    inline shared_ddedge* newMxdEdge() const {
      return new shared_ddedge(getMxdForest());
    }
    
    inline void createMinterms(const int* const* from, const int* const* to, int n, 
      shared_object* ans)
    {
      DCASSERT(mxd_wrap);
      mxd_wrap->createMinterms(from, to, n, ans);
    }

    inline void createMinterms(const int* const* from, const int* const* to, 
      const float* v, int n, shared_object* ans)
    {
      DCASSERT(mxd_wrap);
      mxd_wrap->createMinterms(from, to, v, n, ans);
    }

  private:
    inline shared_ddedge* buildActualEdges() const {
      DCASSERT(edges);
      DCASSERT(states);
      shared_ddedge* actual = newMxdEdge();
      mxd_wrap->selectRows(edges, states, actual);
      return actual;
    }

    template <class INT>
    inline void getNumArcsTemplate(INT &count) const {
      if (uses_potential) {
        shared_object* actual = buildActualEdges();
        mxd_wrap->getCardinality(actual, count);
        Delete(actual);
      } else {
        mxd_wrap->getCardinality(edges, count);
      }
    }

    inline static subengine::error convert(sv_encoder::error e) {
      switch (e) {
        case sv_encoder::Out_Of_Memory:   return subengine::Out_Of_Memory;
        default:                          return subengine::Engine_Failed;
      }
    }

  private:
    bool uses_potential;
    bool convert_to_actual;

    shared_domain* vars;

    meddly_encoder* mxd_wrap;
    shared_ddedge* edges;
    shared_ddedge* states;

    meddly_reachset* mrss;
};

#endif
#endif

