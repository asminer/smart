
// $Id$

#ifndef MDDS_H
#define MDDS_H

#include "../defines.h"
#include "../Base/streams.h"
#include "../Templates/hash.h"

//#define TRACK_DELETIONS
//#define TRACK_CACHECOUNT

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

  /// For peak memory.
  int max_slots;
  /// For stats.
  int active_nodes;

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

  inline int Data(int p, int i) const {
#ifdef DEVELOPMENT_CODE
    DCASSERT(address[p]);
    if (isNodeSparse(p)) {
      CHECK_RANGE(0, i, 2*nnzOf(p));
    } else {
      CHECK_RANGE(0, i, SizeOf(p));
    }
#endif
    return data[address[p]+4+i];
  }

  inline int Incount(int p) const {
    if (p<2) return 1;
    DCASSERT(address[p]);
    DCASSERT(data[address[p]]>=0);
    return data[address[p]];
  }

  inline void Link(int p) { 
    DCASSERT(isNodeActive(p));
    if (p<2) return;
#ifdef TRACK_DELETIONS
    if (0==data[address[p]]) 
	Output << "Node " << p << " back from the dead!\n";
#endif
    data[address[p]]++;
#ifdef TRACK_DELETIONS
    Output << "\t+Node " << p << " count now " << data[address[p]] << "\n";
    Output.flush();
#endif
  }  

  inline int SharedCopy(int p) {
    Link(p);
    return p;
  }

  void Unlink(int p) {
    if (p<2) return;
    DCASSERT(isNodeActive(p));
    // decrement incoming count
    int* foo = data+address[p];
    DCASSERT(foo[0]>0);
    foo[0]--;
#ifdef TRACK_DELETIONS
    Output << "\t-Node " << p << " count now " << data[address[p]] << "\n";
    Output.flush();
#endif
    if (foo[0] || foo[1]) return; // not dead yet
#ifdef TRACK_DELETIONS
    Output << "Deleting node " << p << " from Unlink\t";
    ShowNode(Output, p);
    Output << "\n";
    Output.flush();
#endif
    DeleteNode(p);
  }

  inline void CacheAdd(int p) {
    if (p<2) return;
    DCASSERT(address[p]);
    data[address[p]+1]++;
#ifdef TRACK_CACHECOUNT
    Output << "+Node " << p << " is in " << data[address[p]+1] << " caches\n";
    Output.flush();
#endif
  }

  inline void CacheRemove(int p) {
    if (p<2) return;
    DCASSERT(address[p]);
    int* foo = data + address[p];
    DCASSERT(foo[1]>0);
    foo[1]--;
#ifdef TRACK_CACHECOUNT
    Output << "-Node " << p << " is in " << foo[1] << " caches\n";
    Output.flush();
#endif
    if (foo[0] || foo[1]) return; // not dead yet
#ifdef TRACK_DELETIONS
    Output << "Deleting node " << p << " from CacheRemove\t";
    ShowNode(Output, p);
    Output << "\n";
    Output.flush();
#endif
    DeleteNode(p);
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

  void ShowNode(OutputStream &s, int p) const;

  inline int PeakNodes() const { return a_last-1; }
  inline int CurrentNodes() const { return active_nodes; }
  inline int PeakMemory() const { return max_slots * sizeof(int); }
  inline int CurrentMemory() const { return d_last * sizeof(int); }
  inline int MemoryHoles() const { return hole_slots * sizeof(int); } 

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

