
// $Id$

/*
    Minimalist splay tree classes.
    Basically, classes around the old style of global functions;
    this allows us to keep tighter control over the splay stack.

    For complete functionality (allocation of nodes, etc.), 
    derive a class from "Splay".
    
    For data comparison, there must be a function Compare(DATA *a, DATA *b)
*/

#ifndef SPLAY_H
#define SPLAY_H

#include "defines.h"
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

#endif

