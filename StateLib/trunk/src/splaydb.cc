
// $Id$

#include "splaydb.h"

#include <stdio.h>


// ******************************************************************
// *                                                                *
// *                       splay_db methods                         *
// *                                                                *
// ******************************************************************

splay_db::splay_db(bool indexed, bool storesize)
: bst_db(indexed, storesize)
{
}

splay_db::~splay_db()
{
}

void splay_db::ConvertToDynamic(bool tighten)
{
  if (!is_static) return;
  if (!left)  left = (long*) malloc(nodes_alloc * sizeof(long));
  if (!right)  right = (long*) malloc(nodes_alloc * sizeof(long));
  if (0==left || 0==right) {
    free(left);
    left = 0;
    free(right);
    right = 0;
    throw StateLib::error(StateLib::error::NoMemory);
  }

  allocPath();

#ifdef DEBUG_DYNAMIC
  printf("Converting tree to dynamic mode\n");
  printf("order: [%ld", order[0]);
  for (long s=1; s<num_states; s++) printf(", %ld", order[s]);
  printf("]\n");
#endif
  root = MakeTree(0, num_states);
  is_static = false;
  if (tighten) {
    free(order);
    order = 0;
  }
#ifdef DEBUG_DYNAMIC
  printf(" root: %ld\n", root);
  printf(" left: [%ld", left[0]);
  for (long s=1; s<num_states; s++) printf(", %ld", left[s]);
  printf("]\nright: [%ld", right[0]);
  for (long s=1; s<num_states; s++) printf(", %ld", right[s]);
  printf("]\n");
  // DumpDot(stdout);
  printf("Done dynamic conversion\n");
#endif
  return;
}

long splay_db::MakeTree(long low, long high)
{
  if (low >= high) return -1;
  long mid = (low+high)/2;
  left[order[mid]] = MakeTree(low, mid);
  right[order[mid]] = MakeTree(mid+1, high);
  return order[mid];
}

int splay_db::Splay(long key)
{
  int cmp = FindPath(key);
  long child = Pop();
  long parent = Pop();
  long grandp = Pop();
  long greatgp = Pop();
  while (parent >= 0) {
      SplayStep(child, parent, grandp, greatgp);
      parent = greatgp;
      grandp = Pop();
      greatgp = Pop();
  }
  root = child;
  return cmp;
}

int splay_db::Splay(const int* state, int size)
{
  int cmp = FindPath(state, size);
  long child = Pop();
  long parent = Pop();
  long grandp = Pop();
  long greatgp = Pop();
  while (parent >= 0) {
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
// *                    splay_index_db methods                      *
// *                                                                *
// ******************************************************************

splay_index_db::splay_index_db(bool storesize) : splay_db(true, storesize)
{
}

splay_index_db::~splay_index_db()
{
}

long splay_index_db::InsertState(const int* s, int np)
{
  long key = states->AddState(s, np);
  if (is_static) {
    long ans = staticBinarySearch(key);
    states->PopLast(key);
    if (ans<0) throw StateLib::error(StateLib::error::Static);
    return ans;
  }
  // make space
  if (num_states >= nodes_alloc) {
    enlargeTree();
  }
  if (root<0) {
    // empty tree
    // DCASSERT(0 == h);
    left[0] = -1;
    right[0] = -1;
    num_states++;
    return root = key;
  }
  int cmp = Splay(key);
  if (0==cmp) {
    // duplicate!
    states->PopLast(key);
    return root;
  }
  if (cmp>0) {
    // root > s
    left[num_states] = left[root];
    right[num_states] = root;
    left[root] = -1;
  } else {
    // root < s
    left[num_states] = root;
    right[num_states] = right[root];
    right[root] = -1;
  }
  num_states++;
  root = key;
#ifdef CHECK_DYNAMIC
  long prev = -1;
  if (num_states % check_freq == 0) CheckDynamic(root, prev);
#endif
  return root;
}

long splay_index_db::FindState(const int* state, int size)
{
  long key = states->AddState(state, size);
  if (is_static) {
    long ans = staticBinarySearch(key);
    states->PopLast(key);
    return ans;
  } else {
    int cmp = Splay(key);
    states->PopLast(key);
    if (0==cmp) return root;
    return -1;
  }
}

// ******************************************************************
// *                                                                *
// *                   splay_handle_db  methods                     *
// *                                                                *
// ******************************************************************

splay_handle_db::splay_handle_db(bool storesize) : splay_db(false, storesize)
{
}

splay_handle_db::~splay_handle_db()
{
}

long splay_handle_db::InsertState(const int* s, int np)
{
  if (is_static) {
    return staticBinarySearch(s, np);
  }
  // make space
  if (num_states >= nodes_alloc) {
    enlargeTree();
  }
  if (root<0) {
    // empty tree
    left[0] = -1;
    right[0] = -1;
    long key = states->AddState(s, np);
    root = num_states;
    CHECK_RANGE(0, root, nodes_alloc);
    index2handle[root] = key;
    DCASSERT(0 == root);
    num_states++;
    return root;
  }
  int cmp = Splay(s, np);
  if (0==cmp)  return root; // duplicate!
  if (cmp>0) {
    // root > s
    left[num_states] = left[root];
    right[num_states] = root;
    left[root] = -1;
  } else {
    // root < s
    left[num_states] = root;
    right[num_states] = right[root];
    right[root] = -1;
  }
  long key = states->AddState(s, np);
  root = num_states;
  CHECK_RANGE(0, root, nodes_alloc);
  index2handle[root] = key;
  num_states++;
  return root;
}

long splay_handle_db::FindState(const int* state, int size)
{
  if (is_static) {
    return staticBinarySearch(state, size);
  } else {
    int cmp = Splay(state, size);
    if (0==cmp) return root;
    return -1;
  }
}

