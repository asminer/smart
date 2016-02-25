
// $Id$

#include "topics.h"

#include "../include/defines.h"
#include "../ExprLib/startup.h"
#include "../ExprLib/exprman.h"
#include "../Options/options.h"
#include "../include/heap.h"
#include "../ExprLib/symbols.h"
#include "../SymTabs/symtabs.h"
#include "../Streams/streams.h"
#include "../ExprLib/formalism.h"
#include "../ExprLib/functions.h"

// ******************************************************************
// *                       topic_topics class                       *
// ******************************************************************

class topic_topics : public help_topic {
  const symbol_table* st;
public:
  topic_topics(const symbol_table* s)
   : help_topic("topics", "Shows all available help topics (this list!)") { st = s; }
  virtual void PrintDocs(doc_formatter* df, const char*) const;
};

void topic_topics::PrintDocs(doc_formatter* df, const char*) const
{
  df->begin_heading();
  PrintHeader(df->Out());
  df->end_heading();

  long num_names = st->NumNames();
  const symbol** list = new const symbol*[num_names];
  st->CopyToArray(list);
  
  // get the longest topic name
  int maxname = 0;
  for (long i=0; i<num_names; i++) {
    const symbol* chain = list[i];
    for (; chain; chain = chain->Next()) {
      const help_topic* ht = dynamic_cast <const help_topic*> (chain);
      if (0==ht) continue;
      int len = strlen(ht->Name());
      maxname = MAX(maxname, len);
    }
  }

  df->begin_indent();
  df->Out() << "The following help topics are available:\n\n";
  df->begin_description(maxname);

  for (long i=0; i<num_names; i++) {
    const symbol* chain = list[i];
    for (; chain; chain = chain->Next()) {
      const help_topic* ht = dynamic_cast <const help_topic*> (chain);
      if (0==ht) continue;
      df->item(ht->Name());
      df->Out() << ht->Summary();
    }
  }
  df->end_description();
  df->end_indent();

  delete[] list;
}

// ******************************************************************
// *                       topic_types  class                       *
// ******************************************************************

class topic_types : public help_topic {
public:
  topic_types()
   : help_topic("types", "Shows the available types for declared objects") { }
  virtual void PrintDocs(doc_formatter* df, const char*) const;
};

void topic_types::PrintDocs(doc_formatter* df, const char*) const
{
  df->begin_heading();
  PrintHeader(df->Out());
  df->end_heading();
  df->begin_indent();
  df->Out() << "The Smart language is strictly typed; all objects have a specified type. Basic types can be further modified by *natures*, which specify if the object is deterministic or random. Furthermore, objects may be allowed to depend on the state of a stochastic process, which are again modified by the keyword *proc*. Types are also used for formalisms, formalism variables, and sets of objects.\n\n";
  df->Out() << "Simple types:\n";
  df->begin_indent();
  for (int i=0; i<em->getNumTypes(); i++) {
    const type* t = em->getTypeNumber(i);
    DCASSERT(t);
    if (t->isVoid())                continue;
    if (t->isAFormalism())          continue;
    if (t->isASet())                continue;
    if (t->getModifier() != DETERM) continue;
    if (t->hasProc())               continue;
    df->Out() << t->getName() << "\n";
  }
  df->end_indent();
  df->Out() << "\nStochastic types:\n";
  df->begin_indent();
  for (int i=0; i<em->getNumTypes(); i++) {
    const type* t = em->getTypeNumber(i);
    if (t->getModifier() == DETERM) continue;
    if (t->hasProc())               continue;
    df->Out() << t->getName() << "\n";
  }
  df->end_indent();
  df->Out() << "\nProcess types:\n";
  df->begin_indent();
  for (int i=0; i<em->getNumTypes(); i++) {
    const type* t = em->getTypeNumber(i);
    if (!t->hasProc())    continue;
    df->Out() << t->getName() << "\n";
  }
  df->end_indent();
  df->Out() << "\nFormalism types:\n";
  df->begin_indent();
  for (int i=0; i<em->getNumTypes(); i++) {
    const type* t = em->getTypeNumber(i);
    if (!t->isAFormalism())    continue;
    df->Out() << t->getName() << "\n";
  }
  df->end_indent();
  df->Out() << "\nVoid types (usually within formalisms):\n";
  df->begin_indent();
  for (int i=0; i<em->getNumTypes(); i++) {
    const type* t = em->getTypeNumber(i);
    if (!t->isVoid())    continue;
    df->Out() << t->getName() << "\n";
  }
  df->end_indent();
  df->Out() << "\nSet types:\n";
  df->begin_indent();
  for (int i=0; i<em->getNumTypes(); i++) {
    const type* t = em->getTypeNumber(i);
    if (!t->isASet())    continue;
    df->Out() << t->getName() << "\n";
  }
  df->end_indent();
  df->Out() << "\nSee the help topics \"promotions\" and \"casting\" for details about how Smart changes types, and how you can force a type change.\n";
  df->end_indent();
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
  virtual void PrintDocs(doc_formatter* df, const char*) const;
};

