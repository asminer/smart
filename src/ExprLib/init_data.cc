
// $Id$

#include "init_data.h"
#include "../Options/options.h"
#include "exprman.h"
#include "strings.h"
#include "casting.h"
#include "sets.h"
#include "intervals.h"

#include "ops_bool.h"
#include "ops_int.h"
#include "ops_real.h"
#include "ops_set.h"
#include "ops_misc.h"

#include <errno.h>

//#define REQUIRES_PRECOMPUTING

// #define DEBUG_REAL_TYPE

// ******************************************************************
// *                                                                *
// *                                                                *
// *                     Information for  types                     *
// *                                                                *
// *                                                                *
// ******************************************************************

// ******************************************************************
// *                                                                *
// *                        bool_type  class                        *
// *                                                                *
// ******************************************************************

class bool_type : public simple_type {
public:
  bool_type();
protected:
  virtual bool print_normal(OutputStream &s, const result& r, int w) const;
  virtual void show_normal(OutputStream &s, const result& r) const;
  virtual void assign_normal(result& r, const char* s) const;
  virtual bool equals_normal(const result &x, const result &y) const;
};

// ******************************************************************
// *                       bool_type  methods                       *
// ******************************************************************

bool_type::bool_type() 
: simple_type("bool", "Boolean type", "Legal values are constants 'true' and 'false'.")
{
  setPrintable();
}

bool bool_type::print_normal(OutputStream &s, const result& r, int w) const
{
  if (r.getBool())  s.Put("true", w);
  else              s.Put("false", w);
  return true;
}

void bool_type::show_normal(OutputStream &s, const result& r) const
{
  if (r.getBool())  s.Put("true");
  else              s.Put("false");
}

void bool_type::assign_normal(result& r, const char* s) const
{
  if (0==strcmp(s, "true")) {
    r.setBool(true);
    return;
  }
  if (0==strcmp(s, "false")) {
    r.setBool(false);
    return;
  }
  r.setNull();
}

bool bool_type::equals_normal(const result &x, const result &y) const
{
  return x.getBool() == y.getBool();
}

// ******************************************************************
// *                                                                *
// *                         int_type class                         *
// *                                                                *
// ******************************************************************

class int_type : public simple_type {
public:
  int_type();
  virtual int compare(const result& a, const result& b) const;
protected:
  virtual bool print_normal(OutputStream &s, const result& r) const;
  virtual bool print_normal(OutputStream &s, const result& r, int w) const;
  virtual void show_normal(OutputStream &s, const result& r) const;
  virtual void assign_normal(result& r, const char* s) const;
  virtual bool equals_normal(const result &x, const result &y) const;
};

// ******************************************************************
// *                        int_type methods                        *
// ******************************************************************

int_type::int_type()
: simple_type("int", "Integer type", "Supported range is machine dependent, probably equivalent to a C 'long'.  Can also be infinity.")
{
  setPrintable();
}

int int_type::compare(const result& a, const result& b) const
{
  if (a.isNormal() && b.isNormal()) {
    return SIGN(a.getInt() - b.getInt());
  }
  if (a.isInfinity() && b.isInfinity()) {
    // for purposes of sets, infinity = infinity
    return SIGN(a.signInfinity() - b.signInfinity());
  }
  if (a.isInfinity()) {
    return a.signInfinity();
  }
  if (b.isInfinity()) {
    return -b.signInfinity();
  }
  // what else is left?
  DCASSERT(0); 
  return 0;
}

bool int_type::print_normal(OutputStream &s, const result& r) const
{
  s.Put(r.getInt());
  return true;
}

bool int_type::print_normal(OutputStream &s, const result& r, int w) const
{
  s.Put(r.getInt(), w);
  return true;
}

void int_type::show_normal(OutputStream &s, const result& r) const
{
  s.Put(r.getInt());
}

