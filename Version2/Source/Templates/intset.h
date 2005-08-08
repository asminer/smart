
// $Id$

#ifndef INTSET_H
#define INTSET_H

#include "../defines.h"
#include "../Base/errors.h"

class int_set {
  int* data;
  int last;
  int dsize;

  bool* contains;
  int csize; 
protected:
  void EnlargeData();
public:
  int_set();
  ~int_set();

  void SetMax(int n);
  
  inline void AddElement(int i) {
    CHECK_RANGE(0, i, csize);
    if (contains[i]) return;
    if (last>=dsize) EnlargeData();
    data[last] = i;
    contains[i] = true;
    last++;
  }

  inline bool Contains(int i) const { 
    CHECK_RANGE(0, i, csize);
    return contains[i];
  }

  inline int Card() const { return last; }

  inline bool isEmpty() const { return 0==last; }

  inline int RemoveElement() {
    DCASSERT(last>0); 
    last--;
    contains[data[last]] = false;
    return data[last];
  }
};

#endif

