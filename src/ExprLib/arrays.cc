
#include "arrays.h"
#include "exprman.h"
#include "../Streams/streams.h"
#include "sets.h"
#include "../Streams/strings.h"
#include "../Options/options.h"
#include <stdlib.h>

// #define DEBUG_ACALL

// ******************************************************************
// *                                                                *
// *                       array_item methods                       *
// *                                                                *
// ******************************************************************

array_item::array_item(expr* rhs) : shared_object()
{
  r.setNull();
  e = rhs;
}

array_item::~array_item()
{
  r.deletePtr();
  Delete(e);
}

bool array_item::Print(OutputStream &s, int) const
{
  // DCASSERT(0);
  s << "array_item ";
  if (e) {
    s << "(uncomputed) ";
    e->Print(s, 0);
  } else {
    s << "(computed) " << r.getInt();
  }
  return true;
}

bool array_item::Equals(const shared_object*) const
{
  DCASSERT(0);
  return false;
}

// ******************************************************************
// *                                                                *
// *                        array_desc class                        *
// *                                                                *
// ******************************************************************

/** Array descriptor struct.
    Used as part of the descriptor structure within an array.
    If there is another dimension below this one, then all the
    down pointers will be to other descriptors, otherwise they
    will be to some kind of expression.
*/

struct array_desc : public shared_object {
  /// The values that can be assumed here.
  shared_set* values;
  /** Pointers to the next dimension (another array_desc)
      or to the array values (a variable).
      Note that the dimension of this array is equal to the
      size of the set "values".
   */
  shared_object** down;

  array_desc(shared_set *v);
  ~array_desc();
  virtual bool Print(OutputStream &s, int) const;
  virtual bool Equals(const shared_object*) const;
};

// ******************************************************************
// *                       array_desc methods                       *
// ******************************************************************

array_desc::array_desc(shared_set* v)
{
  values = v;
  if (values) {
    down = new shared_object*[values->Size()];
    for (long i=0; i<values->Size(); i++) down[i] = 0;
  } else {
    down = 0;
  }
}

array_desc::~array_desc()
{
  if (values) {
    for (long i=0; i<values->Size(); i++)  Delete(down[i]);
    delete[] down;
    Delete(values);
  }
}

bool array_desc::Print(OutputStream &s, int) const
{
  DCASSERT(0);
  s << "array_desc";
  return true;
}

bool array_desc::Equals(const shared_object* o) const
{
  DCASSERT(0);
  return false;
}

// ******************************************************************
// *                                                                *
// *                      array_instance class                      *
// *                                                                *
// ******************************************************************

/**   Arrays with actual data.
      Most of the time this is what you want to use.
*/

class array_instance : public array {
protected:
  /// The descriptor, which includes data.
  array_desc* descriptor;
public:
  array_instance(const array* wrapper);
  array_instance(const char* fn, int line, const type* t, char* n, iterator** il, int dim);

  virtual ~array_instance();

  virtual void SetCurrentReturn(expr* retval, bool rename);
  virtual array_item* GetCurrentReturn();
  virtual array_item* GetItem(expr** indexes, result &x);
};

// ******************************************************************
// *                     array_instance methods                     *
// ******************************************************************

array_instance::array_instance(const array* wrapper) : array(wrapper)
{
  descriptor = 0;
}

array_instance::array_instance(const char* fn, int line, const type* t,
        char* n, iterator** il, int dim)
 : array(fn, line, t, n, il, dim)
{
  descriptor = 0;
}

array_instance::~array_instance()
{
  Delete(descriptor);
}

void array_instance::SetCurrentReturn(expr* retval, bool rename)
{
  array_desc* prev = 0;
  shared_object* curr = descriptor;
  long lastindex = 0;
  for (int i=0; i<dimension; i++) {
    if (0==curr) {
      array_desc* foo = new array_desc(index_list[i]->CopyCurrent());
      if (prev) {
        DCASSERT(0==prev->down[lastindex]);
        prev->down[lastindex] = foo;
      } else {
        DCASSERT(0==descriptor);
        descriptor = foo;
      }
      curr = foo;
    }
    lastindex = index_list[i]->Index();
    prev = smart_cast <array_desc*>(curr);
    DCASSERT(prev);
    curr = prev->down[lastindex];
  }
  if (curr) {
    // we already have a value...
    if (em->startInternal(__FILE__, __LINE__)) {
      em->noCause();
      em->internal() << "array reassignment?";
      em->stopIO();
    }
  }
  prev->down[lastindex] = new array_item(retval);

  if (!rename)    return;
  if (0==retval)  return;

  traverse_data x(traverse_data::Compute);
  result ind;
  x.answer = &ind;
  StringStream s;
  s << Name() << "[";
  for (int i=0; i<dimension; i++) {
    DCASSERT(index_list[i]->Type());
    if (i) s << ", ";
    SafeCompute(index_list[i], x);
    index_list[i]->Type()->print(s, ind);
  }
  s << "]";
  shared_string* name = new shared_string(s.GetString());
  retval->Rename(name);
}

