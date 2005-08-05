
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
  hole_bytes = 0;
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
    foo[0] = 1; // #incoming
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

void node_manager::DoneNode(int p)
{
  if (p<2) return;
  // decrement incoming count
  int* foo = (int*) (data+addresses[p]+1);
  DCASSERT(foo[0]>0);
  foo[0]--;
  if (foo[0]) return;
  if (foo[1]) return;  // still in a cache somewhere

  // recycle this node

  // done with children
  char flags = data[addresses[p]];
  if (flags & Sparse) {
    // Sparse encoding
    int* ptr = foo+4;
    for (int sz=foo[3]; sz; sz--) {
      DoneNode(ptr[0]);
      ptr += 2; 
    }
    // Recycle node memory
    MakeHole(addresses[p], 1+4*sizeof(int) + foo[3]*2*sizeof(int));  
  } else {
    // Full encoding
    int* ptr = foo+4;
    for (int sz=foo[3]; sz; sz--) {
      DoneNode(ptr[0]);
      ptr++;
    }
    // Recycle node memory
    MakeHole(addresses[p], 1+(4 + foo[3])*sizeof(int));  
  }

  // recycle the index
  FreeNode(p);
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
    s.flush();	
    s.Put(p, width);
    s << ": ";
    if (p<2) {
      s << "terminal\n";
      continue;
    }
    if (p==x) {
      s << "DELETED\n";
      x=addresses[x];  // next deleted
      continue;
    } 
    s << "addr " << addresses[p] << "\n";
  } // for p
  
  s << "Array by record: \n";
  width = digits(d_last);
  int y = d_unused;
  for (int a=1; a<=d_last; ) {
    s.flush();
    s.Put(a, width);
    s << ": ";
    if (a==y) {
      s << "hole ";
      int* d = (int*) (data+y);
      s << d[1] << " bytes\n";
      y = d[0];
      a+= d[1];
      continue;
    } 
    // must be a node
    int flags = data[a];
    s << "flags " << flags;
    int* foo = (int*) (data+a+1);
    s << "   in " << foo[0];
    s << "   cc " << foo[1];
    s << "   level " << foo[2];
    if (flags & Sparse) {
      s << "  (";
      for (int nz=0; nz<foo[3]; nz++) {
        if (nz) s << ", ";
        s << foo[4+nz*2] << ":" << foo[3+nz*2+1];
      }
      s << ")\n";
      a += 1+4*sizeof(int) + 2*foo[3]*sizeof(int);
    } else {
      s << "  [";
      for (int i=0; i<foo[3]; i++) {
        if (i) s << "|";
	s << foo[4+i];
      }
      s << "]\n";
      a += 1+(4+foo[3])*sizeof(int);
    }
  } // for a

  // Some stats
  s << "Node storage is " << d_last << " bytes with " << hole_bytes << " bytes in holes\n";
  s.flush();
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
  hole_bytes += bytes;

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
    int* pd = (int*) (data+prev);
    if (prev + pd[1] == addr) {
      // prev->addr is one big hole; merge it
      pd[1] += ad[1];
      pd[0] = ad[0];
      // and for simplicity, make addr point to the new hole
      addr = prev;
      bytes = pd[1]; 
      prev = pp;
    } else {
      // prev is separated from addr, set up pointers
      pd[0] = addr;
    }
  } else {
    // addr is front of list now
    d_unused = addr;
  }

  // if addr is the last hole, absorb into free part of array
  if (addr+bytes-1 == d_last) {
    d_last -= bytes;
    hole_bytes -= bytes;
    // remove last hole from list
    if (pp) {
      int* ppd = (int*) (data+pp);
      ppd[0] = 0;  
    } else {
      d_unused = 0;
    }
  }
}

