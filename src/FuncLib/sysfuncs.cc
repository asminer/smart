
#include "sysfuncs.h"

#include <string.h>
#include <stdlib.h>
#include "../_Timer/timerlib.h"

#include "../Utils/strings.h"
#include "../Utils/textfmt.h"
#include "../Utils/splay.h"
#include "../Utils/initializer.h"
#include "../Utils/env.h"

#include "../ExprLib/functions.h"
#include "../ExprLib/help.h"
#include "../ExprLib/symb_tab.h"
#include "../ExprLib/formalism.h"

#include "../Options/optman.h"

// ******************************************************************
// *                   help_topic_traversal class                   *
// ******************************************************************

/**
    Prints documentation for matching help topics.
*/
class helpTopicTraversal : public shared_visitor {
        doc_formatter &df;
        const char* keyword;
    public:
        helpTopicTraversal(doc_formatter &df, const char* keyw);
        virtual void visit(shared_object* item);
};

helpTopicTraversal::helpTopicTraversal(doc_formatter &_d, const char* keyw)
    : df(_d)
{
    keyword = keyw;
}

void helpTopicTraversal::visit(shared_object* item)
{
    const help_topic* ht = dynamic_cast <const help_topic*> (item);
    if (ht) {
        if (!df.Matches(ht->Name(), keyword)) return;
        df.Out() << "\n";
        ht->PrintDocs(df, keyword);
    }
}

// ******************************************************************
// *                        help_base  class                        *
// ******************************************************************

struct ftnode {
  const formalism* ftype;
  ftnode* next;

  ftnode(const formalism* ft, ftnode* n) { ftype = ft; next = n; }

  void Print(std::ostream &s, bool depth) {
    if (0==next && depth) s << " or ";
    else if (depth) s << ", ";
    s << *ftype;
    if (next)  next->Print(s, true);
  }
};

// ******************************************************************
// *                       help_object  class                       *
// ******************************************************************

/*
    Help information to be displayed.
*/
class help_object : public shared_object {
public:
    const symbol* item;
    ftnode* within_models;

    help_object(const symbol* s) { item = s; within_models = 0; }
    virtual ~help_object() {
        while (within_models) {
            ftnode* foo = within_models;
            within_models = within_models->next;
            delete foo;
        }
    }

    virtual bool Print(std::ostream &s, int width=0) const {
        return false;
    }

    virtual int Compare(const shared_object* o) const {
        const symbol* x = dynamic_cast <const symbol*> (o);
        if (!x) {
            const help_object* h = dynamic_cast <const help_object*> (o);
            if (h) x = h->item;
        }
        DCASSERT(item);
        return item->Compare(x);
    }

    inline void AddFormalism(const formalism* ft) {
        if (ft) within_models = new ftnode(ft, within_models);
    }

    void DocumentObject(doc_formatter &df, const char* keyword) const {
        df.Out() << "\n";
        if (!item->DocumentHeader(df))  return;
        df.begin_indent();
        if (within_models) {
            df.Out() << "Allowed in models of type ";
            within_models->Print(df.Out(), false);
            df.Out() << "; cannot be called outside of a model. ";
        }
        item->DocumentBehavior(df);
        df.end_indent();
  }
};

// ******************************************************************
// *                      copy_matching  class                      *
// ******************************************************************

/*
 * Traversal to copy matching items into another splay tree.
 */
class copy_matching : public shared_visitor {
        doc_formatter &df;
        const char* keyword;
        splayOfShared &doctree;
        const formalism* ft;
        help_object* hentry;
    public:
        copy_matching(doc_formatter &df, const char* keyw, splayOfShared &dt);
        inline void changeFormalism(const formalism* _ft) {
            ft = _ft;
        }
        virtual void visit(shared_object* item);
};

copy_matching::copy_matching(doc_formatter &_d, const char* keyw,
        splayOfShared &dt) : df(_d), doctree(dt)
{
    keyword = keyw;
    ft = nullptr;
    hentry = nullptr;
}

