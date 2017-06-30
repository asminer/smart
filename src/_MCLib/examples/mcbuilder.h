
/*
  Common stuff for most test programs.
*/

#ifndef MCBUILDER_H
#define MCBUILDER_H

#include "mclib.h"

typedef struct { long from; long to; double rate; } edge;

// =======================================================================

class my_timer : public GraphLib::timer_hook {
  public:
    my_timer(bool activ);
    virtual ~my_timer();
    virtual void start(const char* w);
    virtual void stop();
  private:
    bool active;
};

// =======================================================================

MCLib::Markov_chain* build_double(const bool discrete, const edge graph[], 
  const long num_nodes, bool verbose);

MCLib::Markov_chain* build_float(const bool discrete, const edge graph[], 
  const long num_nodes, bool verbose);

#endif
