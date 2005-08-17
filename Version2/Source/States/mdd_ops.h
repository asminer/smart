
// $Id$

#include "mdds.h"
#include "../Templates/intset.h"

class binary_cache {
  struct node {
    int a;
    int b;
    int c;
    int next;
  };
  node_manager* mdd;
  node* nodeheap;
  HashTable <binary_cache>* table;
  int heapsize;
  int lastnode;
  int unused_nodes;
  int pings;
  int hits;
protected:
  int NewNode();
  inline void RecycleNode(int h) {
    nodeheap[h].next = unused_nodes;
    unused_nodes = h;
  }
public:
  binary_cache(node_manager *m);
  ~binary_cache();

  void Add(int a, int b, int c) {
    int h = NewNode();
    nodeheap[h].a = a;
    nodeheap[h].b = b;
    nodeheap[h].c = c;
    mdd->CacheAdd(a);
    mdd->CacheAdd(b);
    mdd->CacheAdd(c);
#ifdef DEVELOPMENT_CODE
    int i = table->Insert(h);
    DCASSERT(i==h);
#else
    table->Insert(h);
#endif
  }
  bool Hit(int a, int b, int &c) {
    pings++;
    int q = NewNode();
    nodeheap[q].a = a;
    nodeheap[q].b = b;
    int h = table->Find(q); 
    RecycleNode(q);
    if (h>=0) {
      // potential hit, make sure the result is still alive
      if (mdd->isNodeZombie(nodeheap[h].c)) {
        // Result no longer alive, remove from cache
#ifdef DEVELOPMENT_CODE
	int h1 = table->Remove(h);
    	DCASSERT(h1 == h);
#else
	table->Remove(h);
#endif
        mdd->CacheRemove(nodeheap[h].a);
        mdd->CacheRemove(nodeheap[h].b);
        mdd->CacheRemove(nodeheap[h].c);
        RecycleNode(h);
        return false;
      }
      // result is still good.
      c = nodeheap[h].c;
      hits++;
      return true;
    }
    return false;
  }

  inline int Pings() const { return pings; }
  inline int Hits() const { return hits; }

  void Report(const char* cachename, OutputStream &s) const;

  // Remove all entries
  void Clear(); 

  // required for hash table
public:
  inline int Null() const { return -1; }
  inline int getNext(int h) const { return nodeheap[h].next; }
  inline void setNext(int h, int n) { nodeheap[h].next = n; }
  inline unsigned hash(int h, int M) { 
    return (nodeheap[h].a*256 + nodeheap[h].b) % M; 
  }
  inline bool equals(int h1, int h2) { 
    if (nodeheap[h1].a != nodeheap[h2].a) return false;
    return nodeheap[h1].b == nodeheap[h2].b;
  }
  inline void show(OutputStream &s, int h) {
    s << "(" << nodeheap[h].a << ", " << nodeheap[h].b << ", " << nodeheap[h].c << ")";
  } 
  inline bool isStale(int h) {
    if (0==mdd->Incount(nodeheap[h].a) || 
	0==mdd->Incount(nodeheap[h].b) || 
	0==mdd->Incount(nodeheap[h].c)) {
    
      mdd->CacheRemove(nodeheap[h].a);
      mdd->CacheRemove(nodeheap[h].b);
      mdd->CacheRemove(nodeheap[h].c);
      RecycleNode(h);
      return true;
    }
    return false;
  }
};

class operations {
  node_manager* mdd;  

  // caches
  binary_cache* union_cache;
  binary_cache* fire_cache;

  // used for counting
  int* counts;
  int countsize;

  // used by saturation
  int K;
  int* roots;
  int* sizes;

  int_set** Lset;
public:
  operations(node_manager* m);
  ~operations();

  /** Returns a "new" set encoding the union of sets a and b.
  */
  int Union(int a, int b);
  int Count(int a);

  void Mark(int a, bool* marked);

  // Pregen saturation
  int Saturate(int init, int* roots, int* sizes, int K);

  void CacheReport(OutputStream &s) const {
    union_cache->Report("Union cache", s);
    fire_cache->Report("Firing cache", s);
  }

  inline void ClearUCache() { union_cache->Clear(); }

  inline void ClearFCache() { fire_cache->Clear(); }

protected:
  int TopSaturate(int p);  
  void Saturate(int p);  
  int RecFire(int p, int mxd);
  bool FireRow(int s, int pd, int row);
};
