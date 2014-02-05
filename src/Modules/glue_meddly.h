
// $Id$

/** \file glue_meddly.h

    Thin wrapper and interface glue for our Multi-terminal
    and Edge-valued Decision Diagram LibrarY.
*/

#ifndef GLUE_MEDDLY_H
#define GLUE_MEDDLY_H

#include "../ExprLib/dd_front.h"
#include <gmp.h>
#include "meddly.h"

// ******************************************************************
// *                                                                *
// *                      shared_ddedge  class                      *
// *                                                                *
// ******************************************************************

/** Abstract class (for information hiding only).
    Shared object wrapper around a dd_edge.
*/
class shared_ddedge : public shared_object {
  MEDDLY::enumerator *iter;
public:
  MEDDLY::dd_edge E;  
public:
  shared_ddedge(MEDDLY::forest* p);
  shared_ddedge(const shared_ddedge &e);
protected:
  virtual ~shared_ddedge();
public:
  virtual bool Print(OutputStream &s, int) const;
  virtual bool Equals(const shared_object* x) const;

  inline MEDDLY::forest* getForest() const {
    return E.getForest();
  }

  // handy! for iterating
  void startIterator();
  void startIteratorRow(const int* row_minterm);
  void startIteratorCol(const int* col_minterm);
  void freeIterator();
  inline bool hasIterator() const {
    return iter;
  }
  inline bool isIterDone() const {
    DCASSERT(iter);
    return (false == *iter);
  }
  inline void incIter() {
    DCASSERT(iter);
    ++(*iter);
  }
  inline const int* getIterMinterm() const {
    DCASSERT(iter);
    return iter->getAssignments();
  }
  inline const int* getIterPrimedMinterm() const {
    DCASSERT(iter);
    return iter->getPrimedAssignments();
  }
};

// ******************************************************************
// *                                                                *
// *                      meddly_encoder class                      *
// *                                                                *
// ******************************************************************

/** Abstract class for building decision diagrams using Meddly.
    Still allows for a generic "encoding" of model state variables
    into decision diagram variables.
    Operations specific to Meddly are implemented, though.
    All methods expect that the shared_object DD nodes are
    shared_ddedge objects.
*/
class meddly_encoder : public sv_encoder {
protected:
  /// Forest containing DDs that we manipulate.
  MEDDLY::forest *F;
  /// Name, used for reporting.
  const char* name;
  static bool image_star_uses_saturation;
  friend
  void InitMEDDLy(exprman* em);
public:
  /// Simple constructor.
  meddly_encoder(const char* n, MEDDLY::forest *f);
protected:
  /// Virtual destructor.
  virtual ~meddly_encoder();

public:
  inline static error convert(MEDDLY::error e) {
    if (MEDDLY::error::INSUFFICIENT_MEMORY == e.getCode())
      return Out_Of_Memory;
    else
      return Failed;
  }

public:
  virtual error dumpNode(DisplayStream &s, shared_object* e) const;
  virtual void dumpForest(DisplayStream &s) const;

  virtual shared_object* makeEdge(const shared_object* e);

  virtual bool isValidEdge(const shared_object* e) const;

  virtual error copyEdge(const shared_object* src, shared_object* dest) const;

  virtual error buildSymbolicConst(bool t, shared_object* answer);
  virtual error buildSymbolicConst(long t, shared_object* answer);
  virtual error buildSymbolicConst(double t, shared_object* answer);

  virtual const int* firstMinterm(shared_object* set) const;
  virtual const int* nextMinterm(shared_object* set) const;

  virtual error createMinterms(const int* const* mt, int n, shared_object* ans);
  virtual error createMinterms(const int* const* from, const int* const* to, int n, shared_object* ans);

  virtual error buildUnary(exprman::unary_opcode op, 
                            const shared_object* opnd, shared_object* ans);

  virtual error buildBinary(const shared_object* lt, exprman::binary_opcode op,
                            const shared_object* rt, shared_object* ans);

  virtual error buildAssoc(const shared_object* left, 
                          bool flip, exprman::assoc_opcode op, 
                          const shared_object* right, shared_object* ans);

  virtual error getCardinality(const shared_object* x, long &card);
  virtual error getCardinality(const shared_object* x, double &card);
  virtual error getCardinality(const shared_object* x, result &card);

  virtual error isEmpty(const shared_object* x, bool &empty);

  virtual error preImage(const shared_object* x, 
                          const shared_object* E, 
                          shared_object* ans);

  virtual error postImage(const shared_object* x, 
                          const shared_object* E, 
                          shared_object* ans);

  virtual error preImageStar(const shared_object* x, 
                              const shared_object* E, 
                              shared_object* ans);

  virtual error postImageStar(const shared_object* x, 
                              const shared_object* E, 
                              shared_object* ans);

  virtual error selectRows(const shared_object* E, 
                            const shared_object* rows,
                            shared_object* ans);

  virtual error selectCols(const shared_object* E, 
                            const shared_object* cols,
                            shared_object* ans);

  virtual void reportStats(OutputStream &out);

  // Nice:

  /// Make another version of this encoder, but with a different forest.
  virtual meddly_encoder* 
  copyWithDifferentForest(const char* n, MEDDLY::forest*) const = 0;

  /** Accumulate and destroy list of edges.
      Done by combining adjacent pairs until we reach the "top".
  */
  shared_ddedge* fold(const MEDDLY::binary_opname* op, 
                      shared_ddedge** list, int N, named_msg* debug) const;

  /** Accumulate and destroy list of edges.
      Done by accumulating, in order, until we reach the end.
  */
  shared_ddedge* accumulate(const MEDDLY::binary_opname* op,
                      shared_ddedge** list, int N, named_msg* debug) const;

  // particular to meddly_encoder:

  inline MEDDLY::forest* getForest() { return F; }
};



// **************************************************************************
// *                                                                        *
// *                         Library initialization                         *
// *                                                                        *
// **************************************************************************

void InitMEDDLy(exprman* em);

#endif

