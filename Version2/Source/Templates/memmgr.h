
// $Id$

/*
	Memory manager for lots of small objects of equal size
	(e.g., allocating and recycling nodes of a tree)
*/

#ifndef MEMMGR_H
#define MEMMGR_H

#include "list.h"
#include "../Base/errors.h"

class ObjMgr {
private:
  int objsize;
  PtrList chunklist;
  char* chunk;  
  int chunksize;
  int lastfree;
  void* freelist;   // recycled objects
  int peakobjects;
  int activeobjects;
protected:
  inline void PushFree(void* obj) {
    void** thing = (void**) obj;
    thing[0] = freelist;
    freelist = obj;
  }
  inline void* PopFree() {
    DCASSERT(freelist);
    void* answer = freelist;
    freelist = ((void**) freelist)[0];
    return answer;
  }
  inline void NewChunk() {
    chunk = (char*) malloc(chunksize);
    if (NULL==chunk) OutOfMemoryError("object manager");
    lastfree = 0;
  }
public:
  ObjMgr(int osize, int allocsize=1024) : chunklist(16) {
    if (osize < 4) {
      Internal.Start(__FILE__, __LINE__);
      Internal << "Object manager requires objects of at least 4 bytes\n";
      Internal.Stop();
    }
    objsize = osize;
    chunksize = allocsize * objsize;
    NewChunk();
    freelist = NULL;
    peakobjects = 0;
    activeobjects = 0;
  }
  ~ObjMgr() {
    // more work here
    int i;
    free(chunk);
    for (i=0; i<chunklist.Length(); i++) {
      chunk = (char*) chunklist.VItem(i);
      free(chunk);
    }
  }
  void* GetChunk() {
    activeobjects++;
    if (freelist) { return PopFree(); }
    if (lastfree >= chunksize) {
      chunklist.VAppend(chunk);
      NewChunk();
    }
    void* answer = (chunk + lastfree);
    lastfree += objsize;
    peakobjects++;
    return answer;
  }
  inline void FreeChunk(void* obj) {
    activeobjects--;
    PushFree(obj);
  }
  inline int PeakObjects() { return peakobjects; }
  inline int ActiveObjects() { return activeobjects; }
};

template <class DATA>
class Manager {
  ObjMgr *m;
public:
  Manager(int size) { m = new ObjMgr(sizeof(DATA), size); }
  ~Manager() { delete m; }
  inline DATA* NewObject() { return (DATA *) m->GetChunk(); }
  inline void FreeObject(DATA *x) { m->FreeChunk(x); }
  inline int PeakObjects() { return m->PeakObjects(); }
  inline int ActiveObjects() { return m->ActiveObjects(); }
};

#endif

