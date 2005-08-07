
// $Id$

#ifndef MDDS_H
#define MDDS_H

#include "../defines.h"
#include "../Base/streams.h"
#include "../Templates/hash.h"

/* Temporary, until we have a proper mdd library. */

class node_manager {

  /** For each node, its index in the data array. */
  int* address;
  /** Next pointer for uniqueness table. */
  int* next;
  /// Size of address/next array.
  int a_size;
  /// Last used address.
  int a_last;
  /// Pointer to unused address list.
  int a_unused;
  
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

  /// Uniqueness table
  HashTable <node_manager> *unique;
public:
  node_manager();
  ~node_manager();

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

  inline bool isNodeSparse(int p) const {
    DCASSERT(address[p]);
    return (data[address[p]+3]<0);
  }

  inline int SizeOf(int p) const {
    DCASSERT(address[p]);
    DCASSERT(data[address[p]+3]>0);
    return data[address[p]+3];
  }

  inline int nnzOf(int p) const {
    DCASSERT(address[p]);
    DCASSERT(data[address[p]+3]<0);
    return -data[address[p]+3];
  }

  // this should be read--only!
  inline const int* NodeData(int p) const {
    DCASSERT(address[p]);
    return data+address[p]+4;
  }

  inline void Link(int p) { 
    DCASSERT(isNodeActive(p));
    if (p<2) return;
    data[address[p]]++;
  }  

  void Unlink(int p) {
    if (p<2) return;
    DCASSERT(isNodeActive(p));
    // decrement incoming count
    int* foo = data+address[p];
    DCASSERT(foo[0]>0);
    foo[0]--;
    if (foo[0]) return;
    if (foo[1]) return;  // still in a cache somewhere
    DeleteNode(p);
  }

  inline void CacheInc(int p) {
    if (p<2) return;
    DCASSERT(address[p]);
    data[address[p]+1]++;
  }

  inline bool CacheDec(int p) {
    if (p<2) return false;
    DCASSERT(address[p]);
    int* foo = data + address[p];
    if (foo[0]) return false;
    DCASSERT(foo[1]>0);
    foo[1]--;
    if (0==foo[1]) DeleteNode(p);
    return true;
  }

  inline void SetArc(int p, int i, int d) {
    DCASSERT(isNodeActive(p));
    DCASSERT(!isNodeReduced(p));
    int* sz = data + address[p] + 3;
    DCASSERT(sz[0]>0);
    CHECK_RANGE(0, i, sz[0]);
    sz += i+1;
    Unlink(sz[0]);
    sz[0] = d;
  }

  inline int NodeLevel(int p) const {
    DCASSERT(isNodeActive(p));
    if (p<2) return 0;
    return data[address[p]+2];
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
  int FindHole(int slots);
  void MakeHole(int addr, int slots);
};

#endif