void copy_matching::visit(shared_object* item)
{
    const symbol* sitem = dynamic_cast <symbol*> (item);
    if (0==sitem) return;
    if (!df.Matches(sitem->Name(), keyword)) return;
    //
    // Matching keyword.
    // Now, traverse the list of symbols with the same name,
    // and print documentation but only for non help topics.
    //
    for (; sitem; sitem=sitem->Next()) {
        const help_topic* ht = dynamic_cast <const help_topic*> (sitem);
        if (ht) continue;
        if (!hentry) hentry = new help_object(sitem);
        else         hentry->item = sitem;
        help_object* tree_entry =
            dynamic_cast <help_object*> (doctree.insert(hentry));
        if (tree_entry == hentry) {
            // We added hentry to the tree; so clear it for next time
            hentry = nullptr;
        }
        tree_entry->AddFormalism(ft);
    } // for sitem
}

// ******************************************************************
// *                        docuversal class                        *
// ******************************************************************

/*
 * Traversal to document all help_objects
 */
class docuversal : public shared_visitor {
        doc_formatter &df;
        const char* keyword;
    public:
        docuversal(doc_formatter &_df, const char* keyw);
        virtual void visit(shared_object* item);
};

docuversal::docuversal(doc_formatter &_df, const char* keyw)
    : df(_df)
{
    keyword = keyw;
}

void docuversal::visit(shared_object* item)
{
    const help_object* hitem = dynamic_cast <help_object*> (item);
    if (0==hitem) return;
    hitem->DocumentObject(df, keyword);
}

// ******************************************************************
// *                        help_base  class                        *
// ******************************************************************

class help_base : public simple_internal {
  const symbol** flist;
  long flist_alloc;
//   splayOfShared *doctree;
  doc_formatter df;
public:
  help_base(const char* name, int np);
  virtual ~help_base();
  virtual void Compute(traverse_data &x, expr** pass, int np);
protected:
  virtual void Help(const char* search) = 0;
  void HelpOptions(const char* search);
  void HelpTopics(const char* key);
  void HelpFuncs(const char* key);
};

help_base::help_base(const char* name, int np)
: simple_internal(type::find("void"), name, np),
    df(80, outputStream::globalOut())
{
  flist = 0;
  flist_alloc = 0;
  // doctree = 0;
}

help_base::~help_base()
{
  delete[] flist;
}

void help_base::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(0==x.aggregate);
  DCASSERT(1==np);
  if (x.stopExecution())  return;
  result* answer = x.answer;
  result foo;
  x.answer = &foo;
  SafeCompute(pass[0], x);
  x.answer = answer;
  const char* search;
  if (foo.isNormal()) {
    shared_string *xss = smart_cast <shared_string*> (foo.getPtr());
    DCASSERT(xss);
    search = xss->getStr();
  } else if (foo.isNull()) {
    search = 0;
  } else {
    return;
  }
  Help(search);
}


void help_base::HelpOptions(const char* search)
{
    option_manager::global().DocumentOptions(df, search);
}

void help_base::HelpTopics(const char* search)
{
    helpTopicTraversal T(df, search);
    symbol::chain_visit V(T);
    symbol_table::global().traverse(V);
}

void help_base::HelpFuncs(const char* search)
{
    splayOfShared doctree(32, 0);
    copy_matching T(df, search, doctree);

    // Add ordinary functions
    symbol_table::global().traverse(T);

    // Add formalism functions
    for (unsigned i=0; i<type::numRegistered(); i++) {
        const type* t = type::getRegistered(i);
        if (!t->isAFormalism())    continue;
        const formalism* ft = smart_cast <const formalism*> (t);
        DCASSERT(ft);
        T.changeFormalism(ft);
        ft->traverseSymbols(T);
    }

    // Print documentation for what we collected
    docuversal D(df, search);
    doctree.traverse(D);
    doctree.deleteAndClear();
}

// ******************************************************************
// *                         help_si  class                         *
// ******************************************************************

class help_si : public help_base {
public:
  help_si();
  virtual void Help(const char* search);
};

help_si::help_si() : help_base("help", 1)
{
  SetFormal(0, type::find("string"), "search");
  SetDocumentation("An on-line help mechanism.  Searches for help topics, functions, options, and option constants containing the substring <search>.  Documentation is displayed for all matches.  Use the search string \"topics\" to view the available help topics.  For function documentation, parameters between elipses (\"...\"s) may repeat.");
}