void int_type::assign_normal(result& r, const char* s) const
{
  if (0==strcmp(s, "infinity")) {
    r.setInfinity(1);
    return;
  }
  if (0==strcmp(s, "-infinity")) {
    r.setInfinity(-1);
    return;
  }
  char* foo;
  errno = 0;
  r.setInt(strtol(s, &foo, 10)); // must be in base 10.
  if (foo[0])  r.setNull();   // bad string
  if (errno == ERANGE) {
    // we overflowed or underflowed
    r.setNull();
  }
}

bool int_type::equals_normal(const result &x, const result &y) const
{
  return x.getInt() == y.getInt();
}

// ******************************************************************
// *                                                                *
// *                        real_type  class                        *
// *                                                                *
// ******************************************************************

class real_type : public simple_type {
public:
  real_type();
  virtual int compare(const result& a, const result& b) const;
protected:
  virtual bool print_normal(OutputStream &s, const result& r) const;
  virtual bool print_normal(OutputStream &s, const result& r, int w) const;
  virtual bool print_normal(OutputStream &s, const result& r, int w, int p) const;
  virtual void show_normal(OutputStream &s, const result& r) const;
  virtual void assign_normal(result& r, const char* s) const;
  virtual bool equals_normal(const result &x, const result &y) const;
private:
  static double index_precision;
  static int output_format;
  static int report_format;
  friend void InitTypes(exprman* em);
  friend void MakeRealFormatOptions(exprman* em);
};

double real_type::index_precision = 1e-5;
int real_type::output_format;
int real_type::report_format;

// ******************************************************************
// *                       real_type  methods                       *
// ******************************************************************

real_type::real_type()
: simple_type("real", "Floating-point real type", "Legal range is machine dependent, probably equivalent to a C 'double'.  Can also be infinity.")
{
  setPrintable();
}

int real_type::compare(const result& a, const result& b) const
{
  if (a.isNormal() && b.isNormal()) {
    double d = a.getReal() - b.getReal();
    if (d < -index_precision)  return -1;
    if (d > index_precision)  return 1;
    return 0;
  }
  if (a.isInfinity() && b.isInfinity()) {
    // for purposes of sets, infinity = infinity
    return SIGN(a.signInfinity() - b.signInfinity());
  }
  if (a.isInfinity()) {
    return a.signInfinity();
  }
  if (b.isInfinity()) {
    return -b.signInfinity();
  }
  // what else is left?
  DCASSERT(0); 
  return 0;
}

bool real_type::print_normal(OutputStream &s, const result& r) const
{
#ifdef DEBUG_REAL_TYPE
  fprintf(stderr, "print_normal %lf\n", r.getReal());
#endif
  s.Put(r.getReal());
  shared_object* o = r.getPtr(); 
  if (o) o->Print(s, 0); // confidence interval, or something similar
  return true;
}

bool real_type::print_normal(OutputStream &s, const result& r, int w) const
{
#ifdef DEBUG_REAL_TYPE
  fprintf(stderr, "print_normal %lf:%d\n", r.getReal(), w);
#endif
  s.Put(r.getReal(), w);
  shared_object* o = r.getPtr(); 
  if (o) o->Print(s, 0); // confidence interval, or something similar
  return true;
}

bool real_type::print_normal(OutputStream &s, const result& r, int w, int p) const
{
#ifdef DEBUG_REAL_TYPE
  fprintf(stderr, "print_normal %lf:%d:%d\n", r.getReal(), w, p);
#endif
  s.Put(r.getReal(), w, p);
  shared_object* o = r.getPtr(); 
  if (o) o->Print(s, 0); // confidence interval, or something similar
  return true;
}

void real_type::show_normal(OutputStream &s, const result& r) const
{
  s.Put(r.getReal());
  shared_object* o = r.getPtr();
  if (o) o->Print(s, 0); // confidence interval, or something similar
}

