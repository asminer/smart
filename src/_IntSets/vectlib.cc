
#include "vectlib.h"
#include "revision.h"
#include <stdio.h>
#include <stdlib.h>

// #define DEBUG_ADD
// #define DEBUG_REMOVE
// #define DEBUG_SWITCH
// #define DEBUG_GOSTATIC
// #define DEBUG_GODYNAMIC

const int MAJOR_VERSION = 1;
const int MINOR_VERSION = 0;

const int default_list_to_tree = 256;
const int default_tree_to_list = 128;

void ShowArray(long* x, int N)
{
  printf("[%ld", x[0]);
  for (long n=1; n<N; n++)
  printf(", %ld", x[n]);
  printf("]");
}

void ShowArray(double* x, int N)
{
  printf("[%lf", x[0]);
  for (long n=1; n<N; n++)
  printf(", %lf", x[n]);
  printf("]");
}

// ******************************************************************
// *                      list_or_tree  class                       *
// ******************************************************************

/*
  Basic sparse structure, used by all classes below.
*/
class list_or_tree {
// Global stack
  static long* stack;
  static long stack_top;
  static long stack_size; 
public:
  long* left;
  long* right;
  long* index;
public:
  inline long ListSplay(long root, long key) {
    if (root<0)  return root;
    if (key == index[root]) return root;
    if (index[root] > key) {
      // traverse to the left
      while (left[root] >= 0) {
        root = left[root];
        if (index[root] <= key) return root;
      } // while
      return root;
    }
    // traverse to the right
    while (right[root] >= 0) {
      root = right[root];
      if (index[root] >= key) return root;
    } // while
    return root;
  }
  inline void SplayRotate(long C, long P, long GP) {
    // swap parent and child, preserve BST property
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
  inline void SplayStep(long c, long p, long gp, long ggp) {
    if (gp < 0) {
      SplayRotate(c, p, gp);
      return;
    }
    if ( (right[gp] == p) == (right[p] == c) ) {
      // parent and child are either both right children, or both left children
      SplayRotate(p, gp, ggp);
      SplayRotate(c, p, ggp);
    } else {
      SplayRotate(c, p, gp);
      SplayRotate(c, gp, ggp);
    }
  }
  long TreeSplay(long root, long key);
  inline long binary_search(long low, long high, long key) {
    while (low < high) {
      long mid = (low+high) / 2;
      if (index[mid] == key) return mid;
      if (index[mid] > key)   high = mid;
      else                    low = mid+1;
    }
    return -1;
  }
  inline long Push(long x) {
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
  inline void List2Tree(long root) {
    if (root<0) return;
    long i;
    for (i=left[root]; i>=0; i=left[i]) {
      right[i] = -1;
    }
    for (i=right[root]; i>=0; i=right[i]) {
      left[i] = -1;
    }
  }
  inline void Tree2List(long n) {
    long front = -1;
    StackClear();
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
  inline long Array2List(long last) {
    left[0] = -1;
    for (long i=1; i<last; i++) left[i] = i-1;
    last--;
    right[last] = -1;
    for (long i=0; i<last; i++) right[i] = i+1;
    return last/2;
  }
  inline long Array2Tree(long low, long high) {
    if (low >= high) return -1;
    long mid = (low+high)/2;
    left[mid] = Array2Tree(low, mid);
    right[mid] = Array2Tree(mid+1, high);
    return mid;
  }

  inline long ConnectListRoot(long root, long newroot) {
    if (index[root] > index[newroot]) {
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
    return newroot;
  }

  inline long ConnectTreeRoot(long root, long newroot) {
    if (index[root] > index[newroot]) {
      right[newroot] = root;
      left[newroot] = left[root];
      left[root] = -1;
    } else {
      left[newroot] = root;
      right[newroot] = right[root];
      right[root] = -1;
    }
    return newroot;
  }

  inline long RemoveListRoot(long root) {
    long oldleft = left[root];
    long oldright = right[root];
    root = -1;
    if (oldleft>=0) {
      right[oldleft] = oldright;
      root = oldleft;
    }
    if (oldright>=0) {
      left[oldright] = oldleft;
      root = oldright;
    }
    return root;
  }

  inline long RemoveTreeRoot(long root) {
    long oldleft = left[root];
    long oldright = right[root];
    if (oldleft>=0) {
      oldleft = TreeSplay(oldleft, index[root]);
      right[oldleft] = oldright;
      root = oldleft;
    } else {
      root = oldright;
    }
    return root;
  }
};

long* list_or_tree::stack = 0;
long list_or_tree::stack_top = 0;
long list_or_tree::stack_size = 0;

// ******************************************************************
// *                     list_or_tree  methods                      *
// ******************************************************************

long list_or_tree::TreeSplay(long root, long key)
{
  long child = root;
  StackClear();
  while (child >= 0) {
    if (index[child] == key) break;
    Push(child);
    if (index[child] > key)   child = left[child];
    else                      child = right[child];
  } // while child
  if (child < 0)
    child = Pop();
  long parent = Pop();
  long grandp = Pop();
  long greatgp = Pop();
  while (parent >= 0) {
    SplayStep(child, parent, grandp, greatgp);
    parent = greatgp;
    grandp = Pop();
    greatgp = Pop();
  } // while parent
  return child;  // new root
}

// ******************************************************************
// *                  bitvector_traverse  methods                   *
// ******************************************************************

bitvector_traverse::bitvector_traverse()
{
}

bitvector_traverse::~bitvector_traverse()
{
  // nothing to destruct.
}

// ******************************************************************
// *                   sparse_bitvector  methods                    *
// ******************************************************************

sparse_bitvector::sparse_bitvector()
{
  num_elements = 0;
  is_static = 0;
}

sparse_bitvector::~sparse_bitvector()
{
}

// ******************************************************************
// *                                                                *
// *                        sbv_impl  class                         *
// *                                                                * 
// ******************************************************************

class sbv_impl : public sparse_bitvector {
protected:
  list_or_tree nodes;
  /// Dimension of index, left, right arrays.
  long max_elements;
  /// Last used element of index, left, right arrays.
  long last_element;
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
  sbv_impl(int l2t, int t2l);
  virtual ~sbv_impl();

  virtual bool GetElement(long index);
  virtual int SetElement(long index);
  virtual int ClearElement(long index);
  virtual void ClearAll();
  virtual bool ConvertToStatic(bool tighten);
  virtual bool ConvertToDynamic();
  virtual void Traverse(bitvector_traverse *);
  virtual bool ExportTo(SV_Vector* A) const;

protected:

  inline void Show() {
    printf("free list: %ld\n", free_list);
    printf("root: %ld\n", root);
    printf("index: ");
    ShowArray(nodes.index, last_element);
    printf("\n left: ");
    ShowArray(nodes.left, last_element);
    printf("\nright: ");
    ShowArray(nodes.right, last_element);
    printf("\n");
  }

  void Expand();

  inline void RecycleNode(long x) {
    if (x+1 == last_element) {
      last_element--;
      while (free_list+1 == last_element) {
        if (free_list<0) return;
        free_list = nodes.right[free_list];
        last_element--;
      }
    } else {
      nodes.index[x] = -1;
      nodes.right[x] = free_list;
      nodes.left[x] = free_list;
      free_list = x;
    }
  }
  inline long NewNode() { 
    long ans;
    if (free_list >= 0) {
      ans = free_list;
      free_list = nodes.right[free_list];
    } else {
      if (last_element >= max_elements)  Expand();
      if (last_element >= max_elements)  return -2;  // no memory
      ans = last_element;
      last_element++;
    }
    return ans;
  }
  inline void SwapElements(long i, long j) {
    long tmp = nodes.index[i];
    nodes.index[i] = nodes.index[j];
    nodes.index[j] = tmp;
  }

  void List2Tree();
  void Tree2List();
};

// ******************************************************************
// *                       sbv_impl  methods                        *
// ******************************************************************

sbv_impl::sbv_impl(int l2t, int t2l) : sparse_bitvector()
{
  nodes.index = nodes.left = nodes.right = 0;
  max_elements = 0;
  last_element = 0;
  free_list = -1;
  root = -1;
  list2tree = l2t;
  tree2list = t2l;
  is_list = list2tree > 0;
}

sbv_impl::~sbv_impl()
{
  free(nodes.index);
  free(nodes.left);
  free(nodes.right);
}

bool sbv_impl::GetElement(long index)
{
  if (is_static) {
    if (nodes.binary_search(0, num_elements, index)<0) return 0;
    return 1;
  } 
  if (root < 0) return 0;
  if (is_list) {
    root = nodes.ListSplay(root, index);
  } else {
    root = nodes.TreeSplay(root, index);
  }
  return (nodes.index[root] == index);
}

int sbv_impl::SetElement(long index)
{
  if (is_static) return -1;
  if (root < 0) {
    // same behavior, regardless of tree or list
    root = NewNode();
    if (root < 0)
        return root;
    nodes.index[root] = index;
    nodes.left[root] = -1;
    nodes.right[root] = -1;
    num_elements++;
    return 0;
  }
  if (is_list)
      root = nodes.ListSplay(root, index);
  else
      root = nodes.TreeSplay(root, index);

  if (nodes.index[root] == index)
      return 1;

  // need to add the element
  if (is_list && (num_elements > list2tree))
      List2Tree();

  long newroot = NewNode();
  if (newroot < 0) return newroot;  // out of memory?
  nodes.index[newroot] = index;

  if (is_list) 
      root = nodes.ConnectListRoot(root, newroot);
  else 
      root = nodes.ConnectTreeRoot(root, newroot);

  num_elements++;
#ifdef DEBUG_ADD
  printf("After adding element %ld\n", index);
  Show();
#endif
  return 0;
}

int sbv_impl::ClearElement(long index)
{
  if (is_static) return -1;
  if (root < 0) return 0;
  if (is_list)
      root = nodes.ListSplay(root, index);
  else
      root = nodes.TreeSplay(root, index);

  if (nodes.index[root] != index)
      return 0;

  // need to remove the element
  if (!is_list && (num_elements < tree2list))
      Tree2List();

  long oldroot = root;
  if (is_list)
      root = nodes.RemoveListRoot(root);
  else
      root = nodes.RemoveTreeRoot(root);
  RecycleNode(oldroot);
  num_elements--;
#ifdef DEBUG_REMOVE
  printf("After removing element %ld\n", index);
  Show();
#endif
  return 1;
}

void sbv_impl::ClearAll()
{
  free_list = -1;
  num_elements = 0;
  last_element = 0;
}

bool sbv_impl::ConvertToStatic(bool tighten)
{
  if (is_static) return true;
  // Make sure we are a list, no matter how large
  Tree2List();
  // find the front of the list
  if (root>=0) {
    while (nodes.left[root] >= 0)  root = nodes.left[root];
  }
  // defragment the nodes
  long contig = 0;  // everything before contig is contiguous
  long s;
  while (root >= 0) {
    // follow forwarding arcs as necessary
    while (root < contig)  root = nodes.right[root];
    long next = nodes.right[root];
    // put root in the current slot
    if (root != contig) {
      nodes.right[root] = nodes.right[contig];
      SwapElements(root, contig);
      nodes.right[contig] = root;  // forwarding info
    }
    root = next;
    contig++;
  } // while root
  last_element = num_elements;
  free_list = -1;

  if (tighten) {
    free(nodes.left);
    nodes.left = 0;
    free(nodes.right);
    nodes.right = 0;
    if (max_elements > last_element) {
      nodes.index = (long*) realloc(nodes.index, last_element * sizeof(long));
      max_elements = last_element;
    }
  }
  is_static = true;

#ifdef DEBUG_GOSTATIC
  printf("After converting to static\n");
  printf("index: ");
  ShowArray(nodes.index, last_element);
  printf("\n");
#endif

  return true;
}

bool sbv_impl::ConvertToDynamic()
{
  if (!is_static) return true;
  if (0==nodes.left) {
    nodes.left = (long*) malloc(max_elements * sizeof(long));
    if (0==nodes.left) return false;
  }
  if (0==nodes.right) {
    nodes.right = (long*) malloc(max_elements * sizeof(long));
    if (0==nodes.right) return false;
  }

  if (num_elements > list2tree) {
    root = nodes.Array2Tree(0, num_elements);
    is_list = false;
  } else {
    root = nodes.Array2List(num_elements);
    is_list = true;
  }

  free_list = -1;
  is_static = false;
  
#ifdef DEBUG_GODYNAMIC
  printf("After converting to dynamic\n");
  Show();
#endif

  return true;
}

void sbv_impl::Traverse(bitvector_traverse *foo)
{
  if (0==foo) return;
  long i;
  if (is_static) {
    for (i=0; i<num_elements; i++)
      foo->Visit(nodes.index[i]);
    return;
  }
  if (root < 0) return;
  if (is_list) {
    for (i=root; nodes.left[i] >= 0; i=nodes.left[i]) { }
    for (; i>=0; i=nodes.right[i]) {
      foo->Visit(nodes.index[i]);
    }
    return;
  }
  // non-recursive, inorder tree traversal
  nodes.StackClear();
  i = root;
  while (i>=0) {
    if (nodes.left[i] >= 0) {
      nodes.Push(i);
      i = nodes.left[i];
      continue;
    }
    while (i>=0) {
      foo->Visit(nodes.index[i]);
      if (nodes.right[i] >= 0) {
        i = nodes.right[i];
        break;
      }
      i = nodes.Pop();
    } // inner while
  } // outer while
}

bool sbv_impl::ExportTo(SV_Vector* A) const
{
  if (!is_static) return false;
  if (0==A) return false;
  A->nonzeroes = num_elements;
  A->index = nodes.index;
  A->i_value = 0;
  A->r_value = 0;
  return true;
}

void sbv_impl::Expand()
{
  long new_elements = 2*max_elements;
  if (new_elements > 1024) new_elements = max_elements + 1024;
  if (new_elements < 4) new_elements = 4;
  long* newindex = (long*) realloc(nodes.index, new_elements * sizeof(long));
  long* newleft = (long*) realloc(nodes.left, new_elements * sizeof(long));
  long* newright = (long*) realloc(nodes.right, new_elements * sizeof(long));
  if (newindex) nodes.index = newindex;
  if (newleft) nodes.left = newleft;
  if (newright) nodes.right = newright;
  if ((0==newindex) || (0==newleft) || (0==newright)) return;
  max_elements = new_elements;
}

void sbv_impl::List2Tree()
{
  if (!is_list) return;
  nodes.List2Tree(root);
  is_list = false;
#ifdef DEBUG_SWITCH
  printf("Changing to tree\n");
#endif
}

void sbv_impl::Tree2List()
{
  if (is_list) return;
  nodes.Tree2List(root);
  is_list = true;
#ifdef DEBUG_SWITCH
  printf("Changing to list\n");
#endif
}


// ******************************************************************
// *                  intvector_traverse  methods                   *
// ******************************************************************

intvector_traverse::intvector_traverse()
{
}

intvector_traverse::~intvector_traverse()
{
  // nothing to destruct.
}

// ******************************************************************
// *                   sparse_intvector  methods                    *
// ******************************************************************

sparse_intvector::sparse_intvector()
{
  num_elements = 0;
  is_static = 0;
}

sparse_intvector::~sparse_intvector()
{
}

// ******************************************************************
// *                                                                *
// *                        siv_impl  class                         *
// *                                                                *
// ******************************************************************

class siv_impl : public sparse_intvector {
protected:
  list_or_tree nodes;
  /// Values.
  long* value;
  /// Dimension of index, left, right, value arrays.
  long max_elements;
  /// Last used element of index, left, right, value arrays.
  long last_element;
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
  siv_impl(int l2t, int t2l);
  virtual ~siv_impl();

  virtual long GetElement(long index);
  virtual int ChangeElement(long index, long delta);
  virtual int SetElement(long index, long value);
  virtual void ClearAll();
  virtual bool ConvertToStatic(bool tighten);
  virtual bool ConvertToDynamic();
  virtual void Traverse(intvector_traverse *);
  virtual bool ExportTo(SV_Vector* A) const;


protected:

  inline void Show() {
    printf("free list: %ld\n", free_list);
    printf("root: %ld\n", root);
    printf("index: ");
    ShowArray(nodes.index, last_element);
    printf("\nvalue: ");
    ShowArray(value, last_element);
    printf("\n left: ");
    ShowArray(nodes.left, last_element);
    printf("\nright: ");
    ShowArray(nodes.right, last_element);
    printf("\n");
  }

  void Expand();

  inline void RecycleNode(long x) {
    if (x+1 == last_element) {
      last_element--;
      while (free_list+1 == last_element) {
        if (free_list<0) return;
        free_list = nodes.right[free_list];
        last_element--;
      }
    } else {
      nodes.index[x] = -1;
      nodes.right[x] = free_list;
      nodes.left[x] = free_list;
      free_list = x;
    }
  }
  inline long NewNode() { 
    long ans;
    if (free_list >= 0) {
      ans = free_list;
      free_list = nodes.right[free_list];
    } else {
      if (last_element >= max_elements)  Expand();
      if (last_element >= max_elements)  return -2;  // no memory
      ans = last_element;
      last_element++;
    }
    return ans;
  }
  inline void SwapElements(long i, long j) {
    long tmp = nodes.index[i];
    nodes.index[i] = nodes.index[j];
    nodes.index[j] = tmp;
    long val = value[i];
    value[i] = value[j];
    value[j] = val;
  }

  void List2Tree();
  void Tree2List();
};

// ******************************************************************
// *                       siv_impl  methods                        *
// ******************************************************************

siv_impl::siv_impl(int l2t, int t2l) : sparse_intvector()
{
  nodes.index = nodes.left = nodes.right = 0;
  value = 0;
  max_elements = 0;
  last_element = 0;
  free_list = -1;
  root = -1;
  list2tree = l2t;
  tree2list = t2l;
  is_list = list2tree > 0;
}

siv_impl::~siv_impl()
{
  free(nodes.index);
  free(nodes.left);
  free(nodes.right);
  free(value);
}

long siv_impl::GetElement(long index)
{
  if (is_static) {
    long slot = nodes.binary_search(0, num_elements, index);
    if (slot<0) return 0;
    return value[slot];
  } 
  if (root < 0) return 0;
  if (is_list) {
    root = nodes.ListSplay(root, index);
  } else {
    root = nodes.TreeSplay(root, index);
  }
  if (nodes.index[root] != index) return 0;
  return value[root];
}

int siv_impl::ChangeElement(long index, long delta)
{
  if (is_static) return -1;
  if (root < 0) {
    if (0==delta) return 0;
    // same behavior, regardless of tree or list
    root = NewNode();
    if (root < 0)
        return root;
    nodes.index[root] = index;
    nodes.left[root] = -1;
    nodes.right[root] = -1;
    value[root] = delta;
    num_elements++;
    return 0;
  }
  if (is_list)
      root = nodes.ListSplay(root, index);
  else
      root = nodes.TreeSplay(root, index);

  // Element exists in tree, change its value
  if (nodes.index[root] == index) {
    value[root] += delta;
    if (value[root])  return 1;
    // need to delete this element, it is now zero!
    if (!is_list && (num_elements < tree2list))
        Tree2List();

    long oldroot = root;
    if (is_list)
        root = nodes.RemoveListRoot(root);
    else
        root = nodes.RemoveTreeRoot(root);
    RecycleNode(oldroot);
    num_elements--;
#ifdef DEBUG_REMOVE
    printf("After removing %ld:\n", index);
    Show();
#endif
    return 1;
  }

  if (0==delta) return 0;
  // need to add the element to the tree
  if (is_list && (num_elements > list2tree))
      List2Tree();

  long newroot = NewNode();
  if (newroot < 0) 
      return newroot;
  nodes.index[newroot] = index;
  value[newroot] = delta;

  if (is_list) 
      root = nodes.ConnectListRoot(root, newroot);
  else
      root = nodes.ConnectTreeRoot(root, newroot);
  num_elements++;
#ifdef DEBUG_ADD
  printf("After adding %ld:\n", index);
  Show();
#endif
  return 0;
}

int siv_impl::SetElement(long index, long val)
{
  if (is_static) return -1;
  if (root < 0) {
    if (0==val) return 0;
    // same behavior, regardless of tree or list
    root = NewNode();
    if (root < 0)
      return root;
    nodes.index[root] = index;
    nodes.left[root] = -1;
    nodes.right[root] = -1;
    value[root] = val;
    num_elements++;
    return 0;
  }
  if (is_list)
      root = nodes.ListSplay(root, index);
  else
      root = nodes.TreeSplay(root, index);

  // Element exists in tree, change its value
  if (nodes.index[root] == index) {
    value[root] = val;
    if (value[root])  return 1;
    // need to delete this element, it is now zero!
    if (!is_list && (num_elements < tree2list))
        Tree2List();

    long oldroot = root;
    if (is_list)
        root = nodes.RemoveListRoot(root);
    else
        root = nodes.RemoveTreeRoot(root);
    RecycleNode(oldroot);
    num_elements--;
#ifdef DEBUG_REMOVE
    printf("After removing %ld:\n", index);
    Show();
#endif
    return 1;
  }

  if (0==val) return 0;
  // need to add the element to the tree
  if (is_list && (num_elements > list2tree))
      List2Tree();

  long newroot = NewNode();
  if (newroot < 0) 
      return newroot;
  nodes.index[newroot] = index;
  value[newroot] = val;

  if (is_list) 
      root = nodes.ConnectListRoot(root, newroot);
  else
      root = nodes.ConnectTreeRoot(root, newroot);
  num_elements++;
#ifdef DEBUG_ADD
  printf("After adding %ld:\n", index);
  Show();
#endif
  return 0;
}


void siv_impl::ClearAll()
{
  free_list = -1;
  num_elements = 0;
  last_element = 0;
}

bool siv_impl::ConvertToStatic(bool tighten)
{
  if (is_static) return true;
  // Make sure we are a list, no matter how large
  Tree2List();
  // find the front of the list
  if (root>=0) {
    while (nodes.left[root] >= 0)  root = nodes.left[root];
  }
  // defragment the nodes
  long contig = 0;  // everything before contig is contiguous
  long s;
  while (root >= 0) {
    // follow forwarding arcs as necessary
    while (root < contig)  root = nodes.right[root];
    long next = nodes.right[root];
    // put root in the current slot
    if (root != contig) {
      nodes.right[root] = nodes.right[contig];
      SwapElements(root, contig);
      nodes.right[contig] = root;  // forwarding info
    }
    root = next;
    contig++;
  } // while root
  last_element = num_elements;
  free_list = -1;

  if (tighten) {
    free(nodes.left);
    nodes.left = 0;
    free(nodes.right);
    nodes.right = 0;
    if (max_elements > last_element) {
      nodes.index = (long*) realloc(nodes.index, last_element * sizeof(long));
      value = (long*) realloc(value, last_element * sizeof(long));
      max_elements = last_element;
    }
  }
  is_static = true;

#ifdef DEBUG_GOSTATIC
  printf("After converting to static\n");
  printf("index: ");
  ShowArray(nodes.index, last_element);
  printf("\nvalue: ");
  ShowArray(value, last_element);
  printf("\n");
#endif

  return true;
}

bool siv_impl::ConvertToDynamic()
{
  if (!is_static) return true;
  if (0==nodes.left) {
    nodes.left = (long*) malloc(max_elements * sizeof(long));
    if (0==nodes.left) return false;
  }
  if (0==nodes.right) {
    nodes.right = (long*) malloc(max_elements * sizeof(long));
    if (0==nodes.right) return false;
  }

  if (num_elements > list2tree) {
    root = nodes.Array2Tree(0, num_elements);
    is_list = false;
  } else {
    root = nodes.Array2List(num_elements);
    is_list = true;
  }

  free_list = -1;
  is_static = false;
  
#ifdef DEBUG_GODYNAMIC
  printf("After converting to dynamic\n");
  Show();
#endif

  return true;
}

void siv_impl::Traverse(intvector_traverse *foo)
{
  if (0==foo) return;
  long i;
  if (is_static) {
    for (i=0; i<num_elements; i++)
      foo->Visit(nodes.index[i], value[i]);
    return;
  }
  if (root < 0) return;
  if (is_list) {
    for (i=root; nodes.left[i] >= 0; i=nodes.left[i]) { }
    for (; i>=0; i=nodes.right[i]) {
      foo->Visit(nodes.index[i], value[i]);
    }
    return;
  }
  // non-recursive, inorder tree traversal
  nodes.StackClear();
  i = root;
  while (i>=0) {
    if (nodes.left[i] >= 0) {
      nodes.Push(i);
      i = nodes.left[i];
      continue;
    }
    while (i>=0) {
      foo->Visit(nodes.index[i], value[i]);
      if (nodes.right[i] >= 0) {
        i = nodes.right[i];
        break;
      }
      i = nodes.Pop();
    } // inner while
  } // outer while
}

bool siv_impl::ExportTo(SV_Vector* A) const
{
  if (!is_static) return false;
  if (0==A) return false;
  A->nonzeroes = num_elements;
  A->index = nodes.index;
  A->i_value = value;
  A->r_value = 0;
  return true;
}

void siv_impl::Expand()
{
  long new_elements = 2*max_elements;
  if (new_elements > 1024) new_elements = max_elements + 1024;
  if (new_elements < 4) new_elements = 4;
  long* newindex = (long*) realloc(nodes.index, new_elements * sizeof(long));
  if (newindex) nodes.index = newindex;
  long* newleft = (long*) realloc(nodes.left, new_elements * sizeof(long));
  if (newleft) nodes.left = newleft;
  long* newright = (long*) realloc(nodes.right, new_elements * sizeof(long));
  if (newright) nodes.right = newright;
  long* newvalue = (long*) realloc(value, new_elements * sizeof(long));
  if (newvalue) value = newvalue;
  if ((0==newindex) || (0==newleft) || (0==newright) || (0==newvalue)) return;
  max_elements = new_elements;
}

void siv_impl::List2Tree()
{
  if (!is_list) return;
  nodes.List2Tree(root);
  is_list = false;
#ifdef DEBUG_SWITCH
  printf("Changing to tree\n");
#endif
}

void siv_impl::Tree2List()
{
  if (is_list) return;
  nodes.Tree2List(root);
  is_list = true;
#ifdef DEBUG_SWITCH
  printf("Changing to list\n");
#endif
}


// ******************************************************************
// *                  realvector_traverse methods                   *
// ******************************************************************

realvector_traverse::realvector_traverse()
{
}

realvector_traverse::~realvector_traverse()
{
  // nothing to destruct.
}

// ******************************************************************
// *                   sparse_realvector methods                    *
// ******************************************************************

sparse_realvector::sparse_realvector()
{
  num_elements = 0;
  is_static = 0;
}

sparse_realvector::~sparse_realvector()
{
}

// ******************************************************************
// *                                                                *
// *                        srv_impl  class                         *
// *                                                                *
// ******************************************************************

class srv_impl : public sparse_realvector {
protected:
  list_or_tree nodes;
  /// Values.
  double* value;
  /// Dimension of index, left, right, value arrays.
  long max_elements;
  /// Last used element of index, left, right, value arrays.
  long last_element;
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
  srv_impl(int l2t, int t2l);
  virtual ~srv_impl();

  virtual double GetElement(long index);
  virtual int ChangeElement(long index, double delta);
  virtual int SetElement(long index, double value);
  virtual void ClearAll();
  virtual bool ConvertToStatic(bool tighten);
  virtual bool ConvertToDynamic();
  virtual void Traverse(realvector_traverse *);
  virtual bool ExportTo(SV_Vector* A) const;


protected:

  inline void Show() {
    printf("free list: %ld\n", free_list);
    printf("root: %ld\n", root);
    printf("index: ");
    ShowArray(nodes.index, last_element);
    printf("\nvalue: ");
    ShowArray(value, last_element);
    printf("\n left: ");
    ShowArray(nodes.left, last_element);
    printf("\nright: ");
    ShowArray(nodes.right, last_element);
    printf("\n");
  }

  void Expand();

  inline void RecycleNode(long x) {
    if (x+1 == last_element) {
      last_element--;
      while (free_list+1 == last_element) {
        if (free_list<0) return;
        free_list = nodes.right[free_list];
        last_element--;
      }
    } else {
      nodes.index[x] = -1;
      nodes.right[x] = free_list;
      nodes.left[x] = free_list;
      free_list = x;
    }
  }
  inline long NewNode() { 
    long ans;
    if (free_list >= 0) {
      ans = free_list;
      free_list = nodes.right[free_list];
    } else {
      if (last_element >= max_elements)  Expand();
      if (last_element >= max_elements)  return -2;  // no memory
      ans = last_element;
      last_element++;
    }
    return ans;
  }
  inline void SwapElements(long i, long j) {
    long tmp = nodes.index[i];
    nodes.index[i] = nodes.index[j];
    nodes.index[j] = tmp;
    double val = value[i];
    value[i] = value[j];
    value[j] = val;
  }

  void List2Tree();
  void Tree2List();
};

// ******************************************************************
// *                       srv_impl  methods                        *
// ******************************************************************

srv_impl::srv_impl(int l2t, int t2l) : sparse_realvector()
{
  nodes.index = nodes.left = nodes.right = 0;
  value = 0;
  max_elements = 0;
  last_element = 0;
  free_list = -1;
  root = -1;
  list2tree = l2t;
  tree2list = t2l;
  is_list = list2tree > 0;
}

srv_impl::~srv_impl()
{
  free(nodes.index);
  free(nodes.left);
  free(nodes.right);
  free(value);
}

double srv_impl::GetElement(long index)
{
  if (is_static) {
    long slot = nodes.binary_search(0, num_elements, index);
    if (slot<0) return 0.0;
    return value[slot];
  } 
  if (root < 0) return 0.0;
  if (is_list) {
    root = nodes.ListSplay(root, index);
  } else {
    root = nodes.TreeSplay(root, index);
  }
  if (nodes.index[root] != index) return 0.0;
  return value[root];
}

int srv_impl::ChangeElement(long index, double delta)
{
  if (is_static) return -1;
  if (root < 0) {
    if (0.0==delta) return 0;
    // same behavior, regardless of tree or list
    root = NewNode();
    if (root < 0) 
        return root;
    nodes.index[root] = index;
    nodes.left[root] = -1;
    nodes.right[root] = -1;
    value[root] = delta;
    num_elements++;
    return 0;
  }
  if (is_list)
      root = nodes.ListSplay(root, index);
  else
      root = nodes.TreeSplay(root, index);

  // Element exists in tree, change its value
  if (nodes.index[root] == index) {
    value[root] += delta;
    if (value[root])  return 1;
    // need to delete this element, it is now zero!
    if (!is_list && (num_elements < tree2list))
        Tree2List();

    long oldroot = root;
    if (is_list)
        root = nodes.RemoveListRoot(root);
    else
        root = nodes.RemoveTreeRoot(root);
    RecycleNode(oldroot);
    num_elements--;
#ifdef DEBUG_REMOVE
    printf("After removing %ld:\n", index);
    Show();
#endif
    return 1;
  }

  if (0.0==delta) return 0;
  // need to add the element to the tree
  if (is_list && (num_elements > list2tree))
      List2Tree();

  long newroot = NewNode();
  if (newroot < 0) 
      return newroot;
  nodes.index[newroot] = index;
  value[newroot] = delta;

  if (is_list) 
      root = nodes.ConnectListRoot(root, newroot);
  else
      root = nodes.ConnectTreeRoot(root, newroot);
  num_elements++;
#ifdef DEBUG_ADD
  printf("After adding %ld:\n", index);
  Show();
#endif
  return 0;
}

int srv_impl::SetElement(long index, double val)
{
  if (is_static) return -1;
  if (root < 0) {
    if (0.0==val) return 0;
    // same behavior, regardless of tree or list
    root = NewNode();
    if (root < 0) 
        return root;
    nodes.index[root] = index;
    nodes.left[root] = -1;
    nodes.right[root] = -1;
    value[root] = val;
    num_elements++;
    return 0;
  }
  if (is_list)
      root = nodes.ListSplay(root, index);
  else
      root = nodes.TreeSplay(root, index);

  // Element exists in tree, change its value
  if (nodes.index[root] == index) {
    value[root] = val;
    if (value[root])  return 1;
    // need to delete this element, it is now zero!
    if (!is_list && (num_elements < tree2list))
        Tree2List();

    long oldroot = root;
    if (is_list)
        root = nodes.RemoveListRoot(root);
    else
        root = nodes.RemoveTreeRoot(root);
    RecycleNode(oldroot);
    num_elements--;
#ifdef DEBUG_REMOVE
    printf("After removing %ld:\n", index);
    Show();
#endif
    return 1;
  }

  if (0.0==val) return 0;
  // need to add the element to the tree
  if (is_list && (num_elements > list2tree))
      List2Tree();

  long newroot = NewNode();
  if (newroot < 0) 
      return newroot;
  nodes.index[newroot] = index;
  value[newroot] = val;

  if (is_list) 
      root = nodes.ConnectListRoot(root, newroot);
  else
      root = nodes.ConnectTreeRoot(root, newroot);
  num_elements++;
#ifdef DEBUG_ADD
  printf("After adding %ld:\n", index);
  Show();
#endif
  return 0;
}


void srv_impl::ClearAll()
{
  free_list = -1;
  num_elements = 0;
  last_element = 0;
}

bool srv_impl::ConvertToStatic(bool tighten)
{
  if (is_static) return true;
  // Make sure we are a list, no matter how large
  Tree2List();
  // find the front of the list
  if (root>=0) {
    while (nodes.left[root] >= 0)  root = nodes.left[root];
  }
  // defragment the nodes
  long contig = 0;  // everything before contig is contiguous
  long s;
  while (root >= 0) {
    // follow forwarding arcs as necessary
    while (root < contig)  root = nodes.right[root];
    long next = nodes.right[root];
    // put root in the current slot
    if (root != contig) {
      nodes.right[root] = nodes.right[contig];
      SwapElements(root, contig);
      nodes.right[contig] = root;  // forwarding info
    }
    root = next;
    contig++;
  } // while root
  last_element = num_elements;
  free_list = -1;

  if (tighten) {
    free(nodes.left);
    nodes.left = 0;
    free(nodes.right);
    nodes.right = 0;
    if (max_elements > last_element) {
      nodes.index = (long*) realloc(nodes.index, last_element * sizeof(long));
      value = (double*) realloc(value, last_element * sizeof(double));
      max_elements = last_element;
    }
  }
  is_static = true;

#ifdef DEBUG_GOSTATIC
  printf("After converting to static\n");
  printf("index: ");
  ShowArray(nodes.index, last_element);
  printf("\nvalue: ");
  ShowArray(value, last_element);
  printf("\n");
#endif

  return true;
}

bool srv_impl::ConvertToDynamic()
{
  if (!is_static) return true;
  if (0==nodes.left) {
    nodes.left = (long*) malloc(max_elements * sizeof(long));
    if (0==nodes.left) return false;
  }
  if (0==nodes.right) {
    nodes.right = (long*) malloc(max_elements * sizeof(long));
    if (0==nodes.right) return false;
  }

  if (num_elements > list2tree) {
    root = nodes.Array2Tree(0, num_elements);
    is_list = false;
  } else {
    root = nodes.Array2List(num_elements);
    is_list = true;
  }

  free_list = -1;
  is_static = false;
  
#ifdef DEBUG_GODYNAMIC
  printf("After converting to dynamic\n");
  Show();
#endif

  return true;
}

void srv_impl::Traverse(realvector_traverse *foo)
{
  if (0==foo) return;
  long i;
  if (is_static) {
    for (i=0; i<num_elements; i++)
      foo->Visit(nodes.index[i], value[i]);
    return;
  }
  if (root < 0) return;
  if (is_list) {
    for (i=root; nodes.left[i] >= 0; i=nodes.left[i]) { }
    for (; i>=0; i=nodes.right[i]) {
      foo->Visit(nodes.index[i], value[i]);
    }
    return;
  }
  // non-recursive, inorder tree traversal
  nodes.StackClear();
  i = root;
  while (i>=0) {
    if (nodes.left[i] >= 0) {
      nodes.Push(i);
      i = nodes.left[i];
      continue;
    }
    while (i>=0) {
      foo->Visit(nodes.index[i], value[i]);
      if (nodes.right[i] >= 0) {
        i = nodes.right[i];
        break;
      }
      i = nodes.Pop();
    } // inner while
  } // outer while
}

bool srv_impl::ExportTo(SV_Vector* A) const
{
  if (!is_static) return false;
  if (0==A) return false;
  A->nonzeroes = num_elements;
  A->index = nodes.index;
  A->i_value = 0;
  A->r_value = value;
  return true;
}

void srv_impl::Expand()
{
  long new_elements = 2*max_elements;
  if (new_elements > 1024) new_elements = max_elements + 1024;
  if (new_elements < 4) new_elements = 4;
  long* newindex = (long*) realloc(nodes.index, new_elements * sizeof(long));
  if (newindex) nodes.index = newindex;
  long* newleft = (long*) realloc(nodes.left, new_elements * sizeof(long));
  if (newleft) nodes.left = newleft;
  long* newright = (long*) realloc(nodes.right, new_elements * sizeof(long));
  if (newright) nodes.right = newright;
  double* newvalue = (double*) realloc(value, new_elements * sizeof(double));
  if (newvalue) value = newvalue;
  if ((0==newindex) || (0==newleft) || (0==newright) || (0==newvalue)) return;
  max_elements = new_elements;
}

void srv_impl::List2Tree()
{
  if (!is_list) return;
  nodes.List2Tree(root);
  is_list = false;
#ifdef DEBUG_SWITCH
  printf("Changing to tree\n");
#endif
}

void srv_impl::Tree2List()
{
  if (is_list) return;
  nodes.Tree2List(root);
  is_list = true;
#ifdef DEBUG_SWITCH
  printf("Changing to list\n");
#endif
}


// ******************************************************************
// *                      front-end functions                       *
// ******************************************************************

const char* SV_LibraryVersion()
{
  static char buffer[100];
  snprintf(buffer, sizeof(buffer), "Andy's sparse vector library, version %d.%d.%d",
     MAJOR_VERSION, MINOR_VERSION, REVISION_NUMBER);
  return buffer;
}

sparse_bitvector*  SV_CreateSparseBitvector(int list2tree, int tree2list)
{
  if (list2tree<0) list2tree = default_list_to_tree;
  if (tree2list<0) tree2list = default_tree_to_list;
  return new sbv_impl(list2tree, tree2list);
}

sparse_intvector*  SV_CreateSparseIntvector(int list2tree, int tree2list)
{
  if (list2tree<0) list2tree = default_list_to_tree;
  if (tree2list<0) tree2list = default_tree_to_list;
  return new siv_impl(list2tree, tree2list);
}

sparse_realvector*  SV_CreateSparseRealvector(int list2tree, int tree2list)
{
  if (list2tree<0) list2tree = default_list_to_tree;
  if (tree2list<0) tree2list = default_tree_to_list;
  return new srv_impl(list2tree, tree2list);
}

