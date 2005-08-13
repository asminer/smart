
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

  /** Address of each node.
      If the node is active, this is the offset (>0) in the data array.
      If the node is deleted, this is -next deleted node 
	(part of the unused address list).
  */
  int* address;
  /// Size of address/next array.
  int a_size;
  /// Last used address.
  int a_last;
  /// Pointer to unused address list.
  int a_unused;
  
  /** Data for each node / hole.

      If active, the node data is as follows:

	[0]	incoming pointer count (>=0)
	[1]	number of caches this node is in (>=0)
	[2]	If this node is in the unique table,
		the next pointer (>=-1)
		Otherwise, a value < -1. 
	[3]	Level.  negatives for primed variables.
	[4]	Size.
		If > 0, full storage is used.
		If < 0, sparse storage is used.
		If = 0, the node is deleted but still in caches.

	Full storage:
	[5..4+Size]	Downward pointers (>=0)

	Sparse storage:
	[5..4+nnz]		Indexes (>=0)
	[5+nnz..4+2*nnz]	Downward pointers (>=0)

      If deleted, the hole data is as follows:

	[0]	-size (number of slots in hole)     
	[1]	up
	[2]	down	(used for size data)
	[3]	previous pointer (nodes of same size)
	[4]	next pointer	(nodes of same size)

	[5..size-2]	Unused

	[size-1]	-size
  */
  int* data;
  /// Size of data array.
  int d_size;
  /// Last used data slot.  Also total number of ints "allocated"
  int d_last;
  /// Pointer to top of holes grid
  int holes_top;
  /// Pointer to bottom of holes grid
  int holes_bottom;
  /// Total ints in holes
  int hole_slots;

  int max_hole_chain;

  /// For peak memory.
  int max_slots;
  /// For stats.
  int active_nodes;

  /// Uniqueness table
  HashTable <node_manager> *unique;
public:
  node_manager();
  ~node_manager();

  // Dealing with address

  inline bool isNodeActive(int p) const {
    if (p<2) return true;
    return (address[p]>0);
  }

  inline bool isNodeDeleted(int p) const {
    return (address[p]<=0);
  }

  // Dealing with slot 0

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

  // Dealing with slot 1

  inline int CacheCount(int p) const {
    if (p<2) return 1;
    DCASSERT(address[p]);
    DCASSERT(data[address[p]+1]>=0);
    return data[address[p]+1];
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

  // Dealing with slot 2

  inline bool isNodeReduced(int p) const {
    DCASSERT(address[p]);
    if (p<2) return true;
    return (data[address[p]+2]>=-1);
  }

  inline bool isNodeUnreduced(int p) const {
    DCASSERT(address[p]);
    if (p<2) return false;
    return (data[address[p]+2]<-1);
  }

  // Dealing with slot 3

  inline int NodeLevel(int p) const {
    DCASSERT(isNodeActive(p));
    if (p<2) return 0;
    return data[address[p]+3];
  }

  // Dealing with slot 4

  inline bool isNodeSparse(int p) const {
    DCASSERT(address[p]);
    return (data[address[p]+4]<0);
  }

  inline int SizeOf(int p) const {
    DCASSERT(address[p]);
    DCASSERT(data[address[p]+4]>0);
    return data[address[p]+4];
  }

  inline int nnzOf(int p) const {
    DCASSERT(address[p]);
    DCASSERT(data[address[p]+4]<0);
    return -data[address[p]+4];
  }

  // Dealing with entries

  inline int FullDown(int p, int i) const {
    DCASSERT(address[p]);
    DCASSERT(!isNodeSparse(p));
    CHECK_RANGE(0, i, SizeOf(p));
    return data[address[p]+5+i];
  }

  inline int SparseDown(int p, int i) const {
    DCASSERT(address[p]);
    DCASSERT(isNodeSparse(p));
    CHECK_RANGE(0, i, nnzOf(p));
    return data[address[p]+5+nnzOf(p)+i];
  }
 
  inline int SparseIndex(int p, int i) const {
    DCASSERT(address[p]);
    DCASSERT(isNodeSparse(p));
    CHECK_RANGE(0, i, nnzOf(p));
    return data[address[p]+5+i];
  }

  inline void SetArc(int p, int i, int d) {
    DCASSERT(isNodeActive(p));
    DCASSERT(!isNodeReduced(p));
    int* sz = data + address[p] + 4;
    DCASSERT(sz[0]>0);
    CHECK_RANGE(0, i, sz[0]);
    sz += i+1;
    Unlink(sz[0]);
    sz[0] = d;
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

  inline int MaxHoleChain() const { return max_hole_chain; }

  // For uniqueness table
public:
  inline int Null() const { return 0; }
  inline int getNext(int h) const { 
    DCASSERT(address[h]); 
    DCASSERT(data[address[h]+2]>=-1);
    return data[address[h]+2];
  }
  inline void setNext(int h, int n) const { 
    DCASSERT(address[h]); 
    data[address[h]+2] = n; 
  }
  inline bool isStale(int h) const { return false; }
  inline void show(OutputStream &s, int h) const { s.Put(h); }
  int hash(int h, int M) const;
  bool equals(int h1, int h2) const;
protected:
  void GridInsert(int p);
  inline void MidRemove(int p) {
    int left = data[p+3];
    DCASSERT(left);
    int right = data[p+4];
    data[left+4] = right;
    if (right) data[right+3] = left;
  }
  inline void IndexRemove(int p) {
    DCASSERT(data[p+3]==0);
    int above = data[p+1];
    int below = data[p+2];
    if (above) {
        data[above+2] = below;
    } else {
        holes_top = below;
    }
    if (below) {
        data[below+1] = above;
    } else {
        holes_bottom = below;
    }
  }
  void DeleteNode(int p);
  int NextFreeNode();
  void FreeNode(int p);
  int FindHole(int slots);
  void MakeHole(int addr, int slots);
};

#endif