array_item* array_instance::GetCurrentReturn()
{
  array_desc* prev = 0;
  shared_object* curr = descriptor;
  long lastindex = 0;
  for (int i=0; i<dimension; i++) {
    if (0==curr) {
      array_desc* foo = new array_desc(index_list[i]->CopyCurrent());
      if (prev) {
        DCASSERT(0==prev->down[lastindex]);
        prev->down[lastindex] = foo;
      } else {
        DCASSERT(0==descriptor);
        descriptor = foo;
      }
      curr = foo;
    }
    lastindex = index_list[i]->Index();
    prev = smart_cast <array_desc*> (curr);
    DCASSERT(prev);
    curr = prev->down[lastindex];
  }
  if (0==curr)  return 0;
  array_item* ans = smart_cast <array_item*> (curr);
  DCASSERT(ans);
  return ans;
}

array_item* array_instance::GetItem(expr** il, result& x)
{
  traverse_data foo(traverse_data::Compute);
  foo.answer = &x;
  array_desc* prev = 0;
  shared_object* curr = descriptor;
  for (int i=0; i<dimension; i++) {
    if (0==curr)  return 0;
    prev = smart_cast <array_desc*> (curr);
    DCASSERT(prev);
    SafeCompute(il[i], foo);
    if (x.isNormal()||x.isInfinity()) {
      long ndx = prev->values->IndexOf(x);
      if (ndx<0) {
        // range error
        if (em->startError()) {
          em->causedBy(il[i]);
          em->cerr() << "Bad value: ";
          DCASSERT(il[i]->Type());
          il[i]->Type()->print(em->cerr(), x);
          em->cerr() << " for index " << index_list[i]->Name();
          em->cerr() << " in array " << Name();
          em->stopIO();
        }
        return 0;
      }
      curr = prev->down[ndx];
      continue;
    }
    // there is something strange with x (null, error, etc), bail out
    return 0;
  }
  if (0==curr)  return 0;
  array_item* ans = smart_cast <array_item*> (curr);
  DCASSERT(ans);
  return ans;
}


// ******************************************************************
// *                                                                *
// *                       arrayassign  class                       *
// *                                                                *
// ******************************************************************

/**  A statement used for array assignments.
     These are for assignments not within a converge block.
 */

class arrayassign : public expr {
  array* f;
  expr* retval;
public:
  arrayassign(const char *fn, int l, array *a, expr *e);
  virtual ~arrayassign();

  virtual bool Print(OutputStream &s, int) const;
  virtual void Compute(traverse_data &x);
  virtual void Traverse(traverse_data &x);
};

// ******************************************************************
// *                      arrayassign  methods                      *
// ******************************************************************

arrayassign::arrayassign(const char *fn, int l, array *a, expr *e)
  : expr(fn, l, STMT)
{
  f = a;
  retval = e;
  f->SetModelType(retval->GetModelType());
}

arrayassign::~arrayassign()
{
  Delete(retval);
}

bool arrayassign::Print(OutputStream &s, int w) const
{
  DCASSERT(f);
  s.Pad(' ', w);
  f->PrintHeader(s);
  s << " := ";
  if (retval)   retval->Print(s, 0);
  else          s << "null";
  s << ";\n";
  return true;
}

void arrayassign::Compute(traverse_data &td)
{
  // De-iterate the return value
  expr* rv = (retval) ? (retval->Substitute(0)) : 0;
  expr* arrayval = em->makeConstant(Filename(), Linenumber(), f->Type(), 0, rv, 0);
  f->SetCurrentReturn(arrayval, true);
  if (expr_debug.startReport()) {
    expr_debug.report() << "executing assignment: ";
    arrayval->Print(expr_debug.report(), 0);
    expr_debug.report() << " := ";
    rv->Print(expr_debug.report(), 0);
    expr_debug.report() << "\n";
    expr_debug.stopIO();
  }
}

