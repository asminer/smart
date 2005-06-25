
// $Id$

/*
	Hash table template class.
	
	Passed objects must have a public "next" member; 
	i.e., you must make the template object the hash table nodes.

	Objects must have a method "Hash(int m)"; 
	this is the hashing function for a table of size m (m always prime).

	Objects must have a method "Equals(DATA*)".
*/

#ifndef HASH_H
#define HASH_H

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


template <class DATA>
class HashTable {
protected:
  int size_index;
  int num_entries;
  DATA** table;
public:
  HashTable() {
    size_index = num_entries = 0;
    table = NULL;
    Expand();
  }
  ~HashTable() {
    // Will not delete entries.
    free(table);
  }
  inline int Size() const {
    CHECK_RANGE(0, size_index, num_hash_sizes);
    return hash_sizes[size_index];
  }
  /// Empty the hash table into a list; returns the list.
  DATA* ConvertToList() {
    int i;
    DATA* front = NULL;
    for (i=0; i<Size(); i++) {
      while (table[i]) {
	DATA* foo = table[i]->next;
	table[i]->next = front;
	front = table[i];
	table[i] = foo;
	num_entries--;
      } // while
      if (num_entries <= 0) break;
    } // for i
    return front;
  };
  /// A series of inserts; doesn't check for duplicates or expand.
  void BuildFromList(DATA* front) {
    while (front) {
      DATA* ptr = front;
      front = front->next;
      int h = ptr->Hash(Size());
      ptr->next = table[h];
      table[h] = ptr;
      num_entries++;
    }
  }

  void Expand() {
    if (size_index+1 >= num_hash_sizes) return;
    DATA* ptr = ConvertToList();
    int os = Size();
    size_index++;
    table = (DATA**) realloc(table, sizeof(DATA*) * Size());
    if (Size() && (NULL==table)) OutOfMemoryError("Hash table resize");
    for (int i=os; i<Size(); i++) table[i] = NULL;
    BuildFromList(ptr);
  }

  void Shrink() {
    if (size_index==0) return;
    DATA* ptr = ConvertToList();
    size_index--;
    table = (DATA**) realloc(table, sizeof(DATA*) * Size());
    if (Size() && (NULL==table)) OutOfMemoryError("Hash table resize");
    BuildFromList(ptr);
  }

  /** If table contains key, move it to the front of the list.
      Otherwise, do nothing.
      Returns ptr to the key if found, null otherwise.
  */
  DATA* Find(DATA* key) {
    int h = key->Hash(Size());
    CHECK_RANGE(0, h, Size());
    DATA* parent = NULL;
    DATA* ptr;
    for (ptr = table[h]; ptr; ptr = ptr->next) {
      if (key->Equals(ptr)) break;
      parent = ptr;
    }
    if (ptr) {
      // remove from current spot
      if (parent) parent->next = ptr->next;
      else table[h] = ptr->next;
      // move to front
      ptr->next = table[h];
      table[h] = ptr;
    }
    return ptr;
  }

  /** If table contains key, remove it and return it.
      Otherwise, return NULL.
  */
  DATA* Remove(DATA* key) {
    int h = key->Hash(Size());
    CHECK_RANGE(0, h, Size());
    DATA* parent = NULL;
    DATA* ptr;
    for (ptr = table[h]; ptr; ptr = ptr->next) {
      if (key->Equals(ptr)) break;
      parent = ptr;
    }
    if (ptr) {
      // remove from current spot
      if (parent) parent->next = ptr->next;
      else table[h] = ptr->next;
      num_entries--;
      if (num_entries < hash_sizes[size_index-1]) Shrink();
    }
    return ptr;
  }


  /** If table contains key, move it to the front of the list.
      Otherwise, add key to the front of the list.
      Returns the (new) front of the list.
  */
  DATA* Insert(DATA* key) {
    if (num_entries >= hash_sizes[size_index+2]) Expand();
    int h = key->Hash(Size());
    CHECK_RANGE(0, h, Size());
    DATA* parent = NULL;
    DATA* ptr;
    for (ptr = table[h]; ptr; ptr = ptr->next) {
      if (key->Equals(ptr)) break;
      parent = ptr;
    }
    if (ptr) {
      // remove from current spot
      if (parent) parent->next = ptr->next;
      else table[h] = ptr->next;
      // move to front
      ptr->next = table[h];
      table[h] = ptr;
    } else {
      // add key in front
      key->next = table[h];
      table[h] = key;
      num_entries++;
    }
    return table[h];
  }

};

#endif

