
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
  RT_Implicit
};

class reachset {
  /// The number of states.  Change to "bigint" soon...
  int size; 
  /// The type of reachset encoding.
  reachset_type encoding;
public:
  union {
    /// Used by explicit storage.
    state_array *flat;
    /// Used by implicit storage.
    void* evmdd;
  };
public:
  /// Empty constructor
  reachset();
  /// Destructor
  ~reachset();
  /// Create an enumerated reachset
  void CreateEnumerated(int s);
  /// Create an explicit reachset
  void CreateExplicit(int s, state_array *f);
  /// Create an implicit reachset
  void CreateImplicit(int s, void* e);

  inline int Size() const { return size; }
  inline reachset_type Type() const { return encoding; }

  // Stuff for accessing states here...

};

#endif

