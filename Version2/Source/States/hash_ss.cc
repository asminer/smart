
// $Id$

#include "hash_ss.h"
#include "../Base/memtrack.h"
#include "../Templates/heap.h"

/** @name hash_ss.cc
    @type File
    @args \ 

    Hash tables for generating sets of states.
    
 */

//@{ 


// ******************************************************************
// *                                                                *
// *                    hash_state_nodes methods                    *
// *                                                                *
// ******************************************************************

hash_state_nodes::hash_state_nodes(state_array* s)
{
  ALLOC("hash_state_nodes", sizeof(hash_state_nodes));
  states = s;
  next = new DataList <int> (32);
}

hash_state_nodes::~hash_state_nodes()
{
  delete next;
  FREE("hash_state_nodes", sizeof(hash_state_nodes));
}

// ******************************************************************
// *                                                                *
// *                        hash_sort  class                        *
// *                                                                *
// ******************************************************************

// Used to sort states
struct state_sorter {
  static state_array* states;
  int index;
// handy constructors
  state_sorter() { index = -1; }
  state_sorter(int i) { index = i; }
};

inline bool operator> (const state_sorter &a, const state_sorter &b)
{
  DCASSERT(a.states);
  return (a.states->Compare(a.index, b.index) > 0);
}

state_array* state_sorter::states = NULL;

// ******************************************************************
// *                                                                *
// *                      hash_states  methods                      *
// *                                                                *
// ******************************************************************

hash_states::hash_states(state_array *s) : nodes(s)
{
  ALLOC("hash_states", sizeof(hash_states));
  states = s;
  Table = new HashTable <hash_state_nodes>(&nodes);
}

hash_states::~hash_states()
{
  // don't delete states
  delete Table;
  FREE("hash_states", sizeof(hash_states));
}

void hash_states::FillOrderList(int* order)
{
  if (Verbose.IsActive()) {
    Verbose << "Sorting states\n";
    Verbose.flush();
  }
  // Heapsort the states
  state_sorter::states = states;
  HeapOfObjects <state_sorter> foo(states->NumStates());
  for (int i=0; i<states->NumStates(); i++) foo.Insert(state_sorter(i));
  foo.Sort();
  DCASSERT(foo.Size() == states->NumStates());
  state_sorter* A = foo.MakeArray();
  for (int i=0; i<states->NumStates(); i++) order[i] = A[i].index;
  free(A);
  state_sorter::states = NULL;
}

int hash_states::AddState(const state &s)
{
  int key = states->AddState(s);
  nodes.AddState(key);
  int thing = Table->Insert(key);
  if (thing != key) states->PopLast();
  return thing;
}

int hash_states::FindState(const state &s)
{
  int key = states->AddState(s);
  nodes.AddState(key);
  int thing = Table->Find(key);
  states->PopLast();
  return thing;
}

void hash_states::Report(OutputStream &r)
{
  r << "Hash table report for state space generation\n";
  r << "\tCurrent size: " << Table->Size() << "\n";
  r << "\tMax chain length seen: " << Table->MaxChain() << "\n";
  int total = 0;
  r << "\tTable bytes: " << int(Table->Size() * sizeof(int)) << "\n";
  total += Table->Size() * sizeof(int);
  r << "\tNodes allocated: " << int(nodes.next->alloc) << "\n";
  r << "\tNode bytes: " << int(nodes.next->alloc * sizeof(int)) << "\n";
  total += nodes.next->alloc * sizeof(int);
  r << "\tTotal memory: " << total << "\n";
}

//@}

