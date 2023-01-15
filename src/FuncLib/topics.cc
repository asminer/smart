
#include "topics.h"

#include "../include/defines.h"
#include "../include/heap.h"

#include "../Utils/textfmt.h"
#include "../Utils/initializer.h"

#include "../ExprLib/help.h"
#include "../ExprLib/formalism.h"
#include "../ExprLib/functions.h"
#include "../ExprLib/symb_tab.h"
#include "../ExprLib/casting.h"
#include "../ExprLib/binary.h"
#include "../ExprLib/trinary.h"
#include "../ExprLib/assoc.h"

#include "../Options/optman.h"

// ******************************************************************
// *                       topic_topics class                       *
// ******************************************************************

class topic_topics : public help_topic {
    const symbol_table &st;
public:
    topic_topics(const symbol_table &s);
    virtual void PrintDocs(doc_formatter &df, const char*) const;

    class visitor : public shared_visitor {
            doc_formatter &df;
            unsigned maxname;
            bool firstpass;
        public:
            visitor(doc_formatter &d);
            virtual void visit(shared_object* item);
            inline unsigned getMaxName() const { return maxname; }
            inline void secondPass() { firstpass = false; }
    };
};

topic_topics::visitor::visitor(doc_formatter &d) : df(d)
{
    maxname = 0;
    firstpass = true;
}

