
// $Id$

#include "ops_set.h"
#include "trinary.h"
#include "sets.h"
#include "../Streams/streams.h"
#include <string.h>  // splay needs memcpy
#include <stdlib.h>
#include "exprman.h"
#include "assoc.h"

/**
   Implementation of simple set stuff.
*/

inline const type* 
SetResultType(const exprman* em, const type* lt, const type* rt)
{
  DCASSERT(em);
  if (em->NULTYPE == lt || em->NULTYPE ==rt)  return 0;
  const type* lct = em->getLeastCommonType(lt, rt);
  if (0==lct)             return 0;
  if (!lct->isASet())     return 0;
  return lct;
}

inline int SetAlignDistance(const exprman* em, const type* lt, const type* rt)
{
  DCASSERT(em);
  const type* lct = SetResultType(em, lt, rt);
  if (0==lct)        return -1;

  int dl = em->getPromoteDistance(lt, lct);   DCASSERT(dl>=0);
  int dr = em->getPromoteDistance(rt, lct);   DCASSERT(dr>=0);

  return dl+dr;
}

inline int SetAlignDistance(const exprman* em, expr** x, int N)
{
  DCASSERT(em);
  DCASSERT(x);

  const type* lct = em->SafeType(x[0]);
  for (int i=1; i<N; i++) {
    lct = em->getLeastCommonType(lct, em->SafeType(x[i]));
  }
  if (0==lct)           return -1;
  if (!lct->isASet())   return -1;

  int d = 0;
  for (int i=0; i<N; i++) {
    int dx = em->getPromoteDistance(em->SafeType(x[i]), lct);
    DCASSERT(dx>=0);
    d += dx;
  }
  return d;
}

inline const type* AlignSets(const exprman* em, expr** x, int N)
{
  DCASSERT(em);
  DCASSERT(x);

  const type* lct = em->SafeType(x[0]);
  for (int i=1; i<N; i++) {
    lct = em->getLeastCommonType(lct, em->SafeType(x[i]));
  }
  if (  (0==lct) || !lct->isASet() ) {
    for (int i=0; i<N; i++)  Delete(x[i]);
    return 0;
  }
  for (int i=0; i<N; i++) {
    x[i] = em->promote(x[i], lct);
    DCASSERT(em->isOrdinary(x[i]));
  }
  return lct;
}


// ******************************************************************
// *                                                                *
// *                       int_ivlexpr  class                       *
// *                                                                *
// ******************************************************************

/** An expression to build an integer set interval.
 */
class int_ivlexpr : public trinary {
public:
  int_ivlexpr(const char* fn, int line, expr* s, expr* e, expr* i);
  virtual void Compute(traverse_data &x);
  virtual bool Print(OutputStream &s, int) const;
protected:
  virtual expr* buildAnother(expr* newl, expr* newm, expr* newr) const;
};

// ******************************************************************
// *                      int_ivlexpr  methods                      *
// ******************************************************************

int_ivlexpr::int_ivlexpr(const char* f, int l, expr* s, expr* e, expr* i)
 : trinary(f, l, em->INT->getSetOfThis(), s, e, i)
{
  DCASSERT(s); 
  DCASSERT(e); 
  DCASSERT(i); 
  DCASSERT(Type());
  DCASSERT(s->Type()==Type()->getSetElemType());
  DCASSERT(e->Type()==Type()->getSetElemType());
  DCASSERT(i->Type()==Type()->getSetElemType());
}

void int_ivlexpr::Compute(traverse_data &x)
{
  result s, e, i;
  LMRCompute(s, e, i, x);

  if (!s.isNormal()) {
    x.answer->setNull();
    return;
  }

  if (i.isInfinity() || (i.isNormal() && i.getInt()==0)) {
    // that means an interval with just the start element.
    x.answer->setPtr( MakeSingleton(left->Type(), s) );
    return;
  } 

  if ((!e.isNormal()) || (!i.isNormal())) {
    x.answer->setNull();
    return;
  }

  if (((s.getInt() > e.getInt()) && (i.getInt()>0))
     ||
     ((s.getInt() < e.getInt()) && (i.getInt()<0))) 
  {
    // empty interval
    x.answer->setPtr( MakeSet(Type(), 0, 0, 0) );
    return;
  } 
  
  // we have an ordinary interval
  x.answer->setPtr( MakeRangeSet(s.getInt(), e.getInt(), i.getInt()) );
}

