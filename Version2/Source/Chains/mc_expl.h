
// $Id$

#ifndef MC_EXPL_H
#define MC_EXPL_H

#include "../Templates/graphs.h"
#include "../Engines/sccs.h"

/** Classified Markov chain (or other graph).
	
    This will almost always be a proper Markov chain, but
    I made it a template class in case someone needs additional / alternate
    information in the edges.

    For purposes of this class, a Markov chain contains
	transient states
	recurrent states, which form recurrent classes
	absorbing states (NOT counted as recurrent)

    Even though absorbing states are recurrent classes, we count them
    separately because they are a special case that can be handled VERY easily.
   
*/
template <class LABEL>
struct classified_chain {
  /// Total number of states
  int states;

  /// Number of recurrent classes (not counting absorbing states)
  int recurrent; 

  /** Dimension is #recurrent + 2.
      Specifies the first state number for each class.
	blockstart[0] will always be 0 (for consistency)

	blockstart[1] 
        ... 
	blockstart[c] : first state of recurrent class c
	...
        blockstart[recurrent]

	blockstart[recurrent+1] : first absorbing state
  */
  int* blockstart;

public:
  /** Constructor.
	@param	gr	Labeled digraph to build from.
			This is classified and destroyed.

	Note: this is done "in place", as much as possible:
	the arcs from gr are yanked out and put here
	(that's why gr must be destroyed).
   */ 
  classified_chain(labeled_digraph<LABEL> *gr);
  ~classified_chain() {
    delete[] blockstart;
  }

  inline bool isAbsorbing(int newnumber) const {
    DCASSERT(blockstart);
    return newnumber >= blockstart[recurrent+1];
  } 

  inline bool isTransient(int newnumber) const {
    DCASSERT(blockstart);
    return newnumber < blockstart[1];
  }
  
  inline int numTransient() const { 
    DCASSERT(blockstart);
    return blockstart[1]; 
  }
  inline int numAbsorbing() const { 
    DCASSERT(blockstart);
    return states - blockstart[recurrent+1]; 
  }
  /// Total number of recurrent classes, including absorbing states
  inline int numClasses() const { 
    return recurrent + numAbsorbing(); 
  }
  /** Number of states in recurrent class c.
      Valid for 0 <= c <= numClasses()
   */
  inline int numRecurrent(int c) const { 
    DCASSERT(blockstart);
    DCASSERT(c>=0);  
    DCASSERT(c<=numClasses());
    return (c<=recurrent) ?
	(blockstart[c+1] - blockstart[c])	// a "big" recurrent class
	:
	1; // must be an absorbing class
  }

};


#define DEBUG_CLASSIFY

template <class LABEL>
classified_chain <LABEL> :: classified_chain(labeled_digraph <LABEL> *in)
{
  DCASSERT(in);

#ifdef DEBUG_CLASSIFY
  Output << "Starting to classify chain\n";
  Output.flush();
#endif
}

#endif