void topic_promotions::PrintDocs(doc_formatter* df, const char*) const
{
  df->begin_heading();
  PrintHeader(df->Out());
  df->end_heading();
  df->begin_indent();

  df->Out() << "If necessary, Smart will attempt to promote expressions to other types.  Each promotion has an associated \"distance\", and Smart will normally choose the promotion with least distance (or give an error if it is unable to decide).  A type promotion can be forced using an explicit cast, see the help topic on \"casting\" for details.  Smart uses the following promotions:\n";

  one_promotion* parray = new one_promotion [em->getNumTypes()];

  for (int i=0; i<em->getNumTypes(); i++) {
    const type* from = em->getTypeNumber(i);
    DCASSERT(from);

    // build sorted list of promotions
    int plength = 0;
    for (int j=0; j<em->getNumTypes(); j++) {
      const type* to = em->getTypeNumber(j);
      int d = em->getPromoteDistance(from, to);
      if (d <= 0) continue;
      parray[plength].set(j, d);
      plength++;
    } // for j

    if (0==plength) continue;  // no promotions from here

    HeapSort(parray, plength);
    df->Out() << "\n" << from->getName() << ":\n";
    df->begin_indent();
    df->begin_description(15);

    for (int j=0; j<plength; j++) {
      const type* to = em->getTypeNumber(parray[j].getTypeNo());
      DCASSERT(to);
      df->item(to->getName());
      df->Out() << "(distance " << parray[j].getDistance() << ")\n";
    }

    df->end_description();
    df->end_indent();
  } // for i

  df->end_indent();

  delete[] parray;
}


// ******************************************************************
// *                      topic_casting  class                      *
// ******************************************************************

class topic_casting : public help_topic {
public:
  topic_casting()
   : help_topic("casting", "Which types can be converted to which other types") { }
  virtual void PrintDocs(doc_formatter* df, const char*) const;
};

void topic_casting::PrintDocs(doc_formatter* df, const char*) const
{
  df->begin_heading();
  PrintHeader(df->Out());
  df->end_heading();
  df->begin_indent();

  df->Out() << "An expression can be cast to a different using new_type(expr). This can be done to force promotion of an expression, or to change its type. For example:\n\n";
  df->begin_indent();
  df->Out() << "rand real x := ...;\nrand int i := rand int(x);\n\n";
  df->end_indent();

  df->Out() << "An expression can be explicitly cast from type A to type B if it can be promoted from type A to type B (see help topic \"promotions\").  In addition, the following conversions are allowed:\n\n";

  df->begin_indent();
  for (int i=0; i<em->getNumTypes(); i++) {
    const type* from = em->getTypeNumber(i);
    DCASSERT(from);
    for (int j=0; j<em->getNumTypes(); j++) {
      const type* to = em->getTypeNumber(j);
      DCASSERT(to);
      if (em->isPromotable(from, to))   continue;
      if (!em->isCastable(from, to))    continue;

      df->Out() << "from " << from->getName();
      df->Out() << " to " << to->getName() << "\n";
    } // for j
  } // for i
  df->end_indent();
  df->end_indent();
}


// ******************************************************************
// *                     topic_operators  class                     *
// ******************************************************************

class topic_operators : public help_topic {
public:
  topic_operators()
   : help_topic("operators", "Information about operators") { }
  virtual void PrintDocs(doc_formatter* df, const char*) const;
};

