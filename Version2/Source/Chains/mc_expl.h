
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

  /// Use to renumber states to their classified index
  unsigned long* renumber;

  /// If true, stored by columns, otherwise by rows.
  bool isTransposed;

  /// Total number of edges
  int total_edges;

  /** Column indices (unless we are transposed).
      Array of size total_edges.
      All matrices share this array.
  */ 
  int* column_index;

  /** Values assigned to edges.
      Array of size total_edges.
      All matrices share this array.
  */
  LABEL* value;

  /** Pointers to Recurrent-recurrent matrices.
      Outermost array is of dimension #recurrent.
      (The absorbing-absorbing identity is not stored.)
      Innermost arrays are "row pointers" (unless transposed).
  */
  int** RRarcs;

  /** Pointers to transient-transient, transient-recurrent, 
      and transient-absorbing matrices.
      If there are NO transient states, this is NULL.
      Otherwise, dimension is 1+#recurrent+1.
      Innermost arrays again are "row pointers" unless transposed.
  */
  int** Tarcs;
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
    free(blockstart);
  }

  inline unsigned long Renumber(int old_index) const {  
    DCASSERT(renumber);
    CHECK_RANGE(0, old_index, states);
    return renumber[old_index];
  }

  inline void DoneRenumbering() {
    // use this if we do not need the renumbering array any longer
    delete[] renumber;
    renumber = NULL;
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
    CHECK_RANGE(0, c, numClasses()+1);
    return (c<=recurrent) ?
	(blockstart[c+1] - blockstart[c])	// a "big" recurrent class
	:
	1; // must be an absorbing class
  }
  inline bool isIrreducible() const {
    return (1==recurrent) && (0==numTransient()) && (0==numAbsorbing());
  }
  inline int getClass(int newnumber) const {
    CHECK_RANGE(0, newnumber, states);
    int abs_index = newnumber - blockstart[recurrent+1];
    if (abs_index>=0) return abs_index + recurrent + 1;
    // done 
  }

  /** Display (primarily for debugging purposes).
      Note we must be able to "Put" the LABEL class to the stream.
  */ 
  void Show(OutputStream &s) const;
protected:
  void ArrangeMatricesByRows(labeled_digraph <LABEL> *in);
  void ArrangeMatricesByCols(labeled_digraph <LABEL> *in);
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
  states = in->NumNodes();
  isTransposed = in->isTransposed;
 
  // build state to tscc mapping
  renumber = new unsigned long[states];
  int i;
  for (i=0; i<states; i++) renumber[i] = 0; 
  int count = 1+ComputeTSCCs(in, renumber); 

#ifdef DEBUG_CLASSIFY
  Output << "Got " << count << " classes:\nclass array: [";
  Output.PutArray(renumber, states);
  Output << "]\n";
  Output.flush();
#endif

  // Count number of states per class
  blockstart = (int*) malloc(sizeof(int)*(1+count));
  for (i=0; i<count; i++) blockstart[i] = 0;
  for (i=0; i<states; i++) blockstart[renumber[i]]++;
#ifdef DEBUG_CLASSIFY
  Output << "Number of states per class: [";
  Output.PutArray(blockstart, count);
  Output << "]\n";
  Output.flush();
#endif

  // renumber classes so absorbing states are at the end
  // an absorbing state has exactly one state per class 
  // (except possibly for transient states)
  for (i=1; i<count; i++) blockstart[i]--;
  int classnumber = 1;
  blockstart[0] = 0;
  // first number those with more than one state
  recurrent = 0;
  for (i=1; i<count; i++) if (blockstart[i]) {
    blockstart[i] = classnumber;
    classnumber++;
    recurrent++;  // this is a recurrent class with 2 or more states
  }
  // then number the absorbing states
  for (i=1; i<count; i++) if (0==blockstart[i]) {
    blockstart[i] = classnumber;
    classnumber++;
  }
#ifdef DEBUG_CLASSIFY
  Output << "There are " << recurrent << " recurrent (non-absorbing) classes\n";
  Output << "There are " << count - recurrent - 1 << " absorbing classes\n";
  Output << "Renumbering classes: [";
  Output.PutArray(blockstart, count);
  Output << "]\n";
  Output.flush();
#endif
  // renumber the classes
  for (i=0; i<states; i++) renumber[i] = blockstart[renumber[i]];
