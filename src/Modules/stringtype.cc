
#include "stringtype.h"

#include "../ExprLib/startup.h"
#include "../ExprLib/exprman.h"
#include "../Streams/strings.h"
#include "../ExprLib/binary.h"
#include "../ExprLib/assoc.h"


// ******************************************************************
// *                                                                *
// *                       string_type  class                       *
// *                                                                *
// ******************************************************************

class string_type : public simple_type {
public:
  string_type();
protected:
  virtual void show_normal(OutputStream &s, const result& r) const;
  virtual void assign_normal(result& r, const char* s) const;
};

// ******************************************************************
// *                      string_type  methods                      *
// ******************************************************************

string_type::string_type() : simple_type("string", "String of characters", "String of characters; may be built in various ways and combined with operators.")
{
  setPrintable();
}

void string_type::show_normal(OutputStream &s, const result& r) const
{
  s.Put('"');
  shared_string* foo = smart_cast <shared_string*> (r.getPtr());
  DCASSERT(foo);
  s.Put(foo->getStr());
  s.Put('"');
}

void string_type::assign_normal(result& r, const char* s) const
{
  shared_string* ss = new shared_string;
  ss->CopyFrom(s);
  r.setPtr(ss);
}

// ******************************************************************
// *                                                                *
// *                                                                *
// *                       String expressions                       *
// *                                                                *
// *                                                                *
// ******************************************************************

// ******************************************************************
// *                                                                *
// *                        string_add class                        *
// *                                                                *
// ******************************************************************

/** Addition (concatenation) of strings.
 */
class string_add : public summation {
public:
  string_add(const char* fn, int line, expr **x, int n);
  virtual void Compute(traverse_data &x);
protected:
  virtual expr* buildAnother(expr **x, bool* f, int n) const;
};

// ******************************************************************
// *                       string_add methods                       *
// ******************************************************************

string_add::string_add(const char* fn, int line, expr **x, int n)
 : summation(fn, line, exprman::aop_plus, em->STRING, x, 0, n)
{
}

void string_add::Compute(traverse_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  // strings are accumulated into a string stream
  StringStream acc;
  // Compute strings for each operand
  for (int i=0; i<opnd_count; i++) {
    DCASSERT(operands[i]);
    DCASSERT(operands[i]->Type() == em->STRING);
    operands[i]->Compute(x);
    if (x.answer->isNull()) return;
    shared_string *xss = smart_cast <shared_string*> (x.answer->getPtr());
    DCASSERT(xss);
    acc.Put(xss->getStr());
  }
  // done, collect concatenation
  char* answer = acc.GetString();
  x.answer->setPtr(new shared_string(answer));
}

expr* string_add::buildAnother(expr **x, bool* f, int n) const
{
  DCASSERT(0==f);
  return new string_add(Filename(), Linenumber(), x, n);
}

// ******************************************************************
// *                                                                *
// *                       string_equal class                       *
// *                                                                *
// ******************************************************************

/// Check equality of two string expressions.
class string_equal : public eqop {
public:
  string_equal(const char* fn, int line, expr *l, expr *r);
  virtual void Compute(traverse_data &x);
protected:
  virtual expr* buildAnother(expr *l, expr *r) const;
};

// ******************************************************************
// *                      string_equal methods                      *
// ******************************************************************

string_equal::string_equal(const char* fn, int line, expr *l, expr *r)
 : eqop(fn, line, em->BOOL, l, r)
{
}

void string_equal::Compute(traverse_data &x)
{
  result l, r;
  LRCompute(l, r, x);

  if (l.isNull() || r.isNull()) {
    x.answer->setNull();
  } else {
    shared_string *lss = smart_cast <shared_string*>(l.getPtr());
    shared_string *rss = smart_cast <shared_string*>(r.getPtr());
    DCASSERT(lss);
    DCASSERT(rss);
    x.answer->setBool(strcmp(lss->getStr(), rss->getStr())==0);
  }
}

expr* string_equal::buildAnother(expr *l, expr *r) const
{
  return new string_equal(Filename(), Linenumber(), l, r);
}

// ******************************************************************
// *                                                                *
// *                        string_neq class                        *
// *                                                                *
// ******************************************************************

