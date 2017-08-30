
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
    // CTL traces
    //
    virtual void traceEX(bool revTime, const stateset* p, const stateset* q);
    virtual void traceEU(bool revTime, const stateset* p, const stateset** qs, int n);
    virtual void traceEG(bool revTime, const stateset* p, const stateset* q);

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

    inline void _traceEX(bool revTime, const shared_ddedge* p, const shared_ddedge* q)
    {
        shared_ddedge* f = mrss->newMddEdge();
        DCASSERT(f);

        if (revTime) {
          mxd_wrap->preImage(p, edges, f);
        } else {
          mxd_wrap->postImage(p, edges, f);
        }

        // f := f ^ q
        MEDDLY::apply( MEDDLY::INTERSECTION, f->E, q->E, f->E );
        // f := select(f)
        MEDDLY::apply( MEDDLY::SELECT, f->E, f->E );

        //
        // Cleanup
        //
        Delete(f);
    }

    // ******************************************************************

    inline void _traceEU(bool revTime, const shared_ddedge* p, const shared_ddedge** qs, int nqs)
    {
      shared_ddedge* f = mrss->newMddEdge();
      DCASSERT(f);
      f->E = p->E;

      shared_ddedge* g = mrss->newMddEdge();
      MEDDLY::apply( MEDDLY::INTERSECTION, f->E, qs[0]->E, g->E );
      if (f->E!=g->E) {
        // p must be included in qs[0]
        return;
      }
      Delete(g);

      for (int i = 1; i < nqs; i++) {
        if (revTime) {
          mxd_wrap->preImage(f, edges, f);
        } else {
          mxd_wrap->postImage(f, edges, f);
        }

        // f := f ^ qs[i]
        MEDDLY::apply( MEDDLY::INTERSECTION, f->E, qs[i]->E, f->E );
        // f := select(f)
        MEDDLY::apply( MEDDLY::SELECT, f->E, f->E );
      }

      // Cleanup
      Delete(f);
    }

    // ******************************************************************

    inline void _traceEG(bool revTime, const shared_ddedge* p, const shared_ddedge* q)
    {
        shared_ddedge* f = mrss->newMddEdge();
        DCASSERT(f);
        f->E = p->E;

        shared_ddedge* visited = mrss->newMddEdge();
        visited->E = p->E;
        DCASSERT(visited);
        shared_ddedge* t = mrss->newMddConst(false);
        DCASSERT(t);
        shared_ddedge* empty = mrss->newMddConst(false);
        DCASSERT(empty);

        MEDDLY::apply( MEDDLY::INTERSECTION, f->E, q->E, t->E );
        if (f->E!=t->E) {
          // p must be included in q
          return;
        }

        do {
          if (revTime) {
            mxd_wrap->preImage(f, edges, f);
          } else {
            mxd_wrap->postImage(f, edges, f);
          }

          // t = f /\ visited
          MEDDLY::apply( MEDDLY::INTERSECTION, f->E, visited->E, t->E );
          if (t->E!=empty->E) {
            MEDDLY::apply( MEDDLY::SELECT, t->E, t->E );
            break;
          }

          // f := f /\ q
          MEDDLY::apply( MEDDLY::INTERSECTION, f->E, q->E, f->E );
          // f := select(f)
          MEDDLY::apply( MEDDLY::SELECT, f->E, f->E );

          // visited = visited \/ f
          MEDDLY::apply( MEDDLY::UNION, visited->E, f->E, visited->E);
        } while (true);

        //
        // Cleanup
        //
        Delete(f);
        Delete(t);
        Delete(empty);
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