void real_type::assign_normal(result& r, const char* s) const
{
  if (0==strcmp(s, "infinity")) {
    r.setInfinity(1);
    return;
  }
  if (0==strcmp(s, "-infinity")) {
    r.setInfinity(-1);
    return;
  }
  char* foo;
  r.setReal(strtod(s, &foo));
  if (foo[0])  r.setNull();   // bad string
}

bool real_type::equals_normal(const result &x, const result &y) const
{
  return x.getReal() == y.getReal();
}

// ******************************************************************
// *                                                                *
// *                       rf_selection class                       *
// *                                                                *
// ******************************************************************

class rf_selection : public radio_button {
  OutputStream &os;
  OutputStream::real_format which;
public:
  rf_selection(OutputStream &s, OutputStream::real_format w, 
    const char* n, const char* d, int i);
  
  virtual bool AssignToMe();
};

// ******************************************************************
// *                      rf_selection methods                      *
// ******************************************************************

rf_selection::rf_selection(OutputStream &s, OutputStream::real_format w,
  const char* n, const char* d, int i) : radio_button(n, d, i), os(s)
{
  which = w;
}

bool rf_selection::AssignToMe()
{
  os.SetRealFormat(which);
  return true;
}

option* MakeRFOption(exprman* em, OutputStream &s, const char* name, const char* doc, int &link)
{
  if (!em->hasIO()) return 0;
  radio_button** rfs = new radio_button*[3];
  rfs[0] = new rf_selection(em->cout(), OutputStream::RF_FIXED, 
        "FIXED", "Same as printf(%f)", 0);
  rfs[1] = new rf_selection(em->cout(), OutputStream::RF_GENERAL, 
        "GENERAL", "Same as printf(%g)", 1);
  rfs[2] = new rf_selection(em->cout(), OutputStream::RF_SCIENTIFIC, 
        "SCIENTIFIC", "Same as printf(%e)", 2);
  link = 1;
  return MakeRadioOption(name, doc, rfs, 3, link);
}

void MakeRealFormatOptions(exprman* em)
{
  if (!em->hasIO())  return;
  
  em->addOption( 
    MakeRFOption(
      em, 
      em->cout(), 
      "OutputRealFormat", 
      "Format to use for writing reals to the output stream", 
      real_type::output_format
    )
  );
  
  em->addOption( 
    MakeRFOption(
      em, 
      em->cout(), 
      "ReportRealFormat", 
      "Format to use for writing reals to the reporting stream", 
      real_type::report_format
    )
  );
}

// ******************************************************************
// *                                                                *
// *                     thousands_option class                     *
// *                                                                *
// ******************************************************************

class thousands_option : public custom_option {
  OutputStream &os;
public:
  thousands_option(OutputStream &s, const char* name, const char* doc);
  virtual error SetValue(char* n);
  virtual error GetValue(const char* &v) const;
};

thousands_option
::thousands_option(OutputStream& s, const char* name, const char* doc)
 :custom_option(option::String, name, doc, "any string"), os(s)
{
}

option::error thousands_option::SetValue(char* s)
{
  os.SetThousandsSeparator(s);
  return Success;
}

option::error thousands_option::GetValue(const char* &v) const
{
  v = os.GetThousandsSeparator();
  return Success;
}

void MakeSeparatorOptions(exprman* em)
{
  if (!em->hasIO())  return;
  
  em->addOption(
    new thousands_option(
      em->cout(), 
      "OutputThousandSeparator", 
      "Thousands separator to use for the output stream"
    )
  );

  em->addOption(
    new thousands_option(
      em->report(), 
      "ReportThousandSeparator", 
      "Thousands separator to use for the reporting stream"
    )
  );
}

// ******************************************************************
// *                                                                *
// *                     next_state_type  class                     *
// *                                                                *
// ******************************************************************

class next_state_type : public type {
public:
  next_state_type();
  virtual const simple_type* getBaseType() const;
};

// ******************************************************************
// *                    next_state_type  methods                    *
// ******************************************************************

