
// $Id$

#ifndef TREES_H
#define TREES_H

#include "flatss.h"
#include "../Templates/stack.h"

/** @name trees.h
    @type File
    @args \
 
    Balanced binary trees of states, for dictionary operations.
    (search, add unique)

    Clever trick: we store left and right pointers in arrays,
    so the "node index" is the same as the "state index";
    this saves us a pointer per node
    (but requires us to use state indices, which speed things up anyway).

    IN FUTURE:
	make a similar "binary_forest" for multi-level
	but the above clever trick won't work...
*/

class binary_tree {
protected:
  /// Array of left "pointers"
  int* left;
  /// Array of right "pointers"
  int* right;
  /// Number of tree "nodes" allocated (dimension of left and right)
  int nodes_alloc;
  /// Number of nodes in the tree
  int nodes_used;
  /// Pointer to root node (for single tree version)
  int root;
  /// State data
  state_array *states; 
  /// Stack for searches and rotations
  Stack <int> *path;
public:
  binary_tree(state_array *s);
  ~binary_tree();  

  /// Put states (indices) into the array, in order.
  inline void FillOrderList(int* order) { 
    int i = 0; 
    FillOrderList(root, i, order);
  }
protected:
  /// Traverse subtree, add to order array starting with "index".
  void FillOrderList(int tree, int &index, int* order);

  /** Traverse subtree searching for s.
      The path taken is saved by pushing onto a stack.
  */
  void FindPath(const state &s);
};

/*
class avl_state_tree : public binary_state_tree {
  // allocate a few extra bits per node for balancing
public:
  
  // Insert state s.
  // If state s already is present, its state index is returned.
  // Otherwise, s is added to the tree and a new state index is returned.
  int InsertState(const state &s);
};

class splay_state_tree : public binary_state_tree {
public:
};

*/

#endif
