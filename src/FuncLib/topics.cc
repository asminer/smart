
#include "topics.h"

#include "../include/defines.h"
#include "../include/heap.h"

#include "../Utils/textfmt.h"
#include "../Utils/initializer.h"

#include "../ExprLib/help.h"
#include "../ExprLib/type.h"
#include "../ExprLib/formalism.h"
#include "../ExprLib/functions.h"
#include "../ExprLib/symb_tab.h"
#include "../ExprLib/casting.h"
#include "../ExprLib/binary.h"
#include "../ExprLib/trinary.h"
#include "../ExprLib/assoc.h"

#include "../Options/optman.h"

// ******************************************************************
//
//  A generic 'visit all pairs of types' class.
//
//      virtual void header(const type* t1) const;
//      virtual bool check(const type* t1, const type* t2) const;
//      virtual void display(const type* t2) const;
//
//
// ******************************************************************


class enumerate_pairs : public shared_visitor {
    public:
        // Derive from this to use enumerate_pairs.
        class visitor {
            public:
                // Check if we're going to print anything for types t1, t2
                virtual bool check(const type* t1, const type* t2) const = 0;
                // Show header for t1 (knowing we'll print something)
                virtual void header(doc_formatter &df, const type* t1)
                    const = 0;
                // Show entry for t2 under header for t1
                virtual void display(doc_formatter &df, const type* t1,
                    const type* t2) const = 0;
        };

    public:
        enumerate_pairs(doc_formatter &d, visitor &pv);

        virtual void visit(const shared_object* obj);

    private:
        class inner : public shared_visitor {
                const type* first;
                bool first_pass;
                unsigned width;
            public:
                doc_formatter &df;
                visitor &pv;
            public:
                inner(doc_formatter &d, visitor &pv);
                virtual void visit(const shared_object* obj);

                inline unsigned getWidth() const { return width; }

                inline void new_first(const type* f) {
                    first = f;
                    first_pass = true;
                    width = 0;
                }
                inline void pass_2() {
                    first_pass = false;
                }
        };

    private:
        inner IN;
};

// ******************************************************************

enumerate_pairs::enumerate_pairs(doc_formatter &d, visitor &pv)
    : IN(d, pv)
{
}

void enumerate_pairs::visit(const shared_object* obj)
{
    const type* first = dynamic_cast <const type*> (obj);
    if (!first) return;

    IN.new_first(first);
    type::traverseRegistry(IN);

    unsigned w = IN.getWidth();
    if (!w) return;  // empty list

    IN.pv.header(IN.df, first);
    IN.df.begin_indent();
    IN.df.begin_description(4*(w/4+1));
    IN.pass_2();
    type::traverseRegistry(IN);
    IN.df.end_description();
    IN.df.end_indent();
}

// ******************************************************************

enumerate_pairs::inner::inner(doc_formatter &d, visitor &_pv) : df(d), pv(_pv)
{
    first = nullptr;
}

void enumerate_pairs::inner::visit(const shared_object* obj)
{
    if (!first) return;
    const type* second = dynamic_cast <const type*> (obj);
    if (!second) return;

    if (!pv.check(first, second)) return;

    if (first_pass) {
        width = MAX(width, second->length());
    } else {
        pv.display(df, first, second);
    }
}

// ******************************************************************
// *                       topic_topics class                       *
// ******************************************************************

class topic_topics : public help_topic {
public:
    topic_topics(const symbol_table &s);
    virtual void DocumentBehavior(doc_formatter &df) const;

    class visitor : public shared_visitor {
            doc_formatter &df;
            unsigned maxname;
            bool firstpass;
        public:
            visitor(doc_formatter &d);
            virtual void visit(const shared_object* item);
            inline unsigned getMaxName() const { return maxname; }
            inline void secondPass() { firstpass = false; }
    };

private:
    const symbol_table &st;
};

// ******************************************************************

topic_topics::visitor::visitor(doc_formatter &d) : df(d)
{
    maxname = 0;
    firstpass = true;
}