void topic_topics::visitor::visit(shared_object* item)
{
    const symbol* chain = dynamic_cast <symbol*> (item);
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

topic_topics::topic_topics(const symbol_table &s)
    : help_topic("topics", "Shows all available help topics (this list!)"),
      st(s)
{
}

void topic_topics::PrintDocs(doc_formatter &df, const char*) const
{
  df.begin_heading();
  PrintHeader(df.Out());
  df.end_heading();

  visitor V(df);
  // First pass: determine longest topic name
  st.traverse(V);

  df.begin_indent();
  df.Out() << "The following help topics are available:\n\n";
  df.begin_description(V.getMaxName());

  // Second pass: display topics
  V.secondPass();
  st.traverse(V);

  df.end_description();
  df.end_indent();
}

// ******************************************************************
// *                       topic_types  class                       *
// ******************************************************************

class topic_types : public help_topic {
public:
  topic_types()
   : help_topic("types", "Shows the available types for declared objects") { }
  virtual void PrintDocs(doc_formatter &df, const char*) const;
};

void topic_types::PrintDocs(doc_formatter &df, const char*) const
{
  df.begin_heading();
  PrintHeader(df.Out());
  df.end_heading();
  df.begin_indent();
  df.Out() << "The Smart language is strictly typed; all objects have a specified type. Basic types can be further modified by *natures*, which specify if the object is deterministic or random. Furthermore, objects may be allowed to depend on the state of a stochastic process, which are again modified by the keyword *proc*. Types are also used for formalisms, formalism variables, and sets of objects.\n\n";
  df.Out() << "Simple types:\n";
  df.begin_indent();
  for (unsigned i=0; i<type::numRegistered(); i++) {
    const type* t = type::getRegistered(i);
    DCASSERT(t);
    if (t->isVoid())                continue;
    if (t->isAFormalism())          continue;
    if (t->isASet())                continue;
    if (t->getModifier() != DETERM) continue;
    if (t->hasProc())               continue;
    df.Out() << *t << "\n";
  }
  df.end_indent();
  df.Out() << "\nStochastic types:\n";
  df.begin_indent();
  for (unsigned i=0; i<type::numRegistered(); i++) {
    const type* t = type::getRegistered(i);
    if (t->getModifier() == DETERM) continue;
    if (t->hasProc())               continue;
    df.Out() << *t << "\n";
  }
  df.end_indent();
  df.Out() << "\nProcess types:\n";
  df.begin_indent();
  for (unsigned i=0; i<type::numRegistered(); i++) {
    const type* t = type::getRegistered(i);
    if (!t->hasProc())    continue;
    df.Out() << *t << "\n";
  }
  df.end_indent();
  df.Out() << "\nFormalism types:\n";
  df.begin_indent();
  for (unsigned i=0; i<type::numRegistered(); i++) {
    const type* t = type::getRegistered(i);
    if (!t->isAFormalism())    continue;
    df.Out() << *t << "\n";
  }
  df.end_indent();
  df.Out() << "\nVoid types (usually within formalisms):\n";
  df.begin_indent();
  for (unsigned i=0; i<type::numRegistered(); i++) {
    const type* t = type::getRegistered(i);
    if (!t->isVoid())    continue;
    df.Out() << *t << "\n";
  }
  df.end_indent();
  df.Out() << "\nSet types:\n";
  df.begin_indent();
  for (unsigned i=0; i<type::numRegistered(); i++) {
    const type* t = type::getRegistered(i);
    if (!t->isASet())    continue;
    df.Out() << *t << "\n";
  }
  df.end_indent();
  df.Out() << "\nSee the help topics \"promotions\" and \"casting\" for details about how Smart changes types, and how you can force a type change.\n";
  df.end_indent();
}

// ******************************************************************
// *                     topic_promotions class                     *
// ******************************************************************

class topic_promotions : public help_topic {

  class one_promotion {
    int typeno;
    int distance;
  public:
    one_promotion() { typeno = 0; distance = 0; }
    inline void set(int a, int b) { typeno = a; distance = b; }
    inline bool operator <= (const one_promotion& b) {
      if (distance < b.distance) return true;
      if (distance > b.distance) return false;
      return typeno <= b.typeno;
    }
    inline bool operator >= (const one_promotion& b) {
      if (distance < b.distance) return false;
      if (distance > b.distance) return true;
      return typeno >= b.typeno;
    }
    inline bool operator > (const one_promotion& b) {
      if (distance < b.distance) return false;
      if (distance > b.distance) return true;
      return typeno > b.typeno;
    }
    inline int getTypeNo() const { return typeno; }
    inline int getDistance() const { return distance; }
  };

public:
  topic_promotions()
   : help_topic("promotions", "Which types can be promoted to which other types") { }
  virtual void PrintDocs(doc_formatter &df, const char*) const;
};

void topic_promotions::PrintDocs(doc_formatter &df, const char*) const
{
  df.begin_heading();
  PrintHeader(df.Out());
  df.end_heading();
  df.begin_indent();

  df.Out() << "If necessary, Smart will attempt to promote expressions to other types.  Each promotion has an associated \"distance\", and Smart will normally choose the promotion with least distance (or give an error if it is unable to decide).  A type promotion can be forced using an explicit cast, see the help topic on \"casting\" for details.  Smart uses the following promotions:\n";

  one_promotion* parray = new one_promotion [type::numRegistered()];

  for (unsigned i=0; i<type::numRegistered(); i++) {
    const type* from = type::getRegistered(i);
    DCASSERT(from);

    // build sorted list of promotions
    int plength = 0;
    for (unsigned j=0; j<type::numRegistered(); j++) {
      const type* to = type::getRegistered(j);
      int d = typeconv::getPromoteDistance(from, to);
      if (d <= 0) continue;
      parray[plength].set(j, d);
      plength++;
    } // for j

    if (0==plength) continue;  // no promotions from here

    HeapSort(parray, plength);
    df.Out() << "\n" << *from << ":\n";
    df.begin_indent();
    df.begin_description(15);

    for (int j=0; j<plength; j++) {
      const type* to = type::getRegistered(parray[j].getTypeNo());
      DCASSERT(to);
      df.item(to->getStr());
      df.Out() << "(distance " << parray[j].getDistance() << ")\n";
    }

    df.end_description();
    df.end_indent();
  } // for i

  df.end_indent();

  delete[] parray;
}


// ******************************************************************
// *                      topic_casting  class                      *
// ******************************************************************

class topic_casting : public help_topic {
public:
  topic_casting()
   : help_topic("casting", "Which types can be converted to which other types") { }
  virtual void PrintDocs(doc_formatter &df, const char*) const;
};

void topic_casting::PrintDocs(doc_formatter &df, const char*) const
{
  df.begin_heading();
  PrintHeader(df.Out());
  df.end_heading();
  df.begin_indent();

  df.Out() << "An expression can be cast to a different using new_type(expr). This can be done to force promotion of an expression, or to change its type. For example:\n\n";
  df.begin_indent();
  df.Out() << "rand real x := ...;\nrand int i := rand int(x);\n\n";
  df.end_indent();

  df.Out() << "An expression can be explicitly cast from type A to type B if it can be promoted from type A to type B (see help topic \"promotions\").  In addition, the following conversions are allowed:\n\n";

  df.begin_indent();
  for (unsigned i=0; i<type::numRegistered(); i++) {
    const type* from = type::getRegistered(i);
    DCASSERT(from);
    for (unsigned j=0; j<type::numRegistered(); j++) {
      const type* to = type::getRegistered(j);
      DCASSERT(to);
      if (typeconv::isPromotable(from, to))   continue;
      if (!typeconv::isCastable(from, to))    continue;

      df.Out() << "from " << *from;
      df.Out() << " to " << *to << "\n";
    } // for j
  } // for i
  df.end_indent();
  df.end_indent();
}


// ******************************************************************
// *                     topic_operators  class                     *
// ******************************************************************

class topic_operators : public help_topic {
public:
  topic_operators()
   : help_topic("operators", "Information about operators") { }
  virtual void PrintDocs(doc_formatter &df, const char*) const;
};

void topic_operators::PrintDocs(doc_formatter &df, const char*) const
{
  df.begin_heading();
  PrintHeader(df.Out());
  df.end_heading();
  df.begin_indent();

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

  df.end_indent();
}

// ******************************************************************
// *                      topic_options  class                      *
// ******************************************************************

class topic_options : public help_topic {
public:
  topic_options()
   : help_topic("options", "How to use options") { }
  virtual void PrintDocs(doc_formatter &df, const char*) const;
};

void topic_options::PrintDocs(doc_formatter &df, const char*) const
{
  df.begin_heading();
  PrintHeader(df.Out());
  df.end_heading();
  df.begin_indent();

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

  df.end_indent();
}

// ******************************************************************
// *                      topic_unaryop  class                      *
// ******************************************************************

class topic_unaryop : public help_topic {
  unary_op::opcode op;
public:
  topic_unaryop(unary_op::opcode u);
  virtual void PrintDocs(doc_formatter &df, const char*) const;
};

topic_unaryop::topic_unaryop(unary_op::opcode u)
 : help_topic()
{
  op = u;
  std::stringstream foo;
  foo << "unary " << unary_op::getOp(op);
  setName(foo.str());
  setSummary(unary_op::documentOp(op));
}


void topic_unaryop::PrintDocs(doc_formatter &df, const char*) const
{
  df.begin_heading();
  PrintHeader(df.Out());
  df.end_heading();
  df.begin_indent();

  df.Out() << "Operator " << unary_op::getOp(op) << " is used for ";
  df.Out() << unary_op::documentOp(op);
  df.Out() << ".  It may be used on the following types of expressions:\n";

  df.begin_description(18);
  for (unsigned i=0; i<type::numRegistered(); i++) {
    const type* t = type::getRegistered(i);
    DCASSERT(t);
    const type* u = unary_op::getTypeOf(op, t);
    if (0==u)  continue;
    std::stringstream foo;
    foo << unary_op::getOp(op) << " " << *t;
    df.item(foo.str().c_str());
    df.Out() << "has type " << *u << "\n";
    foo.str("");
  }
  df.end_description();
  df.end_indent();
}

// ******************************************************************
// *                      topic_binaryop class                      *
// ******************************************************************

class topic_binaryop : public help_topic {
  binary_op::opcode op;
public:
  topic_binaryop(binary_op::opcode b);
  virtual void PrintDocs(doc_formatter &df, const char*) const;
};

topic_binaryop::topic_binaryop(binary_op::opcode b)
 : help_topic()
{
  op = b;
  std::stringstream foo;
  foo << "binary " << binary_op::getOp(op);
  setName(foo.str());
  setSummary(binary_op::documentOp(op));
}

void topic_binaryop::PrintDocs(doc_formatter &df, const char*) const
{
  df.begin_heading();
  PrintHeader(df.Out());
  df.end_heading();
  df.begin_indent();

  df.Out() << "Operator " << binary_op::getOp(op) << " is used for ";
  df.Out() << binary_op::documentOp(op);
  df.Out() << ".  It may be used on the following types of expressions:\n";

  df.begin_description(35);
  for (unsigned i=0; i<type::numRegistered(); i++) {
    const type* t = type::getRegistered(i);
    DCASSERT(t);
    for (unsigned j=0; j<type::numRegistered(); j++) {
      const type* u = type::getRegistered(j);
      DCASSERT(u);
      const type* v = binary_op::getTypeOf(t, op, u);
      if (0==v)  continue;
      std::stringstream foo;
      foo << *t << " ";
      foo << binary_op::getOp(op) << " " << *u;
      df.item(foo.str().c_str());
      df.Out() << "has type " << *v << "\n";
      foo.str("");
    }
  }
  df.end_description();
  df.end_indent();
}

// ******************************************************************
// *                     topic_trinaryop  class                     *
// ******************************************************************

class topic_trinaryop : public help_topic {
  trinary_op::opcode op;
public:
  topic_trinaryop(trinary_op::opcode b);
  virtual void PrintDocs(doc_formatter &df, const char*) const;
};

topic_trinaryop::topic_trinaryop(trinary_op::opcode b)
 : help_topic()
{
  op = b;
  std::stringstream foo;
  foo << "trinary " << trinary_op::getFirst(op) << " " << trinary_op::getSecond(op);
  setName(foo.str());
  setSummary(trinary_op::documentOp(op));
}

void topic_trinaryop::PrintDocs(doc_formatter &df, const char*) const
{
  df.begin_heading();
  PrintHeader(df.Out());
  df.end_heading();
  df.begin_indent();

  df.Out() << "Operator " << trinary_op::getFirst(op) << " ";
  df.Out() << trinary_op::getSecond(op) << " is used for ";
  df.Out() << trinary_op::documentOp(op);
  df.Out() << ".  It may be used on the following types of expressions:\n";

  df.begin_description(35);
  for (unsigned i=0; i<type::numRegistered(); i++) {
    const type* t = type::getRegistered(i);
    DCASSERT(t);
    for (unsigned j=0; j<type::numRegistered(); j++) {
      const type* u = type::getRegistered(j);
      DCASSERT(u);
      for (unsigned k=0; k<type::numRegistered(); k++) {
        const type* v = type::getRegistered(k);
        DCASSERT(v);
        const type* w = trinary_op::getTypeOf(op, t, u, v);
        if (0==w)  continue;
        std::stringstream foo;
        foo << *t << " " << trinary_op::getFirst(op) << " ";
        foo << *u << " " << trinary_op::getSecond(op) << " ";
        foo << *v;
        df.item(foo.str().c_str());
        df.Out() << "has type " << *w << "\n";
        foo.str("");
      } // for k
    } // for j
  } // for i
  df.end_description();
  df.end_indent();
}

// ******************************************************************
// *                      topic_assocop  class                      *
// ******************************************************************

class topic_assocop : public help_topic {
  bool flipped;
  assoc_op::opcode op;
public:
  topic_assocop(bool f, assoc_op::opcode b);
  virtual void PrintDocs(doc_formatter &df, const char*) const;
};

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

void topic_assocop::PrintDocs(doc_formatter &df, const char*) const
{
  df.begin_heading();
  PrintHeader(df.Out());
  df.end_heading();
  df.begin_indent();

  df.Out() << "Operator " << assoc_op::getOp(flipped, op) << " is used for ";
  df.Out() << assoc_op::documentOp(flipped, op);
  df.Out() << ".  It may be used on the following types of expressions:\n";

  df.begin_description(35);
  for (unsigned i=0; i<type::numRegistered(); i++) {
    const type* t = type::getRegistered(i);
    DCASSERT(t);
    for (unsigned j=0; j<type::numRegistered(); j++) {
      const type* u = type::getRegistered(j);
      DCASSERT(u);
      const type* v = assoc_op::getTypeOf(t, flipped, op, u);
      if (0==v)  continue;
      std::stringstream foo;
      foo << *t << " ";
      foo << assoc_op::getOp(flipped, op) << " " << *u;
      df.item(foo.str().c_str());
      df.Out() << "has type " << *v << "\n";
      foo.str("");
    }
  }
  df.end_description();
  df.end_indent();
}

// ******************************************************************
// *                       topic_models class                       *
// ******************************************************************

class topic_models : public help_topic {
public:
  topic_models()
   : help_topic("models", "Overview of models") { }
  virtual void PrintDocs(doc_formatter &df, const char*) const;
};

void topic_models::PrintDocs(doc_formatter &df, const char*) const
{
  df.begin_heading();
  PrintHeader(df.Out());
  df.end_heading();
  df.begin_indent();
  df.Out() << "A model is declared with a header that is similar to a function declaration, of the form\n\n";
  df.begin_indent();
  df.Out() << "formalism identifier(params) := { ... }\n\n";
  df.end_indent();
  df.Out() << "where \"formalism\" is one of the formalism types:\n";
  df.begin_indent();
  bool printed = false;
  for (unsigned i=0; i<type::numRegistered(); i++) {
    const type* t = type::getRegistered(i);
    if (!t->isAFormalism())    continue;
    if (printed) df.Out() << ", ";
    df.Out() << *t;
    printed = true;
  }
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
  df.end_indent();
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


