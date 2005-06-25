
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

// Super hack, but keeps memory very tight
struct hash_states_node {
  static state_array* states;
  static hash_states_node* ptr;
  inline int Index() const { return this - ptr; }
  inline int Hash(int m) const {
    DCASSERT(states);
    DCASSERT(ptr);
    return states->Hash(Index(), m);
  }
  inline bool Equals(hash_states_node *key) const {
    DCASSERT(states);
    DCASSERT(ptr);
    DCASSERT(key);
    return (0==states->Compare(Index(), key->Index()));
  }
  hash_states_node* next;
};

bool operator> (const hash_states_node &a, const hash_states_node &b)
{
  return (a.states->Compare(a.Index(), b.Index()) > 0);
}

class hash_states {
protected:
  /// State data
  state_array* states; 
  DataList <hash_states_node> *nodes;
  HashTable <hash_states_node> *Table;
public:
  hash_states(state_array *s);
  ~hash_states();  

  inline void Clear() { Table->ConvertToList(); }

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

