
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
   
    The original P (or Q) matrix is then split into 
	P = Pself + Ptr
    where Pself is a block-diagonal matrix that holds all arcs within 
    recurrent classes (or transient-transient).
    Because the states are reordered, we can get the desired submatrix
    from the appropriate subset of rows/columns.

    Then there is Ptr, the transient-recurrent + transient-absorbing arcs.
    It is stored in a single matrix, because realistically, it is only
    used in a "transient probability" * Ptr multiplication to obtain
    the corresponding "recurrent / absorbing probability".
*/
template <class LABEL>
struct classified_chain {
  /// Total number of states
  int states;

  /** The overall graph that we carve up.
      All fields are shared amongst the classified chain except
 	row_pointer,
      which will be changed around depending on which subgraph we 
      need to access.
  */
  labeled_digraph <LABEL> *graph;

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

  /** The block-diagonal, "self" matrix.
      TT, R1, R2, ..., Rn, and A all stored in one.
      These are row/column pointers of dimension #states - #absorbing
      (because there are no absorbing-absorbing arcs stored). 
  */
  int* self_arcs;

  /** The transient-recurrent/absorbing matrix.
      If there are NO transient states, this is NULL.
      These are row/column pointers of the #transient x #states matrix.
  */
  int* TRarcs;
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
      Valid for 0 <= c <= numClasses().
      c = 0 is for the transient states.
      1 <= c <= recurrent is for recurrent class c.
      c > recurrent is for absorbing states.
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
    // state is not absorbing.
    // find c such that blockstart[c] <= newnumber < blockstart[c+1]
    int low = 0;
    int high = recurrent+1; 
    while (low+1<high) {
      int mid = (low+high)/2;
      if (blockstart[mid] == newnumber) return mid;
      if (blockstart[mid] < newnumber) low=mid; else high=mid;
    }
    return low;
  }

  /** Display (primarily for debugging purposes).
      Note we must be able to "Put" the LABEL class to the stream.
  */ 
  void Show(OutputStream &s) const;
protected:
  void ArrangeMatricesByRows();
  void ArrangeMatricesByCols();
};


#define DEBUG_CLASSIFY

template <class LABEL>
classified_chain <LABEL> :: classified_chain(labeled_digraph <LABEL> *in)
{
  DCASSERT(in);
  graph = in;
  states = graph->NumNodes();
#ifdef DEBUG_CLASSIFY
  Output << "Starting to classify chain\n";
  Output.flush();
#endif
  // build state to tscc mapping
  renumber = new unsigned long[states];
  int i;
  for (i=0; i<states; i++) renumber[i] = 0; 
  int count = 1+ComputeTSCCs(graph, renumber); 

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
  Output.PutArray(renumber, states);
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


  TRarcs = NULL;

  if (isIrreducible()) {
    graph->ConvertToStatic();
    self_arcs = graph->row_pointer;
    DoneRenumbering();  // no renumbering of states necessary
  } else {
    graph->ConvertToDynamic();
    graph->CircularToTerminated();
    if (graph->isTransposed)
      ArrangeMatricesByCols();
    else
      ArrangeMatricesByRows();
  }
}

template <class LABEL>
void classified_chain <LABEL> :: ArrangeMatricesByCols()
{
  DCASSERT(graph->isTransposed);
#ifdef DEBUG_CLASSIFY
  Output << "Arranging matrices by columns\n";
  Output.flush();
#endif
  // Allocate RRarcs and Tarcs
}

template <class LABEL>
void classified_chain <LABEL> :: ArrangeMatricesByRows()
{
  int i;
  DCASSERT(!graph->isTransposed);
#ifdef DEBUG_CLASSIFY
  Output << "Arranging matrices by rows\n";
  Output.flush();
#endif
  // allocate row pointers for T-R, T-A matrix and set all rows empty
  if (numTransient()) {
    TRarcs = (int*) malloc(sizeof(int) * numTransient()+1);
    if (NULL==TRarcs) OutOfMemoryError("Matrix classify");
    for (int j=numTransient(); j>=0; j--) TRarcs[j] = -1;
  }
  // allocate row pointers for "within class" matrix, set all rows empty
  self_arcs = (int*) malloc(sizeof(int) * (1+states - numAbsorbing()));
  if (NULL==self_arcs) OutOfMemoryError("Matrix classify");
  for (int j=states-numAbsorbing(); j>=0; j--) self_arcs[j] = -1;
  
  // Traverse old rows, translate each item and add to appropriate submatrix
  int* oldrow = graph->row_pointer;
  
  for (i=0; i<states; i++) {
    int new_i = Renumber(i);
    if (isAbsorbing(new_i)) continue;
    if (!isTransient(new_i)) graph->row_pointer = self_arcs;
    int e;
    while (oldrow[i]>=0) {
      e = oldrow[i];
      oldrow[i] = graph->next[e];
      int new_j = Renumber(graph->column_index[e]);
      graph->column_index[e] = new_j;
      if (isTransient(new_i)) {
        DCASSERT(TRarcs);
        if (isTransient(new_j))
          graph->row_pointer = self_arcs;
        else
          graph->row_pointer = TRarcs;
      }
      // add this element to the submatrix
      graph->AddToOrderedCircularList(new_i, e);
    } // while oldrow[i]
  } // for i
  free(oldrow);

  // Convert matrices to non-circular lists
  if (numTransient()) {
    graph->num_nodes = numTransient();
    graph->row_pointer = TRarcs;
    graph->CircularToTerminated();
  }
  graph->num_nodes = states - numAbsorbing();
  graph->row_pointer = self_arcs;
  graph->CircularToTerminated();
  
  // Defragment submatrices
  graph->Defragment(0);

  graph->num_nodes = numTransient();
  graph->row_pointer = TRarcs;
  graph->Defragment(self_arcs[states-numAbsorbing()]);

  // Compact arrays and such
  free(graph->next);
  graph->next = NULL;
  graph->ResizeEdges(graph->num_edges);
}

template <class LABEL>
void classified_chain <LABEL> :: Show(OutputStream &s) const
{
  const char* rowname = (graph->isTransposed) ? "column" : "row";
  const char* colname = (graph->isTransposed) ? "row" : "column";
  s.Pad('-', 60);
  s << "\nClassified chain with " << states << " states total\n";
  s << "\t" << numTransient() << " transient states\n";
  s << "\t" << numClasses() << " recurrent classes, of which\n";
  s << "\t" << numAbsorbing() << " are absorbing states\n";
  s << "Matrices, stored by " << rowname << "s\n";
  s.flush();
  s << colname << " indices: [";
  s.PutArray(graph->column_index, graph->NumEdges());
  s << "]\n";
  s.Pad(' ', graph->isTransposed ? 5 : 8);
  s << "values: [";
  s.PutArray(graph->value, graph->NumEdges());
  s << "]\n";
  if (graph->next) {
    s.Pad(' ', graph->isTransposed ? 7 : 10);
    s << "next: [";
    s.PutArray(graph->next, graph->NumEdges());
    s << "]\n";
  }
  s.flush();

  s << "self matrix\n";
  s << "\t" << rowname << " pointers: [";
  s.PutArray(self_arcs, 1+states-numAbsorbing());
  s << "]\n";
  if (numTransient()) {
    s << "TR matrix\n";
    s << "\t" << rowname << " pointers: [";
    s.PutArray(TRarcs, graph->isTransposed ? 1+states : 1+numTransient());
    s << "]\n";
  }
  s << "End of classified chain\n";
  s.Pad('-', 60);
  s << "\n";
  s.flush();
}

#endif