void topic_operators::PrintDocs(doc_formatter* df, const char*) const
{
  df->begin_heading();
  PrintHeader(df->Out());
  df->end_heading();
  df->begin_indent();

  df->Out() << "The following unary operators may be used when constructing expressions in Smart:\n\n";

  df->begin_description(2);
  df->item(em->getOp(exprman::uop_not));
  df->Out() << em->documentOp(exprman::uop_not);
  df->item(em->getOp(exprman::uop_neg));
  df->Out() << em->documentOp(exprman::uop_neg);
  df->end_description();

  df->Out() << "\nThe following binary operators may be used when constructing expressions in Smart:\n\n";

  df->begin_description(2);
  df->item(em->getOp(0, exprman::aop_plus)); 
  df->Out() << em->documentOp(0, exprman::aop_plus);
  df->item(em->getOp(1, exprman::aop_plus));
  df->Out() << em->documentOp(1, exprman::aop_plus);
  df->item(em->getOp(0, exprman::aop_times));
  df->Out() << em->documentOp(0, exprman::aop_times);
  df->item(em->getOp(1, exprman::aop_times));
  df->Out() << em->documentOp(1, exprman::aop_times);
  df->item(em->getOp(exprman::bop_mod));
  df->Out() << em->documentOp(exprman::bop_mod);
  df->item(em->getOp(exprman::bop_diff));
  df->Out() << em->documentOp(exprman::bop_diff);
  df->end_description();
  df->Out() << "\n";
  df->begin_description(2);
  df->item(em->getOp(0, exprman::aop_or));
  df->Out() << em->documentOp(0, exprman::aop_or);
  df->item(em->getOp(0, exprman::aop_and));
  df->Out() << em->documentOp(0, exprman::aop_and);
  df->item(em->getOp(exprman::bop_implies));
  df->Out() << em->documentOp(exprman::bop_implies);
  df->end_description();
  df->Out() << "\n";
  df->begin_description(2);
  df->item(em->getOp(exprman::bop_equals));
  df->Out() << em->documentOp(exprman::bop_equals);
  df->item(em->getOp(exprman::bop_nequal));
  df->Out() << em->documentOp(exprman::bop_nequal);
  df->item(em->getOp(exprman::bop_gt));
  df->Out() << em->documentOp(exprman::bop_gt);
  df->item(em->getOp(exprman::bop_ge));
  df->Out() << em->documentOp(exprman::bop_ge);
  df->item(em->getOp(exprman::bop_lt));
  df->Out() << em->documentOp(exprman::bop_lt);
  df->item(em->getOp(exprman::bop_le));
  df->Out() << em->documentOp(exprman::bop_le);
  df->end_description();
  df->Out() << "\n";
  df->begin_description(2);
  df->item(em->getOp(0, exprman::aop_semi)); 
  df->Out() << em->documentOp(0, exprman::aop_semi);
  df->end_description();

  df->Out() << "\nSee the help topic for an operator name for details about that operator.\n\n";
  df->Out() << "Note that void expressions may be grouped with braces; for example, ";
  df->begin_indent();
  df->Out() << "{ void1; void2; void3; }";
  df->end_indent();
  df->Out() << "produces a new void expression as the sequence of expressions void1, void2, and void3.\n";

  df->end_indent();
}

// ******************************************************************
// *                      topic_options  class                      *
// ******************************************************************

class topic_options : public help_topic {
public:
  topic_options()
   : help_topic("options", "How to use options") { }
  virtual void PrintDocs(doc_formatter* df, const char*) const;
};

