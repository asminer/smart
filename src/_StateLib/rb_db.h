
// $Id$

#ifndef RB_DB_H
#define RB_DB_H

#include "bst_db.h"

#define USE_COMPACTED_BITS

// ======================================================================
// |                                                                    |
// |                Red-black trees, abstract base class                |
// |                                                                    |
// ======================================================================

class redblack_db : public bst_db {
protected: // dynamic mode
#ifdef USE_COMPACTED_BITS
  char* red;  // bitvector :-)
#else
  bool* red;
#endif
public:
  redblack_db(bool indexed, bool storesize);
  virtual ~redblack_db();

  virtual long ReportMemTotal() const;

  virtual void ConvertToDynamic(bool) {
    throw StateLib::error(StateLib::error::NotImplemented);
  }

protected: // helpers
#ifdef USE_COMPACTED_BITS
  inline bool is_red(long n) const {
    char mask = 1 << (n%8); 
    return red[n/8] & mask;
  };
  inline void set_red(long n) {
    char mask = 1 << (n%8);
    red[n/8] |= mask;
  };
  inline void set_black(long n) {
    char mask = ~ (1 << (n%8));
    red[n/8] &= mask;
  };
#else
  inline bool is_red(long n) const {
    DCASSERT(n>=0);
    DCASSERT(n<nodes_aloc);
    return red[n];
  }
  inline void set_red(long n) {
    DCASSERT(n>=0);
    DCASSERT(n<nodes_aloc);
    red[n] = true;
  }
  inline void set_black(long n) {
    DCASSERT(n>=0);
    DCASSERT(n<nodes_aloc);
    red[n] = false;
  }
#endif
  inline bool is4node(long p) const {
    if (left[p]<0 || right[p]<0) return false;  // not enough children
    return is_red(left[p]) && is_red(right[p]);
  }

  virtual void DotAttributes(FILE* out, long node) const;
  void finishInsert(int cmp);

};

// ======================================================================
// |                                                                    |
// |               Red-black trees, collection is indexed               |
// |                                                                    |
// ======================================================================

class redblack_index_db : public redblack_db {
public:
  redblack_index_db(bool storesize);
  virtual ~redblack_index_db();

  virtual long InsertState(const int* state, int size);
  virtual long FindState(const int* state, int size);
};

// ======================================================================
// |                                                                    |
// |             Red-black trees, collection is not indexed             |
// |                                                                    |
// ======================================================================

class redblack_handle_db : public redblack_db {
public:
  redblack_handle_db(bool storesize);
  virtual ~redblack_handle_db();

  virtual long InsertState(const int* state, int size);
  virtual long FindState(const int* state, int size);
};

#endif

