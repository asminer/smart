
#ifndef SCCS_H
#define SCCS_H

/*
  Functions for strongly-connected components
  and variations.
*/

namespace GraphLib {
  class dynamic_graph;
  class generic_graph;
};

long find_sccs(const GraphLib::dynamic_graph* g, bool cons, long* sccmap, long* aux);
void find_tsccs(const GraphLib::dynamic_graph* g, long* sm, long* aux, long scc_count);
long compact(const GraphLib::dynamic_graph* g, long* sm, long* aux, long scc_count);

long find_sccs(const GraphLib::generic_graph* g, bool cons, long* sccmap, long* aux);
void find_tsccs(const GraphLib::generic_graph* g, long* sm, long* aux, long scc_count);
long compact(const GraphLib::generic_graph* g, long* sm, long* aux, long scc_count);

#endif
