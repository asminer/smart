
#include "type.h"
#include "result.h"
#include "help.h"
#include "symb_tab.h"
#include "../Options/optman.h"
#include "../Options/options.h"
#include "../Utils/initializer.h"
#include "../Utils/splay.h"
#include "../Utils/ordarray.h"

#include <sstream>

// ******************************************************************
// *                                                                *
// *                     topic_simpletype class                     *
// *                                                                *
// ******************************************************************

class topic_simpletype : public help_topic {
        const simple_type* st;
    public:
        topic_simpletype(const simple_type* t);
        virtual void PrintDocs(doc_formatter &df, const char*) const;
};

// ******************************************************************
// *                    topic_simpletype methods                    *
// ******************************************************************

topic_simpletype::topic_simpletype(const simple_type* t)
    : help_topic(t->getStr(), t->shortDocs())
{
    st = t;
}

void topic_simpletype::PrintDocs(doc_formatter &df, const char*) const
{
    if (!DocumentHeader(df)) return;
    st->printDocs(df);
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
    modif_type(const std::string &n, modifier m, simple_type* b);

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

modif_type::modif_type(const std::string &n, modifier m, simple_type* b)
 : type(n)
{
    mod = m;
    base = b;
    proc_this = nullptr;
    switch (m) {
        case PHASE:
            b->setPhase(this);
            break;

        case RAND:
            b->setRand(this);
            break;
    }

    // std::cout << "Adding modif type: " << n << "\n";
}

modifier modif_type::getModifier() const
{
    return mod;
}

const type* modif_type::modifyType(modifier m) const
{
    if (mod == m)  return this;
    return nullptr;
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
    DCASSERT(!proc_this);
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
    return nullptr;
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
    proc_type(const std::string &n, type* b);

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

proc_type::proc_type(const std::string &n, type* b) : type(n)
{
    base = b;
    b->setProc(this);

    // std::cout << "Adding proc type: " << n << "\n";
}

modifier proc_type::getModifier() const
{
    return base->getModifier();
}

const type* proc_type::modifyType(modifier m) const
{
    const type* foo = base->modifyType(m);
    if (foo) return foo->addProc();
    return nullptr;
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
    return nullptr;
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
    set_type(const std::string &n, simple_type* b);

    virtual const type* getSetElemType() const;

    virtual const simple_type* getBaseType() const;
    virtual const type* changeBaseType(const type* newbase) const;
};

// ******************************************************************
// *                        set_type methods                        *
// ******************************************************************

set_type::set_type(const std::string &n, simple_type* b) : type(n)
{
    base = b;
    b->setSet(this);

    // std::cout << "Adding set type: " << n << "\n";
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
    return nullptr;
}

// ******************************************************************
// *                                                                *
// *                                                                *
// *                          type methods                          *
// *                                                                *
// *                                                                *
// ******************************************************************

unsigned type::real_format;
const char* type::int_comma;
const char* type::real_comma;
const char* type::pos_infinity_string;
const char* type::neg_infinity_string;
splayOfShared* type::reg_tree;
orderedShared* type::reg_list;
const type* type::null;

type::type(const char* n) : shared_string(n)
{
    init();
}

type::type(const std::string &s) : shared_string(s)
{
    init();
}

void type::init()
{
    is_void = false;
    func_definable = true;
    var_definable = true;
    printable = false;
    is_formalism = false;
}

bool type::matches(const char* n) const
{
    return 0==strcmp(getStr(), n);
}

const type* type::getSetElemType() const
{
    return nullptr;
}

const type* type::getSetOfThis() const
{
    return nullptr;
}

modifier type::getModifier() const
{
    return DETERM;
}

const type* type::modifyType(modifier m) const
{
    if (DETERM == m)  return this;
    return nullptr;
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
    return nullptr;
}

void type::setProc(const type* t)
{
    DCASSERT(0);
}

const type* type::changeBaseType(const type* newbase) const
{
    return nullptr;
}

bool type::print_abnormal(std::ostream &s, const result& r, int width)
{
    // DCASSERT(isPrintable());
    if (r.isUnknown()) {
        s << formatted_string("?", width);
        return true;
    }
    if (r.isInfinity()) {
        if (r.signInfinity() < 0) {
            DCASSERT(neg_infinity_string);
            s << formatted_string(neg_infinity_string, width);
        } else {
            DCASSERT(pos_infinity_string);
            s << formatted_string(pos_infinity_string, width);
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
    if (r.isNormal()) {
        show_normal(s, r);
    } else {
       print_abnormal(s, r);
    }
}

void type::assignFromString(result& r, const char* s) const
{
    if (!s) {
        r.setNull();
        return;
    }
    if (0==strcmp(s, "infinity")) {
        r.setInfinity(1);
        return;
    }
    if (0==strcmp(s, "+infinity")) {
        r.setInfinity(1);
        return;
    }
    if (0==strcmp(s, "-infinity")) {
        r.setInfinity(-1);
        return;
    }
    assign_normal(r, s);
}

int type::compare(const result &x, const result &y) const
{
    if (x.isNormal() && y.isNormal())  return compare_normal(x, y);

    if (x.isInfinity() && y.isInfinity()) {
        return x.signInfinity() - y.signInfinity();
    }

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
        foo->Print(s);
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

modifier type::findModifier(const char* name)
{
    if (strcmp(name, "ph") == 0)    return PHASE;
    if (strcmp(name, "rand") == 0)  return RAND;
    return NO_SUCH_MODIFIER;
}

simple_type* type::registerNew(simple_type* t)
{
    DCASSERT(reg_tree);
    simple_type* tnew = smart_cast <simple_type*> (reg_tree->insert(t));
    DCASSERT(tnew);
    if (tnew != t) {
        // Already registered
        Delete(t);
    } else {
        // New; add a help topic
        symbol_table::addGlobal(new topic_simpletype(t));
    }
    return tnew;
}

void type::finalizeRegistry()
{
    if (reg_tree) {
        reg_list = new orderedShared( *reg_tree );
        delete reg_tree;
        reg_tree = nullptr;
    }
}

unsigned type::numRegistered()
{
    DCASSERT(reg_list);
    return reg_list->numElements();
}

const simple_type* type::getRegistered(unsigned i)
{
    DCASSERT(reg_list);
    return dynamic_cast<simple_type*> (reg_list->get(i));
}

simple_type* type::find(const char* tname)
{
    static const_string S;
    S.setStr(tname);
    if (reg_list) {
        return smart_cast <simple_type*> (reg_list->find(&S));
    }
    if (reg_tree) {
        return smart_cast <simple_type*> (reg_tree->find(&S));
    }
    return nullptr;
}

const type* type::find(bool set, bool proc, modifier mod, const char* tn)
{
    const simple_type* base = find(tn);
    if (!base) return nullptr;
    const type* build = base->modifyType(mod);
    if (!build) return nullptr;
    if (proc) build = build->addProc();
    if (!build) return nullptr;
    if (set) build = build->getSetOfThis();
    return build;
}

void type::allowProc(simple_type* t)
{
    if (!t) return;
    if (t->addProc()) return;

    std::stringstream ss;
    ss << "proc " << *t;
    new proc_type(ss.str(), t);
}

void type::allowProcMod(bool proc, modifier mod, simple_type* t)
{
    if (!t) return;
    if ((mod != PHASE) && (mod != RAND)) return;
    if (t->modifyType(mod)) return;

    std::stringstream ss;
    ss << ((PHASE == mod) ? "ph " : "rand ") << *t;
    type* mt = new modif_type(ss.str(), mod, t);

    if (proc) {
        std::stringstream ps;
        ps << "proc " << ss.str();
        new proc_type(ps.str(), mt);
    }
}

void type::allowSetsOf(simple_type* t)
{
    if (!t) return;
    if (t->getSetOfThis()) return;

    std::stringstream ss;
    ss << '{' << *t << '}';
    new set_type(ss.str(), t);
}

// ******************************************************************
// *                                                                *
// *                        typelist methods                        *
// *                                                                *
// ******************************************************************

typelist::typelist(unsigned n) : shared_object()
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

    for (unsigned i=0; i<nt; i++) {
        if (i) s << ':';
        if (list[i]) {
            s << *list[i];
        } else {
            s << "error";
        }
    }
    return true;
}

int typelist::Compare(const shared_object* o) const
{
    if (o==this) return 0;
    const typelist* otl = dynamic_cast <const typelist*> (o);
    if (0==otl) return 1;
    if (nt > otl->nt) return 1;
    if (nt < otl->nt) return -1;
    for (unsigned i=0; i<nt; i++) {
        if (list[i] != otl->list[i]) return list[i] - otl->list[i];
    }
    return 0;
}

// ******************************************************************
// *                                                                *
// *                      simple_type  methods                      *
// *                                                                *
// ******************************************************************

simple_type::simple_type(const char* n, const char* sd,
    const char* ld) : type (n)
{
    short_docs = sd;
    long_docs = ld;
    phase_this = nullptr;
    rand_this = nullptr;
    proc_this = nullptr;
    set_this = nullptr;
    hidden_type = false;
}

simple_type::~simple_type()
{
}

void simple_type::printDocs(doc_formatter &df) const
{
#ifndef DEVELOPMENT_CODE
    if (isHidden()) return;
#endif
    df.begin_indent();
    if (longDocs()) {
        df.Out() << longDocs();
    } else {
        df.Out() << "undocumented";
    }
    df.end_indent();
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
    return nullptr;
}

const type* simple_type::addProc() const
{
    return proc_this;
}

void simple_type::setProc(const type* t)
{
    DCASSERT(!proc_this);
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
// *                                                                *
// *                       void_type  methods                       *
// *                                                                *
// ******************************************************************

void_type::void_type(const char* n, const char* sd, const char* ld)
    : simple_type(n, sd, ld)
{
    setVoid();
}

int void_type::compare(const result& a, const result &b) const
{
    DCASSERT(getSetOfThis());
    return SIGN(a.getPtr() - b.getPtr());
}


// ******************************************************************
// ******************************************************************
// **                                                              **
// **                        Specific types                        **
// **                                                              **
// ******************************************************************
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
    virtual bool print_normal(std::ostream &s, const result& r, int w,
                    int p=-1) const;
    virtual void show_normal(std::ostream &s, const result& r) const;
    virtual void assign_normal(result& r, const char* s) const;
    virtual int compare_normal(const result &x, const result &y) const;
};

// ******************************************************************
// *                       bool_type  methods                       *
// ******************************************************************

bool_type::bool_type()
: simple_type("bool", "Boolean type", "Legal values are constants 'true' and 'false'.")
{
    setPrintable();
}

bool bool_type::print_normal(std::ostream &s, const result& r, int w, int p) const
{
    if (r.getBool())  s << formatted_string("true", w);
    else              s << formatted_string("false", w);
    return true;
}

void bool_type::show_normal(std::ostream &s, const result& r) const
{
    if (r.getBool())  s << "true";
    else              s << "false";
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

int bool_type::compare_normal(const result &x, const result &y) const
{
    return int(x.getBool()) - int(y.getBool());
}

// ******************************************************************
// *                                                                *
// *                         int_type class                         *
// *                                                                *
// ******************************************************************

class int_type : public simple_type {
public:
    int_type();
protected:
    virtual bool print_normal(std::ostream &s, const result& r, int w,
                    int p=-1) const;
    virtual void show_normal(std::ostream &s, const result& r) const;
    virtual void assign_normal(result& r, const char* s) const;
    virtual int compare_normal(const result &x, const result &y) const;
};

// ******************************************************************
// *                        int_type methods                        *
// ******************************************************************

int_type::int_type()
: simple_type("int", "Integer type", "Supported range is machine dependent, probably equivalent to a C 'long'.  Can also be infinity.")
{
    setPrintable();
}

bool int_type::print_normal(std::ostream &s, const result& r, int w, int p) const
{
    s << formatted_int(r.getInt(), w, int_comma);
    return true;
}

void int_type::show_normal(std::ostream &s, const result& r) const
{
    s << r.getInt();
}

void int_type::assign_normal(result& r, const char* s) const
{
    char* foo;
    errno = 0;
    r.setInt(strtol(s, &foo, 10)); // must be in base 10.
    if (foo[0])  r.setNull();   // bad string
    if (errno == ERANGE) {
        // we overflowed or underflowed
        r.setNull();
    }
}

int int_type::compare_normal(const result &x, const result &y) const
{
    long cmp = x.getInt() - y.getInt();
    if (cmp<0) return -1;
    if (cmp>0) return  1;
    return 0;
}

// ******************************************************************
// *                                                                *
// *                        real_type  class                        *
// *                                                                *
// ******************************************************************

class real_type : public simple_type {
public:
    real_type();
protected:
    virtual bool print_normal(std::ostream &s, const result& r, int w,
                    int p=-1) const;
    virtual void show_normal(std::ostream &s, const result& r) const;
    virtual void assign_normal(result& r, const char* s) const;
    virtual int compare_normal(const result &x, const result &y) const;
private:
    static double index_precision;
    static unsigned output_format;
    static unsigned report_format;
    friend class type_initializer;
};

double real_type::index_precision;

// ******************************************************************
// *                       real_type  methods                       *
// ******************************************************************

real_type::real_type()
: simple_type("real", "Floating-point real type", "Legal range is machine dependent, probably equivalent to a C 'double'.  Can also be infinity.")
{
    setPrintable();
}

bool real_type::print_normal(std::ostream &s, const result& r, int w, int p) const
{
#ifdef DEBUG_REAL_TYPE
    fprintf(stderr, "print_normal %lf:%d\n", r.getReal(), w);
#endif
    switch (real_format) {
        case FIXED:
                    s << fixed_real(r.getReal(), w, p, real_comma);
                    break;

        case SCIENTIFIC:

                    s << scientific_real(r.getReal(), w, p, real_comma);
                    break;

        default:
                    s << general_real(r.getReal(), w, p, real_comma);
    }
    shared_object* o = r.getPtr();
    if (o) o->Print(s, 0); // confidence interval, or something similar
    return true;
}

void real_type::show_normal(std::ostream &s, const result& r) const
{
    s << r.getReal();
    shared_object* o = r.getPtr();
    if (o) o->Print(s); // confidence interval, or something similar
}

void real_type::assign_normal(result& r, const char* s) const
{
    char* foo;
    r.setReal(strtod(s, &foo));
    if (foo[0])  r.setNull();   // bad string
}

int real_type::compare_normal(const result &x, const result &y) const
{
    double d = x.getReal() - y.getReal();
    if (d < -index_precision)  return -1;
    if (d > index_precision)  return 1;
    return 0;
}



// ******************************************************************
// *                                                                *
// *                     next_state_type  class                     *
// *                                                                *
// ******************************************************************

// Used to be derived from type
class next_state_type : public simple_type {
public:
    next_state_type();
    virtual const simple_type* getBaseType() const;
};

// ******************************************************************
// *                    next_state_type  methods                    *
// ******************************************************************

next_state_type::next_state_type()
    : simple_type("next state",
            "Internal, for next-state expressions",
            "Internal type used for next-state expressions, within models. Each event will have an enabling condition (of type bool) and an updating expression (of type next state).")
{
    NoFunctions();
    NoVariables();
    Hidden();
}

const simple_type* next_state_type::getBaseType() const
{
    return nullptr;
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

type_initializer::type_initializer() : initializer("type.cc", 1)
{
    builds_resource("types");
}

void type_initializer::execute()
{
    type::real_format = type::GENERAL;
    type::int_comma = "";
    type::real_comma = "";
    type::pos_infinity_string = "infinity";
//  type::pos_infinity_string = "+infinity";
    type::neg_infinity_string = "-infinity";
    type::reg_tree = new splayOfShared(0, 0);
    type::reg_list = nullptr;

    real_type::index_precision = 1e-5;

    //
    // Initialize types
    //

    simple_type* t_bool = type::registerNew(new bool_type);
    type::allowProc(t_bool);

    simple_type* t_int = type::registerNew(new int_type);
    type::allowProc(t_int);
    type::allowSetsOf(t_int);

    simple_type* t_real = type::registerNew(new real_type);
    type::allowProc(t_real);
    type::allowSetsOf(t_real);

    // Stochastics are set up in Modules/stochtypes.cc

    type::registerNew(new void_type("void",
        "Void type", "Type to indicate 'no value'."));
    simple_type* t_null = type::registerNew(new void_type("null",
        "Null type", "Type of the special 'null' value."));
    t_null->setPrintable();
    type::null = t_null;

    simple_type* t_model = type::registerNew(new simple_type("model",
        "Generic model", "Generic model; can be set from any formalism."));
    t_model->NoFunctions();
    t_model->setFormalism();

    type::registerNew(new next_state_type);

    //
    // Add options
    //
    option_manager &om = option_manager::global();

    option* rfopt = om.addRadioOption("RealFormat",
            "hu",
            3, type::real_format
    );
    rfopt->addRadioButton("FIXED", "Same as printf(%f)", type::FIXED);
    rfopt->addRadioButton("GENERAL", "Same as printf(%g)", type::GENERAL);
    rfopt->addRadioButton("SCIENTIFIC", "Same as printf(%e)", type::SCIENTIFIC);
    rfopt->Finish();

    om.addStringOption(
        "IntThousandSeparator",
        "Thousands separator to use when displaying integers (including bigint)",
        type::int_comma
    );
    om.addStringOption(
        "RealThousandSeparator",
        "Thousands separator to use when displaying reals",
        type::real_comma
    );

    om.addStringOption(
        "PlusInfinityString",
        "Output string for positive infinity.",
        type::pos_infinity_string
    );

    om.addStringOption(
        "MinusInfinityString",
        "Output string for negative infinity.",
        type::neg_infinity_string
    );

    om.addRealOption(
        "IndexPrecision",
        "Epsilon for real set element comparisons.",
        real_type::index_precision,
        true, false, 0,
        false, false, 0
    );

}

