
#ifndef RGR_MEDDLY_H
#define RGR_MEDDLY_H

#include "../Modules/glue_meddly.h"

#ifdef HAVE_MEDDLY_H

#include "graph_llm.h"
#include "../ExprLib/engine.h"
#include "../Modules/meddly_ssets.h"
#include "../Modules/trace.h"
#include "rss_meddly.h"

class meddly_reachset;

// ******************************************************************
// *                                                                *
// *                     meddly_trace_data class                    *
// *                                                                *
// ******************************************************************

class meddly_trace_data : public trace_data {
public:
  meddly_trace_data();

protected:
  ~meddly_trace_data();

public:
  void AppendStage(shared_ddedge* s);
  int Length() const;
  const shared_ddedge* getStage(int i) const;

private:
  List<shared_ddedge> stages;
};

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

    virtual stateset* EX(bool revTime, const stateset* p, trace_data* td = nullptr);
    virtual stateset* AX(bool revTime, const stateset* p);
    virtual stateset* EU(bool revTime, const stateset* p, const stateset* q, trace_data* td = nullptr);
    virtual stateset* unfairAU(bool revTime, const stateset* p, const stateset* q);
    virtual stateset* unfairEG(bool revTime, const stateset* p, trace_data* td = nullptr);
    virtual stateset* AG(bool revTime, const stateset* p);

    //
    // CTL traces
    //
    virtual void traceEX(bool revTime, const stateset* p, const trace_data* td, List<stateset>* ans);
    virtual void traceEU(bool revTime, const stateset* p, const trace_data* td, List<stateset>* ans);
    virtual void traceEG(bool revTime, const stateset* p, const trace_data* td, List<stateset>* ans);

    virtual inline trace_data* makeTraceData() const {
      return new meddly_trace_data();
    }

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

    inline void _EX(bool revTime, const shared_ddedge* p, shared_ddedge* ans, List<shared_ddedge>* extra = nullptr)
    {
        if (revTime) {
          mxd_wrap->postImage(p, edges, ans);
        } else {
          mxd_wrap->preImage(p, edges, ans);
        }

        if (nullptr != extra) {
          extra->Append(Share(ans));
          extra->Append(Share(const_cast<shared_ddedge*>(p)));
        }
    }

    // ******************************************************************

    inline void _EU(bool revTime, const shared_ddedge* p, const shared_ddedge* q, 
      shared_ddedge* ans, List<shared_ddedge>* extra = nullptr)
    {
        //
        // Special case - if p=0 (means true), use saturation
        //
        if (nullptr == p && nullptr == extra) {
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
        if (nullptr != extra) {
          shared_ddedge* t = mrss->newMddEdge();
          t->E = ans->E;
          extra->Append(t);
        }

        while (prev->E != ans->E) {

          // f := pre/post (answer)
          if (revTime) {
            mxd_wrap->postImage(ans, edges, f);
          } else {
            mxd_wrap->preImage(ans, edges, f);
          }

          if (nullptr != p) {
            // f := f ^ p
            MEDDLY::apply( MEDDLY::INTERSECTION, f->E, p->E, f->E );
          }

          // prev := answer
          prev->E = ans->E;

          // answer := answer U f
          MEDDLY::apply( MEDDLY::UNION, ans->E, f->E, ans->E );

          if (nullptr != extra) {
            shared_ddedge* t = mrss->newMddEdge();
            t->E = ans->E;
            extra->Append(t);
          }
        } // iteration

        if (nullptr != extra) {
          extra->Reverse();
        }

        // Cleanup
        Delete(f);
        Delete(prev);
    }

    // ******************************************************************

    inline void _unfairEG(bool revTime, const shared_ddedge* p,
        shared_ddedge* ans, List<shared_ddedge>* extra = nullptr)
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

        if (nullptr != extra) {
          extra->Append(Share(ans));
        }

        //
        // Cleanup
        //
        Delete(dead);
        Delete(prev);
        Delete(f);
    }

    // ******************************************************************

    inline void _traceEX(bool revTime, const shared_ddedge* p, const meddly_trace_data* mtd, List<shared_ddedge>* ans)
    {
        DCASSERT(mtd->Length() == 2);

        shared_ddedge* f = mrss->newMddEdge();
        DCASSERT(f);

        MEDDLY::apply( MEDDLY::INTERSECTION, p->E, mtd->getStage(0)->E, f->E );
        if (p->E != f->E) {
          // p must be included in mtd[0]
          DCASSERT(false);
          return;
        }

        if (revTime) {
          mxd_wrap->preImage(p, edges, f);
        } else {
          mxd_wrap->postImage(p, edges, f);
        }

        // f := f /\ mtd[1]
        MEDDLY::apply( MEDDLY::INTERSECTION, f->E, mtd->getStage(1)->E, f->E );
        // f := select(f)
        MEDDLY::apply( MEDDLY::SELECT, f->E, f->E );

        {
          shared_ddedge* t = mrss->newMddEdge();
          DCASSERT(t);
          t->E = p->E;
          ans->Append(t);
        }
        ans->Append(Share(f));

        //
        // Cleanup
        //
        Delete(f);
    }

    // ******************************************************************

    inline void _traceEU(bool revTime, const shared_ddedge* p, const meddly_trace_data* mtd, List<shared_ddedge>* ans)
    {
      shared_ddedge* f = mrss->newMddEdge();
      DCASSERT(f);
      f->E = p->E;

      DCASSERT(mtd->Length() > 0);

      shared_ddedge* g = mrss->newMddEdge();
      MEDDLY::apply( MEDDLY::INTERSECTION, f->E, mtd->getStage(0)->E, g->E );
      if (f->E != g->E) {
        // p must be included in mtd[0]
        DCASSERT(false);
        return;
      }

      {
        shared_ddedge* t = mrss->newMddEdge();
        DCASSERT(t);
        t->E = f->E;
        ans->Append(t);
      }

      // Binary search to determine the minimum number of steps
      // mtd[0] \superset mtd[1] \superset mtd[2] \superset ...
      // Find a start s.t. p \in mtd[start-1] and p \not\in mtd[start]
      int start = 1;
      int end = mtd->Length() - 1;
      while (start <= end) {
        int mid = (start + end) / 2;
        MEDDLY::apply( MEDDLY::INTERSECTION, p->E, mtd->getStage(mid)->E, g->E );
        if (p->E == g->E) {
          start = mid + 1;
        }
        else {
          end = mid - 1;
        }
      }

      // The state in p can reach a state in mtd[legnth-1] at exactly length-start steps
      for (int i = start; i < mtd->Length(); i++) {
        if (revTime) {
          mxd_wrap->preImage(f, edges, f);
        } else {
          mxd_wrap->postImage(f, edges, f);
        }

        // f := f /\ mtd[i]
        MEDDLY::apply( MEDDLY::INTERSECTION, f->E, mtd->getStage(i)->E, f->E );
        // f := select(f)
        MEDDLY::apply( MEDDLY::SELECT, f->E, f->E );

        shared_ddedge* t = mrss->newMddEdge();
        DCASSERT(t);
        t->E = f->E;
        ans->Append(t);
      }

      // Cleanup
      Delete(f);
      Delete(g);
    }

    // ******************************************************************

    inline void _traceEG(bool revTime, const shared_ddedge* p, const meddly_trace_data* mtd, List<shared_ddedge>* ans)
    {
        shared_ddedge* f = mrss->newMddEdge();
        DCASSERT(f);
        f->E = p->E;

        DCASSERT(mtd->Length() == 1);

        shared_ddedge* g = mrss->newMddEdge();
        DCASSERT(g);
        MEDDLY::apply( MEDDLY::INTERSECTION, p->E, mtd->getStage(0)->E, g->E );
        if (p->E != g->E) {
          // p must be included in mtd[0]
          DCASSERT(false);
          return;
        }

        {
          shared_ddedge* t = mrss->newMddEdge();
          DCASSERT(t);
          t->E = f->E;
          ans->Append(t);
        }

        shared_ddedge* visited = mrss->newMddEdge();
        DCASSERT(visited);
        visited->E = p->E;

        shared_ddedge* emptyset = mrss->newMddConst(false);
        DCASSERT(emptyset);

        do {
          if (revTime) {
            mxd_wrap->preImage(f, edges, f);
          } else {
            mxd_wrap->postImage(f, edges, f);
          }

          // t = f /\ visited
          MEDDLY::apply( MEDDLY::INTERSECTION, f->E, visited->E, g->E );
          if (g->E != emptyset->E) {
            MEDDLY::apply( MEDDLY::SELECT, g->E, g->E );
            break;
          }

          // f := f /\ mtd[0]
          MEDDLY::apply( MEDDLY::INTERSECTION, f->E, mtd->getStage(0)->E, f->E );
          // f := select(f)
          MEDDLY::apply( MEDDLY::SELECT, f->E, f->E );

          shared_ddedge* t = mrss->newMddEdge();
          DCASSERT(t);
          t->E = f->E;
          ans->Append(t);

          // visited = visited \/ f
          MEDDLY::apply( MEDDLY::UNION, visited->E, f->E, visited->E);
        } while (true);

        {
          shared_ddedge* t = mrss->newMddEdge();
          DCASSERT(t);
          t->E = g->E;
          ans->Append(t);
        }

        //
        // Cleanup
        //
        Delete(f);
        Delete(g);
        Delete(visited);
        Delete(emptyset);
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

