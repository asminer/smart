
// $Id$

#include "sysfuncs.h"

#include <string.h>
#include <stdlib.h>
#include "../ExprLib/exprman.h"
#include "../ExprLib/functions.h"
#include "../SymTabs/symtabs.h"
#include "../ExprLib/strings.h"
#include "../Streams/streams.h"
#include "../Options/options.h"
#include "../ExprLib/formalism.h"
#include "../include/splay.h"
#include "../Timers/timers.h"

// ******************************************************************
// *                        help_base  class                        *
// ******************************************************************

struct ftnode {
  const formalism* ftype;
  ftnode* next;

  ftnode(const formalism* ft, ftnode* n) { ftype = ft; next = n; }

  void Print(OutputStream &s, bool depth) {
    if (0==next && depth) s << " or ";
    else if (depth) s << ", ";
    s << ftype->getName();
    if (next)  next->Print(s, true);
  }
};

struct help_object {
  const symbol* item;
  ftnode* within_models;

  help_object() { item = 0; within_models = 0; }
  ~help_object() {
    while (within_models) {
      ftnode* foo = within_models;
      within_models = within_models->next;
      delete foo;
    }
  }

  inline void AddFormalism(const formalism* ft) {
    if (ft) within_models = new ftnode(ft, within_models);
  }
  
  inline int Compare(const symbol* xitem) const {
    DCASSERT(item);
    DCASSERT(xitem);
    DCASSERT(item->Name());
    DCASSERT(xitem->Name());
    int c = strcmp(item->Name(), xitem->Name());
    if (c!=0)  return c;
    return SIGN(SafeID(item) - SafeID(xitem));  // same names, compare IDs.
  }

  inline int Compare(const help_object* x) const {
    DCASSERT(x);
    return Compare(x->item);
  }

  void DocumentObject(doc_formatter* df, const char* keyword) const {
    df->Out() << "\n";
    if (0==within_models) {
      item->PrintDocs(df, keyword);
      return;
    }
    const function* fitem = smart_cast <const function*> (item);
    DCASSERT(fitem);
    if (!fitem->DocumentHeader(df))  return;
    df->begin_indent();
    df->Out() << "Allowed in models of type ";
    within_models->Print(df->Out(), false);
    df->Out() << "; cannot be called outside of a model. ";
    fitem->DocumentBehavior(df);
    df->end_indent();
  }
};

class help_base : public simple_internal {
  const symbol** flist;
  long flist_alloc;
  SplayOfPointers <help_object> *doctree;
  doc_formatter* df;
public:
  help_base(const char* name, int np);
  virtual ~help_base();
  virtual void Compute(traverse_data &x, expr** pass, int np);
protected:
  virtual void Help(const char* search) = 0;
  void HelpOptions(const char* search);
  void HelpTopics(const symbol_table* st, const char* key);
  void HelpFuncs(const symbol_table* st, const char* key);
private:
  void AddFunctions(const char* search, const formalism* ft, long lsize);
  void Alloc(long nsz);
};

help_base::help_base(const char* name, int np)
: simple_internal(em->VOID, name, np)
{
  flist = 0;
  flist_alloc = 0;
  doctree = 0;
  df = MakeTextFormatter(80, em->cout());
}

help_base::~help_base()
{
  delete[] flist;
  delete df;
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
  DCASSERT(em);
  DCASSERT(em->OptMan());
  em->OptMan()->DocumentOptions(df, search);
}

void help_base::HelpTopics(const symbol_table* st, const char* search)
{
  long max_num = st->NumNames();
  Alloc(max_num);
  st->CopyToArray(flist);
  for (long i=0; i<max_num; i++) {
    if (!df->Matches(flist[i]->Name(), search))  continue;
    for (const symbol* chain = flist[i]; chain; chain=chain->Next()) {
      const help_topic* ht = dynamic_cast <const help_topic*> (chain);
      if (0==ht) continue;
      df->Out() << "\n";
      ht->PrintDocs(df, search);
    } // for chain
  } // for i
}