void help_si::Help(const char* search)
{
    // First: help topics
    HelpTopics(search);

    // Next: options
    HelpOptions(search);

    // Finally, functions
    HelpFuncs(search);
}

// ******************************************************************
// *                        helptop_si class                        *
// ******************************************************************

class helptop_si : public help_base {
public:
  helptop_si();
  virtual void Help(const char* search);
};

helptop_si::helptop_si() : help_base("help_topic", 1)
{
  SetFormal(0, type::find("string"), "search");
  SetDocumentation("An on-line help mechanism.  Searches for help topics containing the substring <search>.  Works like \"help\" but displays help topics only.");
}

void helptop_si::Help(const char* search)
{
  HelpTopics(search);
}

// ******************************************************************
// *                        helpopt_si class                        *
// ******************************************************************

class helpopt_si : public help_base {
public:
  helpopt_si();
  virtual void Help(const char* search);
};

helpopt_si::helpopt_si() : help_base("help_option", 1)
{
  SetFormal(0, type::find("string"), "search");
  SetDocumentation("An on-line help mechanism.  Searches for options and option constants containing the substring <search>.  Works like \"help\" but displays options only.");
}

void helpopt_si::Help(const char* search)
{
  HelpOptions(search);
}

// ******************************************************************
// *                       helpfunc_si  class                       *
// ******************************************************************

class helpfunc_si : public help_base {
public:
  helpfunc_si();
  virtual void Help(const char* search);
};

helpfunc_si::helpfunc_si()
 : help_base("help_function", 1)
{
  SetFormal(0, type::find("string"), "search");
  SetDocumentation("An on-line help mechanism.  Searches for functions containing the substring <search>.  Works like \"help\" but displays functions only.");
}

void helpfunc_si::Help(const char* search)
{
  HelpFuncs(search);
}

// ******************************************************************
// *                        version_si class                        *
// ******************************************************************

class version_si : public simple_internal {
  shared_string* version_string;
public:
  version_si(const char* str);
  virtual ~version_si();
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

version_si::version_si(const char* str)
 : simple_internal(type::find("string"), "version", 0)
{
  version_string = new shared_string(str);
  SetDocumentation("Return a string indicating the current version of this software.");
}

version_si::~version_si()
{
    Delete(version_string);
}

void version_si::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(x.parent);
  x.answer->setPtr(Share(version_string));
}


// ******************************************************************
// *                       filename_si  class                       *
// ******************************************************************

class filename_si : public simple_internal {
public:
  filename_si();
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

filename_si::filename_si()
 : simple_internal(type::find("string"), "file_name", 0)
{
  SetDocumentation("Return the name of the current source file being read by the interpreter.  The name \"-\" is used when reading from standard input.");
}

void filename_si::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(x.parent);
  x.answer->setPtr( x.parent->Where().shareFile() );
}

// ******************************************************************
// *                      linenumber_si  class                      *
// ******************************************************************

class linenumber_si : public simple_internal {
public:
  linenumber_si();
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

linenumber_si::linenumber_si()
 : simple_internal(type::find("int"), "line_number", 0)
{
  SetDocumentation("Return the line number of the current source file being read by the interpreter.");
}

void linenumber_si::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(x.parent);
  if (x.parent->Where().getLine()) {
    x.answer->setInt(x.parent->Where().getLine());
  } else {
    x.answer->setNull();
  }
}

// ******************************************************************
// *                          env_si class                          *
// ******************************************************************

class env_si : public simple_internal {
  const char** environment;
public:
  env_si(const char** e);
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

env_si::env_si(const char** e) : simple_internal(type::find("string"), "env", 1)
{
  environment = e;
  SetFormal(0, type::find("string"), "find");
  SetDocumentation("Return the first environment string that matches argument find.");
}

void env_si::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(1==np);
  if (0==environment) {
    x.answer->setNull();
    return;
  }
  SafeCompute(pass[0], x);
  if (!x.answer->isNormal()) return;
  shared_string *xss = smart_cast <shared_string*> (x.answer->getPtr());
  DCASSERT(xss);
  const char* key = xss->getStr();
  int xlen = strlen(key);
  for (int i=0; environment[i]; i++) {
    const char* equals = strstr(environment[i], "=");
    if (0==equals) continue;  // shouldn't happen, right?
    int length = (equals - environment[i]);
    if (length!=xlen) continue;
    if (strncmp(environment[i], key, length)!=0) continue;
    // match
    x.answer->setPtr(new shared_string(strdup(equals+1)));
    return;
  }
  // not found
  x.answer->setNull();
}