next_state_type::next_state_type() : type("next state")
{
  NoFunctions();
  NoVariables();
}

const simple_type* next_state_type::getBaseType() const
{
  return 0;
}

// ******************************************************************
// *                                                                *
// *                                                                *
// *                     Promotions and casting                     *
// *                                                                *
// *                                                                *
// ******************************************************************

// ******************************************************************
// *                                                                *
// *                         null2any class                         *
// *                                                                *
// ******************************************************************

/// This class handles type "promotion" rules of the form null -> X.
class null2any : public general_conv {
public:
  null2any();
  virtual int getDistance(const type* src, const type* dest) const;
  virtual bool requiresConversion(const type*, const type*) const {
    return false;
  }
  virtual expr* convert(const char* fn, int ln, expr* e, const type* t) const {
    return new typecast(fn, ln, t, e);
  }
};

null2any::null2any() : general_conv()
{
}

int null2any::getDistance(const type* src, const type* dest) const
{
  DCASSERT(src != dest);
  DCASSERT(em);
  if (src != em->NULTYPE) return -1;
  return RANGE_EXPAND;
}

// ******************************************************************
// *                                                                *
// *                         elem2set class                         *
// *                                                                *
// ******************************************************************

/// This class handles type promotion rules of the form X -> {X}.
class elem2set : public general_conv {

    class converter : public typecast {
    public:
      converter(const char* fn, int line, const type* nt, expr* x);
      virtual void Compute(traverse_data &x);
    protected:
      virtual expr* buildAnother(expr* x) const {
        return new converter(Filename(), Linenumber(), Type(), x);
      }
    };

public:
  elem2set();
  virtual int getDistance(const type* src, const type* dest) const;
  virtual bool requiresConversion(const type* src, const type* dest) const {
    return true;
  }
  virtual expr* convert(const char* fn, int ln, expr* e, const type* d) const {
    return new converter(fn, ln, d, e);
  }
};

elem2set::converter::converter(const char* fn, int ln, const type* nt, expr* x)
 : typecast(fn, ln, nt, x)
{
  DCASSERT(x);
  DCASSERT(x->Type());
  DCASSERT(nt);
  DCASSERT(nt->getSetElemType() == x->Type());
}

void elem2set::converter::Compute(traverse_data &x)
{
  DCASSERT(opnd);
  DCASSERT(x.answer);
  opnd->Compute(x);
  if (x.answer->isInfinity() || x.answer->isNormal()) {
    shared_set* s = MakeSingleton(opnd->Type(), *x.answer);
    x.answer->setPtr(s);
  }
}

elem2set::elem2set() : general_conv()
{
}

int elem2set::getDistance(const type* src, const type* dest) const 
{
  DCASSERT(src);
  DCASSERT(dest);
  if (src->getSetOfThis() == dest) return MAKE_SET;
  return -1;
}

// ******************************************************************
// *                                                                *
// *                     formalism2model  class                     *
// *                                                                *
// ******************************************************************

/// This class handles type "promotion" rules of the form formalism -> MODEL.
class formalism2model : public general_conv {
public:
  formalism2model();
  virtual int getDistance(const type* src, const type* dest) const;
  virtual bool requiresConversion(const type*, const type*) const {
    return false;
  }
  virtual expr* convert(const char*, int, expr* src, const type*) const {
    return src;
  }
};

formalism2model::formalism2model() : general_conv()
{
}

int formalism2model::getDistance(const type* src, const type* dest) const
{
  DCASSERT(src != dest);
  DCASSERT(em);
  if (dest != em->MODEL) return -1;
  DCASSERT(src);
  return src->isAFormalism() ? 0 : -1;
}

// ******************************************************************
// *                                                                *
// *                       precomp_add  class                       *
// *                                                                *
// ******************************************************************

/** This class handles changes in modifiers that require "precomputation".
    Currently those are
         x -> rand x
         x -> proc x
         x -> proc rand x
      ph x -> proc ph x
*/
class precomp_add : public general_conv {