void topic_topics::visitor::visit(const shared_object* item)
{
    const symbol* chain = dynamic_cast <const symbol*> (item);
    for (; chain; chain = chain->Next()) {
        const help_topic* ht = dynamic_cast <const help_topic*> (chain);
        if (!ht) continue;
#ifndef DEVELOPMENT_CODE
        if (!ht->Summary()) continue;
#endif
        if (firstpass) {
            // Determine longest name
            unsigned len = strlen(ht->Name());
            maxname = MAX(maxname, len);
        } else {
            // Display documentation
            df.item(ht->Name());
            if (ht->Summary()) {
                df.Out() << ht->Summary();
            } else {
                df.Out() << "undocumented";
            }
        }
    } // chain of symbols traversal
}

// ******************************************************************

topic_topics::topic_topics(const symbol_table &s)
    : help_topic("topics", "Shows all available help topics (this list!)"),
      st(s)
{
}

void topic_topics::DocumentBehavior(doc_formatter &df) const
{
    visitor V(df);
    // First pass: determine longest topic name
    st.traverse(V);

    df.Out() << "The following help topics are available:\n\n";
    df.begin_description(V.getMaxName());

    // Second pass: display topics
    V.secondPass();
    st.traverse(V);

    df.end_description();
}

// ******************************************************************
// *                       topic_types  class                       *
// ******************************************************************

class topic_types : public help_topic {
    public:
        topic_types() : help_topic("types",
                "Shows the available types for declared objects")
        { }
        virtual void DocumentBehavior(doc_formatter &df) const;

    private:
        class filtertypes : public shared_visitor {
                std::ostream &out;
            public:
                bool void_off;
                bool form_off;
                bool set_off;
                bool stoch_off;
                bool proc_off;
                bool default_off;
            public:
                filtertypes(std::ostream &o);
                virtual void visit(const shared_object* item);
        };

};

// ******************************************************************

topic_types::filtertypes::filtertypes(std::ostream &o) : out(o)
{
    void_off = true;
    form_off = true;
    set_off = true;
    stoch_off = true;
    proc_off = true;
    default_off = true;
}

void topic_types::filtertypes::visit(const shared_object* item)
{
    const type* t = dynamic_cast <const type*> (item);
    if (!t) return;
    bool deflt = true;
    if (t->isVoid()) {
       if (void_off) return;
       else deflt = false;
    }
    if (t->isAFormalism()) {
        if (form_off) return;
        else deflt = false;
    }
    if (t->isASet()) {
       if (set_off) return;
       else deflt = false;
    }
    if (t->hasProc()) {
        if (proc_off) return;
        else deflt = false;
    } else if (t->getModifier() != DETERM) {
        if (stoch_off) return;
        else deflt = false;
    }
    if (deflt && default_off) return;
    out << *t << "\n";
}

// ******************************************************************

void topic_types::DocumentBehavior(doc_formatter &df) const
{
    df.Out() << "The Smart language is strictly typed; all objects have a specified type. Basic types can be further modified by *natures*, which specify if the object is deterministic or random. Furthermore, objects may be allowed to depend on the state of a stochastic process, which are again modified by the keyword *proc*. Types are also used for formalisms, formalism variables, and sets of objects.\n\n";

    filtertypes ft(df.Out());

    df.Out() << "Simple types:\n";
    ft.default_off = false;
    df.begin_indent();
    type::traverseRegistry(ft, false);
    df.end_indent();

    df.Out() << "\nStochastic types:\n";
    ft.default_off = true;
    ft.stoch_off = false;
    df.begin_indent();
    type::traverseRegistry(ft);
    df.end_indent();

    df.Out() << "\nProcess types:\n";
    ft.stoch_off = true;
    ft.proc_off = false;
    df.begin_indent();
    type::traverseRegistry(ft);
    df.end_indent();

    df.Out() << "\nFormalism types:\n";
    ft.proc_off = true;
    ft.form_off = false;
    df.begin_indent();
    type::traverseRegistry(ft, false);
    df.end_indent();

    df.Out() << "\nVoid types (usually within formalisms):\n";
    ft.form_off = true;
    ft.void_off = false;
    df.begin_indent();
    type::traverseRegistry(ft, false);
    df.end_indent();

    df.Out() << "\nSet types:\n";
    ft.void_off = true;
    ft.set_off = false;
    df.begin_indent();
    type::traverseRegistry(ft);
    df.end_indent();
    df.Out() << "\nSee the help topics \"promotions\" and \"casting\" for details about how Smart changes types, and how you can force a type change.\n";
}

// ******************************************************************
// *                     topic_promotions class                     *
// ******************************************************************

