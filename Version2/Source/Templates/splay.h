
// $Id$

/*
    Minimalist splay tree classes.
    Basically, classes around the old style of global functions;
    this allows us to keep tighter control over the splay stack.

    For complete functionality (allocation of nodes, etc.), 
    derive a class from "SplayWrap".
    
    For data comparison, there must be a function Compare(DATA *a, DATA *b)

    For ArraySplay, the function is Compare(const DATA &a, const DATA &b)
*/

#ifndef SPLAY_H
#define SPLAY_H

#include "../defines.h"
#include "stack.h"

class PtrSplay {
public:
  struct node {
    void* data;
    node* left;
    node* right;
  };
protected:
  /// Handy function #1 
  void SplayRotate(node *C, node *P, node *GP) {
    // swap parent and child
    if (P->left == C) {
      P->left = C->right;
      C->right = P;
    } else {
      DCASSERT(P->right == C);
      P->right = C->left;
      C->left = P;
    }
    if (GP) {
      if (GP->left == P)
	GP->left = C;
      else {
	DCASSERT(GP->right == P);
	GP->right = C;
      }
    }
  }
  /// Handy function #2
  void SplayStep(node *c, node *p, node *gp, node *ggp) {
    if (NULL==gp) {
      SplayRotate(c, p, NULL);
    } else {
      bool ParentIsRight = (gp->right == p);
      bool ChildIsRight = (p->right == c);
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

template <class DATA>
class SplayWrap : public PtrSplay {
protected:
  PtrStack *path;
  inline void Push(node* x) { path->Push(x); }
  inline node* Pop() { return path->Empty() ? NULL : (node*)path->Pop(); }
public:
  SplayWrap() { path = new PtrStack(16); }
  ~SplayWrap() { delete path; }
  /**
      Perform a splay operation: find the desired node and move it to the top.
      @param	root	The root node of the tree (will change)
      @param	key	The item to find, or NULL to find the smallest
      @return	The compare result of the root item:
      		0, the key was found (and moved to root)
		1, an item larger than the key was moved to root
		-1, an item smaller than the key was moved to root
  */
  int Splay(node* &root, DATA* key) {
    int cmp = 1;
    node* child = root;
    path->Clear();
    while (child) {
      if (key) cmp = Compare((DATA*)child->data, key); else cmp = 1;
      if (0==cmp) break;  // found!
      Push(child);
      if (cmp>0) child = child->left; else child = child->right;
    }
    if (NULL==child) child = Pop();
    node* parent = Pop();
    node* grandp = Pop();
    node* greatgp = Pop();
    while (parent) {
      SplayStep(child, parent, grandp, greatgp);
      parent = greatgp;
      grandp = Pop();
      greatgp = Pop();
    }
    root = child;
    return cmp;
  }
};

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

