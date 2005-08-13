
// $Id$

#ifndef SPLAY_H
#define SPLAY_H

#include "../defines.h"
#include "stack.h"

/** Splay tree using a node manager class.
    The class must provide the following (preferably inlined):

    int Null() // which integer handle to use as null.

    int getLeft(int h) 	
    int getRight(int h)
    void setLeft(int h, int l)
    void setRight(int h, int r)
    
    int Compare(int h1, int h2)

    For tree traversal:
    void Visit(int h)
*/

template <class MANAGER>
class SplayTree {
  MANAGER *nodes; 
  Stack <int> *path;
protected:
  inline void Push(int x) { path->Push(x); }
  inline int Pop() { return path->Empty() ? nodes->Null() : path->Pop(); }
  /// Handy function #1 
  void SplayRotate(int C, int P, int GP) {
    // swap parent and child
    if (nodes->getLeft(P) == C) {
      nodes->setLeft(P, nodes->getRight(C));
      nodes->setRight(C, P);
    } else {
      DCASSERT(nodes->getRight(P) == C);
      nodes->setRight(P, nodes->getLeft(C));
      nodes->setLeft(C, P);
    }
    if (GP != nodes->Null()) {
      if (nodes->getLeft(GP) == P)
	nodes->setLeft(GP, C);
      else {
	DCASSERT(nodes->getRight(GP) == P);
	nodes->setRight(GP, C);
      }
    }
  }
  /// Handy function #2
  void SplayStep(int c, int p, int gp, int ggp) {
    if (nodes->Null() == gp) {
      SplayRotate(c, p, nodes->Null());
    } else {
      bool ParentIsRight = (nodes->getRight(gp) == p);
      bool ChildIsRight = (nodes->getRight(p) == c);
      if (ParentIsRight == ChildIsRight) {
	SplayRotate(p, gp, ggp);
	SplayRotate(c, p, ggp);
      } else {
	SplayRotate(c, p, gp);
	SplayRotate(c, gp, ggp);
      }
    }
  }
public:
  SplayTree(MANAGER *n) {
    nodes = n;
    path = new Stack <int> (16);
  }
  ~SplayTree() {
    delete path;
  }
  /**
      Perform a splay operation. 
      After the operation, the root node will contain either the requested 
      key value, or a value that is "close" to the requested value.  
      I.e., if the root has value V < Key, then there are no elements 
      in the tree between V and Key.
     
      @param	root	The root node of the tree (will change)
      @param	key	Handle of a node, not within the tree;  we try
			to find a node with the same data value.

      @return	The compare result of the root item:
      		0, the key was found (and moved to root)
		1, an item larger than the key was moved to root
		-1, an item smaller than the key was moved to root
  */
  int Splay(int &root, int key) {
    int cmp = 1;
    int child = root;
    path->Clear();
    while (child != nodes->Null()) {
      cmp = nodes->Compare(child, key);
      if (0==cmp) break;  // found!
      Push(child);
      if (cmp>0) child = nodes->getLeft(child);
	else     child = nodes->getRight(child);
    } // while child
    if (nodes->Null() == child) 
	child = Pop();
    int parent = Pop();
    int grandp = Pop();
    int greatgp = Pop();
    while (parent != nodes->Null()) {
      SplayStep(child, parent, grandp, greatgp);
      parent = greatgp;
      grandp = Pop();
      greatgp = Pop();
    }
    root = child;
    return cmp;
  }

  /** Inserts key into the tree,
      unless there is already a node with the same data value.
      Key should have null left and right children.
      Returns the root of the tree, which will contain the data value.
  */
  int Insert(int root, int key) {
    int cmp = Splay(root, key);
    if (0==cmp) return root;  // already in the tree
    if (cmp>0) {
      // root > key
      nodes->setRight(key, root);
      if (root != nodes->Null()) {
        nodes->setLeft(key, nodes->getLeft(root));
        nodes->setLeft(root, nodes->Null());
      } 
    } else {
      // root < key
      nodes->setLeft(key, root);
      if (root != nodes->Null()) {
        nodes->setRight(key, nodes->getRight(root));
        nodes->setRight(root, nodes->Null());
      } 
    }
    return key;
  }

