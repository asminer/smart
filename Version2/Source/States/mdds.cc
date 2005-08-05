
// $Id$

#include "mdds.h"
#include "../Base/errors.h"

node_manager::node_manager()
{
  a_size = 1024;
  addresses = (int*) malloc(a_size*sizeof(int));
  flags = (char*) malloc(a_size);
  a_last = 1;
  a_unused = a_unused_tail = 0;
  // just in case
  addresses[0] = 0;
  addresses[1] = 0;
  // Also useful
  flags[0] = flags[1] = Terminal;

  d_size = 1024;
  data = (int*) malloc(d_size*sizeof(int));
  d_last = 0;
  d_unused = 0;
  hole_slots = 0;
}

node_manager::~node_manager()
{
  free(addresses);
  free(flags);
  free(data);
}

void node_manager::Unlink(int p)
{
  if (p<2) return;
  DCASSERT(isActive(p));
  // decrement incoming count
  int* foo = data+addresses[p];
  DCASSERT(foo[0]>0);
  foo[0]--;
  if (foo[0]) return;
  if (foo[1]) return;  // still in a cache somewhere

  // recycle this node

  // done with children
  if (flags[p] & Sparse) {
    // Sparse encoding
    int* ptr = foo+4;
    for (int sz=foo[3]; sz; sz--) {
      Unlink(ptr[0]);
      ptr += 2; 
    }
    // Recycle node memory
    MakeHole(addresses[p], 4 + foo[3]*2);  
  } else {
    // Full encoding
    int* ptr = foo+4;
    for (int sz=foo[3]; sz; sz--) {
      Unlink(ptr[0]);
      ptr++;
    }
    // Recycle node memory
    MakeHole(addresses[p], 4 + foo[3]);  
  }

  // recycle the index
  FreeNode(p);
  flags[p] = Deleted;
}

int node_manager::TempNode(int k, int sz)
{
  DCASSERT(sz>0);
  int p = NextFreeNode();
  addresses[p] = FindHole(4+sz);
  flags[p] = 0; 
  int* foo = (data+addresses[p]);
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

inline int digits(int a) 
{
  int d = 1;
  while (a) { d++; a/=10; }
  return d;
}

void node_manager::Dump(OutputStream &s) const
{
  int nwidth = digits(a_last);
  s << "\nFirst free node: " << a_unused << "\n";
  s << "Nodes: \n#";
  s.Pad(' ', nwidth-1);
  s << " \tFlags\tAddr/next\n";
  int p;
  for (p=0; p<=a_last; p++) {
    s.flush();	
    s.Put(p, nwidth);
    s << " \t";
    if (flags[p] & Terminal) s << 'T'; else s << '.';  
    if (flags[p] & Deleted)  s << 'D'; else s << '.';
    if (flags[p] & Reduced)  s << 'R'; else s << '.';
    if (flags[p] & Sparse)   s << 'S'; else s << 'F';
    // other flags?
    s << "\t" << addresses[p] << "\n";
  } // for p
  
  s << "\nIndex of first hole: " << d_unused << "\n";
  s << "Array by record: \n";
  int awidth = digits(d_last);
  int y = d_unused;
  int a;
  for (a=1; a<=d_last; ) {
    s.flush();
    s.Put(a, awidth);
    s << ": ";
    if (a==y) {
      s << "hole with " << data[a+1] << " slots; next is " << data[a] << "\n";
      y = data[a];
      a+= data[a+1];
      continue;
    } 
    // must be a node
    // VERY slow, but for display, we don't care
    // Find the node index (necessary for flags)
    for (p=0; p<=a_last; p++) {
      if (flags[p] & Terminal) continue;
      if (flags[p] & Deleted) continue;
      if (addresses[p] == a) break;
    }
    DCASSERT(p<=a_last);
    s << "(node ";
    s.Put(p, nwidth);
    s << ")   in " << data[a];
    s << "\t cc " << data[a+1];
    s << "\t level " << data[a+2];
    s << "\t size " << data[a+3] << "\t";
    if (flags[p] & Sparse) {
      s << "  (";
      a += 4; 
      for (int nz=data[a-1]; nz; nz--) {
        s << data[a];
        a++;
        s << ":" << data[a];
 	a++;
        if (nz>1) s << ", ";
      }
      s << ")\n";
    } else {
      s << "  [";
      a += 4;
      for (int i=data[a-1]; i; i--) {
	s << data[a];
	a++;
        if (i>1) s << "|";
      }
      s << "]\n";
    }
  } // for a
  s.Put(a, awidth);
  s << ": End\n";
  

  // Some stats
  s << "\n" << d_last << " slots allocated, ";
  s << hole_slots << " slots in holes\n";
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
    a_size += 1024;
    addresses = (int*) realloc(addresses, a_size * sizeof(int));
    if (NULL==addresses)
      OutOfMemoryError("Too many MDD nodes");
    flags = (char*) realloc(flags, a_size);
    if (NULL==flags)
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

int node_manager::FindHole(int slots)
{
  const int min_node_size = 6;
  DCASSERT(slots>2);

  // look for a hole
  int prev = 0;
  int curr = d_unused;
  while (curr) {
    if (data[curr+1] == slots) {
      // perfect fit, remove hole completely
      if (prev) {
        data[prev] = data[curr];
      } else {
	d_unused = data[curr];
      }
      hole_slots -= slots;
      return curr;
    } 
    if (data[curr+1] >= slots + min_node_size) {
      // fits but creates another hole
      if (prev) {
        data[prev] = curr + slots;
      } else {
	d_unused = curr + slots;
      }
      data[curr+slots+1] = data[curr+1] - slots;  // new hole size
      hole_slots -= slots;
      return curr;
    }
    // this hole not large enough, try the next one
    prev = curr;
    curr = data[curr];
  } // while curr

  // can't recycle; grab from the end
  if (d_last + slots >= d_size) {
    // not enough space, extend
    int np = 1+ (slots / 1024);
    d_size += np * 1024;
    data = (int*) realloc(data, d_size);
    if (NULL==data)
      OutOfMemoryError("No space for MDD nodes");
  }
  int h = d_last+1;
  d_last += slots;
  return h;
}

void node_manager::MakeHole(int addr, int slots)
{
  hole_slots += slots;

  // search for hole placement
  int pp = 0;
  int prev = 0;
  int next = d_unused;
  while (next && next < addr) {
    pp = prev;
    prev = next;
    next = data[next];
  }

  // Deal with addr -> next

  if (addr+slots == next) {
    // addr->next is really one big hole; merge it
    slots += data[next+1];
    data[addr] = data[next];
  } else {
    // addr is separated from next, set up addr hole
    data[addr] = next;
  }
  data[addr+1] = slots;

  // Deal with prev -> addr

  if (prev) {
    if (prev + data[prev+1] == addr) {
      // prev->addr is one big hole; merge it
      data[prev+1] += slots;
      data[prev] = data[addr];
      // and for simplicity, make addr point to the new hole
      addr = prev;
      slots = data[prev+1];
      prev = pp;
    } else {
      // prev is separated from addr, set up pointers
      data[prev] = addr;
    }
  } else {
    // addr is front of list now
    d_unused = addr;
  }

  // if addr is the last hole, absorb into free part of array
  if (addr+slots-1 == d_last) {
    d_last -= slots;
    hole_slots -= slots;
    // remove last hole from list
    if (pp) {
      data[pp] = 0;
    } else {
      d_unused = 0;
    }
  }
}

