
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

// ******************************************************************
// *                                                                *
// *                     red_black_tree methods                     *
// *                                                                *
// ******************************************************************

red_black_tree::red_black_tree(state_array *s) : binary_tree(s)
{
  isRed = new bitvector(0);
  Resize(4);
}

red_black_tree::~red_black_tree()
{
  delete isRed;
}

int red_black_tree::AddState(const state &s)
{
  if (nodes_used >= nodes_alloc) {
    Resize( (nodes_alloc < 1024) ? (nodes_alloc*2) : (nodes_alloc+1024) );
  }
  if (root<0) {
    // empty tree
    root = states->AddState(s);
    DCASSERT(0 == root);
    left[0] = -1;
    right[0] = -1;
    isRed->Unset(0);  // root is always black
    nodes_used++;
    return 0;
  }
  int cmp = FindPath(s);
  if (0==cmp) {
    // state was found, return it
    states->PopLast();
    return Pop();
  }
  // add new leaf node
  int child = nodes_used;
  left[child] = -1;
  right[child] = -1;
  int parent = Pop();  // last node we saw before failed search
  DCASSERT(parent>=0);
  if (cmp>0) {
    DCASSERT(left[parent]==-1);
    left[parent] = child;
  } else {
    DCASSERT(right[parent]==-1);
    right[parent] = child;
  }

  // go back up tree, splitting 4-nodes, rotating as necessary.
  int grandp = Pop();
  int greatgp = Pop();
  while (1) {
    // child is either a 4-node to be split, or a newly-inserted node
    isRed->Set(child);
    if (nodes_used!=child) {
      DCASSERT(left[child]>=0);
      DCASSERT(right[child]>=0);
      isRed->Unset(left[child]);
      isRed->Unset(right[child]);
    }
    // child is now red, check for illegal situations above and fix them
    if (parent<0) break;
    if (!isRed->IsSet(parent)) break; 
    // parent and child are both red, rotation may be necessary...
    // sanity checks...
    DCASSERT(grandp>=0); // otherwise parent cannot be red
    DCASSERT(!isRed->IsSet(grandp));
    if (is4node(grandp)) {
      // grandparent is a 4 node (that's why parent is red),
      // if we split grandpa, this child will be fine
      child = grandp;
      parent = greatgp;
      grandp = Pop();
      greatgp = Pop();
      continue;
    }

    bool c_left_p = (left[parent]==child);
    bool p_left_g = (left[grandp]==parent);
    if (c_left_p != p_left_g) {
      // "double rotation" case
      if (c_left_p) { // ">" case
	// rotate
	right[grandp] = left[child]; 
	left[parent] = right[child]; 
        left[child] = grandp;
	right[child] = parent;
      } else {        // "<" case
        // rotate
	left[grandp] = right[child];
	right[parent] = left[child];
	left[child] = parent;
	right[child] = grandp;
      };
      // fix colors
      isRed->Unset(child);  
      isRed->Set(grandp);
      // fix ggp
      if (greatgp<0) {
	// grandp was root!
	root = child;
      } else {
	if (left[greatgp]==grandp) {
	  left[greatgp] = child;
	} else {
	  DCASSERT(right[greatgp]==grandp);
	  right[greatgp] = child;
	}
      }
    } else {
      // "single rotation" case; we have a line (either to left or right)
      if (c_left_p) { // "/" case
	// rotate
	left[grandp] = right[parent];
	right[parent] = grandp;
      } else {        // "\" case
        // rotate
	right[grandp] = left[parent];
	left[parent] = grandp;
      }
      // fix colors
      isRed->Unset(parent);
      isRed->Set(grandp);
      // fix ggp
      if (greatgp<0) {
	// grandp was root!
	root = parent;
      } else {
	if (left[greatgp]==grandp) {
	  left[greatgp] = parent;
	} else {
	  DCASSERT(right[greatgp]==grandp);
	  right[greatgp] = parent;
	}
      }
    } // if "double" or "single" rotation...

    // If we are here, definitely break out of loop.
    break;
  } // while (1)

  // root remains black
  isRed->Unset(root);  
  return nodes_used++;
}

int red_black_tree::FindState(const state &s)
{
    int h = states->AddState(s);
    DCASSERT(nodes_used == h);
    int subtree = root;
    while (subtree >= 0) {
      int c = states->Compare(subtree, h);
      if (c==0) {
	states->PopLast();
	return subtree;
      }
      if (c>0) subtree = left[subtree];
      else subtree = right[subtree];
    } // while
    states->PopLast();
    return -1;
}

void red_black_tree::Report(OutputStream &r)
{
  r << "Red-black tree report:\n";
  r << "\t" << nodes_alloc << " allocated nodes\n";
  r << "\t" << nodes_used << " used nodes\n";
  int memalloc = nodes_alloc * 2 * sizeof(int) + isRed->MemUsed(); 
  r << "\t" << memalloc << " bytes allocated\n";
  int memused = nodes_used * 2 * sizeof(int) + isRed->MemUsed(); 
  r << "\t" << memused << " bytes used\n";
  r << "Splay stack report:\n";
  r << "\t" << path->AllocEntries() << " stack entries allocated\n";
  r << "\t" << (int)(path->AllocEntries() * sizeof(int)) << " bytes allocated\n";
}

void red_black_tree::Resize(int newsize)
{
  left = (int*) realloc(left, newsize*sizeof(int)); 
  if (newsize>0 && NULL==left) OutOfMemoryError("Tree resize");
  right = (int*) realloc(right, newsize*sizeof(int)); 
  if (newsize>0 && NULL==right) OutOfMemoryError("Tree resize");
  nodes_alloc = newsize;
  isRed->Resize(newsize);
}

//@}