class topic_promotions : public help_topic {
    public:
        topic_promotions();
        virtual void DocumentBehavior(doc_formatter &df) const;

    private:
        class pvisit : public enumerate_pairs::visitor {
            public:
                virtual bool check(const type* t1, const type* t2) const;
                virtual void header(doc_formatter &df, const type* t1) const;
                virtual void display(doc_formatter &df, const type* t1,
                        const type* t2) const;
        };
};

// ************************************************************

bool topic_promotions::pvisit::check(const type* t1, const type* t2) const
{
    if (!t1 || !t2) return false;
    return typeconv::getPromoteDistance(t1, t2) > 0;
}

void topic_promotions::pvisit::header(doc_formatter &df, const type* t1) const
{
    df.Out() << "\nFrom " << *t1 << " to:\n";
}

void topic_promotions::pvisit::display(doc_formatter &df, const type* t1,
        const type* t2) const
{
    df.item(t2->getStr());
    df.Out() << "(distance "
             << typeconv::getPromoteDistance(t1, t2) << ")\n";
}

// ************************************************************

topic_promotions::topic_promotions() : help_topic("promotions",
        "Which types can be promoted to which other types")
{
}

void topic_promotions::DocumentBehavior(doc_formatter &df) const
{
    df.Out() << "If necessary, Smart will attempt to promote expressions to other types.  Each promotion has an associated \"distance\", and Smart will normally choose the promotion with least distance (or give an error if it is unable to decide).  A type promotion can be forced using an explicit cast, see the help topic on \"casting\" for details.  Smart uses the following promotions:\n";

    pvisit pv;
    enumerate_pairs EP(df, pv);
    type::traverseRegistry(EP);
}


// ******************************************************************
// *                      topic_casting  class                      *
// ******************************************************************

class topic_casting : public help_topic {
    public:
        topic_casting();
        virtual void DocumentBehavior(doc_formatter &df) const;

    private:
        class pvisit : public enumerate_pairs::visitor {
            public:
                virtual bool check(const type* t1, const type* t2) const;
                virtual void header(doc_formatter &df, const type* t1) const;
                virtual void display(doc_formatter &df, const type* t1,
                        const type* t2) const;
        };
};

// ************************************************************

bool topic_casting::pvisit::check(const type* t1, const type* t2) const
{
    if (typeconv::isPromotable(t1, t2)) return false;
    return typeconv::isCastable(t1, t2);
}

void topic_casting::pvisit::header(doc_formatter &df, const type* t1) const
{
    df.Out() << "\nFrom " << *t1 << " to:\n";
}

void topic_casting::pvisit::display(doc_formatter &df, const type* t1,
        const type* t2) const
{
    df.item(t2->getStr());
    df.Out() << '\n';
}

// ************************************************************

topic_casting::topic_casting() : help_topic("casting",
        "Which types can be converted to which other types")
{
}

void topic_casting::DocumentBehavior(doc_formatter &df) const
{
    df.Out() << "An expression can be cast to a different using new_type(expr). This can be done to force promotion of an expression, or to change its type. For example:\n\n";
    df.begin_indent();
    df.Out() << "rand real x := ...;\nrand int i := rand int(x);\n\n";
    df.end_indent();

    df.Out() << "An expression can be explicitly cast from type A to type B if it can be promoted from type A to type B (see help topic \"promotions\").  In addition, the following conversions are allowed:\n";

    pvisit pv;
    enumerate_pairs EP(df, pv);
    type::traverseRegistry(EP);
}


// ******************************************************************
// *                     topic_operators  class                     *
// ******************************************************************

class topic_operators : public help_topic {
    public:
        topic_operators();
        virtual void DocumentBehavior(doc_formatter &df) const;
};

// ******************************************************************
topic_operators::topic_operators() : help_topic("operators",
        "Information about operators")
{
}

