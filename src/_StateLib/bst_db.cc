
#include "bst_db.h"

#include <stdio.h>

/// Standard MIN "macro".
template <class T> inline T MIN(T X,T Y) { return ((X<Y)?X:Y); }

// ******************************************************************
// *                                                                *
// *                        bst_db methods                          *
// *                                                                *
// ******************************************************************

void bst_db::allocPath()
{
  if (path) return;
  path = new Stack<long>(path_minsize, path_maxsize);
}

bst_db::bst_db(bool indexed, bool storesize) : state_db() 
{
  states = new main_coll(indexed, storesize);
  nodes_alloc = 16;
  left = (long*) malloc(nodes_alloc * sizeof(long));
  right = (long*) malloc(nodes_alloc * sizeof(long));
  if (!indexed)
    index2handle = (long*) malloc(nodes_alloc * sizeof(long));
  root = -1;
  order = 0;
  path_maxsize = default_path_maxsize;
  path = 0;
  allocPath();
#ifdef DEBUG_PERFORMANCE
  max_search = 0;
  total_search = 0;
  num_search = 0;
#endif
}

bst_db::~bst_db()
{
  free(left);
  free(right);
  free(order);
  delete path;
  delete states;
#ifdef DEBUG_PERFORMANCE
  if (max_search) {
    fprintf(stderr, "\n\t\t\tBST max search: %ld\n", max_search);
    double avg = total_search;
    if (num_search) avg /= num_search;
    fprintf(stderr, "\t\t\tBST avg search: %lf\n", avg);
    fprintf(stderr, "\t\t\t     based on   %ld searches\n\n", num_search);
  }
#endif
}

void bst_db::SetMaximumStackSize(long max_stack)
{
  path_maxsize = max_stack;
  if (path->SetMaxSize(path_maxsize))  return;
  throw StateLib::error(StateLib::error::NoMemory);
}

long bst_db::GetMaximumStackSize() const
{
  return path_maxsize;
}

void bst_db::Clear()
{
  if (is_static) return;

  root = -1;
  num_states = 0;
  states->Clear();
}

void bst_db::ConvertToStatic(bool tighten)
{
  if (is_static) return;
  is_static = true;

  if (0==num_states)  return;

  order = (long*) realloc(order, num_states * sizeof(long));
  if (0==order) throw StateLib::error(StateLib::error::NoMemory);

#ifdef DEBUG_STATIC
  printf("Converting tree to static mode\n");
  printf(" root: %ld\n", root);
  printf(" left: [%ld", left[0]);
  for (long s=1; s<num_states; s++) printf(", %ld", left[s]);
  printf("]\nright: [%ld", right[0]);
  for (long s=1; s<num_states; s++) printf(", %ld", right[s]);
  printf("]\n");
#endif

  // non-recursive, inorder traversal using the stack
  long slot = 0;
  path->Clear();
  path->Push(-1);
  long i = root;
  while (i>=0) {
    if (left[i] >= 0) {
      path->Push(i);
      i = left[i];
      continue;
    }
    while (i>=0) {
      order[slot] = i;
      slot++;
      if (right[i] >= 0) {
        i = right[i];
        break;
      }
      i = path->Pop();
    } // inner while
  } // outer while

  if (tighten) {
    free(left);
    free(right);
    left = 0;
    right = 0;
    nodes_alloc = num_states;
    if (index2handle)
      index2handle = (long*) realloc(index2handle, nodes_alloc * sizeof(long));
  }

#ifdef DEBUG_STATIC
  printf("order: [%ld", order[0]);
  for (long s=1; s<num_states; s++) printf(", %ld", order[s]);
  printf("]\n");
  printf("Done static conversion\n");
#endif

  delete path; 
  path = 0;
}

const StateLib::state_coll* bst_db::GetStateCollection() const
{
  return states;
}

StateLib::state_coll* bst_db::TakeStateCollection()
{
  StateLib::state_coll* ans = states;
  states = new main_coll(0==index2handle, ans->StateSizesAreStored());
  root = -1;
  num_states = 0;
  return ans;
}

long bst_db::ReportMemTotal() const 
{
  long memsize = 0;
  if (path) memsize += path->BytesUsed();
  if (is_static) {
    memsize += num_states * sizeof(long); // order array
  } else {
    memsize += 2 * nodes_alloc * sizeof(long);  // left & right arrays
  }
  if (index2handle) memsize += nodes_alloc * sizeof(long);
  if (states) memsize += states->ReportMemTotal();
  return memsize;
}

long bst_db::GetStateKnown(long index, int* state, int size) const
{
  if (index2handle)
    return states->GetStateKnown(index2handle[index], state, size);
  else
    return states->GetStateKnown(index, state, size);
}

int bst_db::GetStateUnknown(long index, int* state, int size) const
{
  if (index2handle)
    return states->GetStateUnknown(index2handle[index], state, size);
  else
    return states->GetStateUnknown(index, state, size);
}

const unsigned char* bst_db::GetRawState(long index, long &bytes) const
{
  if (index2handle)
    return states->GetRawState(index2handle[index], bytes);
  else
    return states->GetRawState(index, bytes);
}



void bst_db::DumpDot(FILE* out)
{
  if (is_static) return;
  if (0==num_states) return;

  fprintf(out, "digraph bst {\n\tnode [shape=record];\n");
  for (long i=0; i<num_states; i++) {
    fprintf(out, "\tnode%ld [label = \"<f0> | <f1> %ld |<f2>\"", i, i);
    DotAttributes(out, i);
    fprintf(out, "];\n");
  }

  // non-recursive, inorder traversal using the stack
  path->Clear();
  path->Push(-1);
  long i = root;
  while (i>=0) {
    // visit node i
    if (left[i] >= 0) 
      fprintf(out, "\t\"node%ld\":f0 -> \"node%ld\":f1;\n", i, left[i]);
    if (right[i] >= 0) 
      fprintf(out, "\t\"node%ld\":f2 -> \"node%ld\":f1;\n", i, right[i]);
    // deal with children
    if (left[i] >= 0) {
      path->Push(i);
      i = left[i];
      continue;
    }
    while (i>=0) {
      if (right[i] >= 0) {
        i = right[i];
        break;
      }
      i = path->Pop();
    } // inner while
  } // outer while
  fprintf(out, "}\n\n");
}

/*
void bst_db::DumpState(FILE* s, long index) const
{
  fprintf(s, "Internal for state %ld: ", index);
  if (index2handle)   states->DumpState(s, index2handle[index]);
  else                states->DumpState(s, index);
  fprintf(s, "\n");
}
*/

void bst_db::DotAttributes(FILE* out, long node) const
{
}

void bst_db::enlargeTree()
{
  if (num_states < nodes_alloc) return;
  long newsize = MIN(2*nodes_alloc, nodes_alloc + MAX_TREE_ADD);
  left = (long*) realloc(left, newsize*sizeof(long));
  right = (long*) realloc(right, newsize*sizeof(long));
  if (0==left || 0==right) throw StateLib::error(StateLib::error::NoMemory);
  if (index2handle) {
    index2handle = (long*) realloc(index2handle, newsize*sizeof(long));
    if (0==index2handle) throw StateLib::error(StateLib::error::NoMemory);
  }
  nodes_alloc = newsize;
}