/// Check inequality of two string expressions.
class string_neq : public neqop {
public:
  string_neq(const char* fn, int line, expr *l, expr *r);
  virtual void Compute(traverse_data &x);
protected:
  virtual expr* buildAnother(expr *l, expr *r) const;
};

// ******************************************************************
// *                       string_neq methods                       *
// ******************************************************************

string_neq::string_neq(const char* fn, int line, expr *l, expr *r)
 : neqop(fn, line, em->BOOL, l, r)
{
}

void string_neq::Compute(traverse_data &x)
{
  result l, r;
  LRCompute(l, r, x);

  if (l.isNull() || r.isNull()) {
    x.answer->setNull();
  } else {
    shared_string *lss = smart_cast <shared_string*>(l.getPtr());
    shared_string *rss = smart_cast <shared_string*>(r.getPtr());
    DCASSERT(lss);
    DCASSERT(rss);
    x.answer->setBool(strcmp(lss->getStr(), rss->getStr())!=0);
  }
}

expr* string_neq::buildAnother(expr *l, expr *r) const
{
  return new string_neq(Filename(), Linenumber(), l, r);
}

// ******************************************************************
// *                                                                *
// *                        string_gt  class                        *
// *                                                                *
// ******************************************************************

/// Check if one string expression is greater than another.
class string_gt : public gtop {
public:
  string_gt(const char* fn, int line, expr *l, expr *r);
  virtual void Compute(traverse_data &x);
protected:
  virtual expr* buildAnother(expr *l, expr *r) const;
};

// ******************************************************************
// *                       string_gt  methods                       *
// ******************************************************************

string_gt::string_gt(const char* fn, int line, expr *l, expr *r)
 : gtop(fn, line, em->BOOL, l, r)
{
}

void string_gt::Compute(traverse_data &x)
{
  result l, r;
  LRCompute(l, r, x);

  if (l.isNull() || r.isNull()) {
    x.answer->setNull();
  } else {
    shared_string *lss = smart_cast <shared_string*>(l.getPtr());
    shared_string *rss = smart_cast <shared_string*>(r.getPtr());
    DCASSERT(lss);
    DCASSERT(rss);
    x.answer->setBool(strcmp(lss->getStr(), rss->getStr()) > 0);
  }
}

expr* string_gt::buildAnother(expr *l, expr *r) const
{
  return new string_gt(Filename(), Linenumber(), l, r);
}

// ******************************************************************
// *                                                                *
// *                        string_ge  class                        *
// *                                                                *
// ******************************************************************

/// Check if one string expression is greater than or equal another.
class string_ge : public geop {
public:
  string_ge(const char* fn, int line, expr *l, expr *r);
  virtual void Compute(traverse_data &x);
protected:
  virtual expr* buildAnother(expr *l, expr *r) const;
};

// ******************************************************************
// *                       string_ge  methods                       *
// ******************************************************************

string_ge::string_ge(const char* fn, int line, expr *l, expr *r)
 : geop(fn, line, em->BOOL, l, r)
{
}

void string_ge::Compute(traverse_data &x)
{
  result l, r;
  LRCompute(l, r, x);

  if (l.isNull() || r.isNull()) {
    x.answer->setNull();
  } else {
    shared_string *lss = smart_cast <shared_string*>(l.getPtr());
    shared_string *rss = smart_cast <shared_string*>(r.getPtr());
    DCASSERT(lss);
    DCASSERT(rss);
    x.answer->setBool((strcmp(lss->getStr(), rss->getStr()) >= 0));
  }
}

expr* string_ge::buildAnother(expr *l, expr *r) const
{
  return new string_ge(Filename(), Linenumber(), l, r);
}

// ******************************************************************
// *                                                                *
// *                        string_lt  class                        *
// *                                                                *
// ******************************************************************

/// Check if one string expression is less than another.
class string_lt : public ltop {
public:
  string_lt(const char* fn, int line, expr *l, expr *r);
  virtual void Compute(traverse_data &x);
protected:
  virtual expr* buildAnother(expr *l, expr *r) const;
};

// ******************************************************************
// *                       string_lt  methods                       *
// ******************************************************************

string_lt::string_lt(const char* fn, int line, expr *l, expr *r)
 : ltop(fn, line, em->BOOL, l, r)
{
}

