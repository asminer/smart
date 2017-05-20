
/** \file intervals.h

    Module for intervals.
*/

#ifndef INTERVALS_H
#define INTERVALS_H

#include "result.h"

class type;
class exprman;

/** Interval point.
    Used for a single point in an interval.
*/
class interval_point {
    /// This is useful for printing.
    static const type* reals; 
    friend void InitIntervals(const exprman *);

  public:
    enum status_code {
      normal_open,
      normal_closed,
      infinity_open,
      infinity_closed,
      unknown,
      null
    };
  private:
    status_code status;
    double value;

  public:
    interval_point();

    /*
    Handy operators
    */
    inline bool operator==(const interval_point &p) const {
      return (status == p.status) && (value == p.value);
    }

    /*
    Getters
    */

    /// @return true iff the interval contains the point.
    inline bool contains() const { 
      return (normal_closed == status) || (infinity_closed == status);
    }
    /// @return true iff the point is an ordinary real.
    inline bool isNormal() const {
      return (normal_closed == status) || (normal_open == status);
    }
    /// @return true iff the point is +-infinity.
    inline bool isInfinity() const {
      return (infinity_closed == status) || (infinity_open == status);
    }
    /// @return true iff the point is unknown.
    inline bool isUnknown() const {
      return (unknown == status);
    }
    /// @return true iff the point is null.
    inline bool isNull() const {
      return (null == status);
    }
    /// @return The sign of the point, works for finite & infinite.
    inline int getSign() const {
      DCASSERT(isNormal() || isInfinity());
      return SIGN(value);
    }
    /// @return The value of the point, assuming it is an ordinary real.
    inline double getValue() const {
      DCASSERT(isNormal());
      return value;
    }
    /// Fill a result based on this point
    inline void getAsResult(result &v) const {
      switch (status) {
        case null:              v.setNull();                return;
        case unknown:           v.setUnknown();             return;
        case infinity_open:
        case infinity_closed:   v.setInfinity(SIGN(value)); return;
        case normal_open:
        case normal_closed:     v.setReal(value);           return;
        default:  DCASSERT(0);
      }
    }

    /*
    Setters
    */

    /** Set the point to be an ordinary real.
        @param  contains  If true, the interval will be "closed" on this point.
        @param  v         Value to set for the point.
    */
    inline void setNormal(bool contains, double v) {
      status = contains ? normal_closed : normal_open;
      value = v;
    }
    /** Set the point to be infinity.
          @param  contains  If true, the interval will be "closed" on this point.
          @param  s         Sign for infinity.
    */
    inline void setInfinity(bool contains, int s) {
      status = contains ? infinity_closed : infinity_open;
      value = SIGN(s);
    }
    /// Set the point to be "unknown".
    inline void setUnknown() { 
      value = 0;
      status = unknown; 
    }
    /// Set the point to be null.
    inline void setNull() { 
      value = 0;
      status = null; 
    }
    /// Set the point from a result
    void setFrom(const result &v, const type* st);

    /// Set the inclusion value
    inline void setInclusion(bool contains) {
      switch (status) {
        case normal_open:
        case normal_closed:
                              status = contains ? normal_closed : normal_open;
                              return;

        case infinity_open:
        case infinity_closed:
                              status = contains ? infinity_closed : infinity_open;
                              return;

        default:
                              // NO CHANGE
                              return;
      }
    }
};


/** Interval class.
    Used for representing intervals such as
      [3, 4],
      (3, 4],
      (3, 4),
      [3, 3],
      [-infinity, 3],
      [?, infinity)
*/
class interval_object : public shared_object {

  /// This is useful for printing.
  static const type* reals; 
  friend void InitIntervals(const exprman *);

  interval_point left, right;

public:
  /// Empty constructor.
  interval_object();

  /// Construct a point interval from a single value.
  interval_object(const result& r, const type* st);
protected:
  virtual ~interval_object();
public:
  // required for shared_object
  virtual bool Print(OutputStream &s, int width) const;
  virtual bool Equals(const shared_object *o) const;

  inline       interval_point& Left()       { return left; }
  inline const interval_point& Left() const { return left; }

  inline       interval_point& Right()       { return right; }
  inline const interval_point& Right() const { return right; }

};

/** Initialize interval module.
*/
void InitIntervals(const exprman* em);

/** Computes the union of two intervals.
    Note: even if there is a "gap", we return an interval.
    E.g., the union of [1, 3] and [5, 7] is [1, 7].
      @param  c  Will be set to the union of \a a and \a b.
      @param  a  First union operand.
      @param  b  Second union operand.
*/
void computeUnion(interval_object &c, const interval_object &a, const interval_object &b);


/** Computes the minimum of two intervals.
    E.g., the minimum of [1, 5] and [2, 4] is [1, 4].
      @param  c  Will be set to the minimum of \a a and \a b.
      @param  a  First operand.
      @param  b  Second operand.
*/
void computeMinimum(interval_object &c, const interval_object &a, const interval_object &b);

/** Computes the maximum of two intervals.
    E.g., the maximum of [1, 5] and [2, 4] is [2, 5].
      @param  c  Will be set to the maximum of \a a and \a b.
      @param  a  First operand.
      @param  b  Second operand.
*/
void computeMaximum(interval_object &c, const interval_object &a, const interval_object &b);

/**
    Swap a and b if necessary,
    so that the left point of a is less than the left point of b.
*/
void sortLeft(interval_object* &a, interval_object* &b);

/**
    Swap a and b if necessary,
    so that the right point of a is less than the right point of b.
*/
void sortRight(interval_object* &a, interval_object* &b);

#endif
