
// $Id$

#include "../defines.h"
#include "../Base/streams.h"

/* Temporary, until we have a proper mdd library. */

class node_manager {
  /// Flag: terminal node.
  static const char Terminal = 0x80;
  /// Flag: is the node deleted.
  static const char Deleted = 0x40;
  /// Flag: is the node Reduced. 
  static const char Reduced = 0x20;
  /// Flag: is the node stored in sparse format.
  static const char Sparse = 0x10;
  // other flags?

  /** For each node, its index in the data array. */
  int* addresses;
  /** For each node, its flag. */
  char* flags;
  /// Size of addresses/flags array.
  int a_size;
  /// Last used address.
  int a_last;
  /// Pointer to unused address list.
  int a_unused;
  /// Tail of unused address list.
  int a_unused_tail;
  
  /** Data for each node. 
      Each node stores the following 4 integers, then the actual node data:
	incoming reference count
	number of cache entries
	level
	#full entries / #nonzeroes

      For "holes", instead the 2 integers are used:
        next hole index
	hole size (#integers)
  */
  int* data;
  /// Size of data array.
  int d_size;
  /// Last used data slot.  Also total number of ints "allocated"
  int d_last;
  /// Pointer to data holes list.
  int d_unused;
  /// Total ints in holes
  int hole_slots;

public:
  node_manager();
  ~node_manager();

  inline bool isNodeActive(int p) const {
    return ((flags[p] & Deleted)==0);
  }

  inline bool isNodeDeleted(int p) const {
    return (flags[p] & Deleted);
  }

  inline bool isNodeSparse(int p) const {
    return (data[addresses[p]] & Sparse);
  }

  inline bool isNodeReduced(int p) const {
    return (data[addresses[p]] & Reduced);
  }

  inline void Link(int p) { 
    DCASSERT(isNodeActive(p));
    if (p<2) return;
    data[addresses[p]]++;
  }  

  void Unlink(int p);

  inline void SetArc(int p, int i, int d) {
    DCASSERT(isNodeActive(p));
    DCASSERT(!isNodeReduced(p));
    int* sz = data + addresses[p] + 3;
    CHECK_RANGE(0, i, sz[0]);
    sz += i+1;
    if (sz[0] != d) {
      Unlink(sz[0]);  
      Link(d);
      sz[0] = d;
    }
  }

  inline int NodeLevel(int p) const {
    DCASSERT(isNodeActive(p));
    if (p<2) return 0;
    return data[addresses[p]+2];
  }

  inline int NodeSize(int p) const {
    DCASSERT(p>1);
    DCASSERT(isNodeActive(p));
    return data[addresses[p]+3];
  }

  int TempNode(int k, int sz); 
  void Dump(OutputStream &s) const; 
protected:
  int NextFreeNode();
  void FreeNode(int p);
  int FindHole(int slots);
  void MakeHole(int addr, int slots);
};
