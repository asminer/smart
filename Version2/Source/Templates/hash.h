
// $Id$

/*
	Hash table template class.

	Operations are performed on a "node manager" object, where
	nodes are indexed by handles >= 0.  Several hash tables can
	share a single node manager if desired.
	The node manager must provide the following methods 
	(preferably inlined for speed):

	getNext(int h)
	setNext(int h, int nxt)

	hash(int h, int M);

	equals(int h1, int h2);
	
	for debugging:
	show(int h);
*/

#ifndef HASH_H
#define HASH_H

//#define DEBUG_HASH

#include "list.h"

const int num_hash_sizes = 29;
const int hash_sizes[num_hash_sizes] = {
		 0,
		11,
		23,
		53,
	       113,
	       251,
	       509,
	      1019,
	      2039,
	      4079,
	      8179,
	     16369,
	     32749,
	     65521,
	    131063,
	    262133,
	    524269,
	   1048571,
	   2097143,
	   4194287,
	   8388587,
	  16777183,
	  33554393,
	  67108837,
	 134217689,
	 268435399,
	 536870879,
	1073741789,
	2147483647
};

template <class MANAGER>
class HashTable {
protected:
  int size_index;
  int num_entries;
  int* table;
  MANAGER *nodes;
  // stats
  int maxchain;
public:
  HashTable(MANAGER *n) {
    size_index = num_entries = 0;
    table = NULL;
    nodes = n;
    maxchain = 0;
    Expand();
  }
  ~HashTable() {
    // Do not delete nodes.
    free(table);
  }
  inline int Size() const {
    CHECK_RANGE(0, size_index, num_hash_sizes);
    return hash_sizes[size_index];
  }
  inline int MaxChain() const { return maxchain; }
#ifdef DEBUG_HASH
  void Show() const {
    int i;
    for (i=0; i<Size(); i++) {
      Output << "[" << i << "] : ";
      for (int index = table[i]; index >= 0; index = nodes->getNext(index)) {
	nodes->show(index);
        Output << " ";
      }
      Output << "\n";
      Output.flush();
    }
  }
#endif
  /// Empty the hash table into a list; returns the list.
  int ConvertToList() {
    int i;
    int front = -1;
    for (i=0; i<Size(); i++) {
      while (table[i] >= 0) {
	int foo = nodes->getNext(table[i]);
        nodes->setNext(table[i], front);
	front = table[i];
	table[i] = foo;
	num_entries--;
      } // while
      DCASSERT(num_entries >= 0);
      if (num_entries <= 0) break;
    } // for i
    return front;
  };
  /// A series of inserts; doesn't check for duplicates or expand.
  void BuildFromList(int front) {
    while (front >= 0) {
      int next = nodes->getNext(front);
      int h = nodes->hash(front, Size());
      nodes->setNext(front, table[h]);
      table[h] = front;
      front = next;
      num_entries++;
    }
  }

  void Expand() {
    if (size_index+1 >= num_hash_sizes) return;
#ifdef DEBUG_HASH
    Output << "Enlarging table.  Old table:\n";
    Show();
#endif
    int ptr = ConvertToList();
    int os = Size();
    size_index++;
    table = (int*) realloc(table, sizeof(int) * Size());
    if (Size() && (NULL==table)) OutOfMemoryError("Hash table resize");
    for (int i=os; i<Size(); i++) table[i] = -1;
    BuildFromList(ptr);
#ifdef DEBUG_HASH
    Output << "New table:\n";
    Show();
#endif
  }

  void Shrink() {
    if (size_index==0) return;
    int ptr = ConvertToList();
    size_index--;
    table = (int*) realloc(table, sizeof(int) * Size());
    if (Size() && (NULL==table)) OutOfMemoryError("Hash table resize");
    BuildFromList(ptr);
  }

  /** If table contains key, move it to the front of the list.
      Otherwise, do nothing.
      Returns index of the item if found, -1 otherwise.
  */
  int Find(int key) {
    int h = nodes->hash(key, Size());
    CHECK_RANGE(0, h, Size());
    int parent = -1;
    int ptr;
    for (ptr = table[h]; ptr >= 0; ptr = nodes->getNext(ptr)) {
      if (nodes->equals(key, ptr)) break;
      parent = ptr;
    }
    if (ptr >= 0) {
      // remove from current spot
      if (parent >= 0) nodes->setNext(parent, nodes->getNext(ptr));
      else table[h] = nodes->getNext(ptr);
      // move to front
      nodes->setNext(ptr, table[h]);
      table[h] = ptr;
    }
    return ptr;
  }

  /** If table contains key, remove it and return it.
      Otherwise, return -1.
  */
  int Remove(int key) {
    int h = nodes->hash(key, Size());
    CHECK_RANGE(0, h, Size());
    int parent = -1;
    int ptr;
    for (ptr = table[h]; ptr >= 0; ptr = nodes->getNext(ptr)) {
      if (nodes->equals(key, ptr)) break;
      parent = ptr;
    }
    if (ptr >= 0) {
      // remove from current spot
      if (parent >= 0) nodes->setNext(parent, nodes->getNext(ptr));
      else table[h] = nodes->getNext(ptr);
      num_entries--;
    }
    return ptr;
  }


  /** If table contains key, move it to the front of the list.
      Otherwise, add key to the front of the list.
      Returns the (new) front of the list.
  */
  int Insert(int key) {
    if (num_entries >= hash_sizes[size_index+2]) Expand();
    int h = nodes->hash(key, Size());
    CHECK_RANGE(0, h, Size());
    int parent = -1;
    int ptr;
    int thischain = 0;
    for (ptr = table[h]; ptr >= 0; ptr = nodes->getNext(ptr)) {
      thischain++;
      if (nodes->equals(key, ptr)) break;
      parent = ptr;
    }
    maxchain = MAX(maxchain, thischain);
    if (ptr >= 0) {
      // remove from current spot
      if (parent >= 0) nodes->setNext(parent, nodes->getNext(ptr));
      else table[h] = nodes->getNext(ptr);
      // move to front
      nodes->setNext(ptr, table[h]);
      table[h] = ptr;
    } else {
      // add key in front
      nodes->setNext(key, table[h]);
      table[h] = key;
      num_entries++;
    }
    return table[h];
  }

};

#endif

