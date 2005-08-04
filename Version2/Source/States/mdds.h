
// $Id$

#include "../defines.h"
#include "../Base/streams.h"

/* Temporary, until we have a proper mdd library. */

const char Marked = 0x01;
const char Reduced = 0x02;
const char Sparse = 0x04;

class node_manager {
  /** For each node, its index in the data array. */
  int* addresses;
  /// Size of addresses array.
  int a_size;
  /// Last used address.
  int a_last;
  /// Pointer to unused address list.
  int a_unused;
  /// Tail of unused address list.
  int a_unused_tail;
  
  /** Data for each node. 
      The following format is used:
 	1 byte: flags
	1 int: incoming reference count
	1 int: number of cache entries
	1 int: level
	1 int: #full entries / #nonzeroes
	entries follow...
  */
  char* data;
  /// Size of data array.
  int d_size;
  /// Last used data slot. 
  int d_last;
  /// Pointer to data holes list.
  int d_unused;

public:
  node_manager();
  ~node_manager();

  inline void Link(int p) { 
    if (p<2) return;
    int* refcount = (int*) (data+addresses[p]+1);
    (*refcount) ++;
  }  

  inline void Unlink(int p) {
    if (p<2) return;
    int* refcount = (int*) (data+addresses[p]+1);
    DCASSERT(*refcount > 0);
    (*refcount) --;
  }

  inline bool isNodeSparse(int p) const {
    return (data[addresses[p]] & Sparse);
  }

  inline bool isNodeReduced(int p) const {
    return (data[addresses[p]] & Reduced);
  }

  inline void SetArc(int p, int i, int d) {
    DCASSERT(!isNodeReduced(p));
    int* sz = (int*) (data + addresses[p] + 1 + 3*sizeof(int));
    CHECK_RANGE(0, i, sz[0]);
    i++;
    if (sz[i] != d) {
      Unlink(sz[i]);  
      Link(d);
      sz[i] = d;
    }
  }

  inline int NodeLevel(int p) const {
    if (p<2) return 0;
    int* foo = (int*) (data + addresses[p] + 1 + 2*sizeof(int));
    return *foo;
  }

  inline int NodeSize(int p) const {
    int* sz = (int*) (data + addresses[p] + 1 + 3*sizeof(int));
    return *sz; 
  }

  inline int NodeMemory(int p) const {
    if (isNodeSparse(p)) {
      // sparse storage
      int* nnz = (int*) (data + addresses[p] + 1 + 3*sizeof(int));
      return 1+(4+(*nnz)*2)*sizeof(int); 
    } else {
      // full storage
      int* size = (int*) (data + addresses[p] + 1 + 3*sizeof(int));
      return 1+(4+(*size))*sizeof(int);
    }
  }

  int TempNode(int k, int sz); 
  void Dump(OutputStream &s) const; 
  int NextFreeNode();
  void FreeNode(int p);
protected:
  int FindHole(int bytes) { return 0; }
  void MakeHole(int addr) { };
};