void topic_options::PrintDocs(doc_formatter* df, const char*) const
{
  df->begin_heading();
  PrintHeader(df->Out());
  df->end_heading();
  df->begin_indent();

  df->Out() << "An option statement is used to modify the behavior of Smart.  For example, there are options to control the solution algorithms (such as the precision or maximum number of iterations allowed) or the level of verbosity.  Option statements appear on lines beginning with \"#\" (except for the \"# include\" directive, which is handled by the preprocessor).  Different options have different types, and different sets of legal values.  Furthermore, some options are nested within other options.  The syntax of an option statement depends on the option type.  Generally, extra space in an option statement is fine, but newline characters are significant.  Basic options may be set using a statement of the form ";
  df->begin_indent();
  df->Out() << "# IntegerOption 42";
  df->end_indent();
  df->Out() << "which would set an option named \"IntegerOption\" to the value 42.  Options that are sets of switches can be set using either ";
  df->begin_indent();
  df->Out() << "# SwitchOption + switch1 switch2 switch3";
  df->end_indent();
  df->Out() << "to turn on the given switches, or ";
  df->begin_indent();
  df->Out() << "# SwitchOption - switch1 switch2";
  df->end_indent();
  df->Out() << "to turn off the given switches.  Finally, if one option selection has its own options, those may be set using braces, for example: ";
  df->begin_indent();
  df->Out() << "# SelectAlgorithm SUPER_FANCY {\n#~~~~MagicNumber 42\n#~~~~UseAwesomeness true\n# }";
  df->end_indent();
  df->Out() << "The online help can be used to display details about available options.   The following top-level options are available (shown with their current settings):\n\n";
  DCASSERT(em);
  DCASSERT(em->OptMan());
  em->OptMan()->ListOptions(df);

  df->end_indent();
}

// ******************************************************************
// *                      topic_unaryop  class                      *
// ******************************************************************

class topic_unaryop : public help_topic {
  exprman::unary_opcode op;
public:
  topic_unaryop(exprman::unary_opcode u);
  virtual void PrintDocs(doc_formatter* df, const char*) const;
};

topic_unaryop::topic_unaryop(exprman::unary_opcode u)
 : help_topic() 
{ 
  op = u; 
  StringStream foo;
  foo << "unary " << em->getOp(op);
  setName(foo.GetString());
  setSummary(em->documentOp(op));
}


void topic_unaryop::PrintDocs(doc_formatter* df, const char*) const
{
  df->begin_heading();
  PrintHeader(df->Out());
  df->end_heading();
  df->begin_indent();

  df->Out() << "Operator " << em->getOp(op) << " is used for ";
  df->Out() << em->documentOp(op);
  df->Out() << ".  It may be used on the following types of expressions:\n";

  df->begin_description(18);
  StringStream foo;
  for (int i=0; i<em->getNumTypes(); i++) {
    const type* t = em->getTypeNumber(i);
    DCASSERT(t);
    const type* u = em->getTypeOf(op, t);
    if (0==u)  continue;
    foo.flush();
    foo << em->getOp(op) << " " << t->getName();
    df->item(foo.ReadString());
    df->Out() << "has type " << u->getName() << "\n";
  }
  df->end_description();
  df->end_indent();
}

// ******************************************************************
// *                      topic_binaryop class                      *
// ******************************************************************

class topic_binaryop : public help_topic {
  exprman::binary_opcode op;
public:
  topic_binaryop(exprman::binary_opcode b);
  virtual void PrintDocs(doc_formatter* df, const char*) const;
};

topic_binaryop::topic_binaryop(exprman::binary_opcode b)
 : help_topic() 
{ 
  op = b; 
  StringStream foo;
  foo << "binary " << em->getOp(op);
  setName(foo.GetString());
  setSummary(em->documentOp(op));
}

void topic_binaryop::PrintDocs(doc_formatter* df, const char*) const
{
  df->begin_heading();
  PrintHeader(df->Out());
  df->end_heading();
  df->begin_indent();

  df->Out() << "Operator " << em->getOp(op) << " is used for ";
  df->Out() << em->documentOp(op);
  df->Out() << ".  It may be used on the following types of expressions:\n";

  df->begin_description(35);
  StringStream foo;
  for (int i=0; i<em->getNumTypes(); i++) {
    const type* t = em->getTypeNumber(i);
    DCASSERT(t);
    for (int j=0; j<em->getNumTypes(); j++) {
      const type* u = em->getTypeNumber(j);
      DCASSERT(u);
      const type* v = em->getTypeOf(t, op, u);
      if (0==v)  continue;
      foo.flush();
      foo << t->getName() << " ";
      foo << em->getOp(op) << " " << u->getName();
      df->item(foo.ReadString());
      df->Out() << "has type " << v->getName() << "\n";
    }
  }
  df->end_description();
  df->end_indent();
}

// ******************************************************************
// *                     topic_trinaryop  class                     *
// ******************************************************************

class topic_trinaryop : public help_topic {
  exprman::trinary_opcode op;
public:
  topic_trinaryop(exprman::trinary_opcode b);
  virtual void PrintDocs(doc_formatter* df, const char*) const;
};

