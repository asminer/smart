
#include "type.h"
#include "result.h"
#include "../Options/optman.h"
#include "../Options/options.h"
#include "../Utils/initializer.h"

// ******************************************************************
// *                          type methods                          *
// ******************************************************************

unsigned type::real_format;
const char* type::int_comma;
const char* type::real_comma;
const char* type::pos_infinity_string;
const char* type::neg_infinity_string;

type::type(const char* n)
{
  name = n;
  is_void = false;
  func_definable = true;
  var_definable = true;
  printable = false;
}

type::~type()
{
}

bool type::matchesOWD(const char* n) const
{
  return matches(n);
}

bool type::isAFormalism() const
{
  return false;
}

const type* type::getSetElemType() const
{
  return 0;
}

const type* type::getSetOfThis() const
{
  return 0;
}

modifier type::getModifier() const
{
  return DETERM;
}

const type* type::modifyType(modifier m) const
{
  if (DETERM == m)  return this;
  return 0;
}

const type* type::removeModif() const
{
  return this;
}

bool type::hasProc() const
{
  return false;
}

const type* type::removeProc() const
{
  return this;
}

const type* type::addProc() const
{
  return 0;
}

void type::setProc(const type* t)
{
  DCASSERT(0);
}

const type* type::changeBaseType(const type* newbase) const
{
  return 0;
}

bool type::print_abnormal(std::ostream &s, const result& r, int width)
{
    DCASSERT(isPrintable());
    if (r.isUnknown()) {
        s << formatted_string("?", width);
        return true;
    }
    if (r.isInfinity()) {
        if (r.signInfinity() < 0) {
            DCASSERT(neg_infinity_string);
            s << formatted_string(getMinusInfinityString(), width);
        } else {
            DCASSERT(pos_infinity_string);
            s << formatted_string(getPlusInfinityString(), width);
        }
        return true;
    }
    if (r.isNull()) {
        s << formatted_string("null", width);
        return true;
    }
    DCASSERT(false);
    return false;
}

bool type::print(std::ostream &s, const result& r, int w, int p) const
{
    return r.isNormal() ? print_normal(s, r, w, p) : print_abnormal(s, r, w);
}

void type::show(std::ostream &s, const result& r) const
{
  if (r.isUnknown() || r.isInfinity() || r.isNull()) {
    print(s, r);
    return;
  }
  return show_normal(s, r);
}

void type::assignFromString(result& r, const char* s) const
{
  if (0==s) {
    r.setNull();
    return;
  }
  assign_normal(r, s);
}

int type::compare(const result &x, const result &y) const
{
  if (x.isNormal() && y.isNormal())  return compare_normal(x, y);

  if (x.isInfinity() && y.isInfinity())
    return x.signInfinity() - y.signInfinity();

  if (x.isInfinity()) {
      return x.signInfinity();
  }
  if (y.isInfinity()) {
      return -y.signInfinity();
  }

  if (x.isNull() && y.isNull()) return 0;

  if (x.isNull()) return -1;
  return 1;
}

bool type::print_normal(std::ostream &s, const result& r, int w, int p) const
{
    shared_object* foo = r.getPtr();
    if (foo) {
        foo->Print(s, w);
        return true;
    }
    DCASSERT(0);
    return false;
}

void type::show_normal(std::ostream &s, const result& r) const
{
  shared_object* foo = r.getPtr();
  if (foo) {
    foo->Print(s, 0);
    return;
  }
  DCASSERT(0);
}

void type::assign_normal(result& r, const char* s) const
{
  DCASSERT(0);
}

int type::compare_normal(const result &x, const result &y) const
{
  shared_object* xo = x.getPtr();
  shared_object* yo = y.getPtr();
  DCASSERT(xo);
  DCASSERT(yo);
  return xo->Compare(yo);
}

// ******************************************************************
// *                        typelist methods                        *
// ******************************************************************

typelist::typelist(int n) : shared_object()
{
  list = new const type*[n];
  nt = n;
}

typelist::~typelist()
{
  delete[] list;
}

bool typelist::Print(std::ostream &s, int) const
{
  DCASSERT(list);
  s << ( list[0] ? list[0]->getName() : "error" );
  for (int i=1; i<nt; i++) {
    s << ':';
    s << ( list[i] ? list[i]->getName() : "error" );
  }
  return true;
}