void help_base::HelpFuncs(const symbol_table* st, const char* search)
{
  // how many functions are there per table?
  long max_num = st->NumNames();
  for (int i=0; i<em->getNumTypes(); i++) {
    const type* t = em->getTypeNumber(i);
    if (!t->isAFormalism())    continue;
    const formalism* ft = smart_cast <const formalism*> (t);
    DCASSERT(ft);
    max_num = MAX(max_num, ft->numFuncNames());
  } // for i

  Alloc(max_num);
  doctree = new SplayOfPointers <help_object> (32, 0);

  // Add "ordinary" functions
  long nfn = st->NumNames();
  st->CopyToArray(flist);
  AddFunctions(search, 0, nfn);

  // Add "formalism" functions
  for (int i=0; i<em->getNumTypes(); i++) {
    const type* t = em->getTypeNumber(i);
    if (!t->isAFormalism())    continue;
    const formalism* ft = smart_cast <const formalism*> (t);
    DCASSERT(ft);
    nfn = ft->numFuncNames();
    ft->copyFuncsToArray(flist);
    AddFunctions(search, ft, nfn);
  } // for i

  // dump to array
  long hlen = doctree->NumElements();
  help_object** holist = hlen ? new help_object* [hlen] : 0;
  doctree->CopyToArray(holist);
  delete doctree;
  doctree = 0;

  // Print documentation for what we collected
  for (long i=0; i<hlen; i++) {
    DCASSERT(holist[i]);
    holist[i]->DocumentObject(df, search);
    delete holist[i];
    holist[i] = 0;
  } // for i
  delete[] holist;
}

void help_base::AddFunctions(const char* key, const formalism* ft, long lsize)
{
  help_object* tmp = 0;
  for (long j=0; j<lsize; j++) {
    DCASSERT(flist[j]);
    if (!df->Matches(flist[j]->Name(), key))  continue;
    for (const symbol* chain = flist[j]; chain; chain = chain->Next()) {
      // add this function to the tree
      const help_topic* ht = dynamic_cast <const help_topic*> (chain);
      if (ht) continue;  // don't add help topics here.
      if (0==tmp)  tmp = new help_object;
      tmp->item = chain;
      help_object* entry = doctree->Insert(tmp);
      if (entry == tmp)  tmp = 0;
      DCASSERT(entry);
      entry->AddFormalism(ft);
    } // for chain
  } // for j
  delete tmp;
}

void help_base::Alloc(long size)
{
  if (0==size) return;
  if (size < flist_alloc) return;
  delete[] flist;
  flist = new const symbol*[size];
  flist_alloc = size;
}

// ******************************************************************
// *                         help_si  class                         *
// ******************************************************************

class help_si : public help_base {
  const symbol_table* funcs;
public:
  help_si(const symbol_table*);
  virtual void Help(const char* search);
};

help_si::help_si(const symbol_table* fst) : help_base("help", 1)
{
  funcs = fst;
  SetFormal(0, em->STRING, "search");
  SetDocumentation("An on-line help mechanism.  Searches for help topics, functions, options, and option constants containing the substring <search>.  Documentation is displayed for all matches.  Use the search string \"topics\" to view the available help topics.  For function documentation, parameters between elipses (\"...\"s) may repeat.");
}

void help_si::Help(const char* search)
{
  // First: help topics
  HelpTopics(funcs, search);
  
  // Next: options
  HelpOptions(search);

  // Finally, functions
  HelpFuncs(funcs, search);
}

// ******************************************************************
// *                        helptop_si class                        *
// ******************************************************************

class helptop_si : public help_base {
  const symbol_table* funcs;
public:
  helptop_si(const symbol_table*);
  virtual void Help(const char* search);
};

helptop_si::helptop_si(const symbol_table* fst) : help_base("help_topic", 1)
{
  funcs = fst;
  SetFormal(0, em->STRING, "search");
  SetDocumentation("An on-line help mechanism.  Searches for help topics containing the substring <search>.  Works like \"help\" but displays help topics only.");
}

void helptop_si::Help(const char* search)
{
  HelpTopics(funcs, search);
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
  SetFormal(0, em->STRING, "search");
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
  const symbol_table* funcs;
public:
  helpfunc_si(const symbol_table*);
  virtual void Help(const char* search);
};