void topic_operators::DocumentBehavior(doc_formatter &df) const
{
    df.Out() << "The following unary operators may be used when constructing expressions in Smart:\n\n";

    df.begin_description(2);
    df.item(unary_op::getOp(unary_op::uop_not));
    df.Out() << unary_op::documentOp(unary_op::uop_not);
    df.item(unary_op::getOp(unary_op::uop_neg));
    df.Out() << unary_op::documentOp(unary_op::uop_neg);
    df.end_description();

    df.Out() << "\nThe following binary operators may be used when constructing expressions in Smart:\n\n";

    df.begin_description(2);
    df.item(assoc_op::getOp(0, assoc_op::aop_plus));
    df.Out() << assoc_op::documentOp(0, assoc_op::aop_plus);
    df.item(assoc_op::getOp(1, assoc_op::aop_plus));
    df.Out() << assoc_op::documentOp(1, assoc_op::aop_plus);
    df.item(assoc_op::getOp(0, assoc_op::aop_times));
    df.Out() << assoc_op::documentOp(0, assoc_op::aop_times);
    df.item(assoc_op::getOp(1, assoc_op::aop_times));
    df.Out() << assoc_op::documentOp(1, assoc_op::aop_times);
    df.item(binary_op::getOp(binary_op::bop_mod));
    df.Out() << binary_op::documentOp(binary_op::bop_mod);
    df.item(binary_op::getOp(binary_op::bop_diff));
    df.Out() << binary_op::documentOp(binary_op::bop_diff);
    df.end_description();
    df.Out() << "\n";
    df.begin_description(2);
    df.item(assoc_op::getOp(0, assoc_op::aop_or));
    df.Out() << assoc_op::documentOp(0, assoc_op::aop_or);
    df.item(assoc_op::getOp(0, assoc_op::aop_and));
    df.Out() << assoc_op::documentOp(0, assoc_op::aop_and);
    df.item(binary_op::getOp(binary_op::bop_implies));
    df.Out() << binary_op::documentOp(binary_op::bop_implies);
    df.end_description();
    df.Out() << "\n";
    df.begin_description(2);
    df.item(binary_op::getOp(binary_op::bop_equals));
    df.Out() << binary_op::documentOp(binary_op::bop_equals);
    df.item(binary_op::getOp(binary_op::bop_nequal));
    df.Out() << binary_op::documentOp(binary_op::bop_nequal);
    df.item(binary_op::getOp(binary_op::bop_gt));
    df.Out() << binary_op::documentOp(binary_op::bop_gt);
    df.item(binary_op::getOp(binary_op::bop_ge));
    df.Out() << binary_op::documentOp(binary_op::bop_ge);
    df.item(binary_op::getOp(binary_op::bop_lt));
    df.Out() << binary_op::documentOp(binary_op::bop_lt);
    df.item(binary_op::getOp(binary_op::bop_le));
    df.Out() << binary_op::documentOp(binary_op::bop_le);
    df.end_description();
    df.Out() << "\n";
    df.begin_description(2);
    df.item(assoc_op::getOp(0, assoc_op::aop_semi));
    df.Out() << assoc_op::documentOp(0, assoc_op::aop_semi);
    df.end_description();

    df.Out() << "\nSee the help topic for an operator name for details about that operator.\n\n";
    df.Out() << "Note that void expressions may be grouped with braces; for example, ";
    df.begin_indent();
    df.Out() << "{ void1; void2; void3; }";
    df.end_indent();
    df.Out() << "produces a new void expression as the sequence of expressions void1, void2, and void3.\n";
}

// ******************************************************************
// *                      topic_options  class                      *
// ******************************************************************

class topic_options : public help_topic {
    public:
        topic_options();
        virtual void DocumentBehavior(doc_formatter &df) const;
};

// ******************************************************************

topic_options::topic_options() : help_topic("options", "How to use options")
{
}

void topic_options::DocumentBehavior(doc_formatter &df) const
{
    df.Out() << "An option statement is used to modify the behavior of Smart.  For example, there are options to control the solution algorithms (such as the precision or maximum number of iterations allowed) or the level of verbosity.  Option statements appear on lines beginning with \"#\" (except for the \"# include\" directive, which is handled by the preprocessor).  Different options have different types, and different sets of legal values.  Furthermore, some options are nested within other options.  The syntax of an option statement depends on the option type.  Generally, extra space in an option statement is fine, but newline characters are significant.  Basic options may be set using a statement of the form ";
    df.begin_indent();
    df.Out() << "# IntegerOption 42";
    df.end_indent();
    df.Out() << "which would set an option named \"IntegerOption\" to the value 42.  Options that are sets of switches can be set using either ";
    df.begin_indent();
    df.Out() << "# SwitchOption + switch1 switch2 switch3";
    df.end_indent();
    df.Out() << "to turn on the given switches, or ";
    df.begin_indent();
    df.Out() << "# SwitchOption - switch1 switch2";
    df.end_indent();
    df.Out() << "to turn off the given switches.  Finally, if one option selection has its own options, those may be set using braces, for example: ";
    df.begin_indent();
    df.Out() << "# SelectAlgorithm SUPER_FANCY {\n#~~~~MagicNumber 42\n#~~~~UseAwesomeness true\n# }";
    df.end_indent();
    df.Out() << "The online help can be used to display details about available options.   The following top-level options are available (shown with their current settings):\n\n";
    option_manager::global().ListOptions(df);
}

