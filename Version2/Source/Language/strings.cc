
// $Id$

#include "strings.h"
#include "operators.h"
#include "baseops.h"

//#define DEBUG_STRINGS

void PrintString(const result& x, OutputStream &out, int width)
{
  shared_string* ssx = dynamic_cast<shared_string*>(x.other);
  DCASSERT(ssx);
  if (ssx->isDeleted()) { out << "(deleted result)"; return; }
  char* s = ssx->string;
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
  result* xop;
public:
  string_add(const char* fn, int line, expr **x, int n) 
    : addop(fn, line, x, n) { xop = new result[n]; }
  
  string_add(const char* fn, int line, expr *l, expr *r) 
    : addop(fn, line, l, r) { xop = new result[2]; }

  virtual ~string_add() { delete[] xop; }
  
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return STRING;
  }
  virtual void Compute(int i, result &x);
protected:
  virtual expr* MakeAnother(expr **x, int n) {
    return new string_add(Filename(), Linenumber(), x, n);
  }
};


void string_add::Compute(int a, result &x)
{
  DCASSERT(0==a);
  x.Clear();

  int i;

  // Compute strings for each operand
  int total_length = 0;
  for (i=0; i<opnd_count; i++) {
    DCASSERT(operands[i]);
    DCASSERT(operands[i]->Type(0) == STRING);
#ifdef DEBUG_STRINGS
    Output << "Computing operand: " << operands[i] << "\n";
    Output.flush();
#endif
    operands[i]->Compute(0, xop[i]);
#ifdef DEBUG_STRINGS
    Output << "Got operand  " << operands[i] << " = ";
    PrintResult(Output, STRING, xop[i]);
    Output << "\n";
    Output.flush();
#endif
    if (xop[i].isError() || xop[i].isNull()) {
      x = xop[i];
      break;
    }
    if (xop[i].other) {
      shared_string *ss = dynamic_cast<shared_string*>(xop[i].other);
      DCASSERT(ss);
      total_length += ss->length();
    }
  }

  if (x.isError() || x.isNull()) {
    // delete strings and bail out
    for (i=0; i<opnd_count; i++) DeleteResult(STRING, xop[i]);
    return;
  }

#ifdef DEBUG_STRINGS
  Output << "Concatenating operands: ";
  for (i=0; i<opnd_count; i++) PrintResult(Output, STRING, xop[i]);
  Output << "\n";
#endif

  // concatenate all strings
  char* answer = new char[total_length+1]; // save room for terminating 0
  answer[0] = 0;
  for (i=0; i<opnd_count; i++) {
    if (xop[i].other) {
      shared_string *ss = dynamic_cast<shared_string*>(xop[i].other);
      strcat(answer, ss->string);
    }
    DeleteResult(STRING, xop[i]);
  }

  x.other = new shared_string(answer);

#ifdef DEBUG_STRINGS
  Output << "And got answer: " << answer << "\n";
  Output.flush();
#endif
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
  
  virtual void Compute(int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new string_equal(Filename(), Linenumber(), l, r);
  }
};

void string_equal::Compute(int i, result &x)
{
  DCASSERT(0==i);
  DCASSERT(left);
  DCASSERT(right);
  result l;
  result r;
  x.Clear();
  left->Compute(0, l);
  right->Compute(0, r);

  if (l.isNull() || r.isNull()) {
    x.setNull();
  }
  if (l.isError() || r.isError()) {
    x.setError();
    return;
  }
  if (x.isNormal()) {
    shared_string* sl = dynamic_cast<shared_string*>(l.other);
    shared_string* sr = dynamic_cast<shared_string*>(r.other);
    x.bvalue = (0==compare(sl, sr));
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
  virtual void Compute(int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new string_neq(Filename(), Linenumber(), l, r);
  }
};

void string_neq::Compute(int i, result &x)
{
  DCASSERT(0==i);
  DCASSERT(left);
  DCASSERT(right);
  result l;
  result r;
  x.Clear();
  left->Compute(0, l);
  right->Compute(0, r);

  if (l.isNull() || r.isNull()) {
    x.setNull();
  }
  if (l.isError() || r.isError()) {
    x.setError();
  }
  if (x.isNormal()) {
    shared_string* sl = dynamic_cast<shared_string*>(l.other);
    shared_string* sr = dynamic_cast<shared_string*>(r.other);
    x.bvalue = (0!=compare(sl, sr));
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
  
  virtual void Compute(int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new string_gt(Filename(), Linenumber(), l, r);
  }
};

void string_gt::Compute(int i, result &x)
{
  DCASSERT(0==i);
  DCASSERT(left);
  DCASSERT(right);
  result l;
  result r;
  x.Clear();
  left->Compute(0, l);
  right->Compute(0, r);

  if (l.isNull() || r.isNull()) {
    x.setNull();
  }
  if (l.isError() || r.isError()) {
    x.setError();
  }
  if (x.isNormal()) {
    shared_string* sl = dynamic_cast<shared_string*>(l.other);
    shared_string* sr = dynamic_cast<shared_string*>(r.other);
    x.bvalue = (compare(sl, sr) > 0);
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
  virtual void Compute(int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new string_ge(Filename(), Linenumber(), l, r);
  }
};

void string_ge::Compute(int i, result &x)
{
  DCASSERT(0==i);
  DCASSERT(left);
  DCASSERT(right);
  result l;
  result r;
  x.Clear();
  left->Compute(0, l);
  right->Compute(0, r);

  if (l.isNull() || r.isNull()) {
    x.setNull();
  }
  if (l.isError() || r.isError()) {
    x.setError();
  }
  if (x.isNormal()) {
    shared_string* sl = dynamic_cast<shared_string*>(l.other);
    shared_string* sr = dynamic_cast<shared_string*>(r.other);
    x.bvalue = (compare(sl, sr) >= 0);
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
  virtual void Compute(int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new string_lt(Filename(), Linenumber(), l, r);
  }
};

void string_lt::Compute(int i, result &x)
{
  DCASSERT(0==i);
  DCASSERT(left);
  DCASSERT(right);
  result l;
  result r;
  x.Clear();
  left->Compute(0, l);
  right->Compute(0, r);

  if (l.isNull() || r.isNull()) {
    x.setNull();
  }
  if (l.isError() || r.isError()) {
    x.setError();
  }
  if (x.isNormal()) {
    shared_string* sl = dynamic_cast<shared_string*>(l.other);
    shared_string* sr = dynamic_cast<shared_string*>(r.other);
    x.bvalue = (compare(sl, sr) < 0);
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
  virtual void Compute(int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new string_le(Filename(), Linenumber(), l, r);
  }
};

void string_le::Compute(int i, result &x)
{
  DCASSERT(0==i);
  DCASSERT(left);
  DCASSERT(right);
  result l;
  result r;
  x.Clear();
  left->Compute(0, l);
  right->Compute(0, r);

  if (l.isNull() || r.isNull()) {
    x.setNull();
  }
  if (l.isError() || r.isError()) {
    x.setError();
  }
  if (x.isNormal()) { 
    shared_string* sl = dynamic_cast<shared_string*>(l.other);
    shared_string* sr = dynamic_cast<shared_string*>(r.other);
    x.bvalue = (compare(sl, sr) <= 0);
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
  if (x.other == y.other) return true;  // shared or both null
  shared_string* ssx = dynamic_cast<shared_string*>(x.other);
  DCASSERT(ssx);
  shared_string* ssy = dynamic_cast<shared_string*>(y.other);
  DCASSERT(ssy);
  return (0==compare(ssx, ssy));
}
