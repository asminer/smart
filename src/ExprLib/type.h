
// $Id$

#ifndef TYPE_H
#define TYPE_H

#include "../include/defines.h"
#include "../include/shared.h"
#include <string.h>

class result;
class io_environ;
class OutputStream;

// all of this required for infinity string option.
// class option;
// class option_manager;
class exprman;

typedef unsigned char  modifier;

const modifier   DETERM = 0;
const modifier   PHASE = 1;
const modifier  RAND  = 2;
const modifier  ANY_MODIFIER = 254;
const modifier  NO_SUCH_MODIFIER = 255;

// TBD: how much of this can be hidden?

// ******************************************************************
// *                                                                *
// *                           type class                           *
// *                                                                *
// ******************************************************************

class simple_type;

/** New type mechanism.
    This is an abstract base class.
    Since we only have a few types, this should make life significantly
    easier without introducing too much overhead.
*/
class type {
  const char* name;
  bool is_void;
  bool func_definable;
  bool var_definable;
  bool printable;

protected:
  static char* infinity_string;
  friend void InitTypeOptions(exprman* em);

  inline void setVoid() { is_void = true; }
public:
  type(const char* n);
  virtual ~type(); 
  
  static const char* getInfinityString() { return infinity_string; }

  inline const char* getName() const { return name; }
  inline bool matches(const char* n) const { return 0 == strcmp(n, name); }
  virtual bool matchesOWD(const char* n) const;

  inline void NoFunctions() { func_definable = false; }
  inline void NoVariables() { var_definable = false; }

  inline bool canDefineFuncOfThis() const { return func_definable; }
  inline bool canDefineVarOfThis() const { return var_definable; }
  
  inline bool isVoid() const { return is_void; }
  virtual bool isAFormalism() const;

  virtual const type* getSetElemType() const;
  inline bool isASet() const { return getSetElemType(); }
  virtual const type* getSetOfThis() const;

  virtual modifier getModifier() const;
  virtual const type* modifyType(modifier m) const;
  virtual const type* removeModif() const;
  
  virtual bool hasProc() const;
  virtual const type* removeProc() const;
  virtual const type* addProc() const;
  virtual void setProc(const type* t);

  /// Strips all set, proc, modifiers.
  virtual const simple_type* getBaseType() const = 0;

  /// Neat trick: change the base type, keep set, proc, modifier status.
  virtual const type* changeBaseType(const type* newbase) const;

  /** Comparison, for purposes of maintaining sets of this type.
      Default behavior is to throw an error.
      Works like "strcmp". 
      Any total ordering can be used for elements of the type,
      even an ordering different from operators "<", ">", etc.
      But it must be transitive: 
        compare(a,b)>0 and compare(b,c)>0 implies compare(a,c)>0
      and it must be the case that
        compare(a,b)==0 iff a==b
     
      @param  a  First item.
      @param  b  second item.
      @return positive,   if a is larger than b,
              zero,       if a equals b,
              negative,   if a is less than b.
  */
  virtual int compare(const result& a, const result& b) const;

  /// Can we print objects of this type.
  inline bool isPrintable() const { return printable; }
  inline void setPrintable() { printable = true; }

  /** Print a result of this type.
      We must be "printable" according to isPrintable().
        @param  s  Stream to write to.
        @param  r  Result to display.
        @param  w  Width to use.
        @param  p  Precision to use.
  */
  bool print(OutputStream &s, const result& r, int w=0, int p=-1) const;

  /** Show a result.
      Just like print(), except for strings:
      show() will give you "the string.\n",
      while print() will give you the string.
  */
  void show(OutputStream &s, const result& r) const;

  /** Fill the result, from a string.
      Works only for "simple" types.
      If the desired type is "STRING", then we simply
      copy the string.
      If the desired type is "INT" or "REAL",
      then the following special strings are recognized,
      in addition to the usual numerical ones.
        infinity  : for positive infinity
        -infinity  : for negative infinity
        ?    : for "don't know"
      On return, the result will hold the appropriate value,
      or null if the conversion was not possible.
        @param  r  Where the result will be stored.
        @param  s  Input string.
        @return true on success.
  */
  virtual void assignFromString(result& r, const char* s) const;