// ******************************************************************
// *                      topic_unaryop  class                      *
// ******************************************************************

class topic_unaryop : public help_topic {
        unary_op::opcode op;
    public:
        topic_unaryop(unary_op::opcode u);
        virtual void DocumentBehavior(doc_formatter &df) const;
    private:
        class ontype : public shared_visitor {
                unary_op::opcode op;
                doc_formatter &df;
                unsigned width;
                bool firstpass;
            public:
                ontype(doc_formatter &d, unary_op::opcode o);
                virtual void visit(const shared_object* obj);

                inline unsigned nextPass() {
                    firstpass = false;
                    return width;
                }
        };
};

// ******************************************************************

topic_unaryop::ontype::ontype(doc_formatter &d, unary_op::opcode o)
    : df(d)
{
    op = o;
    firstpass = true;
    width = 0;
}

void topic_unaryop::ontype::visit(const shared_object* obj)
{
    const type* t = dynamic_cast <const type*> (obj);
    if (!t) return;
    const type* u = unary_op::getTypeOf(op, t);
    if (!u) return;
    if (firstpass) {
        width = MAX(width, t->length()+2);
    } else {
        std::stringstream foo;
        foo << unary_op::getOp(op) << " " << *t;
        df.item(foo.str().c_str());
        df.Out() << "has type: " << *u << "\n";
    }
}

// ******************************************************************

topic_unaryop::topic_unaryop(unary_op::opcode u)
 : help_topic()
{
    op = u;
    std::stringstream foo;
    foo << "unary " << unary_op::getOp(op);
    setName(foo.str());
    setSummary(unary_op::documentOp(op));
}


void topic_unaryop::DocumentBehavior(doc_formatter &df) const
{
    df.Out() << "Operator " << unary_op::getOp(op) << " is used for ";
    df.Out() << unary_op::documentOp(op);
    df.Out() << ".  It may be used on the following types of expressions:\n\n";

    ontype V(df, op);
    type::traverseRegistry(V);
    const unsigned w = V.nextPass();
    df.begin_description(4*(w/4+1));
    type::traverseRegistry(V);
    df.end_description();
}

// ******************************************************************
// *                      topic_binaryop class                      *
// ******************************************************************

class topic_binaryop : public help_topic {
        binary_op::opcode op;
    public:
        topic_binaryop(binary_op::opcode b);
        virtual void DocumentBehavior(doc_formatter &df) const;

    private:
        class pvisit : public enumerate_pairs::visitor {
                binary_op::opcode op;
            public:
                pvisit(binary_op::opcode op);
                virtual bool check(const type* t1, const type* t2) const;
                virtual void header(doc_formatter &df, const type* t1) const;
                virtual void display(doc_formatter &df, const type* t1,
                        const type* t2) const;
        };
};

// ************************************************************

topic_binaryop::pvisit::pvisit(binary_op::opcode _op)
{
    op = _op;
}

bool topic_binaryop::pvisit::check(const type* t1, const type* t2) const
{
    return binary_op::getTypeOf(t1, op, t2);
}

void topic_binaryop::pvisit::header(doc_formatter &df, const type* t1) const
{
    df.Out() << '\n' << *t1 << ' ' << binary_op::getOp(op) << '\n';
}

void topic_binaryop::pvisit::display(doc_formatter &df, const type* t1,
        const type* t2) const
{
    df.item(t2->getStr());
    const type* v = binary_op::getTypeOf(t1, op, t2);
    df.Out() << "has type: " << *v << '\n';
}

// ******************************************************************

topic_binaryop::topic_binaryop(binary_op::opcode b) : help_topic()
{
    op = b;
    std::stringstream foo;
    foo << "binary " << binary_op::getOp(op);
    setName(foo.str());
    setSummary(binary_op::documentOp(op));
}