void arrayassign::Traverse(traverse_data &td)
{
}

// ******************************************************************
// *                                                                *
// *                          acall  class                          *
// *                                                                *
// ******************************************************************

/**  An expression used to obtain an array element.
 */
class acall : public expr {
protected:
  array* func;
  expr** pass;
  int numpass;
public:
  acall(const char *fn, int line, const type* t, array *f, expr **p, int np);
  virtual ~acall();
  virtual void Compute(traverse_data &x);
  virtual void Traverse(traverse_data &x);
  virtual bool Print(OutputStream &s, int) const;
};

// ******************************************************************
// *                         acall  methods                         *
// ******************************************************************

acall::acall(const char *fn, int line, const type* t, array *f,
    expr **p, int np) : expr(fn, line, t)
{
  func = f;
  pass = p;
  numpass = np;
}

acall::~acall()
{
  // don't delete func
  int i;
  for (i=0; i<numpass; i++) Delete(pass[i]);
  delete[] pass;
}

void acall::Compute(traverse_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  array_item* elem = func->GetItem(pass, *x.answer);

  if (expr_debug.startReport()) {
    expr_debug.report() << "got array element: ";
    if (elem)   elem->Print(expr_debug.report(), 0);
    else        expr_debug.report() << "null";
    expr_debug.report() << "\n";
    expr_debug.stopIO();
  }

  if (elem)   elem->Compute(x, func->IsFixed());
  else        x.answer->setNull();
}

void acall::Traverse(traverse_data &x)
{
  switch (x.which) {
    case traverse_data::Substitute: {
      DCASSERT(x.answer);
      bool changed;

      // replace the array itself, if necessary
      func->Traverse(x);
      symbol* fsub = smart_cast <symbol*> (Share(x.answer->getPtr()));
      DCASSERT(fsub);
      changed = (fsub != func);

      // substitute each index
      expr** newpass = new expr*[numpass];
      for (int n=0; n<numpass; n++) {
        DCASSERT(pass[n]);
        pass[n]->Traverse(x);
        newpass[n] = smart_cast <expr*> (Share(x.answer->getPtr()));
        if (!changed) {
          changed = (newpass[n] != pass[n]);
        }
      } // for n

      if (changed) {
        x.answer->setPtr(
          em->makeArrayCall(Filename(), Linenumber(), fsub, newpass, numpass)
        );
      } else {
        Delete(fsub);
        for (int n=0; n<numpass; n++) {
          Delete(newpass[n]);
        }
        delete[] newpass;
        x.answer->setPtr(Share(this));
      }
      return;
    } // traverse_data::Substitute

    case traverse_data::GetSymbols:
    case traverse_data::GetMeasures:
    case traverse_data::PreCompute: {
      for (int i=0; i<numpass; i++) if (pass[i]) {
        pass[i]->Traverse(x);
      } // for i
      func->Traverse(x);
      return;
    } // traverse_data::PreCompute, GetMeasures, GetSymbols

    default:
      DCASSERT(0);
  } // switch
}

bool acall::Print(OutputStream &s, int) const
{
  if (0==func->Name())  return false;  // hidden?
  s << func->Name();
  DCASSERT(numpass>0);
  for (int i = 0; i<numpass; i++) {
    s << "[";
    if (pass[i])  pass[i]->Print(s, 0);
    else          s << "null";
    s << "]";
  }
  return true;
}

// ******************************************************************
// *                                                                *
// *                         array  methods                         *
// *                                                                *
// ******************************************************************

array::array(const array* wrapper) : symbol(wrapper)
{
  DCASSERT(wrapper);
  dimension = wrapper->dimension;
  index_list = new iterator* [dimension];
  for (int i=0; i<dimension; i++)
    index_list[i] = Share(wrapper->index_list[i]);
  SetSubstitution(false);
  is_fixed = false;
}

array::array(const char* fn, int line, const type* t, char* n,
    iterator** il, int dim) : symbol(fn, line, t, n)
{
  index_list = il;
  dimension = dim;
  SetSubstitution(false);
  is_fixed = false;
}

array::~array()
{
  if (index_list) {
    for (int i=0; i<dimension; i++)  Delete(index_list[i]);
    delete[] index_list;
  }
}

void array::SetCurrentReturn(expr*, bool)
{
  if (em->startInternal(__FILE__, __LINE__)) {
    em->noCause();
    em->internal() << "Attempting to set return value on data-less array.";
    em->stopIO();
  }
}