void string_lt::Compute(traverse_data &x)
{
  result l, r;
  LRCompute(l, r, x);

  if (l.isNull() || r.isNull()) {
    x.answer->setNull();
  } else {
    shared_string *lss = smart_cast <shared_string*>(l.getPtr());
    shared_string *rss = smart_cast <shared_string*>(r.getPtr());
    DCASSERT(lss);
    DCASSERT(rss);
    x.answer->setBool(strcmp(lss->getStr(), rss->getStr()) < 0);
  }
}

expr* string_lt::buildAnother(expr *l, expr *r) const
{
  return new string_lt(Filename(), Linenumber(), l, r);
}

// ******************************************************************
// *                                                                *
// *                        string_le  class                        *
// *                                                                *
// ******************************************************************

/// Check if one string expression is less than or equal another.
class string_le : public leop {
public:
  string_le(const char* fn, int line, expr *l, expr *r);
  virtual void Compute(traverse_data &x);
protected:
  virtual expr* buildAnother(expr *l, expr *r) const;
};

// ******************************************************************
// *                       string_le  methods                       *
// ******************************************************************

string_le::string_le(const char* fn, int line, expr *l, expr *r)
 : leop(fn, line, em->BOOL, l, r)
{
}

void string_le::Compute(traverse_data &x)
{
  result l, r;
  LRCompute(l, r, x);

  if (l.isNull() || r.isNull()) {
    x.answer->setNull();
  } else {
    shared_string *lss = smart_cast <shared_string*>(l.getPtr());
    shared_string *rss = smart_cast <shared_string*>(r.getPtr());
    DCASSERT(lss);
    DCASSERT(rss);
    x.answer->setBool(strcmp(lss->getStr(), rss->getStr()) <= 0);
  }
}

expr* string_le::buildAnother(expr *l, expr *r) const
{
  return new string_le(Filename(), Linenumber(), l, r);
}

// ******************************************************************
// *                                                                *
// *                                                                *
// *                       String  operations                       *
// *                                                                *
// *                                                                *
// ******************************************************************

inline const type*
StringResultType(const exprman* em, const type* lt, const type* rt)
{
  DCASSERT(em);
  DCASSERT(em->STRING);
  if (em->NULTYPE == lt || em->NULTYPE == rt)  return 0;
  const type* lct = em->getLeastCommonType(lt, rt);
  if (0==lct)        return 0;
  if (lct->getBaseType() != em->STRING)  return 0;
  if (lct->isASet())      return 0;
  return lct;
}

inline
int StringAlignDistance(const exprman* em, const type* lt, const type* rt)
{
  DCASSERT(em);
  DCASSERT(em->STRING);
  const type* lct = StringResultType(em, lt, rt);
  if (0==lct)        return -1;

  int dl = em->getPromoteDistance(lt, lct);   DCASSERT(dl>=0);
  int dr = em->getPromoteDistance(rt, lct);   DCASSERT(dr>=0);

  return dl+dr;
}

inline const type* AlignStrings(const exprman* em, expr* &l, expr* &r)
{
  DCASSERT(em);
  DCASSERT(em->STRING);
  DCASSERT(l);
  DCASSERT(r);
  const type* lct = StringResultType(em, l->Type(), r->Type());
  if (0==lct) {
    Delete(l);
    Delete(r);
    return 0;
  }
  l = em->promote(l, lct);   DCASSERT(em->isOrdinary(l));
  r = em->promote(r, lct);   DCASSERT(em->isOrdinary(r));
  return lct;
}

inline int StringAlignDistance(const exprman* em, expr** x, int N)
{
  DCASSERT(em);
  DCASSERT(em->STRING);
  DCASSERT(x);

  const type* lct = em->SafeType(x[0]);
  for (int i=1; i<N; i++) {
    lct = em->getLeastCommonType(lct, em->SafeType(x[i]));
  }
  if (0==lct)        return -1;
  if (lct->getBaseType() != em->STRING)  return -1;
  if (lct->isASet())      return -1;

  int d = 0;
  for (int i=0; i<N; i++) {
    int dx = em->getPromoteDistance(em->SafeType(x[i]), lct);
    DCASSERT(dx>=0);
    d += dx;
  }
  return d;
}