void topic_binaryop::DocumentBehavior(doc_formatter &df) const
{
    df.Out() << "Operator " << binary_op::getOp(op) << " is used for ";
    df.Out() << binary_op::documentOp(op);
    df.Out() << ".  It may be used with the following operand types:\n";

    pvisit pv(op);
    enumerate_pairs EP(df, pv);
    type::traverseRegistry(EP);
}

// ******************************************************************
// *                     topic_trinaryop  class                     *
// ******************************************************************

class topic_trinaryop : public help_topic {
        trinary_op::opcode op;
    public:
        topic_trinaryop(trinary_op::opcode b);
        virtual void DocumentBehavior(doc_formatter &df) const;

    private:
        class op_third : public shared_visitor {
                doc_formatter &df;
                const type* first;
                const type* second;
                trinary_op::opcode op;
                bool printed;
            public:
                op_third(doc_formatter &d, trinary_op::opcode op);
                virtual void visit(const shared_object* obj);
                inline void set_first(const type* F) {
                    first = F;
                    printed = false;
                }
                inline void set_second(const type* S) {
                    second = S;
                }
                inline bool notempty() const { return printed; }
        };

    private:
        class op_second : public shared_visitor {
                op_third &inner;
            public:
                op_second(op_third &i);
                virtual void visit(const shared_object* obj);
                inline virtual void set_first(const type* F) {
                    inner.set_first(F);
                }
                inline bool notempty() const { return inner.notempty(); }
        };

    private:
        class op_first : public shared_visitor {
                doc_formatter &df;
                op_second &inner;
            public:
                op_first(doc_formatter &d, op_second &i);
                virtual void visit(const shared_object* obj);
        };
};

// ******************************************************************

topic_trinaryop::op_third::op_third(doc_formatter &d, trinary_op::opcode o)
    : df(d)
{
    first = nullptr;
    second = nullptr;
    op = o;
}

void topic_trinaryop::op_third::visit(const shared_object* obj)
{
    if (!first) return;
    if (!second) return;
    const type* third = dynamic_cast <const type*> (obj);
    if (!third) return;

    const type* w = trinary_op::getTypeOf(op, first, second, third);
    if (!w) return;

    std::stringstream foo;
    foo << *first << ' ' << trinary_op::getFirst(op) << ' ';
    foo << *second << ' ' << trinary_op::getSecond(op) << ' ' << *third;
    df.item(foo.str().c_str());
    df.Out() << "has type: " << *w << '\n';

    printed = true;
}

// ******************************************************************

topic_trinaryop::op_second::op_second(op_third &i) : inner(i)
{
}

void topic_trinaryop::op_second::visit(const shared_object* obj)
{
    const type* second = dynamic_cast <const type*> (obj);
    if (!second) return;
    inner.set_second(second);
    type::traverseRegistry(inner, false);
}

// ******************************************************************

topic_trinaryop::op_first::op_first(doc_formatter &d, op_second &i)
    : df(d), inner(i)
{
}

void topic_trinaryop::op_first::visit(const shared_object* obj)
{
    const type* first = dynamic_cast <const type*> (obj);
    if (!first) return;

    df.begin_description(24);
    inner.set_first(first);
    type::traverseRegistry(inner, first);
    if (inner.notempty()) df.Out() << '\n';
    df.end_description();
}


// ******************************************************************

topic_trinaryop::topic_trinaryop(trinary_op::opcode b)
 : help_topic()
{
    op = b;
    std::stringstream foo;
    foo << "trinary " << trinary_op::getFirst(op) << " " << trinary_op::getSecond(op);
    setName(foo.str());
    setSummary(trinary_op::documentOp(op));
}

void topic_trinaryop::DocumentBehavior(doc_formatter &df) const
{
    df.Out() << "Operator " << trinary_op::getFirst(op) << " ";
    df.Out() << trinary_op::getSecond(op) << " is used for ";
    df.Out() << trinary_op::documentOp(op);
    df.Out() << ".  It may be used with the following operand types:\n\n";

    op_third inner(df, op);
    op_second middle(inner);
    op_first outer(df, middle);
    type::traverseRegistry(outer, false);
}

// ******************************************************************
// *                      topic_assocop  class                      *
// ******************************************************************