bool int_ivlexpr::Print(OutputStream &s, int) const
{
  s.Put('{');
  left->Print(s, 0);
  s.Put("..");
  middle->Print(s, 0);
  s.Put("..");
  right->Print(s, 0);
  s.Put('}');
  return true;
}

expr* int_ivlexpr::buildAnother(expr* newl, expr* newm, expr* newr) const
{
  return new int_ivlexpr(Filename(), Linenumber(), newl, newm, newr);
}

// ******************************************************************
// *                                                                *
// *                        int_ivlop  class                        *
// *                                                                *
// ******************************************************************

class int_ivlop : public trinary_op {
public:
  int_ivlop();

  virtual int getPromoteDistance(const type* lt, const type* mt, 
        const type* rt) const;
  virtual const type* getExprType(const type* lt, const type* mt, 
        const type* rt) const;
  virtual trinary* makeExpr(const char* fn, int ln, expr* left, 
        expr* middle, expr* right) const;
};

// ******************************************************************
// *                       int_ivlop  methods                       *
// ******************************************************************

int_ivlop::int_ivlop() : trinary_op(exprman::top_interval)
{
}

int int_ivlop::
getPromoteDistance(const type* lt, const type* mt, const type* rt) const
{
  DCASSERT(em);
  DCASSERT(em->INT);
  if (em->NULTYPE == lt || em->NULTYPE == mt || em->NULTYPE == rt) return -1;
  int dl = em->getPromoteDistance(lt, em->INT);
  int dm = em->getPromoteDistance(mt, em->INT);
  int dr = em->getPromoteDistance(rt, em->INT);
  if ( (dl<0) || (dm<0) || (dr<0) )  return -1;
  return dl + dm + dr;
}

const type* int_ivlop::getExprType(const type* lt, const type* mt, 
        const type* rt) const
{
  if (em->NULTYPE == lt || em->NULTYPE == mt || em->NULTYPE == rt) return 0;
  if (!em->isPromotable(lt, em->INT))  return 0;
  if (!em->isPromotable(mt, em->INT))  return 0;
  if (!em->isPromotable(rt, em->INT))  return 0;
  return em->INT->getSetOfThis();
}

trinary* int_ivlop::makeExpr(const char* fn, int ln, expr* left, 
        expr* middle, expr* right) const
{
  left = em->promote(left, em->INT);
  middle = em->promote(middle, em->INT);
  right = em->promote(right, em->INT);

  if ( (!em->isOrdinary(left)) || (!em->isOrdinary(middle)) || (!em->isOrdinary(right)) ) {
    Delete(left);
    Delete(middle);
    Delete(right);
    return 0;
  }

  return new int_ivlexpr(fn, ln, left, middle, right);
}

// ******************************************************************
// *                                                                *
// *                       real_ivlexpr class                       *
// *                                                                *
// ******************************************************************

/** An expression to build a real set interval.
 */
class real_ivlexpr : public trinary {
public:
  real_ivlexpr(const char* fn, int line, expr* s, expr* e, expr* i);
  virtual void Compute(traverse_data &x);
  virtual bool Print(OutputStream &s, int) const;
protected:
  virtual expr* buildAnother(expr* newl, expr* newm, expr* newr) const;
};


// ******************************************************************
// *                      real_ivlexpr methods                      *
// ******************************************************************

real_ivlexpr::real_ivlexpr(const char* f, int l, expr* s, expr* e, expr* i)
 : trinary(f, l, em->REAL->getSetOfThis(), s, e, i)
{
  DCASSERT(s); 
  DCASSERT(e); 
  DCASSERT(i); 
  DCASSERT(s->Type()==Type()->getSetElemType());
  DCASSERT(e->Type()==Type()->getSetElemType());
  DCASSERT(i->Type()==Type()->getSetElemType());
}

