
// $Id$

#include "strings.h"
#include "operators.h"
#include "baseops.h"

//#define DEBUG_STRINGS

void PrintString(const result& x, OutputStream &out, int width)
{
  DCASSERT(x.svalue);
  if (x.svalue->isDeleted()) { out << "(deleted result)"; return; }
  char* s = x.svalue->string;
  int stlen = strlen(s);
  int i;
  if (width>=0) {
    // count length: account for special chars.
    int printlen = 0;
    for (i=0; i<stlen; i++) {
      if (s[i]!='\\') {
        printlen++;
        continue;
      }
      // special character
      i++;
      if (i>=stlen) break;  // trailing \, ignore
      switch (s[i]) {
        case 'n'	:
        case 't'	:
        case '\\'	:
        case 'q'	:
        case 'b'	:
        case 'a'	:
      	  printlen++;
      }
    }
    // pad with spaces to get to desired width
    out.Pad(' ', width-printlen);
  } // if width
  for (i=0; i<stlen; i++) {
    if (s[i]!='\\') {
      out << s[i];
      continue;
    }
    // special character
    i++;
    if (i>=stlen) break;  // trailing \, ignore
    switch (s[i]) {
      case 'n'	:	out << "\n";	break;
      case 't'	:	out << "\t";	break;
      case '\\'	:	out << "\\";	break;
      case 'q'	:	out << '"';	break;
      case 'b'	:	out << "\b";	break;
      case 'a'	:	out << "\a";	break;
      case 'f'  :	out.flush();    break;  // does this work?
    }
  }
}

// ******************************************************************
// *                                                                *
// *                        string_add class                        *
// *                                                                *
// ******************************************************************

/** Addition of strings.
 */
class string_add : public addop {
public:
  string_add(const char* fn, int line, expr **x, int n) 
    : addop(fn, line, x, n) { }
  
  string_add(const char* fn, int line, expr *l, expr *r) 
    : addop(fn, line, l, r) { }

  virtual ~string_add() { }
  
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return STRING;
  }
  virtual void Compute(compute_data &x);
protected:
  virtual expr* MakeAnother(expr **x, int n) {
    return new string_add(Filename(), Linenumber(), x, n);
  }
};


void string_add::Compute(compute_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  // strings are accumulated into a string stream
  StringStream acc;
  // Compute strings for each operand
  for (int i=0; i<opnd_count; i++) {
    DCASSERT(operands[i]);
    DCASSERT(operands[i]->Type(0) == STRING);
    operands[i]->Compute(x);
    if (x.answer->isError() || x.answer->isNull()) return;
    DCASSERT(x.answer->svalue);
    x.answer->svalue->show(acc);
    DeleteResult(STRING, *x.answer);
  }
  // done, collect concatenation
  char* answer = acc.GetString();
  x.answer->svalue = new shared_string(answer);
}


// ******************************************************************
// *                                                                *
// *                       string_equal class                       *
// *                                                                *
// ******************************************************************

/** Check equality of two string expressions.
 */
class string_equal : public eqop {
public:
  string_equal(const char* fn, int line, expr *l, expr *r)
    : eqop(fn, line, l, r) { }
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return BOOL;
  }
  
  virtual void Compute(compute_data &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new string_equal(Filename(), Linenumber(), l, r);
  }
};

void string_equal::Compute(compute_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  result l;
  result r;
  result* answer = x.answer;
  answer->Clear();
  x.answer = &l;
  SafeCompute(left, x);
  x.answer = &r;
  SafeCompute(right, x);
  x.answer = answer;

  if (l.isNull() || r.isNull()) {
    answer->setNull();
  }
  if (l.isError() || r.isError()) {
    answer->setError();
    return;
  }
  if (answer->isNormal()) {
    DCASSERT(l.svalue);
    DCASSERT(r.svalue);
    answer->bvalue = (strcmp(l.svalue->string, r.svalue->string)==0);
  } 
  DeleteResult(STRING, l);
  DeleteResult(STRING, r);
}

// ******************************************************************
// *                                                                *
// *                        string_neq class                        *
// *                                                                *
// ******************************************************************

/** Check inequality of two string expressions.
 */
class string_neq : public neqop {
public:
  string_neq(const char* fn, int line, expr *l, expr *r)
    : neqop(fn, line, l, r) { }
  
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return BOOL;
  }
  virtual void Compute(compute_data &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new string_neq(Filename(), Linenumber(), l, r);
  }
};

void string_neq::Compute(compute_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  result l;
  result r;
  result* answer = x.answer;
  answer->Clear();
  x.answer = &l;
  SafeCompute(left, x);
  x.answer = &r;
  SafeCompute(right, x);
  x.answer = answer;

  if (l.isNull() || r.isNull()) {
    answer->setNull();
  }
  if (l.isError() || r.isError()) {
    answer->setError();
  }
  if (answer->isNormal()) {
    DCASSERT(l.svalue);
    DCASSERT(r.svalue);
    answer->bvalue = (strcmp(l.svalue->string, r.svalue->string)!=0);
  } 
  DeleteResult(STRING, l);
  DeleteResult(STRING, r);
}


// ******************************************************************
// *                                                                *
// *                        string_gt  class                        *
// *                                                                *
// ******************************************************************

/** Check if one string expression is greater than another.
 */
