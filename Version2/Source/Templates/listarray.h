
// $Id$

/*
    Fancy class for arrays of linked lists.
    Allows "compression" 
    Basically, a slightly more general version of digraph.
*/


#ifndef LISTARRAY_H
#define LISTARRAY_H

#include "../defines.h"
#include "../Base/errors.h"
#include "../Base/streams.h"


//#define DEBUG_LISTS

// ******************************************************************
// *                                                                *
// *                      listarray_base class                      *
// *                                                                *
// ******************************************************************

/** Struct for several linked lists.

    In dynamic mode, we have an array of circularly linked lists.

    In static mode, these are compressed similar to row-pointer column-index
    format for sparse matrices.

    In dynamic mode, arrays may be slightly larger than needed,
    and are enlarged if they become too small.
    Also, since we use a (circular) linked-list of entries for each row,
    the array "next" holds the next pointers (array indices).
    Entries are not necessarily in order.

    In static mode, arrays are resized to their minimum size,
    and the "next" array is not used.

    Conversion between dynamic and static can be done "in place"
    (except for addition of a next array if necessary)
    in O(num_edges) time.  ;^)

    These are the common parts; for particular data, use the class
    "listarray", below.
*/
struct listarray_base {
  /// Is it safe to add items/lists?
  bool isDynamic;

  /// Number of lists
  int num_lists;

  /// Total number of items in all lists
  int num_items;

  /** Array of size num_lists + 1 (or larger).
      The extra pointer is unused in "dynamic" mode.
  */
  int* list_pointer;   

  /** Size of list_pointer array.
  */
  int lists_alloc;

  /** The index of the next entry in the list (dynamic mode).
      A negative value is used for "null".
  */
  int* next;

  /** Size of the next and DATA arrays
  */
  int items_alloc;

public:

  /// Constructor
  listarray_base();

  /// Destructor
  ~listarray_base();

  void ResizeLists(int new_nodes);

  inline bool IsStatic() const { return !isDynamic; }
  inline bool IsDynamic() const { return isDynamic; }

  inline int NumLists() const { return num_lists; }
  inline int NumItems() const { return num_items; }

  inline int NewList() {
    DCASSERT(IsDynamic());
    if (num_lists >= lists_alloc) 
      ResizeLists(lists_alloc ? (lists_alloc+1024) : 1023);
    list_pointer[num_lists] = -1; // null
    return num_lists++;
  }

protected:
  /// Append to circular list i
  inline void AppendToCircularList(int i, int ptr) {
    if (list_pointer[i] < 0) {
      // empty list
      next[ptr] = ptr;
    } else {
      // not empty, add to tail and adjust 
      int tail = list_pointer[i];
      next[ptr] = next[tail];  // ptr->next = front of list
      next[tail] = ptr;        // add this node to end
    }
    list_pointer[i] = ptr;	// this is the new last element.
  }

  /** Converts the circular linked lists to null-terminated ones.
      Used right before conversion to static.
  */
  inline void CircularToTerminated() {
    int s;
    for (s=0; s<num_lists; s++) {
      if (list_pointer[s]<0) continue;
      int tail = list_pointer[s];    
      list_pointer[s] = next[tail];  
      next[tail] = -1;
    }
  }

  void ConvertToDynamic();

};


// ******************************************************************
// *                                                                *
// *                        listarray  class                        *
// *                                                                *
// ******************************************************************

/** listarray struct.

    Derived from listarray_base class to save some common implementation.

*/
template <class DATA>
struct listarray : public listarray_base {
  /// Data per "linked list" node
  DATA* value;

public:
  /// Constructor
  listarray() : listarray_base() {
    value = NULL;
  }

  /// Destructor
  ~listarray() {
    free(value);
    value = NULL;
  }

  /// For allocating items
  void ResizeItems(int new_items) {
    DCASSERT(IsDynamic());
    if (items_alloc != new_items) {
      int* foo = (int *) realloc(next, new_items*sizeof(int));
      if (new_items && (NULL==foo)) OutOfMemoryError("Lists resize");
      next = foo;
      DATA* bar = (DATA *) realloc(value, new_items*sizeof(DATA));
      if (new_items && (NULL==bar)) OutOfMemoryError("Lists resize");
      value = bar;
      items_alloc = new_items;
    }
  }