class topic_assocop : public help_topic {
        bool flipped;
        assoc_op::opcode op;
    public:
        topic_assocop(bool f, assoc_op::opcode b);
        virtual void DocumentBehavior(doc_formatter &df) const;

    private:
        class pvisit : public enumerate_pairs::visitor {
                bool flipped;
                assoc_op::opcode op;
            public:
                pvisit(bool f, assoc_op::opcode op);
                virtual bool check(const type* t1, const type* t2) const;
                virtual void header(doc_formatter &df, const type* t1) const;
                virtual void display(doc_formatter &df, const type* t1,
                        const type* t2) const;
        };
};

// ************************************************************

topic_assocop::pvisit::pvisit(bool f, assoc_op::opcode _op)
{
    flipped = f;
    op = _op;
}

bool topic_assocop::pvisit::check(const type* t1, const type* t2) const
{
    return assoc_op::getTypeOf(t1, flipped, op, t2);
}

void topic_assocop::pvisit::header(doc_formatter &df, const type* t1) const
{
    df.Out() << '\n' << *t1 << ' ' << assoc_op::getOp(flipped, op) << '\n';
}

void topic_assocop::pvisit::display(doc_formatter &df, const type* t1,
        const type* t2) const
{
    df.item(t2->getStr());
    const type* v = assoc_op::getTypeOf(t1, flipped, op, t2);
    df.Out() << "has type: " << *v << '\n';
}

// ******************************************************************

topic_assocop::topic_assocop(bool f, assoc_op::opcode b)
 : help_topic()
{
    op = b;
    flipped = f;
    std::stringstream foo;
    foo << "binary " << assoc_op::getOp(flipped, op);
    setName(foo.str());
    setSummary(assoc_op::documentOp(flipped, op));
}

void topic_assocop::DocumentBehavior(doc_formatter &df) const
{
    df.Out() << "Operator " << assoc_op::getOp(flipped, op) << " is used for ";
    df.Out() << assoc_op::documentOp(flipped, op);
    df.Out() << ".  It may be used with the following operand types:\n";

    pvisit pv(flipped, op);
    enumerate_pairs EP(df, pv);
    type::traverseRegistry(EP);
}

// ******************************************************************
// *                       topic_models class                       *
// ******************************************************************

class topic_models : public help_topic {
    public:
        topic_models();
        virtual void DocumentBehavior(doc_formatter &df) const;
    private:
        class print_models : public shared_visitor {
                std::ostream &out;
                bool printed;
            public:
                print_models(std::ostream &o);
                virtual void visit(const shared_object* obj);
        };
};

// ******************************************************************

topic_models::print_models::print_models(std::ostream &o) : out(o)
{
    printed = false;
}

void topic_models::print_models::visit(const shared_object* obj)
{
    const type* t = dynamic_cast <const type*> (obj);
    if (!t) return;
    if (!t->isAFormalism()) return;
    if (printed) out << ", ";
    out << *t;
    printed = true;
}

// ******************************************************************

topic_models::topic_models() : help_topic("models", "Overview of models")
{
}

void topic_models::DocumentBehavior(doc_formatter &df) const
{
    df.Out() << "A model is declared with a header that is similar to a function declaration, of the form\n\n";
    df.begin_indent();
    df.Out() << "formalism identifier(params) := { ... }\n\n";
    df.end_indent();
    df.Out() << "where \"formalism\" is one of the formalism types:\n\n";
    df.begin_indent();
    print_models P(df.Out());
    type::traverseRegistry(P, false);
    df.Out() << ".\n\n";
    df.end_indent();
    df.Out() << "The statements within the braces specify how to build the model when necessary, and may include declarations and function calls specific to the formalism type.  Additionally, there may be statements that define the measures for the model, which are visible outside the model.  Note that a model call has the form\n\n";
    df.begin_indent();
    df.Out() << "model_identifier(params).measure;\n\n";
    df.end_indent();
    df.Out() << "and that a model is not instantiated until needed to compute a measure.  Measures may be classified by solution engine as appropriate, and calling a single measure may cause several measures to be computed (e.g., all steady-state measures will be computed together).  It is possible to declare arrays of measures, and measures of type void which execute the specified instructions whenever they are called.\n";
    df.Out() << "\nSee the help topics for a particular formalism for more information about what may be declared within a model, and functions available to build the model.\n";
    df.Out() <<"\nThere is also a special type named \"model\", which may be assigned to any constructed model.  For example:\n\n";
    df.begin_indent();
    df.Out() << "model m := model_identifier(params);\n";
    df.Out() << "m.any_measure_defined_in_model_identifier;\n\n";
    df.end_indent();
    df.Out() << "It is possible to declare arrays of type \"model\".\n";
}


