
// $Id$

#ifndef SPLAY_H
#define SPLAY_H

#include "defines.h"


// ==================================================================
// ||                                                              ||
// ||                                                              ||
// ||                    SplayOfPointers  class                    ||
// ||                                                              ||
// ||                                                              ||
// ==================================================================

/**  Splay tree template class.
     The tree stores pointers to objects.
     Also, for cleverness, we can convert back and forth to a doubly-linked
     list when there are few enough items.

     To use, there must be a method so that 
        a->Compare(DATA *b) works similar to strcmp(a, b)

*/

template <class DATA>
class SplayOfPointers {
protected:
  /// Stack, used for tree traversals.
  long* stack;
  /// Top of stack.
  long stack_top;
  /// Size of stack (max depth).
  long stack_size; 
  /// Items stored in the tree/list.
  DATA** item;
  /// Left pointers.
  long* left;
  /// Right pointers.
  long* right;
  /// Dimension of item, left, right arrays.
  long max_elements;
  /// Last used element of item, left, right arrays.
  long last_element;
  /// Number of elements in tree/list.
  long num_elements;
  /// Pointer to free space
  long free_list;
  /// Pointer to root node
  long root;
  /// When to change from linked-list to tree, when increasing.
  int list2tree;
  /// When to change from tree to linked-list, when decreasing.
  int tree2list;
  /// Currently, are we a list?  Otherwise we are a tree.
  bool is_list;
public:
  /** Constructor.
      Create a new, empty list/tree.
        @param  l2t   Size at which we change from a list to a tree,
                      when adding elements.
                      If less than 1, we will start with a tree.

        @param  t2l   Size at which we change from a tree to a list,
                      when removing elements.
                      If less than 1, we will never change back.
  */
  SplayOfPointers(int l2t, int t2l);

  /// Destructor.
  ~SplayOfPointers();

  /// Delete all entries, and clear the tree.
  void DeleteAndClear();

  inline long NumElements() const { return num_elements; }

  inline DATA* GetItem(long n) const {
    CHECK_RANGE(0, n, last_element);
    return item[n];
  };

  /** Splay.
      Find the closest element to key in the tree/list, and make it the root.
      Does so in a manner consistent with the underlying data structure
      (i.e., tree or list).
        @param  key  Item to search for
        @return The value of root->Compare(key).
  */
  template <class DATA2>
  int Splay(const DATA2* key);

  /** Find element.
        @param  key  Item to search for.
        @return NULL, if not found;
                an item equal to key according to Compare(), otherwise.
  */
  template <class DATA2>
  inline DATA* Find(const DATA2* key) {
    int cmp = Splay(key);
    if (0==cmp)   return item[root];
    else          return 0;
  }

  /** Find the "index" of an element.
        @param  key   Item to search for.
        @return -1,   if not found;
                the index (0..size-1) of the key, otherwise.
  */
  template <class DATA2>
  inline long FindIndex(const DATA2* key) {
    int cmp = Splay(key);
    if (0==cmp)   return root;
    else          return -1;
  }

  /** Add element.
        @param  key  Item to add.
        @return key,  if it was successfully added to the tree/list.
                item, with item->Compare(key)==0 if one was already present.
                NULL, if there is no more memory to add an item.
  */
  DATA* Insert(DATA* key);

  /** Find and remove element.
        @param  key  Target to find.
        @return item, with item->Compare(key)==0, where item
                      has just been removed from the tree/list.
                0,    if no such item is in the tree/list.
  */
  DATA* Remove(DATA* key);

  /** Copy the elements, in order, into an array.
        @param  a  An array, of dimension NumElements() or larger.
  */
  void CopyToArray(DATA** a);

#ifdef DEBUG
  /** Print the tree/list (for debugging)
      There must be a method:
        Show(OutputStream& s)
      in class DATA for this to work!
  */
  void Show(OutputStream& s) {
    s << "free list: " << free_list << "\n";
    if (is_list)    s << "stored as a list\n"; 
    else            s << "stored as a tree\n";
    s << "root: " << root << "\n";
    s << "items:\n";
    for (long i=0; i<last_element; i++) {
      s << "\t" << i << ": ";
      if (item[i])  item[i]->Show(s);
      else          s << "null";
      s << "\n";
    }
    s << "left: ";
    s.PutArray(left, last_element);
    s << "\nright: ";
    s.PutArray(right, last_element);
    s << "\n";
  }
#endif

protected:
  void Expand();

