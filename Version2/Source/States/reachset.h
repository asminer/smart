
// $Id$

#ifndef REACHSET_H
#define REACHSET_H

#include "flatss.h"

/** @name reachset.h
    @type File
    @args \

    Class used to store a reachability set.

    Instead of virtual functions and inheritance, we use a single
    class which tells you the storage method (see below), and contains
    pointers to the appropriate data structure.  This avoids virtual
    function calls, but usage is a bit trickier.

    The possible storage options for a reachability set are listed
    in the enumerated type "reachset_type", below.

    Note: a single reachability set should include both a tangible
    and vanishing portion.  The vanishing portion will be empty if
    all events are timed, or if the elimination option is used.
*/

/// Possible storage options for compressed reachability set.
enum reachset_type {
  /// Empty reachability set (use for initializing)
  RT_None,
  /// Enumerated.  Used when the states are listed in a model (e.g., DTMC)
  RT_Enumerated,
  /// Explicit flat.  Stored in a state array class.
  RT_Explicit,
  /// EV+MDD.  Not implemented yet.
  RT_Implicit,
  /// There was an error generating it.
  RT_Error
};

class reachset {
  /// The type of reachset encoding.
  reachset_type encoding;
  union {
    /// Used by enumerated storage.  
    int size;
    /// Used by explicit storage.
    flatss* flat;
    /// Used by implicit storage.
    void* evmdd;
  };
public:
  /// Empty constructor
  reachset();
  /// Destructor
  ~reachset();

  inline reachset_type Storage() const { return encoding; }

  /// Create an enumerated reachset
  void CreateEnumerated(int s);
  /// Create an explicit reachset
  void CreateExplicit(flatss* f);
  /// Create an implicit reachset
  void CreateImplicit(void* e);
  /// Create an error reachset
  void CreateError();

  inline int Enumerated() const {
    DCASSERT(encoding == RT_Enumerated);
    return size;
  }
  inline flatss* Explicit() const {
    DCASSERT(encoding == RT_Explicit);
    return flat;
  }
  inline void* Implicit() const {
    DCASSERT(encoding == RT_Implicit);
    return evmdd;
  }

  inline int NumTangible() const { 
    switch (encoding) {
      case RT_Enumerated:	return size; 
      case RT_Explicit: 	return flat->NumTangible();
      default: 			return 0;
    }
  }
  inline int NumVanishing() const { 
    switch (encoding) {
      case RT_Enumerated:	return 0; 
      case RT_Explicit: 	return flat->NumVanishing();
      default: 			return 0;
    }
  }
};

#endif