// ******************************************************************
// *                         exit_si  class                         *
// ******************************************************************

class exit_si : public simple_internal {
public:
  exit_si();
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

exit_si::exit_si()
 : simple_internal(type::find("void"), "exit", 1)
{
  SetFormal(0, type::find("int"), "code");
  SetDocumentation("Exit with specified return code.");
}

void exit_si::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  if (x.stopExecution())  return;
  result foo;
  x.answer = &foo;
  if (pass[0])  pass[0]->Compute(x);
  else          foo.setNull();
  if (foo.isNormal()) exit(foo.getInt());
  else                exit(-1);
}

// ******************************************************************
// *                        timer_base class                        *
// ******************************************************************

class timer_base : public simple_internal {
protected:
  static timer* watches;
  friend class init_sysfuncs;
public:
  timer_base(const type* t, const char* name);
};
timer* timer_base::watches = 0;

timer_base::timer_base(const type* t, const char* name)
 : simple_internal(t, name, 1)
{
  SetFormal(0, type::find("int"), "x");
}

// ******************************************************************
// *                      start_timer_si class                      *
// ******************************************************************

class start_timer_si : public timer_base {
public:
  start_timer_si();
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

start_timer_si::start_timer_si()
 : timer_base(type::find("void"), "start_timer")
{
  SetDocumentation("Starts a CPU timer.  Timers are numbered from 0 to 255; specify the desired timer as the function parameter.  Does nothing if the parameter is out of range.");
}

void start_timer_si::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  if (x.stopExecution())  return;
  result foo;
  x.answer = &foo;
  SafeCompute(pass[0], x);
  if (foo.isNormal() && (foo.getInt() >= 0) && (foo.getInt() < 256)) {
    watches[foo.getInt()].reset();
  }
}


// ******************************************************************
// *                      stop_timer_si class                      *
// ******************************************************************

class stop_timer_si : public timer_base {
public:
  stop_timer_si();
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

stop_timer_si::stop_timer_si()
 : timer_base(type::find("real"), "stop_timer")
{
  SetDocumentation("Stops a CPU timer, and returns the number of seconds of user time elapsed since it was started.  Timers are numbered from 0 to 255; specify the desired timer as the function parameter.  Returns null if the parameter is out of range, or if the timer was never started, or already stopped.");
}

void stop_timer_si::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  SafeCompute(pass[0], x);
  if (x.answer->isNormal()) {
    long id = x.answer->getInt();
    if (id >= 0 && id < 256) {
      x.answer->setReal(watches[id].elapsed_seconds());
      return;
    }
  }
  x.answer->setNull();
}


// ******************************************************************
// *                                                                *
// *                                                                *
// *                         Initialization                         *
// *                                                                *
// *                                                                *
// ******************************************************************

class init_sysfuncs : public initializer {
    public:
        init_sysfuncs();
    protected:
        virtual void execute();
};
static init_sysfuncs the_sysfunc_initializer;

init_sysfuncs::init_sysfuncs() : initializer(__FILE__, 4)
{
    builds_resource("funcs");
    needs_resource("types");
    needs_resource("env");
    needs_resource("string");
}

void init_sysfuncs::execute()
{
    if (!timer_base::watches) {
        timer_base::watches = new timer[256];
    }

    environ* e = dynamic_cast <environ*> (get_object("env"));
    if (e) {
        symbol_table::addGlobal(new version_si(e->version));
        symbol_table::addGlobal(new env_si(e->env)        );
    }

    symbol_table::addGlobal(  new help_si         );
    symbol_table::addGlobal(  new helptop_si      );
    symbol_table::addGlobal(  new helpopt_si      );
    symbol_table::addGlobal(  new helpfunc_si     );
    symbol_table::addGlobal(  new filename_si     );
    symbol_table::addGlobal(  new linenumber_si   );
    symbol_table::addGlobal(  new exit_si         );

    symbol_table::addGlobal(  new start_timer_si  );
    symbol_table::addGlobal(  new stop_timer_si   );
}