  void ConvertToTree();
  void ConvertToList();

  inline void Push(long x) {
    if (stack_top >= stack_size) {
      stack_size += 256;
      stack = (long*) realloc(stack, stack_size * sizeof(long));
    }
    stack[stack_top++] = x;
  }
  inline long Pop() {
    return (stack_top) ? (stack[--stack_top]) : -1;
  }
  inline void StackClear() {
    stack_top = 0;
  }
  inline long NewNode() {
    long ans;
    if (free_list >= 0) {
      ans = free_list;
      free_list = right[free_list];
    } else {
      if (last_element >= max_elements)  Expand();
      if (last_element >= max_elements)  return -1;  // no memory
      ans = last_element;
      last_element++;
    }
    return ans;
  }
  inline void RecycleNode(long x) {
    if (x+1 == last_element) {
      last_element--;
      while (free_list+1 == last_element) {
        if (free_list<0) return;
        free_list = right[free_list];
        last_element--;
      }
    } else {
      item[x] = 0;
      right[x] = free_list;
      left[x] = free_list;
      free_list = x;
    }
  }
  /// swap parent and child, preserve BST property
  inline void TreeRotate(long C, long P, long GP) {
    if (left[P] == C) {
      left[P] = right[C];
      right[C] = P;
    } else {
      right[P] = left[C];
      left[C] = P;
    }
    if (GP >= 0) {
      if (left[GP] == P)
        left[GP] = C;
      else 
        right[GP] = C;
    }
  }
};

// ==================================================================
// ||                                                              ||
// ||                   SplayOfPointers  methods                   ||
// ||                                                              ||
// ==================================================================

template <class DATA>
SplayOfPointers<DATA>::SplayOfPointers(int l2t, int t2l)
{
  list2tree = l2t;
  tree2list = t2l;
  stack = 0; stack_top = stack_size = 0;
  item = 0;
  left = right = 0;
  max_elements = num_elements = last_element = 0;
  free_list = root = -1;
  is_list = (list2tree > 0);
}

template <class DATA>
SplayOfPointers<DATA>::~SplayOfPointers()
{
  free(stack);
  free(item);
  free(left);
  free(right);
}

template <class DATA>
void SplayOfPointers<DATA>::DeleteAndClear()
{
  for (long i=0; i<last_element; i++) {
    delete item[i];
    item[i] = 0;
  }
  last_element = num_elements = 0;
  free_list = root = -1;
  is_list = (list2tree > 0);
}

template <class DATA>
template <class DATA2>
int SplayOfPointers<DATA>::Splay(const DATA2* key)
{
  if (root < 0) return -1;
  int cmp;
  if (is_list) {
    // List splay
    cmp = item[root]->Compare(key);
    if (0==cmp) return 0;  // already at root.
    if (cmp > 0) {
      // traverse to the left
      while (left[root] >= 0) {
        root = left[root];
        cmp = item[root]->Compare(key);
        if (cmp <= 0) return cmp;
      } // while
      return cmp;
    }
    // traverse to the right
    while (right[root] >= 0) {
      root = right[root];
      cmp = item[root]->Compare(key);
      if (cmp >= 0) return cmp;
    } // while
    return cmp;
  }
  // Tree splay
  long child = root;
  StackClear();
  while (child >= 0) {
    Push(child);
    cmp = item[child]->Compare(key);
    if (0==cmp)   break;
    if (cmp > 0)  child = left[child];
    else          child = right[child];
  } // while child
  child = Pop();
  long parent = Pop();
  long grandp = Pop();
  long greatgp = Pop();
  while (parent >= 0) {
    // splay step
    if (grandp < 0) {
      TreeRotate(child, parent, grandp);
      break;
    }
    if ( (right[grandp] == parent) == (right[parent] == child) ) {
      // parent and child are either both right children, or both left children
      TreeRotate(parent, grandp, greatgp);
      TreeRotate(child, parent, greatgp);
    } else {
      TreeRotate(child, parent, grandp);
      TreeRotate(child, grandp, greatgp);
    }
    // continue
    parent = greatgp;
    grandp = Pop();
    greatgp = Pop();
  } // while parent
  root = child;
  return cmp;
}

