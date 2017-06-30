
#ifndef RGR_MEDDLY_H
#define RGR_MEDDLY_H

#include "../Modules/glue_meddly.h"

#ifdef HAVE_MEDDLY_H

#include "graph_llm.h"
#include "../ExprLib/engine.h"
#include "../Modules/meddly_ssets.h"
#include "rss_meddly.h"

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
    virtual stateset* AX(bool revTime, const stateset* p);
    virtual stateset* EU(bool revTime, const stateset* p, const stateset* q);
    virtual stateset* unfairAU(bool revTime, const stateset* p, const stateset* q);
    virtual stateset* unfairEG(bool revTime, const stateset* p);
    virtual stateset* AG(bool revTime, const stateset* p);


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
    
    // ******************************************************************

    inline void _EX(bool revTime, const shared_ddedge* p, shared_ddedge* ans) 
    {
        if (revTime) {
          mxd_wrap->postImage(p, edges, ans);
        } else {
          mxd_wrap->preImage(p, edges, ans);
        }
    }

    // ******************************************************************

    inline void _EU(bool revTime, const shared_ddedge* p, const shared_ddedge* q, 
      shared_ddedge* ans) 
    {
        //
        // Special case - if p=0 (means true), use saturation
        //
        if (0==p) {
          if (revTime) {
            mxd_wrap->postImageStar(q, edges, ans);
          } else {
            mxd_wrap->preImageStar(q, edges, ans);
          }
          return;
        }

        //
        // Non trivial p, use iteration
        // TBD - use saturation here as well
        //

        //
        // Auxiliary sets
        //
        shared_ddedge* prev = mrss->newMddConst(false); // prev := emptyset
        shared_ddedge* f = mrss->newMddEdge();

        DCASSERT(prev);
        DCASSERT(f);

        // answer := q
        ans->E = q->E;

        while (prev->E != ans->E) {

          // f := pre/post (answer)
          if (revTime) {
            mxd_wrap->postImage(ans, edges, f);
          } else {
            mxd_wrap->preImage(ans, edges, f);
          }

          // f := f ^ p
          MEDDLY::apply( MEDDLY::INTERSECTION, f->E, p->E, f->E );

          // prev := answer
          prev->E = ans->E;

          // answer := answer U f
          MEDDLY::apply( MEDDLY::UNION, ans->E, f->E, ans->E );

        } // iteration

        // Cleanup
        Delete(f);
        Delete(prev);
    }

    // ******************************************************************

    inline void _unfairEG(bool revTime, const shared_ddedge* p, shared_ddedge* ans) 
    {
        //
        // Build set of source/deadlocked states 
        //
        shared_ddedge* dead = 0;

        if (revTime) {
          dead = mrss->newMddEdge();
          DCASSERT(dead);
          dead->E = mrss->getInitial();
        } else {
          // Determine deadlocked states: !EX(true)
          dead = mrss->newMddConst(true);
          DCASSERT(dead);
          mxd_wrap->preImage(dead, edges, dead);
          MEDDLY::apply( MEDDLY::COMPLEMENT, dead->E, dead->E);
        }

        //
        // Result and auxiliary sets
        //

        shared_ddedge* prev = mrss->newMddConst(false); // prev := emptyset
        shared_ddedge* f = mrss->newMddEdge();
        DCASSERT(prev);
        DCASSERT(f);

        // answer := p
        ans->E = p->E;

        while (prev->E != ans->E) {

          // f := pre/post (answer)
          if (revTime) {
            mxd_wrap->postImage(ans, edges, f);
          } else {
            mxd_wrap->preImage(ans, edges, f);
          }

          // f := f + dead
          MEDDLY::apply( MEDDLY::UNION, f->E, dead->E, f->E );

          // prev := answer
          prev->E = ans->E;

          // answer := f * p
          MEDDLY::apply( MEDDLY::INTERSECTION, f->E, p->E, ans->E );

        } // iteration

        //
        // Cleanup
        //
        Delete(dead);
        Delete(prev);
        Delete(f);
    }

    // ******************************************************************

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

