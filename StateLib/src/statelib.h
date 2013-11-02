
// $Id$

/*! \file statelib.h

    State storage library.

  New, abstract class interface.
*/

#ifndef STATELIB_H
#define STATELIB_H

namespace StateLib {

  /// Class for errors (a simple one)
  class error {
  public:
    enum code {
      /// Success; no error.
      Success = 0,
      /// Malloc failure.
      NoMemory,
      /// static/dynamic mismatch
      Static,
      /// Stack overflow
      StackOver,
      /// Bad Handle
      BadHandle,
      /// Size mismatch
      SizeMismatch,
      /// Not implemented yet!
      NotImplemented,
      /// Bizarre, internal error
      Internal
    };
    error(code ec) { errcode = ec; }
    const char* getName() const;
  private:
    code errcode;
  };


  // ======================================================================
  // |                                                                    |
  // |                     state collection interface                     |
  // |                                                                    |
  // ======================================================================

  /** Class for a state collection.

      Each state collection holds a (potentially large) number
      of states, stored in a compressed format.

      Note: the actual implementation is derived from this class,
      so we can completely hide implementation details.
  */
  class state_coll {
  protected:
    /// Number of states in the collection.
    long numstates;
  public:
    state_coll();
    virtual ~state_coll();

    inline long Size() const { return numstates; }

    /** Does the collection store the state sizes?

        If yes, we can store variable-sized states
        in the collection, and retrieve the state
        size when we retrieve the state.
  
        If no, we must know the size of the state
        before we retrieve it from the collection.
    */
    virtual bool StateSizesAreStored() const = 0;

    /** Does the collection use state indices for handles?

        If yes, then the first inserted state has handle 0,
        the second inserted state has handle 1, 
        the third inserted state has handle 2, etc.
  
        If no, then all we know about state handles are that
        they are monotone increasing.
    */
    virtual bool StateHandlesAreIndexes() const = 0;

    /** Clear the collection.

        All states in the collection are deleted, but the collection
        itself (and all its memory) is retained.
        Similar to (but much more efficient than) deleting
        and creating a new collection.
    */
    virtual void Clear() = 0;
  
    /** Add a state to the collection.

          @param  state   The state to add.
          @param  size    Size of the state

          @return   A handle to the state, for use with other methods.
                    Throws an error if there is not enough memory.
                    Otherwise, the handle will be a non-negative integer.
                    Handles will be monotone increasing, i.e., if state a
                    is added before state b, then the handle for state a will 
                    be less than the handle for state b.
    */
    virtual long  AddState(const int* state, int size) = 0;


    /** Remove the last added state.
        This allows to "un-do" the last call to AddState().

        @param  hndl  Handle of the last state.
                      Behavior is undefined if \a hndl
                      is not the last state.

        @return true  if a state was removed,
                false otherwise.
    */
    virtual bool  PopLast(long hndl) = 0;


    /** Fill in a state with known size.
        The state must have been previously added to the collection.

          @param  hndl    Handle of the state to obtain.

          @param  state   (Output) array to hold the state.
                          Must have at least \a size elements.

          @param  size    The size of the state.
                          If too small, the end of the state will be trucated.
                          If too large, the end of the state will be appended
                          with zeroes.

          @return  -1,    on failure;
                    0,    if the current state is the last one,
                    the handle of the next state, otherwise.
    */
    virtual long  GetStateKnown(long hndl, int* state, int size) const = 0;


    /** Fill in a state with unknown size.
        The operation must be supported by the collection,
        i.e., StateSizesAreStored() must return true.
        The state must have been previously added to the collection.

        This function may also be used to obtain the size of a state
        in the collection, by passing 0 for parameter \a size.

          @param  hndl  Handle of the state to obtain.

          @param  state (Output) array to hold the state.
                        Must have at least \a size elements.

          @param  size  The size of the \a state array.
                        No more than \a size entries will
                        be written to the array, even if
                        the actual state size is larger.

          @return -1,  on failure;
                  actual size of the state, otherwise.
    */
    virtual int GetStateUnknown(long hndl, int* state, int size) const = 0;


    /** Get the handle of the first state in the collection.

          @return -1, if the collection has no states;
                  a valid handle, otherwise.
    */
    virtual long  FirstHandle() const = 0;


    /** Get the next handle.
        Allows the traversal of all states in the collection.
        Note: the same information is returned by GetStateKnown().

          @param  hndl  Current handle.

          @return -1, if the current handle is the last state;
                  a valid handle, otherwise.
    */
    virtual long  NextHandle(long hndl) const = 0;


