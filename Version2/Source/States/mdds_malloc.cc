
// $Id$

#include "mdds_malloc.h"
#include "../Base/errors.h"

const int Add_Size = 1024;

mdd_node_manager::mdd_node_manager()
{
  a_size = Add_Size;
  address = (int**) malloc(a_size*sizeof(int*));
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

  unique = new HashTable<mdd_node_manager> (this);
}

mdd_node_manager::~mdd_node_manager()
{
  delete unique;
  for (int i=2; i<=a_last; i++) {
    free(address[i]);
  }
  free(address);
  free(next);
}

int mdd_node_manager::Reduce(int p)
{
  DCASSERT(p>1);
  DCASSERT(next[p]<-1);

  // first, compress this node
  int size = address[p][3];
  int nnz = 0;
  int lnz = 0;
  int* ptr = address[p] + 4;
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
    int* newaddr = (int*) malloc((4+2*nnz)*sizeof(int));
    newaddr[0] = address[p][0];
    newaddr[1] = address[p][1];
    newaddr[2] = address[p][2];
    newaddr[3] = -nnz;
    int* newptr = newaddr + 4;
    for (int i=0; i<size; i++) {
      if (ptr[i]) {
        newptr[0] = i;
        newptr++;
        newptr[0] = ptr[i];
	newptr++;
      }
    }
    // trash old node
    free(address[p]);
    address[p] = newaddr;
  } else {
    // full is better
    if (lnz+1<size) {
      // truncate the trailing 0s
      address[p] = (int*) realloc(address[p], (5+lnz)*sizeof(int));
      DCASSERT(address[p]);
      address[p][3] = lnz+1;
    }
  }
  // check unique table here
  int q = unique->Insert(p);
  if (q!=p) { 
    Link(q);
    Unlink(p);
  }
  return q;
}

int mdd_node_manager::TempNode(int k, int sz)
{
  DCASSERT(sz>0);
  int p = NextFreeNode();
  address[p] = (int*) malloc((4+sz)*sizeof(int));
  next[p] = -5; 
  int* foo = address[p];
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

void mdd_node_manager::Dump(OutputStream &s) const
{
  int nwidth = digits(a_last);
  s << "\nFirst free node: " << a_unused << "\n";
  s << "Nodes: \n#";
  s.Pad(' ', nwidth-1);
  s << " \tNext\tin\tcc\tlevel\tsize\tData\n";
  int p;
  for (p=0; p<=a_last; p++) {
    s.flush();	
    s.Put(p, nwidth);
    s << " \t" << next[p];
    if (0==address[p]) {
      s << "\n";
      continue;
    }
    s << "\t" << address[p][0];
    s << "\t" << address[p][1];
    s << "\t" << address[p][2];
    s << "\t" << address[p][3];
    if (address[p][3]<0) {
      // sparse
      s << "  (";
      for (int nz=0; nz<-address[p][3]; nz++) {
        s << address[p][4+nz*2];
        s << ":" << address[p][4+nz*2+1];
        if (nz+1<-address[p][3]) s << ", ";
      }
      s << ")\n";
    } else {
      s << "  [";
      for (int i=0; i<address[p][3]; i++) {
	s << address[p][4+i];
        if (i+1<address[p][3]) s << "|";
      }
      s << "]\n";
    }
  } // for p
  s.flush();	
}

int mdd_node_manager::hash(int h, int M) const 
{
  return 0;
}

bool mdd_node_manager::equals(int h1, int h2) const 
{
  DCASSERT(h1);	
  DCASSERT(h2);
  int* ptr1 = address[h1] + 2;  // points to level of h1
  int* ptr2 = address[h2] + 2;  // points to level of h2
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

void mdd_node_manager::DeleteNode(int p)
{
  int* foo = address[p];
  DCASSERT(p>1);
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
    free(address[p]);
  } else {
    // Full encoding
    int* ptr = foo+4;
    for (int sz=foo[3]; sz; sz--) {
      Unlink(ptr[0]);
      ptr++;
    }
    // Recycle node memory
    free(address[p]);
  }

  // recycle the index
  FreeNode(p);
}

int mdd_node_manager::NextFreeNode()
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
    address = (int**) realloc(address, a_size * sizeof(int*));
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

void mdd_node_manager::FreeNode(int p)
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
}