void real_ivlexpr::Compute(traverse_data &x)
{
  result s, e, i;
  LMRCompute(s, e, i, x);

  if (!s.isNormal()) {
    x.answer->setNull();
    return;
  }

  if (i.isInfinity() || (i.isNormal() && i.getReal()==0.0) ) {
    // that means an interval with just the start element.
    x.answer->setPtr( MakeSingleton(left->Type(), s) );
    return;
  } 

  if ((!e.isNormal()) || (!i.isNormal())) {
    x.answer->setNull();
    return;
  }

  if (((s.getReal() > e.getReal()) && (i.getReal()>0))
     ||
     ((s.getReal() < e.getReal()) && (i.getReal()<0))) 
  {
    // empty interval
    x.answer->setPtr( MakeSet(left->Type(), 0, 0, 0) );
    return;
  } 
  
  // we have an ordinary interval
  x.answer->setPtr( MakeRangeSet(left->Type(), s.getReal(), 
      e.getReal(), i.getReal()) );
}

bool real_ivlexpr::Print(OutputStream &s, int) const
{
  s.Put('{');
  left->Print(s, 0);
  s.Put("..");
  middle->Print(s, 0);
  s.Put("..");
  right->Print(s, 0);
  s.Put('}');
  return true;
}

expr* real_ivlexpr::buildAnother(expr* newl, expr* newm, expr* newr) const
{
  return new real_ivlexpr(Filename(), Linenumber(), newl, newm, newr);
}

// ******************************************************************
// *                                                                *
// *                        real_ivlop class                        *
// *                                                                *
// ******************************************************************

class real_ivlop : public trinary_op {
public:
  real_ivlop();

  virtual int getPromoteDistance(const type* lt, const type* mt, 
        const type* rt) const;
  virtual const type* getExprType(const type* lt, const type* mt, 
        const type* rt) const;
  virtual trinary* makeExpr(const char* fn, int ln, expr* left, 
        expr* middle, expr* right) const;
};

// ******************************************************************
// *                       real_ivlop methods                       *
// ******************************************************************

real_ivlop::real_ivlop() : trinary_op(exprman::top_interval)
{
}

int real_ivlop::
getPromoteDistance(const type* lt, const type* mt, const type* rt) const
{
  DCASSERT(em);
  DCASSERT(em->REAL);
  if (em->NULTYPE == lt || em->NULTYPE == mt || em->NULTYPE == rt) return -1;
  int dl = em->getPromoteDistance(lt, em->REAL);
  int dm = em->getPromoteDistance(mt, em->REAL);
  int dr = em->getPromoteDistance(rt, em->REAL);
  if ( (dl<0) || (dm<0) || (dr<0) )  return -1;
  return dl + dm + dr;
}

const type* real_ivlop::getExprType(const type* lt, const type* mt, 
        const type* rt) const
{
  if (em->NULTYPE == lt || em->NULTYPE == mt || em->NULTYPE == rt) return 0;
  if (!em->isPromotable(lt, em->REAL))  return 0;
  if (!em->isPromotable(mt, em->REAL))  return 0;
  if (!em->isPromotable(rt, em->REAL))  return 0;
  return em->REAL->getSetOfThis();
}

trinary* real_ivlop::makeExpr(const char* fn, int ln, expr* left, 
        expr* middle, expr* right) const
{
  left = em->promote(left, em->REAL);
  middle = em->promote(middle, em->REAL);
  right = em->promote(right, em->REAL);

  if ( (!em->isOrdinary(left)) || (!em->isOrdinary(middle)) || (!em->isOrdinary(right)) ) {
    Delete(left);
    Delete(middle);
    Delete(right);
    return 0;
  }

  return new real_ivlexpr(fn, ln, left, middle, right);
}



// ******************************************************************
// *                                                                *
// *                                                                *
// *                         Union operator                         *
// *                                                                *
// *                                                                *
// ******************************************************************


