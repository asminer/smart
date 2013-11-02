
// $Id$

#include <limits.h>
#include "hash_db.h"
#include <stdio.h>
#include <stdlib.h>

/// Standard MIN "macro".
template <class T> inline T MIN(T X,T Y) { return ((X<Y)?X:Y); }

// ******************************************************************
// *                                                                *
// *                       hash_db  methods                         *
// *                                                                *
// ******************************************************************

hash_db::hash_db(bool indexed, bool storesize) : state_db()
{
  states = new main_coll(indexed, storesize);

  hash_bits = 4;
  table = (long*) malloc (size()*sizeof(long));
  for (int i=size()-1; i>=0; i--) table[i] = -1;

  next_alloc = 16;
  next = (long*) malloc(next_alloc * sizeof(long));
  for (int i=next_alloc-1; i>=0; i--) next[i] = -1;

  if (!indexed)
    index2handle = (long*) malloc(next_alloc * sizeof(long));

#ifdef DEBUG_PERFORMANCE
  maxchain = 0;
  totalchain = 0;
  numchain = 0;
#endif
}

hash_db::~hash_db()
{
  free(table);
  free(next);
  delete states;
#ifdef DEBUG_PERFORMANCE
  fprintf(stderr, "\n\t\t\tHash table max chain: %ld\n", maxchain);
  double avg = totalchain;
  if (numchain) avg /= numchain;
  fprintf(stderr, "\t\t\tHash table avg chain: %lf\n", avg);
  fprintf(stderr, "\t\t\t           based on   %ld searches\n\n", numchain);
#endif
}

void hash_db::SetMaximumStackSize(long max_stack)
{
  // no stack required for hashing
}

long hash_db::GetMaximumStackSize() const
{
  return 0;
}

void hash_db::Clear()
{
  for (int i=size()-1; i>=0; i--) table[i] = -1;
  for (int i=next_alloc-1; i>=0; i--) next[i] = -1;
  num_states = 0;
  states->Clear();
}

void hash_db::ConvertToStatic(bool)
{
  // TBD: think about what makes sense here
}

void hash_db::ConvertToDynamic(bool)
{
  // TBD: and here
}

const StateLib::state_coll* hash_db::GetStateCollection() const
{
  return states;
}

StateLib::state_coll* hash_db::TakeStateCollection()
{
  StateLib::state_coll* ans = states;
  states = new main_coll(0==index2handle, ans->StateSizesAreStored());
  num_states = 0;
  return ans;
}

long hash_db::ReportMemTotal() const 
{
  long memsize = 0;
  memsize += size() * sizeof(long); // table itself
  memsize += next_alloc * sizeof(long); // next pointers
  if (index2handle) memsize += next_alloc * sizeof(long);
  if (states) memsize += states->ReportMemTotal();
  return memsize;
}

long hash_db::GetStateKnown(long index, int* state, int size) const
{
  if (index2handle)
    return states->GetStateKnown(index2handle[index], state, size);
  else
    return states->GetStateKnown(index, state, size);
}

int hash_db::GetStateUnknown(long index, int* state, int size) const
{
  if (index2handle)
    return states->GetStateUnknown(index2handle[index], state, size);
  else
    return states->GetStateUnknown(index, state, size);
}

void hash_db::DumpDot(FILE* out)
{
  fprintf(out, "digraph hash {\n\trankdir=LR;\n");
  fprintf(out, "\tnode [shape=record, width=.1, height=.1];\n\n");
  fprintf(out, "\ttable [label = \"<f0>0");
  for (long i=1; i<size(); i++) {
    fprintf(out, "|<f%ld>%ld", i, i);
  }
  fprintf(out, "\"];\n\n\tnode [width=0.5];\n");
  for (long i=0; i<num_states; i++) {
    fprintf(out, "\tnode%ld [label= \"{<n> %ld | <p>}\"];\n", i, i);
  }
  fprintf(out, "\n");
  
  for (long i=0; i<size(); i++) {
    if (table[i]<0) continue;
    fprintf(out, "\ttable:f%ld -> node%ld:n;\n", i, table[i]);
    long ptr = table[i];
    while (next[ptr]>0) {
      fprintf(out, "\tnode%ld:p -> node%ld:n;\n", ptr, next[ptr]);
      ptr = next[ptr];
    }
  }
  fprintf(out, "\n}\n");
}

// ******************************************************************
// *                                                                *
// *                    hash_index_db  methods                      *
// *                                                                *
// ******************************************************************

hash_index_db::hash_index_db(bool storesize) : hash_db(true, storesize)
{
}

hash_index_db::~hash_index_db()
{
}

long hash_index_db::InsertState(const int* s, int np)
{
  long key = states->AddState(s, np);
  DCASSERT(num_states == key);
  // make node space
  if (num_states >= next_alloc) {
    long newsize = MIN(2*next_alloc, next_alloc + MAX_NODE_ADD);
    next = (long*) realloc(next, newsize * sizeof(long));
    if (0==next) throw StateLib::error(StateLib::error::NoMemory);
    next_alloc = newsize;
  }
  CHECK_RANGE(0, key, next_alloc);
  // make hash table space
  if (needToExpand()) {
    hash_bits++;
    table = (long*) realloc(table, size() * sizeof(long));
    if (0==table) throw StateLib::error(StateLib::error::NoMemory);
    for (int i=size()-1; i>=0; i--) table[i] = -1;
    // rehash everything, but we know there are no duplicates
    for (int i=0; i<num_states; i++) {
      unsigned long h = states->Hash(i, hash_bits);
      CHECK_RANGE(0, h, size());
      next[i] = table[h];
      table[h] = i;
    }
  }
  // ok, hash key and see if there's a match
  unsigned long h = states->Hash(key, hash_bits);
  CHECK_RANGE(0, h, size());
  if (move_to_front(table[h], key)) {
    // duplicate entry
    states->PopLast(key);
  } else {
    // new entry
    next[key] = table[h];
    table[h] = key;
    num_states++;
  }
  return table[h];
}

long hash_index_db::FindState(const int* s, int np)
{
  long key = states->AddState(s, np);
  unsigned long h = states->Hash(key, hash_bits);
  CHECK_RANGE(0, h, size());
  long ans = move_to_front(table[h], key) ? table[h] : -1;
  states->PopLast(key);
  return ans;
}

bool hash_index_db::move_to_front(long &chain, long key)
{
  long prev = -1;
  long curr = chain;
#ifdef DEBUG_PERFORMANCE
  numchain++;
  long searches = 0;
#endif
  while (curr>=0) {
#ifdef DEBUG_PERFORMANCE
    searches++;
#endif
    if (states->CompareHH(key, curr)) {
      // no match, keep looking
      prev = curr;
      curr = next[curr];
    } else {
      if (prev>=0) {  // not yet at front
        next[prev] = next[curr];
        next[curr] = chain;
        chain = curr;
      }
#ifdef DEBUG_PERFORMANCE
      if (searches > maxchain) maxchain = searches;
      totalchain += searches;
#endif
      return true;
    }
  }
#ifdef DEBUG_PERFORMANCE
  if (searches > maxchain) maxchain = searches;
  totalchain += searches;
#endif
  return false;
}
