
// $Id$

#ifndef SCCS_H
#define SCCS_H

#include "../Templates/graphs.h"

/*
	Functions for strongly-connected components
   	and variations.

 */

/** 	Compute strongly connected components for a graph.

	@param 	g	The graph.
	@param	sccmap	An array of dimension #nodes.
			ON INPUT:
			For node k in the graph to be considered,
			sccmap[k] must be 0; to be not considered,
			sccmap[k] must be more than 2*#nodes.
			ON OUTPUT:
			sccmap[k] is between #nodes+1 and 2*#nodes if
			this was a node to be considered;
			all nodes within the same scc will have the same value.

	@return		The number of sccs, or -1 if there was some error.
			(Any errors possible?)
*/
int 	ComputeSCCs(digraph *g, unsigned int* sccmap);

// other functions that could be useful

// compute the terminal sccs (as for mc state classification)
// int	ComputeTSCCs(const digraph &g, int* sccmap);

#endif
