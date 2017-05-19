
// $Id$

#ifndef INTSET_H
#define INTSET_H


/** Representation for a set of integers.
    Specifically, for representing any 
    subset of the integer interval [0, n) = {0, 1, ..., n-1}.
*/
class intset {
  static char* version;

  class bitvector; // nice.

  bitvector* data;
  bool flip;
  long size;
public:
  /** Constructor.
      Builds an uninitialized subset of [0, N).
        @param  N  Range of values.
  */
  intset(long N=0);
  /// Copy constructor.
  intset(const intset &x);
  ~intset();

  static const char* getVersion();

  /** Resets the size N. 
  */
  void resetSize(long N);

  /** Gives the value N, where this set is a subset of [0, N).
  */
  inline long getSize() const { return size; }

  /** Gives the cardinality of the set.
  */
  long cardinality() const;

  /** Is the set empty?
      Usually more efficent than "cardinality() == 0"
  */
  bool isEmpty() const;

  // element manipulation

  /** Complement the set.
  */
  inline void complement() { flip = !flip; }

  /** Adds an element to the set.
      If the element is out of range, nothing happens.
        @param  n   Element to add.
                    Should be in the range
                    0 <= n < size
  */
  void addElement(long n);

  /** Remove an element from the set.
      If the element is out of range, or not in the set, nothing happens.
        @param  n   Element to add.
                    Should be in the range
                    0 <= n < size
  */
  void removeElement(long n);

  /** Adds a range of elements to the set.
      Equivalent to calling addElement(a), addElement(a+1), ..., addElement(b).
      If b < a, then nothing is added.
        @param  a  First element to add.
        @param  b  Last element to add.
  */
  void addRange(long a, long b); 

  /** Removes a range of elements from the set.
      Equivalent to calling removeElement(a), 
      removeElement(a+1), ..., removeElement(b).
      If b < a, then nothing is added.
        @param  a  First element to remove.
        @param  b  Last element to remove.
  */
  void removeRange(long a, long b); 

  /// Add all elements to the set.
  void addAll();

  /// Remove all elements from the set.
  void removeAll();

  /** Determine if an element is in the set.
      @param  n   Element to check.
      @return 0,  if n < 0 or n >= size, or n not in the set.
              1   if n is in the set.
  */
  bool contains(long n) const;

  /** Test an element, then add to the set.
      Equivalent to calling contains() and then addElement().
  */
  bool testAndAdd(long n);

  /** Test an element, then remove from the set.
      Equivalent to calling contains() and then removeElement().
  */
  bool testAndRemove(long n);

  /** For enumerating elements.
      @param  n   Last integer visited, can be anything.
      @return     The smallest integer i in the set, with n < i.
                  If no such i exists, returns -1.
  */
  long getSmallestAfter(long n) const;

  // assignment operators

  /** Assignment operator.
      Makes this set equal to x.
      Deep copy.
        @param  x   Another set.
  */
  void assignFrom(const intset &x);

  /** Assignment operator. 
      Makes this set equal to x.
      Shallow copy.
        @param  x  Another set.
  */
  void operator=(const intset &x);

  // In-place operators

  /// Add elements of x to this set.
  void operator += (const intset &x);

  /// Intersect this set with x, modifying this set.
  void operator *= (const intset &x);

  /// Remove elements of x from this set.
  void operator -= (const intset &x);

  // friends

  // comparison operators

  friend bool operator==(const intset &x, const intset &y);
  friend bool operator<=(const intset &x, const intset &y);
  friend bool operator< (const intset &x, const intset &y);

  // set manipulation operators

  friend intset operator+ (const intset &x, const intset &y);
  friend intset operator* (const intset &x, const intset &y);
  friend intset operator- (const intset &x, const intset &y);
  friend intset operator! (const intset &x);

#ifdef INTSET_DEVELOPMENT_CODE
  void dump(FILE* strm);
#endif
};

/** Check equality of two sets.
  @param  x   first set
  @param  y   second set, not necessarily same size as x
  @return 1   iff sets x and y contain the same elements
*/
bool operator==(const intset &x, const intset &y);

/** Check inequality of two sets.
  @param  x   first set
  @param  y   second set, not necessarily same size as x
  @return 1   iff sets x and y do not contain the same elements
*/
inline bool operator!=(const intset &x, const intset &y)
{
  return ! (x==y);
}

/** Check for subsets
  @param  x   first set
  @param  y   second set, not necessarily same size as x
  @return 1   iff x is a subset of y
*/
bool operator<=(const intset &x, const intset &y);

/** Check for supersets
  @param  x   first set
  @param  y   second set, not necessarily same size as x
  @return 1   iff x is a superset of y
*/
inline bool operator>=(const intset &x, const intset &y)
{
  return y <= x;
}

/** Check for proper subsets
  @param  x   first set
  @param  y   second set, not necessarily same size as x
  @return 1   iff x is a subset of y, and x != y
*/
inline bool operator< (const intset &x, const intset &y)
{
  if (! (x <= y)) return false;
  return (x != y);
}

/** Check for proper supersets
  @param  x   first set
  @param  y   second set, not necessarily same size as x
  @return 1   iff x is a superset of y, and x != y
*/
inline bool operator> (const intset &x, const intset &y)
{
  if (! (y <= x)) return false;
  return (x != y);
}

/// Builds the union of sets x and y.
intset operator+ (const intset &x, const intset &y);

  /// Builds the intersection of sets x and y.
intset operator* (const intset &x, const intset &y);

  /// Builds the difference of sets x and y.
intset operator- (const intset &x, const intset &y);

  /// Builds the complement of set x.
intset operator! (const intset &x);


#endif
