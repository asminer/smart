
// $Id$

#ifndef MEMTRACK_H
#define MEMTRACK_H

#ifdef MEM_TRACE_ON

#include "streams.h"

class memlog {
  struct logitem {
    const char* item;
    int count;
    int size;
    logitem* next;
  };
  logitem* head; 
  int peaksize;
  int currsize;
  bool active;
public:
  memlog();
  ~memlog();

  inline void Start() { active = true; }
  void Stop(OutputStream& report);

  void Alloc(const char* item, int size);
  void Free(const char* item, int size);
protected:
  void MoveToFront(const char* item);
};

extern memlog Memory_Log;

#define ALLOC(X, Y) 	Memory_Log.Alloc(X, Y)
#define FREE(X, Y)	Memory_Log.Free(X, Y)

#else

#define ALLOC(X, Y)
#define FREE(X, Y)

#endif

#endif

