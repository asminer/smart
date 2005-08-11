
// $Id$

#ifndef MDDS_MALLOC
#define MDDS_MALLOC

#include "../defines.h"
#include "../Base/streams.h"
#include "../Templates/hash.h"

//#define TRACK_DELETIONS

/* Temporary, until we have a proper mdd library. */

class mdd_node_manager {

  /** Array of node pointers.
      Each node stores the following 4 integers, then the actual node data:
	incoming reference count
	number of cache entries
	level
	#full entries (if positive) / #nonzeroes (if negative)

      If the node is "full", then the remainder of the node is
      a full array of pointers (with the specified number of entries).

      If the node is "sparse", then there are two arrays filling the
      remainder of the node: an index array and a down pointer array.
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
  
  /// For peak memory.
  int peak_mem;
  /// Current memory
  int curr_mem;
  /// For stats.
  int active_nodes;

  /// Uniqueness table
  HashTable <mdd_node_manager> *unique;
public:
  mdd_node_manager();
  ~mdd_node_manager();

  inline bool isActive(int p) const {
    if (p<2) return true;
    return (address[p]);
  }

  inline bool isDeleted(int p) const {
    return (0==address[p]);
  }

  inline bool isReduced(int p) const {
    DCASSERT(address[p]);
    return (next[p]>=-1);
  }

  inline bool isSparse(int p) const {
    DCASSERT(address[p]);
    return (address[p][3] < 0);
  }

  inline int SizeOf(int p) const {
    DCASSERT(!isSparse(p));
    return address[p][3];
  }

  inline int nnzOf(int p) const {
    DCASSERT(isSparse(p));
    return -address[p][3];
  }

  inline const int* FullDownOf(int p) const {
    DCASSERT(!isSparse(p));
    return address[p]+4;
  }

  inline const int* IndexesOf(int p) const {
    DCASSERT(isSparse(p));
    return address[p]+4;
  }

  inline const int* SparseDownOf(int p) const {
    DCASSERT(isSparse(p));
    return address[p]+4+nnzOf(p);
  }

  inline int Incount(int p) const {
    if (p<2) return 1;
    DCASSERT(address[p]);
    return address[p][0];
  }

  inline void Link(int p) { 
    if (p<2) return;
    DCASSERT(address[p]);
#ifdef TRACK_DELETIONS
    if (0==address[p][0]) 
	Output << "Node " << p << " back from the dead!\n";
#endif
    address[p][0]++;
#ifdef TRACK_DELETIONS
    Output << "\t+Node " << p << " count now " << address[p][0] << "\n";
    Output.flush();
#endif
  }  

  inline int SharedCopy(int p) {
    Link(p);
    return p;
  }

  void Unlink(int p) {
    if (p<2) return;
    DCASSERT(address[p]);
    // decrement incoming count
    DCASSERT(address[p][0]>0);
    address[p][0]--;
#ifdef TRACK_DELETIONS
    Output << "\t-Node " << p << " count now " << address[p][0] << "\n";
    Output.flush();
#endif
    if (address[p][0]) return; 
    if (address[p][1]) return;
#ifdef TRACK_DELETIONS
    Output << "Deleting node " << p << " from Unlink\n";
    Output.flush();
#endif
    DeleteNode(p);
  }

  inline void CacheAdd(int p) {
    if (p<2) return;
    DCASSERT(address[p]);
    address[p][1]++;
  }

  bool CacheRemove(int p) {
    if (p<2) return false;
    DCASSERT(address[p]);
    DCASSERT(address[p][1]>0);
    address[p][1]--;
    if (0==address[p][1] && 0==address[p][0]) {
#ifdef TRACK_DELETIONS
      Output << "Deleting node " << p << " from CacheRemove\n";
      Output.flush();
#endif
      DeleteNode(p);
    }
    return true;
  }

  inline void SetArc(int p, int i, int d) {
    DCASSERT(isActive(p));
    DCASSERT(!isReduced(p));
    int* sz = address[p] + 3;
    DCASSERT(sz[0]>0);
    CHECK_RANGE(0, i, sz[0]);
    sz += i+1;
    Unlink(sz[0]);
    sz[0] = d;
  }

  inline int LevelOf(int p) const {
    DCASSERT(isActive(p));
    if (p<2) return 0;
    return address[p][2];
  }

  int Reduce(int p);

  int TempNode(int k, int sz); 
  void Dump(OutputStream &s) const; 

  void ShowNode(OutputStream &s, int p) const;

  inline int PeakNodes() const { return a_last-1; }
  inline int CurrentNodes() const { return active_nodes; }
  inline int PeakMemory() const { return peak_mem; }
  inline int CurrentMemory() const { return curr_mem; }

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

#endif
