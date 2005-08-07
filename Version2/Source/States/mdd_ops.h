
// $Id$

#include "mdds.h"

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
      c = nodeheap[h].c;
      hits++;
      return true;
    }
    return false;
  }

  inline int Pings() const { return pings; }
  inline int Hits() const { return hits; }

  // required for hash table
public:
  inline int getNext(int h) const { return nodeheap[h].next; }
  inline void setNext(int h, int n) { nodeheap[h].next = n; }
  inline int hash(int h, int M) { 
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
    bool sa = mdd->CacheDec(nodeheap[h].a);
    bool sb = mdd->CacheDec(nodeheap[h].b);
    bool sc = mdd->CacheDec(nodeheap[h].c);
    if (sa || sb || sc) {
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
public:
  operations(node_manager* m);
  ~operations();

  int Union(int a, int b);
};