array_item* array::GetCurrentReturn()
{
  if (em->startInternal(__FILE__, __LINE__)) {
    em->noCause();
    em->internal() << "Attempting to obtain value from data-less array.";
    em->stopIO();
  }
  return 0;
}

array_item* array::GetItem(expr**, result &x)
{
  if (em->startInternal(__FILE__, __LINE__)) {
    em->noCause();
    em->internal() << "Attempting to obtain value from data-less array.";
    em->stopIO();
  }
  return 0;
}

bool array::checkArrayCall(const char* fn, int ln, expr** indexes, int dim) const
{
  // check that dim matches our dimension
  if (GetDimension() != dim) {
    if (em->startError()) {
      em->causedBy(fn, ln);
      em->cerr() << "Array " << Name();
      em->cerr() << " has dimension " << GetDimension();
      em->stopIO();
    }
    return false;
  }

  // type checking
  for (int i=0; i<dim; i++) {
    if (!em->isPromotable(indexes[i]->Type(), GetIndexType(i))) {
      if (em->startError()) {
        em->causedBy(fn, ln);
        em->cerr() << "Array ";
        PrintHeader(em->cerr());
        const type* at = GetIndexType(i);
        DCASSERT(at);
        em->cerr() << " expects type " << at->getName();
        em->cerr() << " for index " << GetIndexName(i);
        em->stopIO();
      }
      return false;
    }
    indexes[i] = em->promote(indexes[i], GetIndexType(i));
    DCASSERT(indexes[i]);
    if (em->isError(indexes[i]))  return false;
  } // for i
  return true;
}

void array::Traverse(traverse_data &x)
{
  switch (x.which) {
    case traverse_data::Affix:
        is_fixed = true;
        return;

    default:
        symbol::Traverse(x);
  }
}

void array::PrintHeader(OutputStream &s) const
{
  s << Name();
  for (int i=0; i<dimension; i++) {
    s << "[";
    s << index_list[i]->Name();
    s << "]";
  }
}

array* array::instantiateMe() const
{
  return new array_instance(this);
}


// ******************************************************************
// *                                                                *
// *                        exprman  methods                        *
// *                                                                *
// ******************************************************************

symbol* exprman::makeArray(const char* fn, int ln, const type* t, char* n, symbol** indexes, int dim) const
{
  if (0==indexes) {
    free(n);
    return 0;
  }
  // Check indexes
  for (int i=0; i<dim; i++) {
    iterator* it = dynamic_cast <iterator*> (indexes[i]);
    if (it)  continue;
    // bad iterator, bail out
    for (int j=0; j<dim; j++)  Delete(indexes[j]);
    delete[] indexes;
    free(n);
    return 0;
  }

  return new array_instance(fn, ln, t, n, (iterator**) indexes, dim);
}


expr* exprman::makeArrayAssign(const char* fn, int ln,
      symbol* arr, expr* rhs) const
{
  array* a = dynamic_cast <array*> (arr);
  if (0==a) {
    Delete(rhs);
    return 0;
  }
  if (!isOrdinary(rhs))    return 0;

  // Check return type
  if (!isPromotable(rhs->Type(), a->Type())) {
    if (startError()) {
      causedBy(fn, ln);
      cerr() << "Type mismatch in assignment for array ";
      cerr() << a->Name();
      stopIO();
    }
    Delete(rhs);
    return 0;
  }

  rhs = promote(rhs, a->Type());
  if (isError(rhs))  return 0;

  // This array is not in a converge, so we can use the values immediately:
  a->setDefined();
  a->Affix();
  return new arrayassign(fn, ln, a, rhs);
}


expr* exprman::makeArrayCall(const char* fn, int ln,
      symbol* arr, expr** indexes, int dim) const
{
  if (0==indexes)  return makeError();
  bool nul = false;
  bool err = false;
  array* a = dynamic_cast <array*> (arr);
  if (0==a) {
    err = true;
  } else {
    for (int i=0; i<dim; i++) {
      if (0 == indexes[i]) {
        nul = true;
        break;
      }
      if (isError(indexes[i]))  err = true;
    } // for i
  }

  // check that dim matches dimension of a!
  if (!err && !nul) {
    err = !a->checkArrayCall(fn, ln, indexes, dim);
  }

  if (err || nul) {
    for (int i=0; i<dim; i++)  Delete(indexes[i]);
    delete[] indexes;
    if (nul)  return 0;
    else      return makeError();
  }

  return new acall(fn, ln, a->Type(), a, indexes, dim);
}