    /** Compares the encodings of two states.
        Note: this is done by comparing the encodings of the states
        without unpacking.  Requires index handles.
        Very fast (based on memcmp).

        Useful, say, for use with a "dictionary" data structure to
        keep track of unique states, where the sequence would be:
        add new state; check if it exists already; if so, remove last added.

          @param  h1  Handle of state 1.
          @param  h2  Handle of state 2.

          @return An integer with the same sign as
                  (encoding of state \a h1) - (encoding of state \a h2).
                  If index handles are not used, or if
                  either handle is invalid, returns 0.
    */
    virtual int CompareHH(long h1, long h2) const = 0;


    /** Compares an encoded state with a full state.
        The encoded state is unpacked "on the fly", but
        only as much as needed.
        Slower that CompareHH(), but does not require
        index handles.

          @param  hndl  Handle of encoded state (within \a coll).
          @param  size  Size of the full state.
          @param  state Full state; an array of dimension \a size.

          @return An integer with the same sign as
                  (encoding of unpacked state \a hndl) - (\a state).
                  If the handle is invalid, returns 0.
    */
    virtual int CompareHF(long hndl, int size, const int* state) const = 0;


    /** Gives a hash value for a state.
        For speed, the hash function is computed on the encoding,
        not the unpacked state.
        Requires index handles.

          @param  hndl  Handle of encoded state (within \a coll).
          @param  b     Number of bits to hash into.
                        Must be less than the number of bits in an
                        unsigned long.

          @return A b-bit hash based on the state.
                  If index handles are not used, or if
                  the encoding is invalid, returns 0.
    */
    virtual unsigned long Hash(long hndl, int b) const = 0; 


    /** Convert from "index handles" to "arbitrary handles".
        Useful to reduce memory requirements, say, after
        a set of states has been constructed.

        @return   An array of size "number of states", where
                  a[i] gives the arbitrary handle of state
                  with index i.
                  If the collection didn't use index handles,
                  then returns a null pointer.
    */
    virtual long* RemoveIndexHandles() = 0;

    // reporting methods


    /// Total memory used.
    virtual long ReportMemTotal() const = 0;


    /// Number of methods used to encode states.
    virtual int NumEncodingMethods() const = 0;


    /** Get a description of an encoding method.
      
          @param  m  Between 0 and NumEncodingMethods() - 1.

          @return   NULL, if m is out of bounds,
                    a description of the desired encoding method, otherwise.
    */
    virtual const char* EncodingMethod(int m) const = 0;


    /** Number of states encoded in a certain way.
  
          @param  m   The method to count for.
                      Between 0 and NumEncodingMethods() - 1.

          @return 0, if m is out of bounds;
                  number of states encoded with method \a m, otherwise.
    */
    virtual long ReportEncodingCount(int m) const = 0;
  };


  // ======================================================================
  // |                                                                    |
  // |                     state "database" interface                     |
  // |                                                                    |
  // ======================================================================

  /** Class for a state database.

      A "dictionary" structure (for unique inserts and searches)
      plus a state collection.

      There are two modes: static and dynamic.  In static mode,
      states cannot be added, but we may require less memory.

      Note: the actual implementation is derived from this class,
      so we can (mostly) hide implementation details.
  */
  class state_db {
  protected:
    bool is_static;
    long num_states;
    long* index2handle;
  public:
    state_db();
    virtual ~state_db();

    inline long Size() const { return num_states; }
    inline bool IsStatic() const { return is_static; }

    inline bool CollectionUsesIndexes() const { return 0==index2handle; }
    inline bool CollectionUsesHandles() const { return index2handle; }

    // Raw speed.  Doesn't check anything!
    inline long getHandle(long index) const { return index2handle[index]; }
  
    /** Adjust the stack size, as appropriate.

        For certain implementations, a stack is used during state searches.
        This method allows the maximum stack size to be specified.
        If the current stack size is larger than the one specified,
        we will reduce the stack size if possible.

        Throws an error if there is not enough memory.

        @param  max_stack   Maximum number of stack entries.
    */
    virtual void SetMaximumStackSize(long max_stack) = 0;

    /** Get the current maximum stack size.
      
          @return Upper stack limit.  Can be 0 if the implementation
                  does not require any stack.
    */
    virtual long GetMaximumStackSize() const = 0;

    /** Clear the database.

        All states in the database (and underlying collection) are deleted, 
        but all memory is retained.

        Does nothing if we are "static".
    */
    virtual void Clear() = 0;

    /** Convert the database to static form.
        This means no insertions are allowed, but it might
        free a significant amount of memory.
        Quick success if the database is already static.

        Throws an appropriate error when necessary.

        @param  tighten   Should we free up any unnecessary memory.
                          (i.e., memory used only by dynamic mode).
                          Usually a good idea, unless we plan to
                          convert back to dynamic mode.
  
    */
    virtual void ConvertToStatic(bool tighten) = 0;

