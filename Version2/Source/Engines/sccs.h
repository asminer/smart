
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
			sccmap[k] is between 0 and #sccs if
			this was a node to be considered;
			all nodes within the same scc will have the same value.

	@return		The number of sccs, or -1 if there was some error.
			(Any errors possible?)
*/
int 	ComputeSCCs(const digraph *g, int* sccmap);

// other functions that could be useful

/**	Compute the terminal sccs (as for mc state classification).

	@param 	g	The graph.
	@param	sccmap	An array of dimension #nodes.
			ON INPUT:
			For node k in the graph to be considered,
			sccmap[k] must be 0; to be not considered,
			sccmap[k] must be more than 2*#nodes.
			ON OUTPUT:
			sccmap[k] is 0 if node k is "transient",
                        between 1 and #classes if k is "recurrent",
			assuming node k was to be considered.

	@return		The number of terminal sccs (recurrent classes), 
			or -1 if there was some error.
*/
int	ComputeTSCCs(const digraph *g, int* sccmap);

/**	Compute sccs that contain loops.
	Differs from ordinary sccs in that single-state sccs
	are not counted unless they have a self-loop.
	Used for computing ctl operator EG.

	@param 	g	The graph.
	@param	sccmap	An array of dimension #nodes.
			ON INPUT:
			For node k in the graph to be considered,
			sccmap[k] must be 0; to be not considered,
			sccmap[k] must be more than 2*#nodes.
			ON OUTPUT:
			sccmap[k] is 0 if node k is its own scc with no loop,
                        between 1 and #sccs if it is within an scc that loops,
			assuming node k was to be considered.

	@return		The number of "looping" sccs
			or -1 if there was some error.
*/
int	ComputeLoopedSCCs(const digraph *g, int* sccmap);


#endif