int typelist::Compare(const shared_object* o) const
{
  if (o==this) return 0;
  const typelist* otl = dynamic_cast <const typelist*> (o);
  if (0==otl) return 1;
  if (nt != otl->nt) return nt - otl->nt;
  for (int i=0; i<nt; i++)
    if (list[i] != otl->list[i]) return list[i] - otl->list[i];
  return 0;
}

// ******************************************************************
// *                      simple_type  methods                      *
// ******************************************************************

simple_type::simple_type(const char* n, const char* sd, const char* ld)
: type (n)
{
  short_docs = sd;
  long_docs = ld;
  phase_this = 0;
  rand_this = 0;
  proc_this = 0;
  set_this = 0;
}

simple_type::~simple_type()
{
}

const type* simple_type::getSetOfThis() const
{
  return set_this;
}

const type* simple_type::modifyType(modifier m) const
{
  switch (m) {
    case DETERM:  return this;
    case PHASE:   return phase_this;
    case RAND:    return rand_this;
  }
  return 0;
}

const type* simple_type::addProc() const
{
  return proc_this;
}

void simple_type::setProc(const type* t)
{
  DCASSERT(0==proc_this);
  proc_this = t;
}

const simple_type* simple_type::getBaseType() const
{
  return this;
}

const type* simple_type::changeBaseType(const type* newbase) const
{
  return newbase->getBaseType();
}

// ******************************************************************
// *                       void_type  methods                       *
// ******************************************************************

void_type::void_type(const char* n, const char* sd, const char* ld)
    : simple_type(n, sd, ld)
{
    setVoid();
}

int void_type::compare(const result& a, const result &b) const
{
    DCASSERT(getSetOfThis());
    return SIGN(long(a.getPtr()) - long(b.getPtr()));
}

// ******************************************************************
// *                                                                *
// *                        modif_type class                        *
// *                                                                *
// ******************************************************************

/** Modified type, such as ph int.
*/
class modif_type : public type {
  const type* base;
  const type* proc_this;
  modifier mod;
public:
  modif_type(const char* n, modifier m, simple_type* b);
  virtual ~modif_type();

  virtual modifier getModifier() const;
  virtual const type* modifyType(modifier m) const;
  virtual const type* removeModif() const;
  virtual const type* addProc() const;
  virtual void setProc(const type* t);

  virtual const simple_type* getBaseType() const;
  virtual const type* changeBaseType(const type* newbase) const;
};

// ******************************************************************
// *                       modif_type methods                       *
// ******************************************************************

modif_type::modif_type(const char* n, modifier m, simple_type* b)
 : type(n)
{
  mod = m;
  base = b;
  proc_this = 0;
  switch (m) {
    case PHASE:
        b->setPhase(this);
        break;

    case RAND:
        b->setRand(this);
        break;
  }
}

modif_type::~modif_type()
{
}

modifier modif_type::getModifier() const
{
  return mod;
}

const type* modif_type::modifyType(modifier m) const
{
  if (mod == m)  return this;
  return 0;
}

const type* modif_type::removeModif() const
{
  return base;
}

const type* modif_type::addProc() const
{
  return proc_this;
}

void modif_type::setProc(const type* t)
{
  DCASSERT(0==proc_this);
  proc_this = t;
}

const simple_type* modif_type::getBaseType() const
{
  return base->getBaseType();
}

const type* modif_type::changeBaseType(const type* newbase) const
{
  const type* foo = base->changeBaseType(newbase);
  if (foo) return foo->modifyType(mod);
  return 0;
}

// ******************************************************************
// *                                                                *
// *                        proc_type  class                        *
// *                                                                *
// ******************************************************************

/** Procified type, such as proc int, or proc rand int.
*/
class proc_type : public type {
  const type* base;
public:
  proc_type(const char* n, type* b);
  virtual ~proc_type();

  virtual modifier getModifier() const;
  virtual const type* modifyType(modifier m) const;
  virtual const type* removeModif() const;
  virtual bool hasProc() const;
  virtual const type* removeProc() const;

