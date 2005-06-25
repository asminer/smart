
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

state_array* hash_states_node::states = NULL;
hash_states_node* hash_states_node::ptr = NULL;

// ******************************************************************
// *                                                                *
// *                      hash_states  methods                      *
// *                                                                *
// ******************************************************************

hash_states::hash_states(state_array *s)
{
  ALLOC("hash_states", sizeof(hash_states));
  states = s;
  nodes = new DataList <hash_states_node> (32);
  Table = new HashTable <hash_states_node>;
}

hash_states::~hash_states()
{
  // don't delete states
  delete Table;
  delete nodes;
  FREE("hash_states", sizeof(hash_states));
}

void hash_states::FillOrderList(int* order)
{
  if (Verbose.IsActive()) {
    Verbose << "Sorting states\n";
    Verbose.flush();
  }
  // remove everything from hash table
  Table->ConvertToList();
  // yank the node array
  hash_states_node* array = nodes->CopyAndClearArray();
  // hack: use the next pointer as the value to sort
  for (int i=0; i<states->NumStates(); i++) 
    array[i].next = array + i;
  // heapsort
  HeapOfObjects <hash_states_node> foo(array, states->NumStates());
  foo.Sort();
  for (int i=0; i<states->NumStates(); i++)
    order[i] = array[i].next->Index();
}

int hash_states::AddState(const state &s)
{
  int key = states->AddState(s);
  if (key >= nodes->Length()) nodes->AppendBlank(); 
  DCASSERT(nodes->alloc > key);

  nodes->data[key].states = states;
  nodes->data[key].ptr = nodes->data;
  nodes->data[key].next = NULL;

  hash_states_node* thing = Table->Insert(nodes->data + key);
  if (thing->Index() != key) {
    states->PopLast();
  }
  return thing->Index();
}

int hash_states::FindState(const state &s)
{
  int key = states->AddState(s);
  if (key >= nodes->Length()) nodes->AppendBlank(); 
  DCASSERT(nodes->alloc > key);

  nodes->data[key].states = states;
  nodes->data[key].ptr = nodes->data;
  nodes->data[key].next = NULL;

  hash_states_node* thing = Table->Insert(nodes->data + key);
  states->PopLast();

  if (thing->Index() != key) {
    return -1;
  }
  return thing->Index();
}

void hash_states::Report(OutputStream &r)
{
  r << "Hash table report for state space generation\n";
}

//@}

