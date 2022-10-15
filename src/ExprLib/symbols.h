
#ifndef SYMBOLS_H
#define SYMBOLS_H

/** \file symbols.h

    Base class for symbols.  These are simply expressions with names.

*/

#include "expr.h"

class shared_string;
class doc_formatter;
class function;

/**   The base class of all symbols.
      That includes formal parameters, for loop iterators,
      functions, and model objects like states, places, and transitions.

      Note: we derive symbols from expressions because it 
      greatly simplifies for-loop iterators
      (otherwise you have an expression wrapped around
      an iterator, and that is unnecessary overhead.)
*/  

class symbol : public expr {
private:
  /// The symbol name.
  shared_string* name;
  /// Should we substitute our value?  Default: true.
  bool substitute_value;
  /// Next item in the symbol table.
  symbol* next;
  /// List of symbols waiting on us
  List <symbol> *waitlist;
public:
  symbol(const char* fn, int line, const type* t, char* n);
  symbol(const char* fn, int line, typelist* t, char* n);
  symbol(const symbol* wrapper);
protected:
  virtual ~symbol();
public:
  inline symbol* Next() const { return next; }
  inline void LinkTo(symbol* n) { next = n; }

  virtual const char* Name() const;
  virtual shared_object* SharedName() const;
  virtual void Rename(shared_object* newname);

  /** Change our substitution rule.
      @param sv   If sv is true, whenever an expression calls
                  "Substitute", we will replace ourself with
                  a constant equal to our current value.
                  If sv is false, whenever an expression calls
                  "Substitute", we will copy ourself.
   */ 
  inline void SetSubstitution(bool sv) { substitute_value = sv; }

  inline bool WillSubstitute() const { return substitute_value; }

  virtual bool Print(OutputStream &s, int width) const;

  virtual void Traverse(traverse_data &x);

  /// Display documentation for this symbol.
  virtual void PrintDocs(doc_formatter* df, const char* keyword) const;

  /// Add a symbol to our waiting list.
  void addToWaitList(symbol* w);

  /** This is how symbols on a waiting list are notified.
      See method notifyList() below.
      Default behavior is to print debugging info.
        @param  parent  The symbol that is notifying us
                        (we were on their waiting list).
  */
  virtual void notifyFrom(const symbol* parent);

  /// Check if a symbol could be waiting for us (perhaps indirectly).
  bool couldNotify(const symbol* s) const;

  /// Notify everyone on our waiting list, and destroy the list.
  void notifyList();
};


/** Derive from this class for adding new help topics.
*/
class help_topic : public symbol {
  const char* summary;
public:
  help_topic(const char* topic_name, const char* summary);
  help_topic();
protected:
  virtual ~help_topic();
  void setName(char* name);
  inline void setSummary(const char* sum) {
    DCASSERT(0==summary);
    summary = sum;
  }
public:
  void PrintHeader(OutputStream &s) const;
  inline const char* Summary() const { return summary; }
};

/**
    A help topic that consists of documentation,
    and lists of symbols and/or options.
*/
class help_group : public help_topic {
  const char* docs;
  List <function> funcs;
  List <option> options;
public:
  help_group(const char* name, const char* summary, const char* docs);
protected:
  virtual ~help_group();
public:
  virtual void PrintDocs(doc_formatter* df, const char* keyword) const;

  inline void addFunction(function* s) {
    if (s) funcs.Append(s);
  }
  inline void addOption(option* o) {
    if (o) options.Append(o);
  }
};


#endif
