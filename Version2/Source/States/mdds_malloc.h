
// $Id$

#include "../defines.h"
#include "../Base/streams.h"
#include "../Templates/hash.h"

/* Temporary, until we have a proper mdd library. */

class mdd_node_manager {

  /** Array of node pointers.
      Each node stores the following 4 integers, then the actual node data:
	incoming reference count
	number of cache entries
	level
	#full entries (if positive) / #nonzeroes (if negative)
  */
  int** address;
  /** Next pointer for uniqueness table. */
  int* next;
  /// Size of address/next array.
  int a_size;
  /// Last used address.
  int a_last;
  /// Pointer to unused address list.
  int a_unused;
  
  /// Uniqueness table
  HashTable <mdd_node_manager> *unique;
public:
  mdd_node_manager();
  ~mdd_node_manager();

  inline bool isNodeActive(int p) const {
    if (p<2) return true;
    return (address[p]);
  }

  inline bool isNodeDeleted(int p) const {
    return (0==address[p]);
  }

  inline bool isNodeReduced(int p) const {
    DCASSERT(address[p]);
    return (next[p]>=-1);
  }

  inline void Link(int p) { 
    DCASSERT(isNodeActive(p));
    if (p<2) return;
    address[p][0]++;
  }  

  void Unlink(int p) {
    if (p<2) return;
    DCASSERT(isNodeActive(p));
    // decrement incoming count
    DCASSERT(address[p][0]>0);
    address[p][0]--;
    if (address[p][0]) return; 
    if (address[p][1]) return;
    DeleteNode(p);
  }

  bool CacheDec(int p) {
    if (p<2) return false;
    DCASSERT(address[p]);
    if (address[p][0]) return false;
    DCASSERT(address[p][1]>0);
    address[p][1]--;
    if (0==address[p][1]) DeleteNode(p);
    return true;
  }

  inline void SetArc(int p, int i, int d) {
    DCASSERT(isNodeActive(p));
    DCASSERT(!isNodeReduced(p));
    int* sz = address[p] + 3;
    DCASSERT(sz[0]>0);
    CHECK_RANGE(0, i, sz[0]);
    sz += i+1;
    Unlink(sz[0]);
    sz[0] = d;
  }

  inline int NodeLevel(int p) const {
    DCASSERT(isNodeActive(p));
    if (p<2) return 0;
    return address[p][2];
  }

  int Reduce(int p);

  int TempNode(int k, int sz); 
  void Dump(OutputStream &s) const; 

  // For uniqueness table
public:
  inline int getNext(int h) const { DCASSERT(address[h]); return next[h]; }
  inline void setNext(int h, int n) const { DCASSERT(address[h]); next[h] = n; }
  inline bool isStale(int h) const { return false; }
  inline void show(OutputStream &s, int h) const { s.Put(h); }
  int hash(int h, int M) const;
  bool equals(int h1, int h2) const;
protected:
  void DeleteNode(int p);
  int NextFreeNode();
  void FreeNode(int p);
};
