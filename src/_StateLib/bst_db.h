
#ifndef BST_DB_H
#define BST_DB_H

#include <stdio.h>

#include "coll.h"
#include "stack.h"

// #define DEBUG_PERFORMANCE

// ======================================================================
// |                                                                    |
// |              Binary search trees, abstract base class              |
// |                                                                    |
// ======================================================================

class bst_db : public StateLib::state_db {
#ifdef DEBUG_PERFORMANCE
  long max_search;
  long total_search;
  long num_search;
#endif
protected:
  main_coll* states;  // try: change this to not a pointer
protected: // dynamic mode
  long* left;
  long* right;
  long nodes_alloc;
  long root;
protected: // static mode
  long* order;
private:
  static const int MAX_TREE_ADD = 2048;
  static const long path_minsize = 256;
  static const long default_path_maxsize = 1000000;
  long path_maxsize;
  Stack <long> *path;
protected:
  inline void ClearPath() { path->Clear(); }
  inline long Pop() { return (path->Empty()) ? -1 : path->Pop(); }
  inline void Push(long x) { path->Push(x); }

  void allocPath();

public:
  bst_db(bool indexed, bool storesize);
  virtual ~bst_db();

  virtual void SetMaximumStackSize(long max_stack);
  virtual long GetMaximumStackSize() const;

  virtual void Clear();
  virtual void ConvertToStatic(bool tighten);
  virtual const StateLib::state_coll* GetStateCollection() const;
  virtual StateLib::state_coll* TakeStateCollection();
  virtual long ReportMemTotal() const;

  virtual long  GetStateKnown(long index, int* state, int size) const;
  virtual int GetStateUnknown(long index, int* state, int size) const;
  virtual const unsigned char* GetRawState(long hndl, long &bytes) const;

  // for debugging, dump the binary search tree to a dot file
  virtual void DumpDot(FILE* out);
  // virtual void DumpState(FILE* s, long index) const;
protected:
  virtual void DotAttributes(FILE* out, long node) const;
  void enlargeTree();
  inline long staticBinarySearch(long key) const {
    // ASSUMES static!
    long low = 0;
    long high = num_states;
    while (low < high) {
      long mid = (high+low)/2;
      int cmp = states->CompareHH(order[mid], key);
      if (0==cmp) {
        return order[mid];
      }
      if (cmp>0)  high = mid;
      else        low = mid+1; 
    }
    return -1;
  }
  inline long staticBinarySearch(const int* state, int size) const {
    // ASSUMES static!
    long low = 0;
    long high = num_states;
    while (low < high) {
      long mid = (high+low)/2;
      CHECK_RANGE(0, order[mid], nodes_alloc);
      int cmp = states->CompareHF(index2handle[order[mid]], size, state);
      if (0==cmp) {
        return order[mid];
      }
      if (cmp>0)  high = mid;
      else        low = mid+1; 
    }
    return -1;
  }

#ifdef DEBUG_PERFORMANCE
  inline void recordSearch(long searchdepth) {
    num_search++;
    total_search += searchdepth;
    if (searchdepth > max_search) max_search = searchdepth;
  }
#endif

  inline int FindPath(long key) {
#ifdef DEBUG_PERFORMANCE
    long sd = 0;
#endif
    int cmp = 1;
    ClearPath();
    for (long child = root; child >= 0; ) {
#ifdef DEBUG_PERFORMANCE
      sd++;
#endif
      Push(child);
      cmp = states->CompareHH(child, key);
      if (0==cmp) {
#ifdef DEBUG_PERFORMANCE
        recordSearch(sd);
#endif
        return 0;
      }
      if (cmp>0)  child = left[child];
      else        child = right[child];
    } // for
#ifdef DEBUG_PERFORMANCE
    recordSearch(sd);
#endif
    return cmp;
  };

  inline int FindPath(const int* state, int size) {
#ifdef DEBUG_PERFORMANCE
    long sd = 0;
#endif
    int cmp = 1;
    ClearPath();
    for (long child = root; child >= 0; ) {
#ifdef DEBUG_PERFORMANCE
      sd++;
#endif
      Push(child);
      CHECK_RANGE(0, child, nodes_alloc);
      cmp = states->CompareHF(index2handle[child], size, state);
      if (0==cmp) {
#ifdef DEBUG_PERFORMANCE
        recordSearch(sd);
#endif
        return 0;
      }
      if (cmp>0)  child = left[child];
      else        child = right[child];
    } // for
#ifdef DEBUG_PERFORMANCE
    recordSearch(sd);
#endif
    return cmp;
  };
};

#endif
