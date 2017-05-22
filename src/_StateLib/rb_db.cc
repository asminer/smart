
#include "rb_db.h"

#include <stdio.h>
#include <stdlib.h>


// ******************************************************************
// *                                                                *
// *                     redblack_db  methods                       *
// *                                                                *
// ******************************************************************

redblack_db::redblack_db(bool indexed, bool storesize)
 : bst_db(indexed, storesize)
{
#ifdef USE_COMPACTED_BITS
  red = (char*) malloc(nodes_alloc / 8);
#else
  red = (bool*) malloc(nodes_alloc * sizeof(bool));
#endif
}

redblack_db::~redblack_db()
{
  free(red);
}

long redblack_db::ReportMemTotal() const 
{
  long memsize = bst_db::ReportMemTotal();
  if (red) memsize += nodes_alloc / 8;
  return memsize;
}

void redblack_db::DotAttributes(FILE* out, long node) const
{
  if (is_red(node)) {
    fprintf(out, ",color=red");
  } else {
    fprintf(out, ",color=black");
  }
}

void redblack_db::finishInsert(int cmp)
{
  // add new leaf
  long child = num_states;
  left[child] = -1;
  right[child] = -1;
  long parent = Pop();
  DCASSERT(parent>=0);
  if (cmp>0) {
    DCASSERT(left[parent]<0);
    left[parent] = child;
  } else {
    DCASSERT(right[parent]<0);
    right[parent] = child;
  }

  // go back up tree, splitting 4-nodes, rotating as necessary.

  long grandp = Pop();
  long greatgp = Pop();
  for (;;) {

    // child is either a 4-node to be split, or a newly-inserted node
    set_red(child);
    if (child != num_states) {
      DCASSERT(left[child]>=0);
      DCASSERT(right[child]>=0);
      set_black(left[child]);
      set_black(right[child]);
    }

    // child is now red, check for illegal situations in ancestorsa
    if (parent<0) return;
    if (!is_red(parent)) return;

    // sanity checks...
    DCASSERT(grandp>=0); // otherwise parent cannot be red
    DCASSERT(!is_red(grandp));

    // parent and child are both red, rotation may be necessary...

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
      set_black(child);
      set_red(grandp);

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
      set_black(parent);
      set_red(grandp);

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

    // can break out now
    return;
  } // for (;;)
}


// ******************************************************************
// *                                                                *
// *                  redblack_index_db  methods                    *
// *                                                                *
// ******************************************************************

redblack_index_db::redblack_index_db(bool storesize)
: redblack_db(true, storesize)
{
}

redblack_index_db::~redblack_index_db()
{
}

long redblack_index_db::InsertState(const int* state, int size)
{
  long key = states->AddState(state, size);
  if (is_static) {
    long ans = staticBinarySearch(key);
    states->PopLast(key);
    if (ans<0) throw StateLib::error(StateLib::error::Static);
    return ans;
  }

  // make space
  if (num_states >= nodes_alloc) {
    enlargeTree();
#ifdef USE_COMPACTED_BITS
    red = (char*)realloc(red, nodes_alloc/8);
#else 
    red = (bool*) realloc(red, nodes_alloc * sizeof(bool));
#endif
    if (0==red) throw StateLib::error(StateLib::error::NoMemory);
  }

  // check empty tree case
  if (root<0) {
    DCASSERT(0==key);
    left[0] = -1;
    right[0] = -1;
    set_black(0);
    root = 0;
    num_states++;
    return 0;
  }

  int cmp = FindPath(key);
  if (0==cmp) {
    // duplicate!
    states->PopLast(key);
    return Pop();
  }

  finishInsert(cmp);

  // root remains black
  set_black(root);
  return num_states++;
}

long redblack_index_db::FindState(const int* state, int size)
{
  long key = states->AddState(state, size);
  if (is_static) {
    long ans = staticBinarySearch(key);
    states->PopLast(key);
    return ans;
  } else {
    int cmp = FindPath(key);
    states->PopLast(key);
    if (0==cmp) return Pop();
    return -1;
  }
}

// ******************************************************************
// *                                                                *
// *                  redblack_handle_db methods                    *
// *                                                                *
// ******************************************************************

redblack_handle_db::redblack_handle_db(bool storesize)
: redblack_db(false, storesize)
{
}

redblack_handle_db::~redblack_handle_db()
{
}

long redblack_handle_db::InsertState(const int* state, int size)
{
  if (is_static) {
    long ans = staticBinarySearch(state, size);
    if (ans<0) throw StateLib::error(StateLib::error::Static);
    return ans;
  }

  // make space
  if (num_states >= nodes_alloc) {
    enlargeTree();
#ifdef USE_COMPACTED_BITS
    red = (char*)realloc(red, nodes_alloc/8);
#else
    red = (bool*) realloc(red, nodes_alloc * sizeof(bool));
#endif
    if (0==red) throw StateLib::error(StateLib::error::NoMemory);
  }

  // check empty tree case
  if (root<0) {
    left[0] = -1;
    right[0] = -1;
    set_black(0);
    root = 0;
    index2handle[root] = states->AddState(state, size);
    num_states++;
    return 0;
  }

  int cmp = FindPath(state, size);
  if (0==cmp) return Pop(); // duplicate!

  index2handle[num_states] = states->AddState(state, size);
  finishInsert(cmp);

  // root remains black
  set_black(root);
  return num_states++;
}

long redblack_handle_db::FindState(const int* state, int size)
{
  if (is_static) {
    return staticBinarySearch(state, size);
  } else {
    int cmp = FindPath(state, size);
    if (0==cmp) return Pop();
    return -1;
  }
}