  void InorderTraverse(int root);
};

template <class MANAGER>
void SplayTree <MANAGER> :: InorderTraverse(int root)
{
  if (root != nodes->Null()) {
    InorderTraverse(nodes->getLeft(root));
    nodes->Visit(root);
    InorderTraverse(nodes->getRight(root));
  }
}

/** Simple splay tree of objects.
    Splay tree nodes are stored in separate arrays.
*/
template <class DATA>
class ArraySplay {
protected:
  /// Array of left "pointers"
  int* left;
  /// Array of right "pointers"
  int* right;
  /// Data values.
  DATA* value;
  /// Number of tree "nodes" allocated (dimension of left, right, value)
  int nodes_alloc;
  /// Number of nodes in the tree
  int nodes_used;
  /// Pointer to root node
  int root;
  /// Actual splay tree
  SplayTree < ArraySplay <DATA> > *tree;

  // Used by FillOrderList
  int* order_list;
  int order_ptr;
protected:
  void Resize(int newsize) {
    left = (int*) realloc(left, newsize*sizeof(int)); 
    if (newsize>0 && NULL==left) OutOfMemoryError("Tree resize");
    right = (int*) realloc(right, newsize*sizeof(int)); 
    if (newsize>0 && NULL==right) OutOfMemoryError("Tree resize");
    value = (DATA*) realloc(value, newsize*sizeof(DATA)); 
    if (newsize>0 && NULL==value) OutOfMemoryError("Tree resize");
    nodes_alloc = newsize;
  }
public:
  ArraySplay() {
    left = right = NULL;
    value = NULL;
    nodes_alloc = nodes_used = 0;
    root = Null();
    tree = new SplayTree < ArraySplay <DATA> > (this);
    Resize(4);
  }

  ~ArraySplay() {
    free(left);
    free(right);
    free(value);
    delete tree;
  }

  inline int NumNodes() const { return nodes_used; }

  /// Return the index of key, if it is in the tree.
  inline int FindElement(const DATA& key) {
    value[nodes_used] = key;
    int cmp = tree->Splay(root, nodes_used);
    if (0==cmp) return root;	
    return Null();
  }
  /// Insert a unique copy of key.
  int AddElement(const DATA& key) {
    value[nodes_used] = key;
    left[nodes_used] = Null();
    right[nodes_used] = Null();
    root = tree->Insert(root, nodes_used);
    if (root == nodes_used) {
      // the element was added to the tree
      nodes_used++;
      if (nodes_used >= nodes_alloc) {
        Resize( (nodes_alloc < 1024) ? (nodes_alloc*2) : (nodes_alloc+1024) );
      }
    }
#ifdef DEBUG_ARRAY_SPLAY
    Output << "Left: [";
    Output.PutArray(left, nodes_used+1);
    Output << "]\nRight: [";
    Output.PutArray(right, nodes_used+1);
    Output << "]\nValues: [";
    Output.PutArray(value, nodes_used+1);
    Output << "]\n";
    Output.flush();
#endif
    return root;
  }
  DATA* Compress() {
    free(left);
    free(right);
    left = NULL;
    right = NULL;
    DATA* steal = (DATA*) realloc(value, nodes_used*sizeof(DATA)); 
    if (nodes_used>0 && NULL==steal) OutOfMemoryError("Tree resize");
    nodes_alloc = nodes_used = 0;
    value = NULL;
    return steal;
  } 
  /// Fill order array, such that order[i] gives index of ith smallest.
  inline void FillOrderList(int* order) { 
    DCASSERT(order);
    order_list = order;
    order_ptr = 0;
    tree->InorderTraverse(root);
  }
  // stuff for SplayTree
  inline void Visit(int root) { 
    order_list[order_ptr] = root;
    order_ptr++;
  }
  inline int Null() const { return -1; }
  inline int getLeft(int h) const { return left[h]; }
  inline int getRight(int h) const { return right[h]; }
  inline void setLeft(int h, int n) { left[h] = n; }
  inline void setRight(int h, int n) { right[h] = n; }
  inline int Compare(int h1, int h2) const {
    if (value[h1] == value[h2]) return 0;
    return (value[h1] > value[h2]) ? 1 : -1;
  }
};


#endif

