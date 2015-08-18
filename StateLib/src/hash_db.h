
// $Id$

#ifndef HASH_DB_H
#define HASH_DB_H

#include <stdio.h>
#include "coll.h"

// #define DEBUG_PERFORMANCE

// ======================================================================
// |                                                                    |
// |                  Hash tables, abstract base class                  |
// |                                                                    |
// ======================================================================

class hash_db : public StateLib::state_db {
protected:
#ifdef DEBUG_PERFORMANCE
  long maxchain;
  long totalchain;
  long numchain;
#endif
  static const int MAX_NODE_ADD = 2048;
  main_coll* states;
  long* table;
  long* next;
  long next_alloc;
  int hash_bits;
public:
  hash_db(bool indexed, bool storesize);
  virtual ~hash_db();

  virtual void SetMaximumStackSize(long max_stack);
  virtual long GetMaximumStackSize() const;

  virtual void Clear();
  virtual void ConvertToStatic(bool);
  virtual void ConvertToDynamic(bool);
  virtual const StateLib::state_coll* GetStateCollection() const;
  virtual StateLib::state_coll* TakeStateCollection();
  virtual long ReportMemTotal() const;

  virtual long  GetStateKnown(long index, int* state, int size) const;
  virtual int GetStateUnknown(long index, int* state, int size) const;
  virtual const unsigned char* GetRawState(long hndl, long &bytes) const;

  virtual void DumpDot(FILE*);
protected:
  inline long size() const {
    return ((long)1) << hash_bits;
  }
  inline bool needToExpand() const {
    return num_states > 2*size();
  }
};

// ======================================================================
// |                                                                    |
// |                 Hash tables, collection is indexed                 |
// |                                                                    |
// ======================================================================

class hash_index_db : public hash_db {
public:
  hash_index_db(bool storesize);
  virtual ~hash_index_db();

  virtual long InsertState(const int* state, int size);
  virtual long FindState(const int* state, int size);

protected:
  bool move_to_front(long &chain, long key);
};

#endif

