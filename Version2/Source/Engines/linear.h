
// $Id$

/** Linear solver frontends
*/

#ifndef LINEAR_H
#define LINEAR_H

#include "../Templates/graphs.h"
#include "../Templates/sparsevect.h"

/**  
     Solves pi*Q = 0, subject to pi*1 = 1.
     Max number of iterations, precision, etc. are determined by options.

     @param	pi	Solution vector pi.
     @param	Q	Matrix Q, not including diagonals.
     @param	h	Holding time vector (1/diagonals)
     @param	start	Starting row of Q
     @param	stop	1+Stopping row of Q

			E.g., for irreducible chains, use start=0, stop=#states

     @return	true if desired precision was achieved
*/
bool SSSolve(double *pi, labeled_digraph <float> *Q, float *h, 
		int start, int stop);

/**  
     Solves n*Q = -init.
     Max number of iterations, precision, etc. are determined by options.

     @param	n	Solution vector n.
     @param	Q	Matrix Q, not including diagonals.
     @param	h	Holding time vector (1/diagonals)
     @param	init	Initial state distribution
     @param	start	Starting row of Q
     @param	stop	1+Stopping row of Q

			E.g., for irreducible chains, use start=0, stop=#states

     @return	true if desired precision was achieved
*/
bool MTTASolve(double *n, labeled_digraph <float> *Q, float *h,
	       sparse_vector <float> *init, int start, int stop);




/** Initialize linear solver options */
void InitLinear();

#endif