  virtual const simple_type* getBaseType() const;
  virtual const type* changeBaseType(const type* newbase) const;
};

// ******************************************************************
// *                       proc_type  methods                       *
// ******************************************************************

proc_type::proc_type(const char* n, type* b) : type(n)
{
  base = b;
  b->setProc(this);
}

proc_type::~proc_type()
{
}

modifier proc_type::getModifier() const
{
  return base->getModifier();
}

const type* proc_type::modifyType(modifier m) const
{
  const type* foo = base->modifyType(m);
  if (foo) return foo->addProc();
  return 0;
}

const type* proc_type::removeModif() const
{
  return base->removeModif();
}

bool proc_type::hasProc() const
{
  return true;
}

const type* proc_type::removeProc() const
{
  return base;
}

const simple_type* proc_type::getBaseType() const
{
  return base->getBaseType();
}

const type* proc_type::changeBaseType(const type* newbase) const
{
  const type* foo = base->changeBaseType(newbase);
  if (foo) return foo->addProc();
  return 0;
}


// ******************************************************************
// *                                                                *
// *                         set_type class                         *
// *                                                                *
// ******************************************************************

/** Set types, such as {int} or {place}.
*/
class set_type : public type {
  const type* base;
public:
  set_type(const char* n, simple_type* b);
  virtual ~set_type();

  virtual const type* getSetElemType() const;

  virtual const simple_type* getBaseType() const;
  virtual const type* changeBaseType(const type* newbase) const;
};

// ******************************************************************
// *                        set_type methods                        *
// ******************************************************************

set_type::set_type(const char* n, simple_type* b) : type(n)
{
  base = b;
  b->setSet(this);
}

set_type::~set_type()
{
}

const type* set_type::getSetElemType() const
{
  return base;
}

const simple_type* set_type::getBaseType() const
{
  return base->getBaseType();
}

const type* set_type::changeBaseType(const type* newbase) const
{
  const type* foo = base->changeBaseType(newbase);
  if (foo) return foo->getSetOfThis();
  return 0;
}

// ******************************************************************
// *                                                                *
// *                         Initialization                         *
// *                                                                *
// ******************************************************************

class type_initializer : public initializer {
    public:
        type_initializer();
    protected:
        virtual void execute();
};
static type_initializer the_type_initializer;

type_initializer::type_initializer() : initializer("type.cc", 1, 1)
{
    builds_resource(0, "type.cc");
    needs_resource(1, "OM");
}

void type_initializer::execute()
{
    type::real_format = type::GENERAL;
    type::int_comma = "";
    type::real_comma = "";
    type::pos_infinity_string = "infinity";
//  type::pos_infinity_string = "+infinity";
    type::neg_infinity_string = "-infinity";

    //
    // Add options
    //
    option_manager* om = dynamic_cast <option_manager*> (get_object(1, "OM"));
    if (!om) return;

    option* rfopt = om->addRadioOption("RealFormat",
            "hu",
            3, type::real_format
    );
    rfopt->addRadioButton("FIXED", "Same as printf(%f)", type::FIXED);
    rfopt->addRadioButton("GENERAL", "Same as printf(%g)", type::GENERAL);
    rfopt->addRadioButton("SCIENTIFIC", "Same as printf(%e)", type::SCIENTIFIC);
    rfopt->Finish();

    om->addStringOption(
        "IntThousandSeparator",
        "Thousands separator to use when displaying integers (including bigint)",
        type::int_comma
    );
    om->addStringOption(
        "RealThousandSeparator",
        "Thousands separator to use when displaying reals",
        type::real_comma
    );

    om->addStringOption(
        "PlusInfinityString",
        "Output string for positive infinity.",
        type::pos_infinity_string
    );

    om->addStringOption(
        "MinusInfinityString",
        "Output string for negative infinity.",
        type::neg_infinity_string
    );
}

// ******************************************************************
// *                                                                *
// *                           Front  end                           *
// *                                                                *
// ******************************************************************

type* newModifiedType(const char* n, modifier m, simple_type* base)
{
  return new modif_type(n, m, base);
}

type* newProcType(const char* n, type* base)
{
  return new proc_type(n, base);
}

type* newSetType(const char* n, simple_type* base)
{
  return new set_type(n, base);
}

