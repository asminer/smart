
// $Id$

#include "mdds.h"
#include "../Base/errors.h"

//#define DONT_USE_SPARSE
//#define DONT_USE_FULL

// #define MEMORY_TRACE

const int Add_Size = 1024;

node_manager::node_manager()
{
  a_size = Add_Size;
  address = (int*) malloc(a_size*sizeof(int));
  a_last = 1;
  a_unused = 0;
  // just in case
  address[0] = 0;
  address[1] = 0;
#ifdef DEVELOPMENT_CODE
  for (int f=2; f<a_size; f++) address[f] = 0;
#endif

  d_size = Add_Size;
  data = (int*) malloc(d_size*sizeof(int));
  d_last = 0;
  holes_top = holes_bottom = 0;
  max_slots = hole_slots = 0;
  active_nodes = 0;

  max_hole_chain = 0;

  unique = new HashTable<node_manager> (this);
}

node_manager::~node_manager()
{
  delete unique;
  free(address);
  free(data);
}

int node_manager::Reduce(int p)
{
  DCASSERT(p>1);
  DCASSERT(isNodeActive(p));
  DCASSERT(isNodeUnreduced(p));
  DCASSERT(isNodeFull(p));

  // first, compress this node
  int size = SizeOf(p);
  int nnz = 0;
  int truncsize = 0;
  int* ptr = data + address[p] + 5;
  for (int i=0; i<size; i++) {
    if (ptr[i]) {
      nnz++;
      truncsize = i;
    }
  }
  truncsize++;

  if (0==nnz) {
    // duplicate of 0
    Unlink(p);
    return 0;
  }

#ifdef DONT_USE_SPARSE
  nnz = size; // cheat
#endif
#ifdef DONT_USE_FULL
  truncsize = 2*nnz+1;
#endif

  // right now, tie goes to truncated full.
  if (2*nnz < truncsize) {
    // sparse is better; convert
    int newaddr = FindHole(5+2*nnz);
    // incount
    data[newaddr] = data[address[p]];
    // cachecount
    data[newaddr+1] = data[address[p]+1];
    // don't bother with next
    // level
    data[newaddr+3] = data[address[p]+3];
    // size
    data[newaddr+4] = -nnz;
    int* indexptr = data + newaddr + 5;
    int* downptr = data + newaddr + 5 + nnz;
    // can't rely on previous ptr
    int* ptr = data + address[p] + 5;
    for (int i=0; i<size; i++) {
      if (ptr[i]) {
        indexptr[0] = i;
        indexptr++;
        downptr[0] = ptr[i];
	downptr++;
      }
    }
    // trash old node
    MakeHole(address[p], 5+size);
    address[p] = newaddr;
  } else {
    // full is better
    if (truncsize<size) {
      // truncate the trailing 0s
      int newaddr = FindHole(5+truncsize);
      // incount
      data[newaddr] = data[address[p]];
      // cachecount
      data[newaddr+1] = data[address[p]+1];
      // don't bother with next
      // level
      data[newaddr+3] = data[address[p]+3];
      // size
      data[newaddr+4] = truncsize;
      // elements
      memcpy(data+newaddr+5, data+address[p]+5, truncsize*sizeof(int));
      // trash old node
      MakeHole(address[p], 5+size);
      address[p] = newaddr;
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

int node_manager::TempNode(int k, int sz)
{
  DCASSERT(sz>0);
  int p = NextFreeNode();
  address[p] = FindHole(5+sz);
  int* foo = (data+address[p]);
  foo[0] = 1;	// #incoming
  foo[1] = 0;   // cache count
  foo[2] = -5;	// unreduced next
  foo[3] = k;	// level
  foo[4] = sz;	// size
  // zero out the downward pointers
  bzero(foo+5, sz*sizeof(int)); 
  //
#ifdef TRACK_DELETIONS
  Output << "Creating node " << p << "\n";
  Output.flush();
#endif
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
  for (int p=0; p<=a_last; p++) {
    s.Put(p, nwidth);
    s << "\t";
    ShowNode(s, p);
    s << "\n";
    s.flush();
  }
}

void node_manager::DumpInternal(OutputStream &s) const
{
  s << "Internal forest storage\n";
  s << "First unused node index: " << a_unused << "\n";
  int awidth = digits(d_last);
  s << " Node# :  ";
  for (int p=0; p<=a_last; p++) {
    if (p) s << " ";
    s.Put(p, awidth);
  }
  s << "\n";
  s << "Address: [";
  for (int p=0; p<=a_last; p++) {
    if (p) s << "|";
    s.Put(address[p], awidth);
  }
  s << "]\n\n";

  s << "Last slot used: " << d_last << "\n";
  s << "Grid: top = " << holes_top << "\t bottom = " << holes_bottom << "\n";
  s << "Data array by record: \n";
  int a;
  for (a=1; a<=d_last; ) {
    s.flush();
    s.Put(a, awidth);
    s << " : [" << data[a];
    for (int i=1; i<5; i++) {
      s << "|" << data[a+i];
    }
    if (data[a]<0) { 
      // hole
      s << "| ... |";
      a -= data[a];  
      s << data[a-1] << "]\n";
    } else {
      // proper node
      if (data[a+4]>0) {
        // Full storage
        for (int i=0; i<data[a+4]; i++) {
          s << "|" << data[a+5+i];
        }
        a += 5+data[a+4];
        s << "]\n";
      } else {
        // sparse storage
        for (int i=0; i<-2*data[a+4]; i++) {
          s << "|" << data[a+5+i];
        }
        a += 5-2*data[a+4];
        s << "]\n";
      }
    }
  } // for a
  s.Put(a, awidth);
  s << " : free slots\n";
  s.flush();
  DCASSERT(a == d_last+1);
}

void node_manager::ShowNode(OutputStream &s, int p) const
{
  if (p<2) {
    s << "(terminal)";
    return;
  }
  if (isNodeDeleted(p)) {
    s << "DELETED";
    return;
  }
  int a = address[p];
  s << "in: " << data[a];
  s << " cc: " << data[a+1];
  s << " level: " << ABS(data[a+3]);
  if (data[a+3]<0) s << "'";
  if (isNodeSparse(p)) {
      // sparse
      s << "  nnz: " << nnzOf(p) << " \t (";
      for (int z=0; z<nnzOf(p); z++) {
        if (z) s << ", ";
        s << SparseIndex(p, z);
        s << ":" << SparseDown(p, z);
      }
      s << ")";
  } else {
      s << " size: " << SizeOf(p) << " \t [";
      for (int i=0; i<SizeOf(p); i++) {
        if (i) s << "|";
	s << FullDown(p, i);
      }
      s << "]";
  }
}

int node_manager::hash(int h, int M) const 
{
  DCASSERT(h);
  DCASSERT(M);
  int sz = data[address[h]+4];
  int a = 0;
  if (sz>0) {
    int* ptr = data + address[h] + 4 + sz;
    int ops = -2;
    int skip = 1;
    for (int i=sz; i>0; i-=skip) {
      a *= 256;
      a += ptr[0];  
      a %= M;
      ops++;
      if (ops > 2*skip) {  // accelerate through huge nodes
        skip++;
      } 
      ptr -= skip;
    }
  }  else {
    int* ptr = data + address[h] + 4 - 2*sz;
    int ops = -2;
    int skip = 1;
    for (int i=-sz; i>0; i-=skip) {
      a *= 256;
      a += ptr[0];  
      a %= M;
      ops++;
      if (ops > 2*skip) {  // accelerate through huge nodes
        skip++;
      } 
      ptr -= 2*skip;
    }
  }
  return a;
}

bool node_manager::equals(int h1, int h2) const 
{
  DCASSERT(h1);	
  DCASSERT(h2);
  int* ptr1 = data + address[h1] + 3;  // points to level of h1
  int* ptr2 = data + address[h2] + 3;  // points to level of h2
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

void node_manager::DeleteNode(int p)
{
  DCASSERT(p>1);
  active_nodes--;
  int* foo = data + address[p];
  if (isNodeReduced(p)) {
#ifdef DEELOPMENT_CODE 
    int x = unique->Remove(p);
    DCASSERT(x==p);
#else
    unique->Remove(p);
#endif
  }

  // done with children
  if (foo[4]<0) {
    // Sparse encoding
    int* downptr = foo + 5 - foo[4];
    int* stop = downptr - foo[4];
    for (; downptr < stop; downptr++) {
      Unlink(downptr[0]);
    }
    // Recycle node memory
    MakeHole(address[p], 5 -2*foo[4]);  
  } else {
    // Full encoding
    int* downptr = foo + 5;
    int* stop = downptr + foo[4];
    for (; downptr < stop; downptr++) {
      Unlink(downptr[0]);
    }
    // Recycle node memory
    MakeHole(address[p], 5 + foo[4]);  
  }

  // recycle the index
  FreeNode(p);
}

int node_manager::NextFreeNode()
{
  active_nodes++;
  if (a_unused) {
    // grab a recycled index
    int p = a_unused;
    DCASSERT(address[p]<1);
    a_unused = -address[p];
    return p;
  }
  // new index
  a_last++;
  if (a_last>=a_size) {
    a_size += Add_Size;
    address = (int*) realloc(address, a_size * sizeof(int));
    if (NULL==address)
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
  address[p] = -a_unused;
  a_unused = p;
}

void node_manager::GridInsert(int p)
{
  DCASSERT(data[p] == data[p-data[p]-1]);
  // special case: empty
  if (0==holes_bottom) {
    data[p+1] = data[p+2] = data[p+3] = data[p+4] = 0;
    holes_top = holes_bottom = p;
    return;
  }
  // special case: at top
  if (data[p] < data[holes_top]) {
    data[p+1] = data[p+3] = data[p+4] = 0;
    data[p+2] = holes_top;
    data[holes_top+1] = p;
    holes_top = p;
    return;
  }
  int chain = 0;
  int above = holes_bottom;
  int below = 0;
  while (data[p] < data[above]) {
    below = above;
    above = data[below+1];
    chain++;
    DCASSERT(data[above+2] == below);
    DCASSERT(above);  
  }
  max_hole_chain = MAX(max_hole_chain, chain);
  if (data[p] == data[above]) {
    // Found, add this to chain
    int right = data[above+4];
    data[p+3] = above;
    data[p+4] = right;
    if (right) data[right+3] = p;
    data[above+4] = p;
    return; 
  }
  // we should have above < p < below  (remember, -sizes)
  data[p+1] = above;
  data[p+2] = below;
  data[p+3] = data[p+4] = 0;
  data[above+2] = p;
  if (below) {
    data[below+1] = p;
  } else {
    DCASSERT(above == holes_bottom);
    holes_bottom = p;
  }
}

void node_manager::IndexRemove(int p)
{
  DCASSERT(data[p+3]==0);
  int above = data[p+1];
  int below = data[p+2];
  int right = data[p+4];
  if (right) {
    // right will replace us as index node
    data[right+1] = above;
    data[right+2] = below;
    data[right+3] = 0;
    if (above) {
      data[above+2] = right;
    } else {
      holes_top = right;
    }
    if (below) {
      data[below+1] = right;
    } else {
      holes_bottom = right;
    }
  } else {
    // no replacement, this size is gone
    if (above) {
      data[above+2] = below;
    } else {
      holes_top = below;
    }
    if (below) {
      data[below+1] = above;
    } else {
      holes_bottom = above;
    }
  }
}

int node_manager::FindHole(int slots)
{
  const int min_node_size = 6;
  DCASSERT(slots>=min_node_size);

  // First, try for a hole exactly of this size
  int chain = 0;
  int curr = holes_bottom;
  while (curr) {
    if (slots == -data[curr]) break;
    if (slots < -data[curr]) {
      // no exact match possible
      curr = 0;
      break;
    }
    curr = data[curr+1];
    chain++;
  }
  max_hole_chain = MAX(max_hole_chain, chain);
  if (curr) {
    // perfect fit
    hole_slots -= slots;
    // try to not remove the "index" node
    int next = data[curr+4];
    if (next) {
      MidRemove(next);
#ifdef MEMORY_TRACE
      Output << "Removed Hole " << next << "\n";
      DumpInternal(Output);
#endif
      return next;
    }
    IndexRemove(curr);
#ifdef MEMORY_TRACE
    Output << "Removed Hole " << curr << "\n";
    DumpInternal(Output);
#endif
    return curr;
  }
   
  // No hole with exact size, try the largest hole
  curr = holes_top;
  if (slots < -data[curr] - min_node_size) {
    // we have a hole large enough
    hole_slots -= slots;
    if (data[curr+4]) {
      // remove middle node
      curr = data[curr+4];
      MidRemove(curr);
    } else {
      // remove index node
      IndexRemove(curr);
    }
    // create a hole for the leftovers
    int newhole = curr+slots;
    int newsize = -data[curr] - slots;
    data[newhole] = -newsize;
    data[newhole+newsize-1] = -newsize;
    GridInsert(newhole); 
#ifdef MEMORY_TRACE
    data[curr] = -slots;  // only necessary for display
    Output << "Removed part of hole " << curr << "\n";
    DumpInternal(Output);
#endif
    return curr;
  }

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
  max_slots = MAX(max_slots, d_last);
  return h;
}


void node_manager::MakeHole(int addr, int slots)
{
#ifdef MEMORY_TRACE
  Output << "Calling MakeHole(" << addr << ", " << slots << ")\n";
#endif
  hole_slots += slots;
  data[addr] = data[addr+slots-1] = -slots;

  // Check for a hole to the left
  if (data[addr-1]<0) {
    // Merge!
    int lefthole = addr + data[addr-1];
    DCASSERT(data[lefthole] == data[addr-1]);
    if (data[lefthole+3]) MidRemove(lefthole);
    else IndexRemove(lefthole);
    slots += (-data[lefthole]);
    addr = lefthole;
    data[addr] = data[addr+slots-1] = -slots;
  }
  
  // if addr is the last hole, absorb into free part of array
  if (addr+slots-1 == d_last) {
    d_last -= slots;
    hole_slots -= slots;
#ifdef MEMORY_TRACE
    Output << "Made Last Hole " << addr << "\n";
    DumpInternal(Output);
#endif
    return;
  }

  // Check for a hole to the right
  if (data[addr+slots]<0) {
    // Merge!
    int righthole = addr+slots;
    if (data[righthole+3]) MidRemove(righthole);
    else IndexRemove(righthole);
    slots += (-data[righthole]);
    data[addr] = data[addr+slots-1] = -slots;
  }

  // Add hole to grid
  GridInsert(addr);

#ifdef MEMORY_TRACE
  Output << "Made Hole " << addr << "\n";
  DumpInternal(Output);
#endif
}

