
// $Id$

#include "trees.h"
#include "../Base/memtrack.h"

/** @name trees.cc
    @type File
    @args \ 

    Balanced trees for generating sets of states.
    
 */

//@{ 

// ******************************************************************
// *                                                                *
// *                      binary_tree  methods                      *
// *                                                                *
// ******************************************************************

binary_tree::binary_tree(state_array *s)
{
  ALLOC("binary_tree", sizeof(splay_state_tree));
  states = s;
  DCASSERT(states->UsesIndexHandles());
  left = NULL;  
  right = NULL;
  nodes_alloc = 0;
  nodes_used = 0;
  root = -1;
  path = new Stack <int> (16);
  // allocated in derived classes...
}

binary_tree::~binary_tree()
{
  FREE("binary_tree", sizeof(splay_state_tree));
  free(left);
  free(right);
  delete path;
}

void binary_tree::FillOrderList(int tree, int &index, int* order)
{
  DCASSERT(0);
}

int binary_tree::FindPath(const state &s)
{
  int c = 1;
  path->Clear();
  int h = states->AddState(s);
  DCASSERT(nodes_used == h);
  int subtree = root;
  while (subtree >= 0) {
    path->Push(subtree);
    c = states->Compare(subtree, h);
    if (c==0) return 0;
    if (c>0) subtree = left[subtree];
    else subtree = right[subtree];
  } // while
  return c;
}

// ******************************************************************
// *                                                                *
// *                    splay_state_tree methods                    *
// *                                                                *
// ******************************************************************

splay_state_tree::splay_state_tree(state_array *s) : binary_tree(s)
{
  Resize(4);
}

int splay_state_tree::AddState(const state &s)
{
  if (nodes_used >= nodes_alloc) {
    Resize( (nodes_alloc < 1024) ? (nodes_alloc*2) : (nodes_alloc+1024) );
  }
  if (root<0) {
    // empty tree
    int h = states->AddState(s);
    DCASSERT(0 == h);
    left[0] = -1;
    right[0] = -1;
    return root = nodes_used++;
  }
  int cmp = Splay(s);
  if (0==cmp) {
    // duplicate!
    states->PopLast();
    return root;
  }
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
  // root = nodes_used;
  // nodes_used++;
  return root = nodes_used++;
}

void splay_state_tree::Report(OutputStream &r)
{
  r << "Splay tree report:\n";
  r << "\t" << nodes_alloc << " allocated nodes\n";
  r << "\t" << nodes_used << " used nodes\n";
  r << "\t" << (int)(nodes_alloc * 2 * sizeof(int)) << " bytes allocated\n";
  r << "\t" << (int)(nodes_used * 2 * sizeof(int)) << " bytes used\n";
  r << "Splay stack report:\n";
  r << "\t" << path->AllocEntries() << " stack entries allocated\n";
  r << "\t" << (int)(path->AllocEntries() * sizeof(int)) << " bytes allocated\n";
}

void splay_state_tree::Resize(int newsize)
{
  left = (int*) realloc(left, newsize*sizeof(int)); 
  if (newsize>0 && NULL==left) OutOfMemoryError("Tree resize");
  right = (int*) realloc(right, newsize*sizeof(int)); 
  if (newsize>0 && NULL==right) OutOfMemoryError("Tree resize");
  nodes_alloc = newsize;
}

int splay_state_tree::Splay(const state &s) 
{
    int cmp = FindPath(s);
    int child = Pop();
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

//@}

