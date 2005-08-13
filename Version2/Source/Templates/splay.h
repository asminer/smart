
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

// ------------------------------------------------------------

class ArraySplayBase {
protected:
  /// Array of left "pointers"
  int* left;
  /// Array of right "pointers"
  int* right;
  /// Number of tree "nodes" allocated (dimension of left and right)
  int nodes_alloc;
  /// Number of nodes in the tree
  int nodes_used;
  /// Pointer to root node
  int root;
  /// Stack for searches and rotations
  Stack <int> *path;
public:
  ArraySplayBase();
  ~ArraySplayBase();  

  /// Fill order array, such that order[i] gives index of ith smallest.
  inline void FillOrderList(int* order) { 
    int i = 0; 
    FillOrderList(root, i, order);
  }
  inline int NumNodes() const { return nodes_used; }
protected:
  inline int Pop() { return (path->Empty()) ? -1 : path->Pop(); }
  /// Traverse subtree, add to order array starting with "index".
  void FillOrderList(int tree, int &index, int* order);
  inline void SplayRotate(int C, int P, int GP) {
    // swap parent and child
    DCASSERT(P>=0);
    if (left[P] == C) {
      left[P] = right[C];
      right[C] = P;
    } else {
      DCASSERT(C==right[P]);
      right[P] = left[C];
      left[C] = P;
    } 
    if (GP>=0) {
      if (P==left[GP]) {
	left[GP] = C;
      } else {
	DCASSERT(P==right[GP]);
	right[GP] = C;
      }
    } // if gp
  }
  inline void SplayStep(int c, int p, int gp, int ggp) {
    if (gp<0) {
      SplayRotate(c, p, -1);
    } else {
      bool ParentIsRight = (right[gp] == p);
      bool ChildIsRight = (right[p] == c);
      if (ParentIsRight == ChildIsRight) {
	SplayRotate(p, gp, ggp);
	SplayRotate(c, p, ggp);
      } else {
	SplayRotate(c, p, gp);
	SplayRotate(c, gp, ggp);
      }
    }
  }
};

// ------------------------------------------------------------

template <class DATA>
class ArraySplay : public ArraySplayBase {
protected:
  DATA* value;
public:
  ArraySplay() : ArraySplayBase() { value = NULL; Resize(4); }
  ~ArraySplay() { free(value); }
  DATA* Compress() {
    free(left);
    free(right);
    left = NULL;
    right = NULL;
    value = (DATA*) realloc(value, nodes_used*sizeof(DATA)); 
    if (nodes_used>0 && NULL==value) OutOfMemoryError("Tree resize");
    nodes_alloc = 0; 
    DATA* steal = value;
    value = NULL;
    return steal;
  } 
  inline int FindElement(const DATA& key) {
    if (root<0) return -1;
    int cmp = Splay(key);
    if (0==cmp) return root;	
    return -1;
  }
  int AddElement(const DATA& key) {
    if (nodes_used >= nodes_alloc) {
      Resize( (nodes_alloc < 1024) ? (nodes_alloc*2) : (nodes_alloc+1024) );
    }
    if (root<0) {
      // empty tree
      value[0] = key;
      left[0] = -1;
      right[0] = -1;
    } else {
      int cmp = Splay(key);
      if (0==cmp) return root;	// duplicate
      value[nodes_used] = key;
      if (cmp>0) {
        // root > s
        left[nodes_used] = left[root];
        right[nodes_used] = root;
        left[root] = -1;
      } else {
        // root < s
        left[nodes_used] = root;
        right[nodes_used] = right[root];
        right[root] = -1;
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
    return root = nodes_used++;
  }
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
  /**
      Perform a splay operation: find the desired node and move it to the top.
      @param	root	The root node of the tree (will change)
      @param	key	The item to find
      @return	The compare result of the root item:
      		0, the key was found (and moved to root)
		1, an item larger than the key was moved to root
		-1, an item smaller than the key was moved to root
  */
  int Splay(const DATA &key) {
    int cmp = 1;
    int child = root;
    path->Clear();
    while (child>=0) {
      if (key == value[child]) {
        cmp = 0; 
        break;
      }
      path->Push(child);
      if (value[child] > key) {
        child = left[child];
	if (child<0) cmp = 1;
      } else {
        child = right[child];
	if (child<0) cmp = -1;
      }
    }
    if (child<0) child = Pop();
    int parent = Pop();
    int grandp = Pop();
    int greatgp = Pop();
    while (parent>=0) {
      SplayStep(child, parent, grandp, greatgp);
      parent = greatgp;
      grandp = Pop();
      greatgp = Pop();
    }
    root = child;
    return cmp;
  }
};



#endif