/**  Splay of results class.
     The tree nodes store objects.
     Also, for cleverness, we can convert back and forth to a doubly-linked
     list when there are few enough items.
*/
class SplayOfResults {
protected:
  /// Type of items in the tree
  const type* item_type;
  /// Stack, used for tree traversals.
  long* stack;
  /// Top of stack.
  long stack_top;
  /// Size of stack (max depth).
  long stack_size; 
  /// Items stored in the tree/list.
  result* item;
  /// Left pointers.
  long* left;
  /// Right pointers.
  long* right;
  /// Dimension of item, left, right arrays.
  long max_elements;
  /// Number of elements in tree/list.
  long num_elements;
  /// Pointer to root node
  long root;
  /// When to change from linked-list to tree, when increasing.
  int list2tree;
  /// Currently, are we a list?  Otherwise we are a tree.
  bool is_list;
public:
  /** Constructor.
      Create a new, empty list/tree.
        @param  it  Type of the items we will add to the tree/list.
        @param  l2t Size at which we change from a list to a tree,
                    when adding elements.
                    If less than 1, we will start with a tree.

  */
  SplayOfResults(const type* it, int l2t);

  /// Destructor.
  ~SplayOfResults();

  inline long NumElements() const { return num_elements; }

  /** Splay.
      Find the closest element to key in the tree/list, and make it the root.
      Does so in a manner consistent with the underlying data structure
      (i.e., tree or list).
        @param  key  Item to search for
        @return The value of compare(root, key);
  */
  int Splay(const result &key);

  /** Add element.
        @param  key  Item to add.
        @return true,   if it was successfully added to the tree/list.
                false,  if it was not added, either because one was already
                        present, or there is no more memory to add an item.
  */
  bool Insert(const result &key);

  /** Write to an order array.
      On return, element i of the array will give the index of
      the ith item, in order.
        @param  a  An array, of dimension NumElements() or larger.
  */
  void FillOrderArray(long* a);

  /** Write to a value array.
      On return, element i of the array will be the ith item
      inserted in the tree.
        @param  a  An array, of dimension NumElements() or larger.
  */
  void FillValueArray(result* a) const;

protected:
  void Expand();

  void ConvertToTree();

  inline void Push(long x) {
    if (stack_top >= stack_size) {
      stack_size += 256;
      stack = (long*) realloc(stack, stack_size * sizeof(long));
    }
    stack[stack_top++] = x;
  }
  inline long Pop() {
    return (stack_top) ? (stack[--stack_top]) : -1;
  }
  inline void StackClear() {
    stack_top = 0;
  }
  inline long NewNode() {
    long ans;
    if (num_elements >= max_elements)  Expand();
    if (num_elements >= max_elements)  return -1;  // no memory
    ans = num_elements;
    num_elements++;
    return ans;
  }
  /// swap parent and child, preserve BST property
  inline void TreeRotate(long C, long P, long GP) {
    if (left[P] == C) {
      left[P] = right[C];
      right[C] = P;
    } else {
      right[P] = left[C];
      left[C] = P;
    }
    if (GP >= 0) {
      if (left[GP] == P)
        left[GP] = C;
      else 
        right[GP] = C;
    }
  }
};


// ==================================================================
// ||                                                              ||
// ||                    SplayOfResults methods                    ||
// ||                                                              ||
// ==================================================================

SplayOfResults::SplayOfResults(const type* it, int l2t)
{
#ifdef DEBUG_MEM
  printf("Creating splay of results\n");
#endif
  item_type = it;
  DCASSERT(item_type);
  list2tree = l2t;
  stack = 0; stack_top = stack_size = 0;
  item = 0;
  left = right = 0;
  max_elements = num_elements = 0;
  root = -1;
  is_list = (list2tree > 0);
}

SplayOfResults::~SplayOfResults()
{
#ifdef DEBUG_MEM
  printf("Deleting splay of results\n");
#endif
  for (int i=0; i<num_elements; i++) item[i].deletePtr();
  free(stack);
  free(item);
  free(left);
  free(right);
}

