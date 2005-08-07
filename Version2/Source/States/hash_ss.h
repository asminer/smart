
// $Id$

#ifndef HASHSS_H
#define HASHSS_H

#include "flatss.h"
#include "../Templates/hash.h"
#include "../Templates/list.h"

/** @name hash_ss.h
    @type File
    @args \
 
    Hash table for states during state space generation.

*/

// node manager for hash table

struct hash_state_nodes {
  state_array* states;
  DataList <int> *next;
public:
  hash_state_nodes(state_array* s);
  ~hash_state_nodes();
  inline void AddState(int handle) {
    if (handle >= next->Length()) next->AppendBlank();
    DCASSERT(next->alloc > handle);
  }
  // Required for hash tables
  inline int getNext(int h) const { 
    CHECK_RANGE(0, h, next->last);
    return next->data[h]; 
  }
  inline void setNext(int h, int nxt) { 
    CHECK_RANGE(0, h, next->last);
    next->data[h] = nxt; 
  } 
  inline int hash(int h, int M) const { return states->Hash(h, M); }
  inline bool equals(int h1, int h2) const { 
    return (0==states->Compare(h1, h2));
  }
  inline bool isStale(int h) const { return false; }
  inline void show(OutputStream &s, int h) const { s.Put(h); }
};

class hash_states {
protected:
  /// State data
  state_array* states; 
  hash_state_nodes nodes;
  HashTable <hash_state_nodes> *Table;
public:
  hash_states(state_array *s);
  ~hash_states();  

  inline void Clear() { int dumb; Table->ConvertToList(dumb); }

  /// Put states (indices) into the array, in order.
  void FillOrderList(int* order);

  /** Insert this state into the tree.
      If a duplicate is already present, do not insert the state.
      @param 	s	The state to insert.

      @return	The handle (index) of the state.
  */
  int AddState(const state &s);

  /** Find this state in the tree.
      @param	s 	The state to find.
      @return	The handle (index) of the state, if found;
		otherwise, -1.
  */
  int FindState(const state &s);
  void Report(OutputStream &r);
};

#endif

