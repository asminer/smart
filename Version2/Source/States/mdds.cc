
// $Id$

#include "mdds.h"
#include "../Base/errors.h"

node_manager::node_manager()
{
  a_size = 256;
  addresses = (int*) malloc(a_size*sizeof(int));
  a_last = 1;
  a_unused = a_unused_tail = 0;
  // just in case
  addresses[0] = 0;
  addresses[1] = 0;

  d_size = 1024;
  data = (char*) malloc(d_size);
  d_last = 0;
  d_unused = 0;
}

node_manager::~node_manager()
{
  free(addresses);
  free(data);
}

int node_manager::TempNode(int k, int sz)
{
    DCASSERT(sz>0);
    int p = NextFreeNode();
    addresses[p] = FindHole(1+(4+sz)*sizeof(int));
    char* flags = data+addresses[p];
    *flags = 0;
    int* foo = (int*) (flags+1);
    foo[0] = 0; // #incoming
    foo++;
    foo[0] = 0; // #cache entries
    foo++;
    foo[0] = k; // level
    foo++;
    foo[0] = sz; // size
    // downward pointers
    for (; sz; sz--) {
      foo++;
      foo[0] = 0;
    }
}

inline int digits(int a) 
{
  int d = 1;
  while (a) { d++; a/=10; }
  return d;
}

void node_manager::Dump(OutputStream &s) const
{
  s << "Nodes: \n";
  int width = digits(a_last);
  int p;
  int x = a_unused;
  for (p=0; p<=a_last; p++) {
    s.Put(p, width);
    s << ": ";
    if (p<2) {
      s << "terminal\n";
      s.flush();	
      continue;
    }
    if (p==x) {
      s << "DELETED\n";
      x=addresses[x];  // next deleted
      s.flush();	
      continue;
    } 
    s << "addr " << addresses[p] << "\n";
    // show node details...
    s.flush();	
  } // for p
}

// ------------------------------------------------------------------
//  Protected methods
// ------------------------------------------------------------------

int node_manager::NextFreeNode()
{
  if (a_unused) {
    // grab a recycled index
    int p = a_unused;
    a_unused = addresses[p];
    if (0==a_unused) a_unused_tail = 0;
    return p;
  }
  // new index
  a_last++;
  if (a_last>=a_size) {
    a_size += 256;
    addresses = (int*) realloc(addresses, a_size * sizeof(int));
    if (NULL==addresses)
      OutOfMemoryError("Too many MDD nodes");
  }
  return a_last;
}

void node_manager::FreeNode(int p)
{
  DCASSERT(p>1);
  if (p==a_last) { 
    // special case
    a_last--;
    return;
  }
  if (p>a_unused_tail) {
    // Definitely the tail
    addresses[p] = 0;
    if (a_unused_tail) {
      // we're at the end of an existing list
      addresses[a_unused_tail] = p;
      a_unused_tail = p;
    } else {
      // empty list
      a_unused = a_unused_tail = p;
    }
    return;
  }
  // find spot in list
  int prev = 0;
  int next = a_unused;
  while (next && next<p) {
    prev = next;
    next = addresses[next];
  }
  addresses[p] = next;
  DCASSERT(next);  // we should have handled this case already
  if (prev) addresses[prev] = p;
  else a_unused = p;
}

int node_manager::FindHole(int bytes)
{
  DCASSERT(bytes>2*sizeof(int));
  if (d_unused) {
    // look for a hole; implement later
  }
  // can't recycle; grab from the end
  if (d_last + bytes >= d_size) {
    // not enough space, extend
    int np = 1+ (bytes / 1024);
    d_size += np * 1024;
    data = (char*) realloc(addresses, a_size);
    if (NULL==data)
      OutOfMemoryError("No space for MDD nodes");
  }
  int h = d_last+1;
  d_last += bytes;
  return h;
}

void node_manager::MakeHole(int addr, int bytes)
{
  // search for hole placement
  int pp = 0;
  int prev = 0;
  int next = d_unused;
  while (next && next < addr) {
    pp = prev;
    prev = next;
    int* foo = (int*) (data+next);
    next = foo[0];
  }

  // Deal with addr -> next

  int* ad = (int*) (data+addr);
  if (addr+bytes == next) {
    // addr->next is really one big hole; merge it
    int* nd = (int*) (data+next);
    bytes += nd[1];
    ad[1] = bytes;
    ad[0] = nd[0];
  } else {
    // addr is separated from next, set up addr hole
    ad[1] = bytes;
    ad[0] = next;
  }

  // Deal with prev -> addr

  if (prev) {
    // pp -> prev -> addr -> next
    int* pd = (int*) (data+prev);
    pd[0] = addr;
    
  } 
}

