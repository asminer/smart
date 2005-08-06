
// $Id$

#include "mdds.h"
#include "../Base/errors.h"

const int Add_Size = 1024;

node_manager::node_manager()
{
  a_size = Add_Size;
  address = (int*) malloc(a_size*sizeof(int));
  next = (int*) malloc(a_size*sizeof(int));
  a_last = 1;
  a_unused = 0;
  // just in case
  address[0] = 0;
  address[1] = 0;
  next[0] = 0;
  next[1] = 1;
#ifdef DEVELOPMENT_CODE
  for (int f=2; f<a_size; f++) address[f] = 0;
#endif

  d_size = Add_Size;
  data = (int*) malloc(d_size*sizeof(int));
  d_last = 0;
  d_unused = 0;
  hole_slots = 0;

  unique = new HashTable<node_manager> (this);
}

node_manager::~node_manager()
{
  delete unique;
  free(address);
  free(next);
  free(data);
}

void node_manager::Unlink(int p)
{
  if (p<2) return;
  DCASSERT(isNodeActive(p));
  // decrement incoming count
  int* foo = data+address[p];
  DCASSERT(foo[0]>0);
  foo[0]--;
  if (foo[0]) return;
  if (foo[1]) return;  // still in a cache somewhere

  // recycle this node
  if (next[p]>-2) {
#ifdef DEELOPMENT_CODE 
    int x = unique->Remove(p);
    DCASSERT(x==p);
#else
    unique->Remove(p);
#endif
  }

  // done with children
  if (foo[3]<0) {
    // Sparse encoding
    int* ptr = foo+5;
    for (int sz = foo[3]; sz; sz++) {
      Unlink(ptr[0]);
      ptr += 2; 
    }
    // Recycle node memory
    MakeHole(address[p], 4 -2*foo[3]);  
  } else {
    // Full encoding
    int* ptr = foo+4;
    for (int sz=foo[3]; sz; sz--) {
      Unlink(ptr[0]);
      ptr++;
    }
    // Recycle node memory
    MakeHole(address[p], 4 + foo[3]);  
  }

  // recycle the index
  FreeNode(p);
}

int node_manager::Reduce(int p)
{
  DCASSERT(p>1);
  DCASSERT(next[p]<-1);

  // first, compress this node
  int size = data[address[p]+3];
  int nnz = 0;
  int lnz = 0;
  int* ptr = data + address[p] + 4;
  for (int i=0; i<size; i++) {
    if (ptr[i]) {
      nnz++;
      lnz = i;
    }
  }

  if (0==nnz) {
    // duplicate of 0
    Unlink(p);
    return 0;
  }

  if (2*nnz < lnz) {
    // sparse is better; convert
    int newaddr = FindHole(4+2*nnz);
    data[newaddr] = data[address[p]];
    data[newaddr+1] = data[address[p]+1];
    data[newaddr+2] = data[address[p]+2];
    data[newaddr+3] = -nnz;
    int* newptr = data + newaddr + 4;
    for (int i=0; i<size; i++) {
      if (ptr[i]) {
        newptr[0] = i;
        newptr++;
        newptr[0] = ptr[i];
	newptr++;
      }
    }
    // trash old node
    MakeHole(address[p], 4+size);
    address[p] = newaddr;
  } else {
    // full is better
    if (lnz+1<size) {
      // truncate the trailing 0s
      int newaddr = FindHole(4+lnz+1);
      data[newaddr] = data[address[p]];
      data[newaddr+1] = data[address[p]+1];
      data[newaddr+2] = data[address[p]+2];
      data[newaddr+3] = lnz+1;
      memcpy(data+newaddr+4, ptr, (1+lnz)*sizeof(int));
      MakeHole(address[p], 4+size);
      address[p] = newaddr;
    }
  }
  // check unique table here
  int q = unique->Insert(p);
  if (q!=p) Unlink(p);
  return q;
}

int node_manager::TempNode(int k, int sz)
{
  DCASSERT(sz>0);
  int p = NextFreeNode();
  address[p] = FindHole(4+sz);
  next[p] = -5; 
  int* foo = (data+address[p]);
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
  return p;
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
  s << " \tAddress\tNext\n";
  int p;
  for (p=0; p<=a_last; p++) {
    s.flush();	
    s.Put(p, nwidth);
    s << " \t" << address[p] << "\t" << next[p] << "\n";
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
      if (address[p] == a) break;
    }
    if (p>a_last) {
      s << " lost node?\n";
      return;
    }
    DCASSERT(p<=a_last);
    s << "(node ";
    s.Put(p, nwidth);
    s << ")   in " << data[a];
    s << "\t cc " << data[a+1];
    s << "\t level " << data[a+2];
    s << "\t size " << data[a+3] << "\t";
    if (data[a+3]<0) {
      // sparse
      s << "  (";
      a += 4; 
      for (int nz=data[a-1]; nz; nz++) {
        s << data[a];
        a++;
        s << ":" << data[a];
 	a++;
        if (nz<-1) s << ", ";
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

int node_manager::hash(int h, int M) const 
{
  return 0;
}

bool node_manager::equals(int h1, int h2) const 
{
  DCASSERT(h1);	
  DCASSERT(h2);
  int* ptr1 = data + address[h1] + 2;  // points to level of h1
  int* ptr2 = data + address[h2] + 2;  // points to level of h2
  if (ptr1[0] != ptr2[0]) return false;
  int sz1 = ptr1[1];
  int sz2 = ptr2[1];
  if (sz1 != sz2) return false;
  if (sz1<0) {
    // both sparse
    return 0==memcmp(ptr1+2, ptr2+2, -2*sz1*sizeof(int));
  }
  // both full
  return 0==memcmp(ptr1+2, ptr2+2, sz1*sizeof(int));
}

// ------------------------------------------------------------------
//  Protected methods
// ------------------------------------------------------------------

int node_manager::NextFreeNode()
{
  if (a_unused) {
    // grab a recycled index
    int p = a_unused;
    a_unused = next[p];
    DCASSERT(0==address[p]);
    return p;
  }
  // new index
  a_last++;
  if (a_last>=a_size) {
    a_size += Add_Size;
    address = (int*) realloc(address, a_size * sizeof(int));
    if (NULL==address)
      OutOfMemoryError("Too many MDD nodes");
    next = (int*) realloc(next, a_size * sizeof(int));
    if (NULL==next)
      OutOfMemoryError("Too many MDD nodes");
#ifdef DEVELOPMENT_CODE
    for (int f=a_last; f<a_size; f++) address[f] = 0;
#endif
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
  next[p] = a_unused;
  address[p] = 0;
  a_unused = p;

/* Old way: Sorted

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
*/
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
    int np = 1+ (slots / Add_Size);
    d_size += np * Add_Size;
    data = (int*) realloc(data, d_size*sizeof(int));
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