  /** Add an item to the specified list.
  */
  inline void AddItem(int listno, const DATA &v) {
    // Sanity checks
    DCASSERT(IsDynamic());
    // enlarge if necessary
    if (num_items >= items_alloc) 
	ResizeItems(items_alloc+1024);
    DCASSERT(items_alloc > num_items);
    // fix a new edge and add it to the appropriate list
    value[num_items] = v;
    AppendToCircularList(listno, num_items);
    num_items++;
  }

  int AddItemInOrder(int listno, const DATA &v) {
    // Sanity checks
    DCASSERT(IsDynamic());
    CHECK_RANGE(0, listno, num_lists);
    // enlarge if necessary
    if (num_items >= items_alloc) 
	ResizeItems(items_alloc+1024);
    DCASSERT(items_alloc > num_items);
    // fix a new edge
    value[num_items] = v;
    // add it to the list
    int spot = InsertInOrderedCircularList(listno, num_items);
    if (spot==num_items) num_items++;
    else value[spot] += v;
    return spot;
  }

  void Defragment(int first_slot);

  inline void ConvertToStatic() {
    if (IsStatic()) return;
    CircularToTerminated();
    Defragment(0);
    // resize arrays to be "tight"
    ResizeLists(num_lists);
    ResizeItems(num_items);
    // Trash next array
    free(next);
    next = NULL;
    isDynamic = false;
  }

  /// Dump to a stream (human readable)
  void ShowNodeList(OutputStream &s, int node);

protected:
  /// Insert in ordered circular list i
  inline int InsertInOrderedCircularList(int i, int ptr) {
    if (list_pointer[i] < 0) {
      // empty list
      return next[ptr] = list_pointer[i] = ptr;
    } 
    // not empty
    int prev = list_pointer[i];
    if (value[ptr] > value[prev]) {
      // New item goes at the end
      next[ptr] = next[prev];
      return next[prev] = list_pointer[i] = ptr;
    }
    // Find the spot for this edge
    while (1) {
      int curr = next[prev];
      if (value[curr] > value[ptr]) {
	// goes here
  	next[ptr] = curr;
	next[prev] = ptr;
	return ptr;
      }
      if (value[curr] == value[ptr]) {
	// duplicate edge, bail
	return curr;
      }
      prev = curr;
    } // while 1
  }
};



// ******************************************************************
// *                                                                *
// *                     labeled_digraph methods                    *
// *                                                                *
// ******************************************************************


template <class DATA>
void listarray <DATA> :: Defragment(int first_slot)
/* Exactly the same as the unlabeled case, except we
   also must swap values.
*/
{
  DCASSERT(IsDynamic());
  // make lists contiguous by swapping, forwarding pointers
  int i = first_slot; // everything before i is contiguous, after i is linked
  int s;
  for (s=0; s<num_lists; s++) {
    int list = list_pointer[s];
    list_pointer[s] = i;
    while (list >= 0) {
      // traverse forwarding arcs if necessary...
      while (list<i) list = next[list];
      int nextlist = next[list];
      if (i!=list) {
        // swap i and list, set up forward
        next[list] = next[i];
        SWAP(value[i], value[list]);
        next[i] = list;  // forwarding info
      }
      list = nextlist;
      i++;
    } // while list
  } // for s
  if (list_pointer) list_pointer[num_lists] = i;
}

template <class DATA>
void listarray <DATA> :: ShowNodeList(OutputStream &s, int listno)
{
  int e;
  if (IsStatic()) {
    s << "\tList " << listno << "\n";
    for (e=list_pointer[listno]; e<list_pointer[listno+1]; e++) {
      s << "\t\t" << value[e] << "\n";
    }
  } else {
    s << "\tList " << listno << "\n";
    if (list_pointer[listno]<0) return;
    int front = next[list_pointer[listno]];
    e = front;
    do {
      s << "\t\t" << value[e] << "\n";
      e = next[e];
    } while (e!=front);
  }
}



#endif