int SplayOfResults::Splay(const result &key)
{
  if (root < 0) return -1;
  int cmp;
  if (is_list) {
    // List splay
    cmp = item_type->compare(item[root], key);
    if (0==cmp)      return cmp;
    if (cmp > 0) {
      // traverse to the left
      while (left[root] >= 0) {
        root = left[root];
        cmp = item_type->compare(item[root], key);
        if (cmp > 0)    continue;
        return cmp;
      } // while
      return cmp;
    }
    // traverse to the right
    while (right[root] >= 0) {
      root = right[root];
      cmp = item_type->compare(item[root], key);
      if (cmp < 0)    continue;
      return cmp;
    } // while
    return cmp;
  }
  // Tree splay
  long child = root;
  StackClear();
  while (child >= 0) {
    Push(child);
    cmp = item_type->compare(item[child], key);
    if (0==cmp)  break;
    if (cmp > 0) {
      child = left[child];
      cmp = 1;
    } else {
      child = right[child];
      cmp = -1;
    }
  } // while child
  child = Pop();
  long parent = Pop();
  long grandp = Pop();
  long greatgp = Pop();
  while (parent >= 0) {
    // splay step
    if (grandp < 0) {
      TreeRotate(child, parent, grandp);
      break;
    }
    if ( (right[grandp] == parent) == (right[parent] == child) ) {
      // parent and child are either both right children, or both left children
      TreeRotate(parent, grandp, greatgp);
      TreeRotate(child, parent, greatgp);
    } else {
      TreeRotate(child, parent, grandp);
      TreeRotate(child, grandp, greatgp);
    }
    // continue
    parent = greatgp;
    grandp = Pop();
    greatgp = Pop();
  } // while parent
  root = child;
  return cmp;
}

bool SplayOfResults::Insert(const result& key)
{
  if (root < 0) {
    // same behavior, regardless of tree or list
    root = NewNode();
    if (root < 0)  return false;
    item[root].constructFrom(key);
    left[root] = -1;
    right[root] = -1;
    return true;
  }
  int cmp = Splay(key);
  if (0==cmp)  return false;

  // need to add the element
  if (is_list && (num_elements > list2tree))
  ConvertToTree();

  long newroot = NewNode();
  if (newroot < 0)   return false;  // out of memory?
  item[newroot].constructFrom(key);

  // connect new root to old one.
  if (is_list) {
    if (cmp > 0) {
      long l = left[root];
      left[newroot] = l;
      if (l>=0) right[l] = newroot;     
      right[newroot] = root;
      left[root] = newroot;
    } else {
      long r = right[root];
      right[newroot] = r;
      if (r>=0) left[r] = newroot;
      left[newroot] = root;
      right[root] = newroot;
    }
  } else {
    if (cmp > 0) {
      right[newroot] = root;
      left[newroot] = left[root];
      left[root] = -1;
    } else {
      left[newroot] = root;
      right[newroot] = right[root];
      right[root] = -1;
    }
  }
  root = newroot;
  return true;
}

void SplayOfResults::FillOrderArray(long* a)
{
  if (root < 0) return;
  long i;
  long slot = 0;
  if (is_list) {
    for (i=root; left[i] >= 0; i=left[i]) { }
    for (; i>=0; i=right[i]) {
      a[slot] = i;
      slot++;
    }
    return;
  } 
  // non-recursive, inorder tree traversal
  StackClear();
  i = root;
  while (i>=0) {
    if (left[i] >= 0) {
      Push(i);
      i =left[i];
      continue;
    }
    while (i>=0) {
      a[slot] = i;
      slot++;
      if (right[i] >= 0) {
        i = right[i];
        break;
      }
      i = Pop();
    } // inner while
  } // outer while
}

void SplayOfResults::FillValueArray(result* a) const
{
  for (long i=0; i<num_elements; i++) a[i] = item[i];
}

void SplayOfResults::Expand()
{
  long new_elements = 2*max_elements;
  if (new_elements > 1024) new_elements = max_elements + 1024;
  if (new_elements < 4) new_elements = 4;
  result* newitem = (result*) realloc(item, new_elements * sizeof(result));
  long* newleft = (long*) realloc(left, new_elements * sizeof(long));
  long* newright = (long*) realloc(right, new_elements * sizeof(long));
  if (newitem) item = newitem;
  if (newleft) left = newleft;
  if (newright) right = newright;
  if ((0==newitem) || (0==newleft) || (0==newright)) return;
  max_elements = new_elements;
}