    class converter : public typecast {
      result cached;
      bool precomputed;
    public:
      converter(const char* fn, int line, const type* nt, expr* x);
      virtual void Compute(traverse_data &x);
      virtual void Traverse(traverse_data &x);
    protected:
      virtual expr* buildAnother(expr* x) const {
        return new converter(Filename(), Linenumber(), Type(), x);
      }
    };

public:
  precomp_add();
  virtual int getDistance(const type* src, const type* dest) const;
  virtual bool requiresConversion(const type*, const type*) const {
    return true;
  }
  virtual expr* convert(const char* fn, int ln, expr* e, const type* t) const {
    return new converter(fn, ln, t, e);
  }
};

precomp_add::converter
 ::converter(const char* fn, int line, const type* nt, expr* x)
 : typecast(fn, line, nt, x) 
{ 
  precomputed = false;
  cached.setNull();
  silent = true;
}

void precomp_add::converter::Compute(traverse_data &x)
{
  if (!precomputed) {
#ifdef REQUIRES_PRECOMPUTING
    if (em->startInternal(__FILE__, __LINE__)) {
      em->causedBy(this);
      em->internal() << "Expression not precomputed: ";
      Print(em->internal(), 0);
      em->stopIO();
    }
    exit(1);
#endif
    SafeCompute(opnd, x);
  } else {
    x.answer[0] = cached;
  }
}

void precomp_add::converter::Traverse(traverse_data &x)
{
  switch (x.which) {
    case traverse_data::PreCompute:
        x.which = traverse_data::Compute;
        x.answer = &cached;
        SafeCompute(opnd, x);
        precomputed = true;
        x.which = traverse_data::PreCompute;
        return;

    case traverse_data::FindRange: {
        x.which = traverse_data::Compute;
        SafeCompute(opnd, x);
        interval_object *range = new interval_object(*x.answer, Type(0));
        x.answer->setPtr(range);
        x.which = traverse_data::FindRange;
        return;
    }

    default:
        typecast::Traverse(x);
        return;
  }
}


precomp_add::precomp_add() : general_conv()
{
}

int precomp_add::getDistance(const type* src, const type* dest) const
{
  DCASSERT(src != dest);
  DCASSERT(em);
  if (src->hasProc()) return -1;
  if (src->isASet()) return -1;
  if (dest->isASet()) return -1;
  if (src->getBaseType() != dest->getBaseType()) return -1;

  int d = 0;
  if (dest->hasProc()) d += MAKE_PROC;

  if (src->getModifier() == DETERM) {
    switch (dest->getModifier()) {
      case DETERM:  return d;
      case RAND:    return d + MAKE_RAND;
      default:      return -1;
    } // switch
  }

  if (src->getModifier() != PHASE) return -1;
  if (dest->getModifier() != PHASE) return -1;
  return d;
}


// ******************************************************************
// *                                                                *
// *                       noop_addproc class                       *
// *                                                                *
// ******************************************************************

/** This class handles additions of "proc" that do not require anything.
    These are currently
      rand x -> proc rand x
      proc x -> proc rand x
*/
class noop_addproc : public general_conv {
public:
  noop_addproc();
  virtual int getDistance(const type* src, const type* dest) const;
  virtual bool requiresConversion(const type*, const type*) const {
    return false;
  }
  virtual expr* convert(const char* fn, int ln, expr* e, const type* t) const {
    return new typecast(fn, ln, t, e);
  }
};

noop_addproc::noop_addproc() : general_conv()
{
}