  /** Determine if two results of this type are equal.
        @param  x  First result.
        @param  y  Second result.
        @return true  iff x = y.
  */
  virtual bool equals(const result &x, const result &y) const;

protected:
  virtual bool print_normal(OutputStream &s, const result& r, int w=0, int p=-1) const;
  virtual void show_normal(OutputStream &s, const result& r) const;
  virtual void assign_normal(result& r, const char* s) const;
  virtual bool equals_normal(const result &x, const result &y) const;
};


/// Safe way to get a modifier
inline modifier GetModifier(const type* t) 
{
  if (t)  return t->getModifier();
  return NO_SUCH_MODIFIER;
}

inline bool HasProc(const type* t)
{
  if (t)  return t->hasProc();
  return false;
}

inline const simple_type* GetBase(const type* t)
{
  if (t)  return t->getBaseType();
  return 0;
}

inline const type* ModifyType(modifier m, const type* t)
{
  if (t)  return t->modifyType(m);
  return 0;
}

inline const type* ProcifyType(const type* t)
{
  if (t)  return t->addProc();
  return 0;
}

inline const type* ApplyPM(const type* pm, const type* bt)
{
  if (0==pm)  return bt;
  return pm->changeBaseType(bt);
}

// ******************************************************************
// *                                                                *
// *                         typelist class                         *
// *                                                                *
// ******************************************************************

/** Handy class for aggregate types.
    And it allows us to "share" them :^)
*/
class typelist : public shared_object {
  const type** list;
  int nt;
public:
  typelist(int nt);
  virtual ~typelist();
  inline void SetItem(int n, const type* t) {
    DCASSERT(list);
    CHECK_RANGE(0, n, nt);
    DCASSERT(t);
    list[n] = t;
  }
  inline const type* GetItem(int n) const {
    DCASSERT(list);
    CHECK_RANGE(0, n, nt);
    return list[n];
  }
  inline int Length() const {
    return nt;
  }
  virtual bool Print(OutputStream &s, int) const;
  virtual bool Equals(const shared_object *o) const;
};


// ******************************************************************
// *                                                                *
// *                       simple_type  class                       *
// *                                                                *
// ******************************************************************

/** Simple type with value, such as integer or boolean.
*/
class simple_type : public type {
  const type* phase_this;
  const type* rand_this;
  const type* proc_this;
  const type* set_this;
protected:
  const char* short_docs;
  const char* long_docs;
public:
  simple_type(const char* n, const char* sd, const char* ld);
  virtual ~simple_type();

  // Documentation
  inline const char* shortDocs() const { return short_docs; }
  inline const char* longDocs() const { return long_docs; }


  inline void setPhase(const type* t) {
    DCASSERT(0==phase_this);
    phase_this = t;
  }
  inline void setRand(const type* t) {
    DCASSERT(0==rand_this);
    rand_this = t;
  }
  inline void setSet(const type* t) {
    DCASSERT(0==set_this);
    set_this = t;
  }

  virtual const type* getSetOfThis() const;
  virtual const type* modifyType(modifier m) const;
  virtual const type* addProc() const;
  virtual void setProc(const type* t);

  virtual const simple_type* getBaseType() const;
  virtual const type* changeBaseType(const type* newbase) const;
};


// ******************************************************************
// *                                                                *
// *                        void_type  class                        *
// *                                                                *
// ******************************************************************

/** void types with no value, such as void, "state" or "place".
*/
class void_type : public simple_type {
public:
  void_type(const char* n, const char* sd, const char* ld);
  
  /// Default comparison: check addresses of "other" field pointers.
  virtual int compare(const result& a, const result& b) const;
};

// ******************************************************************
// *                                                                *
// *                           Front  end                           *
// *                                                                *
// ******************************************************************

/// Handy routine to go from x ph y -> x rand y.
inline const type* Phase2Rand(const type* lct)
{
  if (0==lct) return 0;
  if (lct->getModifier() != PHASE) return lct;
  const type* ans = lct->getBaseType();
  DCASSERT(ans);
  ans = ans->modifyType(RAND);
  DCASSERT(ans);
  if (lct->hasProc()) ans = ans->addProc();
  return ans;
}


type* newModifiedType(const char* n, modifier m, simple_type* base);
type* newProcType(const char* n, type* base);
type* newSetType(const char* n, simple_type* base);

void InitTypeOptions(exprman* em);


#endif