// ******************************************************************
// *                                                                *
// *                                                                *
// *                         Initialization                         *
// *                                                                *
// *                                                                *
// ******************************************************************

class init_helpfuncs : public initializer {
    public:
        init_helpfuncs();
    protected:
        virtual void execute();
};
static init_helpfuncs the_helpfunc_initializer;

init_helpfuncs::init_helpfuncs() : initializer(__FILE__, 1)
{
    builds_resource("helpfuncs");
    // TBD
}

void init_helpfuncs::execute()
{
    symbol_table::addGlobal(new help_group(
        "#include",
        "Preprocessor directive to include files",
        "A source file can include other source files using the #include preprocessing directive, as in C.  An #include directive is ignored if it causes a circular dependency."
    ));

    symbol_table::addGlobal(new help_group(
        "comments",
        "Preprocessor rules for comments",
        "Source files can contain C and C++ style comments, which are stripped by the lexer.  The rules are:\n  (1) Characters on a line following \"//\" are ignored.\n  (2) Characters between \"/*\" and \"*/\" are ignored."
    ));

    symbol_table::addGlobal(new help_group(
        "functions",
        "Function call rules",
        "Built-in and user functions can be called using the usual, C-style syntax.  When functions are overloaded, Smart determines which function to call by summing the promotion distance (see the help topic on promotions) from the passed parameter to the formal parameter. Smart will also promote a function if necessary, by adding the modifier rand and/or proc to every formal parameter and to the return type of the function.  For instance, the definition:\n \t rand real mydist := sqrt(uniform(0, 1));\n is legal because the function\n \t real sqrt(real x)\n is automatically promoted to the form\n \t rand real sqrt(rand real x)."
    ));

    symbol_table::addGlobal(  new topic_topics(symbol_table::global())      );
    symbol_table::addGlobal(  new topic_types                               );
    symbol_table::addGlobal(  new topic_promotions                          );
    symbol_table::addGlobal(  new topic_casting                             );
    symbol_table::addGlobal(  new topic_operators                           );
    symbol_table::addGlobal(  new topic_options                             );

    symbol_table::addGlobal(  new topic_unaryop(unary_op::uop_not)          );
    symbol_table::addGlobal(  new topic_unaryop(unary_op::uop_neg)          );

    symbol_table::addGlobal(  new topic_binaryop(binary_op::bop_implies)    );
    symbol_table::addGlobal(  new topic_binaryop(binary_op::bop_mod)        );
    symbol_table::addGlobal(  new topic_binaryop(binary_op::bop_diff)       );
    symbol_table::addGlobal(  new topic_binaryop(binary_op::bop_equals)     );
    symbol_table::addGlobal(  new topic_binaryop(binary_op::bop_nequal)     );
    symbol_table::addGlobal(  new topic_binaryop(binary_op::bop_gt)         );
    symbol_table::addGlobal(  new topic_binaryop(binary_op::bop_ge)         );
    symbol_table::addGlobal(  new topic_binaryop(binary_op::bop_lt)         );
    symbol_table::addGlobal(  new topic_binaryop(binary_op::bop_le)         );

    symbol_table::addGlobal(  new topic_trinaryop(trinary_op::top_interval) );

    symbol_table::addGlobal(  new topic_assocop(false, assoc_op::aop_and)   );
    symbol_table::addGlobal(  new topic_assocop(false, assoc_op::aop_or)    );
    symbol_table::addGlobal(  new topic_assocop(false, assoc_op::aop_plus)  );
    symbol_table::addGlobal(  new topic_assocop(true , assoc_op::aop_plus)  );
    symbol_table::addGlobal(  new topic_assocop(false, assoc_op::aop_times) );
    symbol_table::addGlobal(  new topic_assocop(true , assoc_op::aop_times) );

    symbol_table::addGlobal(  new topic_assocop(false, assoc_op::aop_semi)  );
    symbol_table::addGlobal(  new topic_assocop(false, assoc_op::aop_union) );

    symbol_table::addGlobal(  new topic_models                              );
}