int noop_addproc::getDistance(const type* src, const type* dest) const
{
  DCASSERT(src != dest);
  DCASSERT(em);
  if (src->isASet()) return -1;
  if (dest->isASet()) return -1;
  if (!dest->hasProc()) return -1;
  if (dest->getModifier() != RAND) return -1;
  if (src->getBaseType() != dest->getBaseType()) return -1;

  int d = 0;
  if (!src->hasProc())  d = MAKE_PROC;

  switch (src->getModifier()) {
    case DETERM:    return MAKE_RAND + d;
    case RAND:      return d;
    default:        return -1;
  } // switch
}


// ******************************************************************
// *                                                                *
// *                         int2real class                         *
// *                                                                *
// ******************************************************************


/** Type promotions from int to real.
    Note this also handles {int} to {real}.
*/
class int2real : public specific_conv {

    class converter : public typecast {
    public:
      converter(const char* fn, int line, const type* nt, expr* x);
      virtual void Compute(traverse_data &x);
    protected:
      virtual expr* buildAnother(expr* x) const {
        return new converter(Filename(), Linenumber(), Type(), x);
      }
    };

    class setconv : public typecast {
 
        class int2real_convert : public set_converter::element_convert {
        public:
          virtual void convert(result &x) const {
            if (x.isNormal()) x.setReal(x.getInt());
          }
          virtual void revert(result &x) const {
            if (x.isNormal()) x.setInt(long(x.getReal()));
          }
        };

  static int2real_convert foo;

    public:
      setconv(const char* fn, int line, const type* nt, expr* x);
      virtual void Compute(traverse_data &x);
    protected:
      virtual expr* buildAnother(expr* x) const {
        return new setconv(Filename(), Linenumber(), Type(), x);
      }
    };

public:
  int2real();
  virtual int getDistance(const type* src) const {
    DCASSERT(src);
    if (src->getBaseType() != em->INT)  return -1;
    if (src->getModifier() == PHASE)    return -1;  // different rule.
    return SIMPLE_CONV;
  }
  virtual const type* promotesTo(const type* src) const;
  virtual expr* convert(const char*, int, expr*, const type*) const;
};

int2real::setconv::int2real_convert int2real::setconv::foo;

int2real::converter::converter(const char* fn, int ln, const type* nt, expr* x)
 : typecast(fn, ln, nt, x) 
{ 
}

void int2real::converter::Compute(traverse_data &x) 
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(opnd);
  opnd->Compute(x);
  if (x.answer->isNormal()) {
    x.answer->setReal(x.answer->getInt());
  }
}

int2real::setconv::setconv(const char* fn, int ln, const type* nt, expr* x)
 : typecast(fn, ln, nt, x) 
{ 
}

void int2real::setconv::Compute(traverse_data &x) 
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(opnd);
  opnd->Compute(x);
  if (x.answer->isNull()) return;

  shared_set* xs = smart_cast<shared_set*> (Share(x.answer->getPtr()));
  DCASSERT(xs);
  x.answer->setPtr(new set_converter(foo, xs));
}



int2real::int2real() : specific_conv(false) 
{
}

const type* int2real::promotesTo(const type* src) const
{
  DCASSERT(src);
  DCASSERT(em->INT == src->getBaseType());
  DCASSERT(src->getModifier() != PHASE);
  const type* dest = em->REAL;
  DCASSERT(dest);
  if (src->getModifier() != DETERM) dest = dest->modifyType(RAND);
  DCASSERT(dest);
  if (src->hasProc()) dest = dest->addProc();
  DCASSERT(dest);
  if (src->isASet()) dest = dest->getSetOfThis();
  DCASSERT(dest);
  return dest;
}

expr* int2real::convert(const char* fn, int ln, expr* e, const type* nt) const
{
  if (nt->isASet())  return new setconv(fn, ln, nt, e);
  return new converter(fn, ln, nt, e);
}

// ******************************************************************
// *                                                                *
// *                         real2int class                         *
// *                                                                *
// ******************************************************************

/** Type casting from real to int.
    Note this also handles {real} to {int}.
*/
class real2int : public specific_conv {