void SplayOfResults::ConvertToTree()
{
  long i;
  for (i=left[root]; i>=0; i=left[i]) {
    right[i] = -1;
  }
  for (i=right[root]; i>=0; i=right[i]) {
    left[i] = -1;
  }
  is_list = false;
}

// ******************************************************************
// *                                                                *
// *                        set_union  class                        *
// *                                                                *
// ******************************************************************

/** An associative operator to handle the union of sets.
    (Used when we make those ugly for loops, or list out elements.)
*/
class set_union : public summation {
public:
  set_union(const char* fn, int line, const type* settype, expr** x, int n);
  virtual void Compute(traverse_data &x);
protected:
  virtual expr* buildAnother(expr** newx, bool* f, int newn) const;
};

// ******************************************************************
// *                       set_union  methods                       *
// ******************************************************************

set_union
::set_union(const char* fn, int line, const type* settype, expr** x, int n)
 : summation(fn, line, exprman::aop_union, settype, x, 0, n)
{
}

void set_union::Compute(traverse_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  const type* elemtype = Type()->getSetElemType();
  DCASSERT(elemtype);
  SplayOfResults answer(elemtype, 64);
  for (int i=0; i<opnd_count; i++) {
    SafeCompute(operands[i], x);
    if (!x.answer->isNormal())  return; // bail out
    shared_set *s = smart_cast <shared_set*> (x.answer->getPtr());
    DCASSERT(s);
    for (long e=0; e<s->Size(); e++) {
      result item;
      s->GetElement(e, item);
      answer.Insert(item);
    } 
  }
  // prepare result
  long newsize = answer.NumElements();
  long* order = (newsize) ? (new long[newsize]) : 0;
  answer.FillOrderArray(order);
  result* values = (newsize) ? (new result[newsize]) : 0;
  answer.FillValueArray(values);
  x.answer->setPtr( MakeSet(elemtype, newsize, values, order) );
}

expr* set_union::buildAnother(expr** newx, bool* f, int newn) const
{
  DCASSERT(0==f);
  return new set_union(Filename(), Linenumber(), Type(), newx, newn);
}


// ******************************************************************
// *                                                                *
// *                       set_union_op class                       *
// *                                                                *
// ******************************************************************

class set_union_op : public assoc_op {
public:
  set_union_op();
  virtual int getPromoteDistance(expr** list, bool* flip, int N) const;
  virtual int getPromoteDistance(bool flip, const type* lt, 
          const type* rt) const;
  virtual const type* getExprType(bool flip, const type* lt, 
          const type* rt) const;
  virtual assoc* makeExpr(const char* fn, int ln, expr** list, 
          bool* flip, int N) const;
};

// ******************************************************************
// *                      set_union_op methods                      *
// ******************************************************************

set_union_op::set_union_op() : assoc_op(exprman::aop_union)
{
}

int set_union_op::getPromoteDistance(expr** list, bool* flip, int N) const
{
  return SetAlignDistance(em, list, N);
}

int set_union_op
::getPromoteDistance(bool flip, const type* lt, const type* rt) const
{
  if (flip)  return -1;
  return SetAlignDistance(em, lt, rt);
}

const type* set_union_op
::getExprType(bool flip, const type* lt, const type* rt) const
{
  if (flip)  return 0;
  return SetResultType(em, lt, rt);
}

assoc* set_union_op::makeExpr(const char* fn, int ln, expr** list, 
        bool* flip, int N) const
{
  delete[] flip;
  const type* lct = AlignSets(em, list, N);
  if (lct)  return new set_union(fn, ln, lct, list, N);
  // there was an error
  delete[] list;
  return 0;
}



// ******************************************************************
// *                                                                *
// *                           Front  end                           *
// *                                                                *
// ******************************************************************

void InitSetOps(exprman* em)
{
  if (0==em)  return;
  em->registerOperation(  new int_ivlop     );
  em->registerOperation(  new real_ivlop    );
  em->registerOperation(  new set_union_op  );
}