topic_trinaryop::topic_trinaryop(exprman::trinary_opcode b)
 : help_topic() 
{ 
  op = b; 
  StringStream foo;
  foo << "trinary " << em->getFirst(op) << " " << em->getSecond(op);
  setName(foo.GetString());
  setSummary(em->documentOp(op));
}

void topic_trinaryop::PrintDocs(doc_formatter* df, const char*) const
{
  df->begin_heading();
  PrintHeader(df->Out());
  df->end_heading();
  df->begin_indent();

  df->Out() << "Operator " << em->getFirst(op) << " ";
  df->Out() << em->getSecond(op) << " is used for ";
  df->Out() << em->documentOp(op);
  df->Out() << ".  It may be used on the following types of expressions:\n";

  df->begin_description(35);
  StringStream foo;
  for (int i=0; i<em->getNumTypes(); i++) {
    const type* t = em->getTypeNumber(i);
    DCASSERT(t);
    for (int j=0; j<em->getNumTypes(); j++) {
      const type* u = em->getTypeNumber(j);
      DCASSERT(u);
      for (int k=0; k<em->getNumTypes(); k++) {
        const type* v = em->getTypeNumber(k);
        DCASSERT(v);
        const type* w = em->getTypeOf(op, t, u, v);
        if (0==w)  continue;
        foo.flush();
        foo << t->getName() << " " << em->getFirst(op) << " ";
        foo << u->getName() << " " << em->getSecond(op) << " ";
        foo << v->getName();
        df->item(foo.ReadString());
        df->Out() << "has type " << w->getName() << "\n";
      } // for k
    } // for j
  } // for i
  df->end_description();
  df->end_indent();
}

// ******************************************************************
// *                      topic_assocop  class                      *
// ******************************************************************

class topic_assocop : public help_topic {
  bool flipped;
  exprman::assoc_opcode op;
public:
  topic_assocop(bool f, exprman::assoc_opcode b);
  virtual void PrintDocs(doc_formatter* df, const char*) const;
};

topic_assocop::topic_assocop(bool f, exprman::assoc_opcode b)
 : help_topic() 
{ 
  op = b; 
  flipped = f;
  StringStream foo;
  foo << "binary " << em->getOp(flipped, op);
  setName(foo.GetString());
  setSummary(em->documentOp(flipped, op));
}

void topic_assocop::PrintDocs(doc_formatter* df, const char*) const
{
  df->begin_heading();
  PrintHeader(df->Out());
  df->end_heading();
  df->begin_indent();

  df->Out() << "Operator " << em->getOp(flipped, op) << " is used for ";
  df->Out() << em->documentOp(flipped, op);
  df->Out() << ".  It may be used on the following types of expressions:\n";

  df->begin_description(35);
  StringStream foo;
  for (int i=0; i<em->getNumTypes(); i++) {
    const type* t = em->getTypeNumber(i);
    DCASSERT(t);
    for (int j=0; j<em->getNumTypes(); j++) {
      const type* u = em->getTypeNumber(j);
      DCASSERT(u);
      const type* v = em->getTypeOf(t, flipped, op, u);
      if (0==v)  continue;
      foo.flush();
      foo << t->getName() << " ";
      foo << em->getOp(flipped, op) << " " << u->getName();
      df->item(foo.ReadString());
      df->Out() << "has type " << v->getName() << "\n";
    }
  }
  df->end_description();
  df->end_indent();
}

// ******************************************************************
// *                     topic_simpletype class                     *
// ******************************************************************

class topic_simpletype : public help_topic {
  const simple_type* st;
public:
  topic_simpletype(const simple_type* t);
  virtual void PrintDocs(doc_formatter* df, const char*) const;
};

topic_simpletype::topic_simpletype(const simple_type* t)
 : help_topic(t->getName(), t->shortDocs()) 
{ 
  st = t;
}

void topic_simpletype::PrintDocs(doc_formatter* df, const char*) const
{
  df->begin_heading();
  PrintHeader(df->Out());
  df->end_heading();
  df->begin_indent();
  df->Out() << st->longDocs();
  df->end_indent();
}


// ******************************************************************
// *                     topic_formalism  class                     *
// ******************************************************************