    class converter : public typecast {
    public:
      converter(const char* fn, int line, const type* nt, expr* x);
      virtual void Compute(traverse_data &x);
    protected:
      virtual expr* buildAnother(expr* x) const {
        return new converter(Filename(), Linenumber(), Type(), x);
      }
    };

public:
  real2int();
  virtual int getDistance(const type* src) const {
    DCASSERT(src);
    if (src->getBaseType() != em->REAL) return -1;
    return SIMPLE_CONV;
  }
  virtual const type* promotesTo(const type* src) const;
  virtual expr* convert(const char*, int, expr*, const type*) const;
};

real2int::converter::converter(const char* fn, int ln, const type* nt, expr* x)
 : typecast(fn, ln, nt, x) 
{ 
}

void real2int::converter::Compute(traverse_data &x) 
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(opnd);
  opnd->Compute(x);
  if (x.answer->isNormal()) {
    x.answer->setInt(long(x.answer->getReal()));
  }
}

real2int::real2int() : specific_conv(true) 
{
}

const type* real2int::promotesTo(const type* src) const
{
  DCASSERT(src);
  DCASSERT(em->REAL == src->getBaseType());
  const type* dest = em->INT;
  DCASSERT(dest);
  if (src->getModifier() != DETERM) dest = dest->modifyType(RAND);
  DCASSERT(dest);
  if (src->hasProc()) dest = dest->addProc();
  DCASSERT(dest);
  if (src->isASet()) dest = dest->getSetOfThis();
  DCASSERT(dest);
  return dest;
}

expr* real2int::convert(const char* fn, int ln, expr* e, const type* nt) const
{
  // TBD: {real} to {int}
  return new converter(fn, ln, nt, e);
}

// ******************************************************************
// *                                                                *
// *                                                                *
// *                           Front  end                           *
// *                                                                *
// *                                                                *
// ******************************************************************

void InitTypes(exprman* em)
{
  if (0==em)  return;

  simple_type* t_bool  = new bool_type;
  simple_type* t_int   = new int_type;
  simple_type* t_real  = new real_type;

  type* t_void    = new void_type("void", "Void type", "Type to indicate 'no value'.");
  type* t_null    = new void_type("null", "Null type", "Type of the special 'null' value.");
  t_null->setPrintable();
  type* t_model    = new simple_type("model", "Generic model", "Generic model; can be set from any formalism.");
  t_model->NoFunctions();

  type* t_proc_bool  = newProcType("proc bool", t_bool);
  type* t_proc_int  = newProcType("proc int", t_int);
  type* t_proc_real  = newProcType("proc real", t_real);

  type* t_set_int  = newSetType("{int}", t_int);
  type* t_set_real  = newSetType("{real}", t_real);

  // register everything
  em->registerType(t_null);
  em->registerType(t_void);
  em->registerType(t_bool);
  em->registerType(t_int);
  em->registerType(t_real);
  em->registerType(t_model);

  em->registerType(new next_state_type);

  em->setFundamentalTypes();

  em->registerType(t_proc_bool);
  em->registerType(t_proc_int);
  em->registerType(t_proc_real);

  em->registerType(t_set_int);
  em->registerType(t_set_real);

  // Type changes
  em->registerConversion( new null2any        );
  em->registerConversion( new formalism2model );
  em->registerConversion( new precomp_add     );
  em->registerConversion( new noop_addproc    );
  em->registerConversion( new elem2set        );
  em->registerConversion( new int2real        );
  em->registerConversion( new real2int        );

  // options
  option* ip = MakeRealOption(
      "IndexPrecision", 
      "Epsilon for real set element comparisons.",
      real_type::index_precision,
      true, false, 0,
      false, false, 0
  );

  em->addOption(ip);
  MakeRealFormatOptions(em);
  MakeSeparatorOptions(em);

  // Operators
  InitBooleanOps(em);
  InitIntegerOps(em);
  InitRealOps(em);
  InitSetOps(em);
  InitMiscOps(em);
}

