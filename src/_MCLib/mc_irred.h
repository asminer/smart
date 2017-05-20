
// $Id$

#ifndef MC_IRRED
#define MC_IRRED

#include "mcbase.h"
#include "../_GraphLib/graphlib.h"

// ******************************************************************
// *                                                                *
// *                         mc_irred class                         *
// *                                                                *
// ******************************************************************

// for building irreducible chains.
class mc_irred : public mc_base {
public:
  mc_irred(bool disc, long ns, long ne);
  virtual ~mc_irred();
  virtual long addState();
  virtual long addAbsorbing();
  virtual bool addEdge(long from, long to, double v);
  virtual void finish(const finish_options &o, renumbering &r);
  virtual void clear();
};

#endif