class string_gt : public gtop {
public:
  string_gt(const char* fn, int line, expr *l, expr *r)
    : gtop(fn, line, l, r) { }
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return BOOL;
  }
  
  virtual void Compute(compute_data &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new string_gt(Filename(), Linenumber(), l, r);
  }
};

void string_gt::Compute(compute_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  result l;
  result r;
  result* answer = x.answer;
  answer->Clear();
  x.answer = &l;
  SafeCompute(left, x);
  x.answer = &r;
  SafeCompute(right, x);
  x.answer = answer;

  if (l.isNull() || r.isNull()) {
    answer->setNull();
  }
  if (l.isError() || r.isError()) {
    answer->setError();
  }
  if (answer->isNormal()) {
    DCASSERT(l.svalue);
    DCASSERT(r.svalue);
    answer->bvalue = (strcmp(l.svalue->string, r.svalue->string)>0);
  } 
  DeleteResult(STRING, l);
  DeleteResult(STRING, r);
}

// ******************************************************************
// *                                                                *
// *                        string_ge  class                        *
// *                                                                *
// ******************************************************************

/** Check if one string expression is greater than or equal another.
 */
class string_ge : public geop {
public:
  string_ge(const char* fn, int line, expr *l, expr *r)
    : geop(fn, line, l, r) { }
  
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return BOOL;
  }
  virtual void Compute(compute_data &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new string_ge(Filename(), Linenumber(), l, r);
  }
};

void string_ge::Compute(compute_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  result l;
  result r;
  result* answer = x.answer;
  answer->Clear();
  x.answer = &l;
  SafeCompute(left, x);
  x.answer = &r;
  SafeCompute(right, x);
  x.answer = answer;

  if (l.isNull() || r.isNull()) {
    answer->setNull();
  }
  if (l.isError() || r.isError()) {
    answer->setError();
  }
  if (answer->isNormal()) {
    DCASSERT(l.svalue);
    DCASSERT(r.svalue);
    answer->bvalue = (strcmp(l.svalue->string, r.svalue->string)>=0);
  } 
  DeleteResult(STRING, l);
  DeleteResult(STRING, r);
}

// ******************************************************************
// *                                                                *
// *                        string_lt  class                        *
// *                                                                *
// ******************************************************************

/** Check if one string expression is less than another.
 */
class string_lt : public ltop {
public:
  string_lt(const char* fn, int line, expr *l, expr *r)
    : ltop(fn, line, l, r) { }
  
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return BOOL;
  }
  virtual void Compute(compute_data &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new string_lt(Filename(), Linenumber(), l, r);
  }
};

void string_lt::Compute(compute_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  result l;
  result r;
  result* answer = x.answer;
  answer->Clear();
  x.answer = &l;
  SafeCompute(left, x);
  x.answer = &r;
  SafeCompute(right, x);
  x.answer = answer;

  if (l.isNull() || r.isNull()) {
    answer->setNull();
  }
  if (l.isError() || r.isError()) {
    answer->setError();
  }
  if (answer->isNormal()) {
    DCASSERT(l.svalue);
    DCASSERT(r.svalue);
    answer->bvalue = (strcmp(l.svalue->string, r.svalue->string)<0);
  } 
  DeleteResult(STRING, l);
  DeleteResult(STRING, r);
}

// ******************************************************************
// *                                                                *
// *                        string_le  class                        *
// *                                                                *
// ******************************************************************

/** Check if one string expression is less than or equal another.
 */
class string_le : public leop {
public:
  string_le(const char* fn, int line, expr *l, expr *r)
    : leop(fn, line, l, r) { }
  
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return BOOL;
  }
  virtual void Compute(compute_data &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new string_le(Filename(), Linenumber(), l, r);
  }
};

void string_le::Compute(compute_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  result l;
  result r;
  result* answer = x.answer;
  answer->Clear();
  x.answer = &l;
  SafeCompute(left, x);
  x.answer = &r;
  SafeCompute(right, x);
  x.answer = answer;

  if (l.isNull() || r.isNull()) {
    answer->setNull();
  }
  if (l.isError() || r.isError()) {
    answer->setError();
  }
  if (answer->isNormal()) {
    DCASSERT(l.svalue);
    DCASSERT(r.svalue);
    answer->bvalue = (strcmp(l.svalue->string, r.svalue->string)<=0);
  } 
  DeleteResult(STRING, l);
  DeleteResult(STRING, r);
}

// ******************************************************************
// *                                                                *
// *             Global functions  to build expressions             *
// *                                                                *
// ******************************************************************

expr* MakeStringAdd(expr** opnds, int n, const char* file, int line)
{
  return new string_add(file, line, opnds, n);
}

expr* MakeStringBinary(expr* left, int op, expr* right, const char* file, int
line)
{
  switch (op) {
    case PLUS:
      return new string_add(file, line, left, right);
    case EQUALS:
      return new string_equal(file, line, left, right);
    case NEQUAL:
      return new string_neq(file, line, left, right);
    case GT:
      return new string_gt(file, line, left, right);
    case GE:
      return new string_ge(file, line, left, right);
    case LT:
      return new string_lt(file, line, left, right);
    case LE:
      return new string_le(file, line, left, right);
  }
  return NULL;
}

bool StringEquals(const result &x, const result &y)
{
  if (x.svalue == y.svalue) return true;  // shared or both null
  return (0==strcmp(x.svalue->string, y.svalue->string));
}