inline const type* AlignStrings(const exprman* em, expr** x, int N)
{
  DCASSERT(em);
  DCASSERT(em->STRING);
  DCASSERT(x);

  const type* lct = em->SafeType(x[0]);
  for (int i=1; i<N; i++) {
    lct = em->getLeastCommonType(lct, em->SafeType(x[i]));
  }
  if (  (0==lct) || (lct->getBaseType() != em->STRING) || lct->isASet() ) {
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
// *                      string_add_op  class                      *
// *                                                                *
// ******************************************************************

class string_add_op : public assoc_op {
public:
  string_add_op();
  virtual int getPromoteDistance(expr** list, bool* flip, int N) const;
  virtual int getPromoteDistance(bool flip, const type* lt,
            const type* rt) const;
  virtual const type* getExprType(bool flip, const type* lt,
            const type* rt) const;
  virtual assoc* makeExpr(const char* fn, int ln, expr** list,
        bool* flip, int N) const;
};

// ******************************************************************
// *                     string_add_op  methods                     *
// ******************************************************************

string_add_op::string_add_op() : assoc_op(exprman::aop_plus)
{
}

int string_add_op::getPromoteDistance(expr** list, bool* flip, int N) const
{
  if (flip) for (int i=0; i<N; i++) if (flip[i]) return -1;
  return StringAlignDistance(em, list, N);
}

int string_add_op
::getPromoteDistance(bool flip, const type* lt, const type* rt) const
{
  if (flip)  return -1;
  return StringAlignDistance(em, lt, rt);
}

const type* string_add_op
::getExprType(bool flip, const type* lt, const type* rt) const
{
  if (flip)  return 0;
  return StringResultType(em, lt, rt);
}

assoc* string_add_op::makeExpr(const char* fn, int ln, expr** list,
        bool* flip, int N) const
{
  const type* lct = AlignStrings(em, list, N);
  if (flip) for (int i=0; i<N; i++) if (flip[i]) lct = 0;
  delete[] flip;
  if (lct)  return new string_add(fn, ln, list, N);
  // there was an error
  delete[] list;
  return 0;
}

// ******************************************************************
// *                                                                *
// *                     string_binary_op class                     *
// *                                                                *
// ******************************************************************

class string_binary_op : public binary_op {
public:
  string_binary_op(exprman::binary_opcode op);
  virtual int getPromoteDistance(const type* lt, const type* rt) const;
  virtual const type* getExprType(const type* l, const type* r) const;
  virtual binary* makeExpr(const char* fn, int ln, expr* l, expr* r) const;
protected:
  virtual binary* makeValid(const char* fn, int ln, expr* l, expr* r) const = 0;
};

// ******************************************************************
// *                    string_binary_op methods                    *
// ******************************************************************

string_binary_op::string_binary_op(exprman::binary_opcode op) : binary_op(op)
{
}

int string_binary_op::getPromoteDistance(const type* lt, const type* rt) const
{
  return StringAlignDistance(em, lt, rt);
}

const type* string_binary_op::getExprType(const type* l, const type* r) const
{
  return StringResultType(em, l, r);
}

binary* string_binary_op
::makeExpr(const char* fn, int ln, expr* l, expr* r) const
{
  const type* lct = AlignStrings(em, l, r);
  if (0==lct)  return 0;
  return makeValid(fn, ln, l, r);
}

// ******************************************************************
// *                                                                *
// *                     string_equal_op  class                     *
// *                                                                *
// ******************************************************************

class string_equal_op : public string_binary_op {
public:
  string_equal_op();
  virtual binary* makeValid(const char* fn, int ln, expr* l, expr* r) const;
};

// ******************************************************************
// *                    string_equal_op  methods                    *
// ******************************************************************

string_equal_op::string_equal_op() : string_binary_op(exprman::bop_equals)
{
}

binary* string_equal_op::makeValid(const char* fn, int ln, expr* l, expr* r) const
{
  const type* lct = AlignStrings(em, l, r);
  if (0==lct)  return 0;
  return new string_equal(fn, ln, l, r);
}

// ******************************************************************
// *                                                                *
// *                      string_neq_op  class                      *
// *                                                                *
// ******************************************************************

class string_neq_op : public string_binary_op {
public:
  string_neq_op();
  virtual binary* makeValid(const char* fn, int ln, expr* l, expr* r) const;
};

// ******************************************************************
// *                     string_neq_op  methods                     *
// ******************************************************************

string_neq_op::string_neq_op() : string_binary_op(exprman::bop_nequal)
{
}

binary* string_neq_op::makeValid(const char* fn, int ln, expr* l, expr* r) const
{
  const type* lct = AlignStrings(em, l, r);
  if (0==lct)  return 0;
  return new string_neq(fn, ln, l, r);
}

// ******************************************************************
// *                                                                *
// *                       string_gt_op class                       *
// *                                                                *
// ******************************************************************

class string_gt_op : public string_binary_op {
public:
  string_gt_op();
  virtual binary* makeValid(const char* fn, int ln, expr* l, expr* r) const;
};

// ******************************************************************
// *                      string_gt_op methods                      *
// ******************************************************************

string_gt_op::string_gt_op() : string_binary_op(exprman::bop_gt)
{
}

binary* string_gt_op::makeValid(const char* fn, int ln, expr* l, expr* r) const
{
  const type* lct = AlignStrings(em, l, r);
  if (0==lct)  return 0;
  return new string_gt(fn, ln, l, r);
}

// ******************************************************************
// *                                                                *
// *                       string_ge_op class                       *
// *                                                                *
// ******************************************************************

class string_ge_op : public string_binary_op {
public:
  string_ge_op();
  virtual binary* makeValid(const char* fn, int ln, expr* l, expr* r) const;
};

// ******************************************************************
// *                      string_ge_op methods                      *
// ******************************************************************

string_ge_op::string_ge_op()
 : string_binary_op(exprman::bop_ge)
{
}

binary* string_ge_op::makeValid(const char* fn, int ln, expr* l, expr* r) const
{
  const type* lct = AlignStrings(em, l, r);
  if (0==lct)  return 0;
  return new string_ge(fn, ln, l, r);
}

// ******************************************************************
// *                                                                *
// *                       string_lt_op class                       *
// *                                                                *
// ******************************************************************

class string_lt_op : public string_binary_op {
public:
  string_lt_op();
  virtual binary* makeValid(const char* fn, int ln, expr* l, expr* r) const;
};

// ******************************************************************
// *                      string_lt_op methods                      *
// ******************************************************************

string_lt_op::string_lt_op() : string_binary_op(exprman::bop_lt)
{
}

binary* string_lt_op::makeValid(const char* fn, int ln, expr* l, expr* r) const
{
  const type* lct = AlignStrings(em, l, r);
  if (0==lct)  return 0;
  return new string_lt(fn, ln, l, r);
}

// ******************************************************************
// *                                                                *
// *                       string_le_op class                       *
// *                                                                *
// ******************************************************************

class string_le_op : public string_binary_op {
public:
  string_le_op();
  virtual binary* makeValid(const char* fn, int ln, expr* l, expr* r) const;
};

// ******************************************************************
// *                      string_le_op methods                      *
// ******************************************************************

string_le_op::string_le_op() : string_binary_op(exprman::bop_le)
{
}

binary* string_le_op::makeValid(const char* fn, int ln, expr* l, expr* r) const
{
  const type* lct = AlignStrings(em, l, r);
  if (0==lct)  return 0;
  return new string_le(fn, ln, l, r);
}

// ******************************************************************
// *                                                                *
// *                                                                *
// *                         Initialization                         *
// *                                                                *
// *                                                                *
// ******************************************************************

class init_strings : public initializer {
  public:
    init_strings();
    virtual bool execute();
};
init_strings the_string_initializer;

init_strings::init_strings() : initializer("init_strings")
{
  usesResource("em");
  buildsResource("stringtype");
  buildsResource("types");
}

bool init_strings::execute()
{
  if (0==em)  return false;

  em->registerType(  new string_type  );
  em->setFundamentalTypes();

  em->registerOperation(  new string_add_op   );
  em->registerOperation(  new string_equal_op );
  em->registerOperation(  new string_neq_op   );
  em->registerOperation(  new string_gt_op    );
  em->registerOperation(  new string_ge_op    );
  em->registerOperation(  new string_lt_op    );
  em->registerOperation(  new string_le_op    );

  return true;
}

