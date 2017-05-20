
#ifndef SPLAYDB_H
#define SPLAYDB_H

#include "bst_db.h"

// ======================================================================
// |                                                                    |
// |                  Splay trees, abstract base class                  |
// |                                                                    |
// ======================================================================

class splay_db : public bst_db {
public:
  splay_db(bool indexed, bool storesize);
  virtual ~splay_db();

  virtual void ConvertToDynamic(bool tighten);

protected:
  inline void SplayRotate(long C, long P, long GP) {
    if (left[P] == C) {
      left[P] = right[C];
      right[C] = P;
    } else {
      right[P] = left[C];
      left[C] = P;
    } 
    if (GP>=0) {
      if (P==left[GP])  left[GP] = C;
      else              right[GP] = C;
    } // if gp
  }
  inline void SplayStep(long c, long p, long gp, long ggp) {
    if (gp<0) {
      SplayRotate(c, p, -1);
    } else {
      if ( (right[gp]==p) == (right[p]==c) ) {
        SplayRotate(p, gp, ggp);
        SplayRotate(c, p, ggp);
      } else {
        SplayRotate(c, p, gp);
        SplayRotate(c, gp, ggp);
      }
    }
  }
  long MakeTree(long low, long high);
  int Splay(long key_handle);
  int Splay(const int* state, int size);
};

// ======================================================================
// |                                                                    |
// |                 Splay trees, collection is indexed                 |
// |                                                                    |
// ======================================================================

class splay_index_db : public splay_db {
public:
  splay_index_db(bool storesize);
  virtual ~splay_index_db();

  virtual long InsertState(const int* state, int size);
  virtual long FindState(const int* state, int size);
};

// ======================================================================
// |                                                                    |
// |               Splay trees, collection is not indexed               |
// |                                                                    |
// ======================================================================

class splay_handle_db : public splay_db {
public:
  splay_handle_db(bool storesize);
  virtual ~splay_handle_db();

  virtual long InsertState(const int* state, int size);
  virtual long FindState(const int* state, int size);
};



#endif