helpfunc_si::helpfunc_si(const symbol_table* fst)
 : help_base("help_function", 1)
{
  funcs = fst;
  SetFormal(0, em->STRING, "search");
  SetDocumentation("An on-line help mechanism.  Searches for functions containing the substring <search>.  Works like \"help\" but displays functions only.");
}

void helpfunc_si::Help(const char* search)
{
  HelpFuncs(funcs, search);
}

// ******************************************************************
// *                        version_si class                        *
// ******************************************************************

class version_si : public simple_internal {
  const char* version_string;
public:
  version_si(const char* str);
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

version_si::version_si(const char* str)
 : simple_internal(em->STRING, "version", 0)
{
  version_string = str;
  SetDocumentation("Return a string indicating the current version of this software.");
}

void version_si::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(x.parent);
  shared_object* ans;
  ans = version_string ? new shared_string(strdup(version_string)) : 0;
  x.answer->setPtr(ans);
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
 : simple_internal(em->STRING, "file_name", 0)
{
  SetDocumentation("Return the name of the current source file being read by the interpreter.  The name \"-\" is used when reading from standard input.");
}

void filename_si::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(x.parent);
  const char* fn = x.parent->Filename();
  shared_object* ans = fn ? new shared_string(strdup(fn)) : 0;
  x.answer->setPtr(ans);
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
 : simple_internal(em->INT, "line_number", 0)
{
  SetDocumentation("Return the line number of the current source file being read by the interpreter.");
}

void linenumber_si::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(x.parent);
  if (x.parent->Linenumber() < 0)
    x.answer->setNull();
  else
    x.answer->setInt(x.parent->Linenumber());
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

env_si::env_si(const char** e) : simple_internal(em->STRING, "env", 1)
{
  environment = e;
  SetFormal(0, em->STRING, "find");
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
 : simple_internal(em->VOID, "exit", 1)
{
  SetFormal(0, em->INT, "code");
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
  static timer** watches;
  friend void AddSysFunctions(symbol_table*, const exprman*, const char**, const char*);
public:
  timer_base(const type* t, const char* name);
};
timer** timer_base::watches = 0;

timer_base::timer_base(const type* t, const char* name)
 : simple_internal(t, name, 1)
{
  SetFormal(0, em->INT, "x");
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
 : timer_base(em->VOID, "start_timer")
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
    if (0==watches[foo.getInt()]) watches[foo.getInt()] = makeTimer();
    watches[foo.getInt()]->reset();
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
 : timer_base(em->REAL, "stop_timer")
{
  SetDocumentation("Stops a CPU timer, and returns the number of seconds of user time elapsed since it was started.  Timers are numbered from 0 to 255; specify the desired timer as the function parameter.  Returns null if the parameter is out of range, or if the timer was never started, or already stopped.");
}

void stop_timer_si::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  SafeCompute(pass[0], x);
  if (x.answer->isNormal()) {
    long id = x.answer->getInt();
    if (id >= 0 && id < 256 && watches[id]) {
      x.answer->setReal(watches[id]->elapsed());
      doneTimer(watches[id]);
      return;
    } 
  }
  x.answer->setNull();
}


// ******************************************************************
// *                                                                *
// *                           front  end                           *
// *                                                                *
// ******************************************************************

void AddSysFunctions(symbol_table* st, const exprman* em, const char** env, const char* version)
{
  if (0==st || 0==em)  return;

  if (0==timer_base::watches) {
    timer_base::watches = new timer*[256];
  }

  st->AddSymbol(  new help_si(st)     );
  st->AddSymbol(  new helptop_si(st)  );
  st->AddSymbol(  new helpopt_si      );
  st->AddSymbol(  new helpfunc_si(st) );
  if (version) st->AddSymbol(new version_si(version));
  st->AddSymbol(  new filename_si     );
  st->AddSymbol(  new linenumber_si   );
  st->AddSymbol(  new env_si(env)     );
  st->AddSymbol(  new exit_si         );

  st->AddSymbol(  new start_timer_si  );
  st->AddSymbol(  new stop_timer_si   );
}