template <class DATA>
DATA* SplayOfPointers<DATA>::Insert(DATA* key)
{
  if (root < 0) {
    // same behavior, regardless of tree or list
    root = NewNode();
    if (root < 0)   
        return 0;
    item[root] = key;
    left[root] = -1;
    right[root] = -1;
    num_elements++;
    return key;
  }
  int cmp = Splay(key);
  if (0==cmp)  return item[root];

  // need to add the element
  if (is_list && (num_elements > list2tree))
  ConvertToTree();

  long newroot = NewNode();
  if (newroot < 0)   return 0;  // out of memory?
  item[newroot] = key;
  num_elements++;

  // connect new root to old one.
  if (is_list) {
    if (cmp > 0) {
      long l = left[root];
      left[newroot] = l;
      if (l>=0) right[l] = newroot;     
      right[newroot] = root;
      left[root] = newroot;
    } else {
      long r = right[root];
      right[newroot] = r;
      if (r>=0) left[r] = newroot;
      left[newroot] = root;
      right[root] = newroot;
    }
  } else {
    if (cmp > 0) {
      right[newroot] = root;
      left[newroot] = left[root];
      left[root] = -1;
    } else {
      left[newroot] = root;
      right[newroot] = right[root];
      right[root] = -1;
    }
  }
  root = newroot;
  return key;
}

template <class DATA>
DATA* SplayOfPointers<DATA>::Remove(DATA* key)
{
  if (root < 0) return 0;
  int cmp = Splay(key);
  if (cmp)   return 0;

  // need to remove the element
  if (!is_list && (num_elements < tree2list))
  ConvertToList();

  long oldroot = root;
  long oldleft = left[root];
  long oldright = right[root];
  if (is_list) {
    root = -1;
    if (oldleft>=0) {
      right[oldleft] = oldright;
      root = oldleft;
    }
    if (oldright>=0) {
      left[oldright] = oldleft;
      root = oldright;
    }
  } else {
    if (oldleft>=0) {
      root = oldleft;
      Splay(item[oldroot]);
      right[root] = oldright;
    } else {
      root = oldright;
    }
  }

  RecycleNode(oldroot);
  num_elements--;
  DATA* tmp = item[oldroot];
  item[oldroot] = 0;
  return tmp;
}

template <class DATA>
void SplayOfPointers<DATA>::CopyToArray(DATA** a)
{
  if (root < 0) return;
  long i;
  long slot = 0;
  if (is_list) {
    for (i=root; left[i] >= 0; i=left[i]) { }
    for (; i>=0; i=right[i]) {
      a[slot] = item[i];
      slot++;
    }
    return;
  } 
  // non-recursive, inorder tree traversal
  StackClear();
  i = root;
  while (i>=0) {
    if (left[i] >= 0) {
      Push(i);
      i =left[i];
      continue;
    }
    while (i>=0) {
      a[slot] = item[i];
      slot++;
      if (right[i] >= 0) {
        i = right[i];
        break;
      }
      i = Pop();
    } // inner while
  } // outer while
}

template <class DATA>
void SplayOfPointers<DATA>::Expand()
{
  long new_elements = 2*max_elements;
  if (new_elements > 1024) new_elements = max_elements + 1024;
  if (new_elements < 4) new_elements = 4;
  DATA** newitem = (DATA**) realloc(item, new_elements * sizeof(DATA*));
  long* newleft = (long*) realloc(left, new_elements * sizeof(long));
  long* newright = (long*) realloc(right, new_elements * sizeof(long));
  if (newitem) item = newitem;
  if (newleft) left = newleft;
  if (newright) right = newright;
  if ((0==newitem) || (0==newleft) || (0==newright)) return;
  max_elements = new_elements;
}

template <class DATA>
void SplayOfPointers<DATA>::ConvertToTree()
{
  long i;
  for (i=left[root]; i>=0; i=left[i]) {
    right[i] = -1;
  }
  for (i=right[root]; i>=0; i=right[i]) {
    left[i] = -1;
  }
  is_list = false;
}

template <class DATA>
void SplayOfPointers<DATA>::ConvertToList()
{
  long front = -1;
  StackClear();
  long n = root;
  // "reverse" inorder traversal using the stack
  while (n>=0) {
    if (right[n] >= 0) {
      Push(n);
      n = right[n];
      continue; 
    }
    while (n>=0) {
      // Visit...
      right[n] = front;
      if (front>=0) left[front] = n;
      front = n;
      // ...end of visit
      if (left[n]>=0) {
        n = left[n];
        break;
      }
      n = Pop();
    } // inner while
  } // outer while
}

#endif