    /** Convert the database to dynamic form.
        Insertions and searches are allowed.
        Quick success if the database is already dynamic.

        An exception will be thrown on error.

          @param  tighten   Should we free up any unnecessary memory.
                            (i.e., memory used only by static mode).
                            Usually a good idea, unless we plan to
                            convert back to static mode.

    */
    virtual void ConvertToDynamic(bool tighten) = 0;

    /** Unique insertion.
        If the given state is already in the database, return its index;
        otherwise, add the state and return the (new) index.
        The database must be "dynamic".

        An exception will be thrown on error.
        Note that a "Static" error is thrown only if the state
        would need to be inserted.
  
          @param  state   The state to insert.
          @param  size    The size of the state.

          @return   The index of the inserted (or existing) state.
    */  
    virtual long InsertState(const int* state, int size) = 0;

    /** Search.
        If the given state is already in the database, return its index.
        Otherwise, return -1.
        The database can be static or dynamic.

          @param  state   The state to search for.
          @param  size    The size of the state.

          @return   The index of the state if it is present;
                    -1 if not found.
    */
    virtual long FindState(const int* state, int size) = 0;


    /** Fill in a state with known size.
        Calls method of the same name for the database's underlying
        state collection, of type state_coll.

        @param  index   Index of the state to obtain.
        @param  state   (Output) array to hold the state.
                        Must have at least \a size elements.
        @param  size    The size of the state.
                        If too small, the end of the state will be trucated.
                        If too large, the end of the state will be appended
                        with zeroes.

        @return  -1,  on failure;
                  0,  if the current state is the last one,
                  the index of the next state, otherwise.
    */
    virtual long  GetStateKnown(long index, int* state, int size) const = 0;

    /** Fill in a state with unknown size.
        Calls method of the same name for the database's underlying
        state collection, of type state_coll.
  
        @param  index   Index of the state to obtain.
        @param  state   (Output) array to hold the state.
                        Must have at least \a size elements.
        @param  size    The size of the \a state array.
                        No more than \a size entries will
                        be written to the array, even if
                        the actual state size is larger.
        @return -1, on failure;
                actual size of the state, otherwise.
    */
    virtual int GetStateUnknown(long index, int* state, int size) const = 0;


    /** Get the underlying state collection.
        This must not be modified.
          @return    Pointer to the collection of states.
    */
    virtual const state_coll* GetStateCollection() const = 0;


    /** Grab the underlying state collection, destroy the database.
          @return    Pointer to the collection of states.
    */
    virtual state_coll* TakeStateCollection() = 0;

    /** Grab the index to handle mapping, if any, destroy the database.
        May be called before or after TakeStateCollection().
          @return   0, or pointer to array that converts from
                    indexes to handles.
    */
    long* TakeIndexMap();

    /// Total memory used, including by the underlying state collection.
    virtual long ReportMemTotal() const = 0;

    /// For debugging: dump the memory used to represent a state.
    // virtual void DumpState(FILE* s, long index) const = 0;

    /// For debugging: dump the internal representation, as an input to dot.
    virtual void DumpDot(FILE*) = 0;
  };


  // ======================================================================
  // |                                                                    |
  // |                        Front-end  interface                        |
  // |                                                                    |
  // ======================================================================


  /** Get the name and version info of the library.
      The string should not be modified or deleted.
        @return    Information string.
  */
  const char*  LibraryVersion();


  /** Create a new state collection.
      The collection can be destroyed using "delete".
  
        @param  useindices  If true, the state handles are also indices:
                            handle 0 is the first state added, 
                            handle 1 is the second state added, etc.
                            This requires extra storage (an integer 
                            per state in the collection).

                            If false, the handles are monotone but
                            have no meaning outside the library.

        @param  storesize   If true, we store the size of each state
                            in the collection.  This allows the use
                            of function STATE_CollGetUnknown().
                            This may increase the memory required to
                            store the states.

                            If false, we must pass the state size 
                            when retrieving a state from the collection.

        @return   0  on error, e.g., out of memory;
                  a valid collection, otherwise.
  */
  state_coll*  CreateCollection(bool useindices, bool storesize);


  /** Types for state databases.
      I.e., different explicit dictionary structures.
  */
  enum state_db_type {
    SDBT_Splay,
    SDBT_RedBlack,
    SDBT_Hash
  };


  /** Create a new state database.
      The database can be destroyed using "delete".
      The database will create its own state collection.

        @param  which       The type of "database" to use.
  
        @param  useindices  Passed to the created state collection.

        @param  storesize   Passed to the created state collection.

        @return   0 on error, e.g., out of memory;
                  a valid database, otherwise.
  */
  state_db*  CreateStateDB(state_db_type which,
              bool useindices, bool storesize);


} // namespace

#endif