#ifdef DEBUG_CLASSIFY
  Output << "Renumbered class array: [";
  Output.PutArray(renumber, in->NumNodes());
  Output << "]\n";
  Output.flush();
#endif

  // Re-count number of states per class
  for (i=0; i<count; i++) blockstart[i] = 0;
  for (i=0; i<states; i++) blockstart[renumber[i]]++;
  
  // Sanity check
  for (i=1; i<=recurrent; i++) 
	ASSERT(blockstart[i]>1);
  for (; i<count; i++) 
	ASSERT(1==blockstart[i]);

  // shift the blockstart array
  for (i=count; i; i--) blockstart[i] = blockstart[i-1];
  // accumulate blockstart array
  blockstart[0] = 0;
  for (i=2; i<=count; i++) blockstart[i] += blockstart[i-1];

#ifdef DEBUG_CLASSIFY
  Output << "Accumulated class sizes: [";
  Output.PutArray(blockstart, 1+count);
  Output << "]\n";
  Output.flush();
#endif

  // Change the state class mapping into the state renumbering array, in place!
  for (i=0; i<states; i++) renumber[i] = blockstart[renumber[i]]++; 

  // shift the class sizes back
  for (i=count; i; i--) blockstart[i] = blockstart[i-1];
  blockstart[0] = 0;

  // shrink the blockstart array to its final size
  // (cut off the tail, which holds absorbing state stuff)

  int* foo = (int*) realloc(blockstart, sizeof(int) * (2+recurrent));
  if (NULL==foo)
    OutOfMemoryError("Array resize in MC state classification");
 
  blockstart = foo;

#ifdef DEBUG_CLASSIFY
  Output << "\n\nFinal state classification information:\n";
  Output << "Blockstart array: [";
  Output.PutArray(blockstart, 2+recurrent);
  Output << "]\nRenumber array: [";
  Output.PutArray(renumber, states);
  Output << "]\n";
  Output.flush();
#endif

  // Allocate RRarcs and Tarcs

  total_edges = in->NumEdges();
  RRarcs = new int*[recurrent];
  Tarcs = (numTransient()) ? (new int*[2+recurrent]) : NULL;

  if (isIrreducible()) {
    in->ConvertToStatic();
    RRarcs[0] = in->row_pointer;
    in->row_pointer = NULL;
    column_index = in->column_index;
    in->column_index = NULL;
    value = in->value;
    in->value = NULL;
    delete in;
    DoneRenumbering();  // no renumbering of states necessary
  } else {
    if (isTransposed)
      ArrangeMatricesByCols(in);
    else
      ArrangeMatricesByRows(in);
  }
}

template <class LABEL>
void classified_chain <LABEL> :: ArrangeMatricesByCols(labeled_digraph <LABEL> *in)
{
  DCASSERT(isTransposed);
  DCASSERT(in->isTransposed);
#ifdef DEBUG_CLASSIFY
  Output << "Arranging matrices by columns\n";
  Output.flush();
#endif
}

template <class LABEL>
void classified_chain <LABEL> :: ArrangeMatricesByRows(labeled_digraph <LABEL> *in)
{
  DCASSERT(!isTransposed);
  DCASSERT(!in->isTransposed);
#ifdef DEBUG_CLASSIFY
  Output << "Arranging matrices by columns\n";
  Output.flush();
#endif
}

template <class LABEL>
void classified_chain <LABEL> :: Show(OutputStream &s) const
{
  const char* rowname = (isTransposed) ? "column" : "row";
  const char* colname = (isTransposed) ? "row" : "column";
  s.Pad('-', 60);
  s << "\nClassified chain with " << states << " states total\n";
  s << "\t" << numTransient() << " transient states\n";
  s << "\t" << numClasses() << " recurrent classes, of which\n";
  s << "\t" << numAbsorbing() << " are absorbing states\n";
  s << "Matrices, stored by " << rowname << "s\n";
  s.flush();
  s << colname << " indices: [";
  s.PutArray(column_index, total_edges);
  s << "]\n";
  s.Pad(' ', isTransposed ? 5 : 8);
  s << "values: [";
  s.PutArray(value, total_edges);
  s << "]\n";
  s.flush();

  s << "End of classified chain\n";
  s.Pad('-', 60);
  s << "\n";
  s.flush();
}

#endif

