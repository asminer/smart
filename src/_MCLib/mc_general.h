
#ifndef MC_GENERAL
#define MC_GENERAL

#include "mcbase.h"

#ifndef DISABLE_OLD_INTERFACE

// ******************************************************************
// *                                                                *
// *                        mc_general class                        *
// *                                                                *
// ******************************************************************

// for building irreducible chains.
class mc_general : public mc_base {
public:
  mc_general(bool disc, long ns, long ne);
  virtual ~mc_general();
  virtual long addState();
  virtual long addAbsorbing();
  virtual bool addEdge(long from, long to, double v);
  virtual void finish(const finish_options &o, renumbering &r);
  virtual void clear();

protected:
  inline bool IsIdentity(const long* renumber) {
    for (long i=num_states-1; i>=0; i--)
      if (renumber[i] != i) return false;
    return true;
  }
};

#endif // #ifndef DISABLE_OLD_INTERFACE

#endif