class topic_formalism : public help_topic {
  const formalism* ft;
public:
  topic_formalism(const formalism* t);
  virtual void PrintDocs(doc_formatter* df, const char*) const;
};

topic_formalism::topic_formalism(const formalism* t)
 : help_topic(t->getName(), t->shortDocs()) 
{ 
  ft = t;
}

void topic_formalism::PrintDocs(doc_formatter* df, const char*) const
{
  df->begin_heading();
  PrintHeader(df->Out());
  df->end_heading();
  df->begin_indent();
  df->Out() << ft->longDocs();
  df->Out() << "\n\nLegal variable types:";
  df->begin_indent();
  for (int i=0; i<em->getNumTypes(); i++) {
    const type* t = em->getTypeNumber(i);
    DCASSERT(t);
    if (ft->canDeclareType(t)) df->Out() << t->getName() << "\n";
  }
  df->end_indent();

  long num_names;
  // Print formalism identifiers, if any
  num_names = ft->numIdentNames();
  if (num_names) {
    df->Out() << "\nIdentifiers usable in this formalism:\n";
    const symbol** list = new const symbol*[num_names];
    ft->copyIdentsToArray(list);
    df->begin_indent();
    for (long i=0; i<num_names; i++) {
      DCASSERT(list[i]);
      list[i]->PrintType(df->Out());
      df->Out() << " " << list[i]->Name() << "\n";
    }
    df->end_indent();
    delete[] list;
  } // if num_names

  // Print formalism functions, if any 
  num_names = ft->numFuncNames();
  if (num_names) {
    df->Out() << "\nFunctions usable in this formalism:\n";

    const symbol** list = new const symbol*[num_names];
    ft->copyFuncsToArray(list);

    df->begin_indent();
    for (long i=0; i<num_names; i++) {
      const symbol* chain = list[i];
      for (; chain; chain = chain->Next()) {
        const function* foo = dynamic_cast <const function*> (chain);
        if (0==foo) continue;
        foo->PrintHeader(df->Out(), true);
        df->Out() << "\n";
      }
    }
    df->end_indent();
    delete[] list;
  } // if num_names

  df->end_indent();

}

// ******************************************************************
// *                       topic_models class                       *
// ******************************************************************

class topic_models : public help_topic {
public:
  topic_models()
   : help_topic("models", "Overview of models") { }
  virtual void PrintDocs(doc_formatter* df, const char*) const;
};

void topic_models::PrintDocs(doc_formatter* df, const char*) const
{
  df->begin_heading();
  PrintHeader(df->Out());
  df->end_heading();
  df->begin_indent();
  df->Out() << "A model is declared with a header that is similar to a function declaration, of the form\n\n";
  df->begin_indent();
  df->Out() << "formalism identifier(params) := { ... }\n\n";
  df->end_indent();
  df->Out() << "where \"formalism\" is one of the formalism types:\n";
  df->begin_indent();
  bool printed = false;
  for (int i=0; i<em->getNumTypes(); i++) {
    const type* t = em->getTypeNumber(i);
    if (!t->isAFormalism())    continue;
    if (printed) df->Out() << ", ";
    df->Out() << t->getName();
    printed = true;
  }
  df->Out() << ".\n\n";
  df->end_indent();
  df->Out() << "The statements within the braces specify how to build the model when necessary, and may include declarations and function calls specific to the formalism type.  Additionally, there may be statements that define the measures for the model, which are visible outside the model.  Note that a model call has the form\n\n";
  df->begin_indent();
  df->Out() << "model_identifier(params).measure;\n\n";
  df->end_indent();
  df->Out() << "and that a model is not instantiated until needed to compute a measure.  Measures may be classified by solution engine as appropriate, and calling a single measure may cause several measures to be computed (e.g., all steady-state measures will be computed together).  It is possible to declare arrays of measures, and measures of type void which execute the specified instructions whenever they are called.\n";
  df->Out() << "\nSee the help topics for a particular formalism for more information about what may be declared within a model, and functions available to build the model.\n";
  df->Out() <<"\nThere is also a special type named \"model\", which may be assigned to any constructed model.  For example:\n\n";
  df->begin_indent();
  df->Out() << "model m := model_identifier(params);\n";
  df->Out() << "m.any_measure_defined_in_model_identifier;\n\n";
  df->end_indent();
  df->Out() << "It is possible to declare arrays of type \"model\".\n";
  df->end_indent();
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
    virtual bool execute();
};
init_helpfuncs the_helpfunc_initializer;

