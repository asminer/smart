
/*
    Tests for vanishing chains
*/

#include <iostream>
#include "mclib.h"

#define VERBOSE

using namespace GraphLib;
using namespace std;
using namespace MCLib;

typedef struct { long from; long to; double rate; } edge;

// ==============================> Graph 1 <==============================

const edge tv1[] = {
  {0, 0, 1}, 
  {1, 0, 2}, 
  {0, 1, 3},
  {0, 2, 4},
  {1, 1, 5},
  {2, 0, 6},
  {0, 3, 7},
  {0, 4, 8},
  {0, 5, 9},
  {1, 2, 10},
  {2, 1, 11},
  {3, 0, 12},
  {0, 6, 13},
  {0, 7, 14},
  {0, 8, 15},
  {0, 9, 16},
  {1, 3, 17},
  {2, 2, 18},
  {3, 1, 19},
  {-1, -1, -1}
};

bool discrete1 = true;
const long num_tan1 = 15;
const long num_van1 = 15;

// =======================================================================

void fillVanishing(vanishing_chain &v, const edge tt[], const edge tv[], 
  const edge vv[], const edge vt[])
{
  for (long i=0; tt[i].from >= 0; i++) {
    v.addTTedge(tt[i].from, tt[i].to, tt[i].rate);
  }
  for (long i=0; tv[i].from >= 0; i++) {
    v.addTVedge(tv[i].from, tv[i].to, tv[i].rate);
  }
  for (long i=0; vv[i].from >= 0; i++) {
    v.addVVedge(vv[i].from, vv[i].to, vv[i].rate);
  }
  for (long i=0; vt[i].from >= 0; i++) {
    v.addVTedge(vt[i].from, vt[i].to, vt[i].rate);
  }
}

// =======================================================================

int main()
{
  vanishing_chain foo(discrete1, num_tan1, num_van1);
  fillVanishing(foo, tv1, tv1, tv1, tv1);
  LS_Options opt;
  try {
    foo.eliminateVanishing(opt);
  }
  catch (GraphLib::error e) {
    cout << "Caught GraphLib error: " << e.getString() << "\n";
    return 1;
  }
  catch (MCLib::error e) {
    cout << "Caught MCLib error: " << e.getString() << "\n";
    return 1;
  }
  return 0;
}

