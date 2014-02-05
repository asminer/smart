
// $Id$

#ifndef HASH_H
#define HASH_H

#include "defines.h"
#include <stdlib.h> // for realloc

class voidhash {
protected:
  static const long SIZES[];
  void** table;
  int size_index;
  long elements;
  // stats
  long maxchain;
  int max_size_index;
  long maxelems;
  long num_resizes;
public:
  voidhash() {
    size_index = 0;
    max_size_index = 0;
    elements = 0;
    maxelems = 0;
    maxchain = 0;
    num_resizes = 0;
    table = (void**) malloc(getSize() * sizeof(void*));
    for (long i=getSize()-1; i>=0; i--) table[i] = 0;
  }
  ~voidhash() {
    free(table);
  }
  inline long getSize() const { return SIZES[size_index]; }
  inline long getMaxSize() const { return SIZES[max_size_index]; }
  inline long getNumElements() const { return elements; }
  inline long getMaxElements() const { return maxelems; }
  inline long getMaxChain() const { return maxchain; }
  inline long getNumResizes() const { return num_resizes; }
protected:
  inline long nextSize() const { return SIZES[size_index+1]; }
  inline long prevSize() const { return size_index ? SIZES[size_index-1] : 0; }
  inline void Enlarge() {
    void** newt = (void**) realloc(table, nextSize() * sizeof(void*));
    if (0==newt) return;
    table = newt;
    for (long i=getSize(); i<nextSize(); i++) table[i] = 0;
    size_index++;
    if (size_index > max_size_index) max_size_index = size_index;
    num_resizes++;
  };
  inline void Shrink() {
    void** newt = (void**) realloc(table, prevSize() * sizeof(void*));
    if (0==newt) return;
    table = newt;
    size_index--;
    num_resizes++;
  }
};

const long voidhash::SIZES[] = {
    11, 23, 47, 97, 197, 397, 797, 1597, 3203, 6421, 12853, 25717, 51437,
    102877, 205759, 411527, 823117, 1646237, 3292489, 6584983, 13169977,
    26339969, 52679969, 105359939, 210719881, 421439783, 842879579, 1685759167,
    -1
};

/** Hash table template class.

    To use this template, you must define the following member functions
    for the class to store objects in the hash table.

    \begin{description}
 
    \item[long Signature(long M) const]  
    The hash function.  M is the size of the hash table.  Should return 
    something between 0 and M-1, where M is guaranteed to be prime.

    \item[bool Equals(DATA\_TYPE *) const]  
    So we can catch equal objects in the hash table.  Note that equal objects
    (according to Equals) MUST have the same signature regardless of M.  

    \item[void SetNext(DATA\_TYPE *)]
    The hash table uses chaining, and the objects must store the next pointer.
    This method sets the next pointer.

    \item[DATA\_TYPE* GetNext() const]
    Get the next pointer.

    \end{description}

*/
template <class DATA_TYPE>
class HashTable : public voidhash {
public:
  HashTable() : voidhash() { }

  /// Find an object equal to the one specified; if none present, return 0.
  DATA_TYPE* Find(const DATA_TYPE* key);

  /** Insert an object into the table.
      If a duplicate object is already present, the original is not inserted
      and the duplicate is returned.  Otherwise, the original is inserted
      and is returned.
  */
  DATA_TYPE* UniqueInsert(DATA_TYPE*);

  /// Removes the object specified, if present.
  void Remove(DATA_TYPE*);
  
protected:
  inline bool MoveToFront(long slot, const DATA_TYPE* key) {
    DATA_TYPE* list = (DATA_TYPE*) table[slot];
    if (0==list) return false;
    long c = 0;
    DATA_TYPE* prev = 0;
    DATA_TYPE* curr = list;
    for (; curr; curr=curr->GetNext()) {
      c++;
      if (key->Equals(curr)) break;
      prev = curr;
    }
    maxchain = MAX(maxchain, c);
    if (0==curr) return false;
    if (prev) {
      prev->SetNext(curr->GetNext());
      curr->SetNext(list);
      table[slot] = curr;
    }
    return true;
  };

  DATA_TYPE* ConvertToList();
  void BuildFromList(DATA_TYPE*);
};


template <class DT>
DT* HashTable <DT> :: Find(const DT* key)
{
  long slot = key->Signature(getSize());
  if (MoveToFront(slot, key)) {
    return (DT*) table[slot];
  } 
  return 0;
}

template <class DT>
DT* HashTable <DT> :: UniqueInsert(DT* key)
{
  long slot = key->Signature(getSize());
  if (MoveToFront(slot, key)) {
    return (DT*) table[slot];
  }
  key->SetNext((DT*)table[slot]);
  table[slot] = key;
  elements++;
  maxelems = MAX(maxelems, elements);
  if (nextSize()>0 && elements > nextSize()) {
    DT* list = ConvertToList();
    Enlarge();
    BuildFromList(list);
  }
  return key;
}

template <class DT>
void HashTable <DT> :: Remove(DT* key)
{
  long slot = key->Signature(getSize());
  if (!MoveToFront(slot, key)) return;
  if (table[slot] != key) return;
  table[slot] = key->GetNext();
  key->SetNext(0);
  elements--;
  if (elements*2 < prevSize()) {
    DT* list = ConvertToList();
    Shrink();
    BuildFromList(list);
  }
}

template <class DT>
DT* HashTable <DT> :: ConvertToList()
{
  DT* biglist = 0;
  long i = getSize();
  for (i--; i>=0; i--) {
    DT* list = (DT*) table[i];
    table[i] = 0;
    while (list) {
      DT* next = list->GetNext();
      list->SetNext(biglist);
      biglist = list;
      list = next;
    }
  }
  return biglist;
}

template <class DT>
void HashTable <DT> :: BuildFromList(DT* list)
{
  while (list) {
    long slot = list->Signature(getSize());
    DT* next = list->GetNext();
    list->SetNext((DT*)table[slot]);
    table[slot] = list;
    list = next;
  }
}

#endif