init_helpfuncs::init_helpfuncs() : initializer("init_helpfuncs")
{
  usesResource("em");
  usesResource("st");
  usesResource("types");
  usesResource("formalisms");
}

bool init_helpfuncs::execute()
{
  if (0==st || 0==em)  return false;

  st->AddSymbol(new help_group(
    "#include",
    "Preprocessor directive to include files",
    "A source file can include other source files using the #include preprocessing directive, as in C.  An #include directive is ignored if it causes a circular dependency."
  ));

  st->AddSymbol(new help_group(
    "comments",
    "Preprocessor rules for comments",
    "Source files can contain C and C++ style comments, which are stripped by the lexer.  The rules are:\n  (1) Characters on a line following \"//\" are ignored.\n  (2) Characters between \"/*\" and \"*/\" are ignored."
  ));

  st->AddSymbol(new help_group(
    "functions",
    "Function call rules",
    "Built-in and user functions can be called using the usual, C-style syntax.  When functions are overloaded, Smart determines which function to call by summing the promotion distance (see the help topic on promotions) from the passed parameter to the formal parameter. Smart will also promote a function if necessary, by adding the modifier rand and/or proc to every formal parameter and to the return type of the function.  For instance, the definition:\n \t rand real mydist := sqrt(uniform(0, 1));\n is legal because the function\n \t real sqrt(real x)\n is automatically promoted to the form\n \t rand real sqrt(rand real x)."
  ));

  st->AddSymbol(  new topic_topics(st)                          );
  st->AddSymbol(  new topic_types                               );
  st->AddSymbol(  new topic_promotions                          );
  st->AddSymbol(  new topic_casting                             );
  st->AddSymbol(  new topic_operators                           );
  st->AddSymbol(  new topic_options                             );

  st->AddSymbol(  new topic_unaryop(exprman::uop_not)           );
  st->AddSymbol(  new topic_unaryop(exprman::uop_neg)           );

  st->AddSymbol(  new topic_binaryop(exprman::bop_implies)      );
  st->AddSymbol(  new topic_binaryop(exprman::bop_mod)          );
  st->AddSymbol(  new topic_binaryop(exprman::bop_diff)         );
  st->AddSymbol(  new topic_binaryop(exprman::bop_equals)       );
  st->AddSymbol(  new topic_binaryop(exprman::bop_nequal)       );
  st->AddSymbol(  new topic_binaryop(exprman::bop_gt)           );
  st->AddSymbol(  new topic_binaryop(exprman::bop_ge)           );
  st->AddSymbol(  new topic_binaryop(exprman::bop_lt)           );
  st->AddSymbol(  new topic_binaryop(exprman::bop_le)           );

  st->AddSymbol(  new topic_trinaryop(exprman::top_interval)    );

  st->AddSymbol(  new topic_assocop(false, exprman::aop_and)    );
  st->AddSymbol(  new topic_assocop(false, exprman::aop_or)     );
  st->AddSymbol(  new topic_assocop(false, exprman::aop_plus)   );
  st->AddSymbol(  new topic_assocop(true , exprman::aop_plus)   );
  st->AddSymbol(  new topic_assocop(false, exprman::aop_times)  );
  st->AddSymbol(  new topic_assocop(true , exprman::aop_times)  );

  st->AddSymbol(  new topic_assocop(false, exprman::aop_semi)   );
  st->AddSymbol(  new topic_assocop(false, exprman::aop_union)  );

  st->AddSymbol(  new topic_models                              );

  //
  // Automatically add help topics for formalisms or simple types
  // (neat trick!)
  //
  for (int i=0; i<em->getNumTypes(); i++) {
    const type* t = em->getTypeNumber(i);
    if (t->getBaseType() != t) continue;
    //
    // t is a simple type
    //
    if (t->isAFormalism()) {
      const formalism* ft = smart_cast <const formalism*> (t);
      DCASSERT(ft);
      st->AddSymbol(  new topic_formalism(ft)       );
    } else {
      st->AddSymbol(  new topic_simpletype(t->getBaseType())       );
    }
  }

  return true;
}


