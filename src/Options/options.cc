
#include "../include/defines.h"
#include "options.h"
#include "opt_enum.h"
#include "optman.h"
#include "../Streams/streams.h"
#include <stdlib.h>
#include "../include/splay.h"

#include <string.h>

//#define DEBUG_SORT


// **************************************************************************
// *                            checklist  items                            *
// **************************************************************************

class checklist_item : public checklist_enum {
  bool &is_set;
public:
  checklist_item(const char* n, const char* d, bool &l);
  virtual bool CheckMe();
  virtual bool UncheckMe();
  virtual bool IsChecked() const;
};

checklist_item::checklist_item(const char* n, const char* d, bool &l)
 : checklist_enum(n, d), is_set(l)
{
}

bool checklist_item::CheckMe()
{
  is_set = true;
  return true;
}

bool checklist_item::UncheckMe()
{
  is_set = false;
  return true;
}

bool checklist_item::IsChecked() const
{
  return is_set;
}

checklist_enum* MakeChecklistConstant(const char* n, const char* d, bool &link)
{
  return new checklist_item(n, d, link);
}

// **************************************************************************
// *                            checklist groups                            *
// **************************************************************************

class checklist_group : public checklist_enum {
  checklist_enum** items;
  int num_items;
public:
  checklist_group(const char* n, const char* d, checklist_enum** i, int ni);
  virtual ~checklist_group();
  virtual bool CheckMe();
  virtual bool UncheckMe();
  virtual bool IsChecked() const;
};

checklist_group
::checklist_group(const char* n, const char* d, checklist_enum** i, int ni)
 : checklist_enum(n, d)
{
  items = i;
  num_items = ni;
}

checklist_group::~checklist_group()
{
  delete[] items;
}

bool checklist_group::CheckMe()
{
  for (int i=num_items-1; i>=0; i--) {
    if (!items[i]->CheckMe())  return false;
  }
  return true;
}

bool checklist_group::UncheckMe()
{
  for (int i=num_items-1; i>=0; i--) {
    if (!items[i]->UncheckMe())  return false;
  }
  return true;
}

bool checklist_group::IsChecked() const
{
  return false;
}

checklist_enum* MakeChecklistGroup(const char* name, const char* doc, checklist_enum** items, int ni)
{
  return new checklist_group(name, doc, items, ni);
}


// **************************************************************************
// *                             checkall class                             *
// **************************************************************************

/// Class to check or uncheck everything on a checklist.
class checkall : public checklist_enum {
  option* parent;
public:
  checkall(const char* n, const char* d, option* p);
  virtual bool CheckMe();
  virtual bool UncheckMe();
  virtual bool IsChecked() const;
};

checkall::checkall(const char* n, const char* d, option* p)
 : checklist_enum(n,d)
{
  parent = p;
}

bool checkall::CheckMe()
{
  long i = parent->NumConstants();
  checklist_enum* item = 0;
  for (i--; i>=0; i--) {
    item = smart_cast <checklist_enum*> (parent->GetConstant(i));
    DCASSERT(item);
    if (item != this) {
      if (!item->CheckMe())  return false;
    }
  }
  return true;
}

bool checkall::UncheckMe()
{
  long i = parent->NumConstants();
  checklist_enum* item = 0;
  for (i--; i>=0; i--) {
    item = smart_cast <checklist_enum*> (parent->GetConstant(i));
    DCASSERT(item);
    if (item != this) {
      if (!item->UncheckMe())  return false;
    }
  }
  return true;
}

bool checkall::IsChecked() const
{
  return false;
}

// **************************************************************************
// *                             option methods                             *
// **************************************************************************

option::watcher::watcher()
{
    next = 0;
}

option::watcher::~watcher()
{
}


option::option(type t, const char *n, const char* d)
{
  mytype = t;
  name = n;
  documentation = d;
  hidden = false;
  watchlist = 0;
}

option::~option()
{
    while (watchlist) {
        watcher* next = watchlist->next;
        delete watchlist;
        watchlist = next;
    }
}

void option::show(OutputStream &s) const
{
  s.Put('#');
  s.Put(name);
}

option::error option::SetValue(bool)
{
  return WrongType;
}

option::error option::SetValue(long)
{
  return WrongType;
}

option::error option::SetValue(double)
{
  return WrongType;
}

option::error option::SetValue(char*)
{
  return WrongType;
}

option::error option::SetValue(option_enum*)
{
  return WrongType;
}

option_enum* option::FindConstant(const char*) const
{
  return 0;
}

option::error option::GetValue(bool &v) const
{
  return WrongType;
}

option::error option::GetValue(long &v) const
{
  return WrongType;
}

option::error option::GetValue(double &v) const
{
  return WrongType;
}

option::error option::GetValue(const char* &v) const
{
  return WrongType;
}

int option::NumConstants() const
{
  return 0;
}

option_enum* option::GetConstant(long i) const
{
  return 0;
}

void option::ShowCurrent(OutputStream &s) const
{
  ShowHeader(s);
}

option::error option::AddCheckItem(option_enum* v)
{
  return WrongType;
}

option::error option::AddRadioButton(option_enum* v)
{
  return WrongType;
}

void option::Finish()
{
  // default: nothing
}

int option::Compare(const option* b) const
{
  if (b)  return strcmp(Name(), b->Name());
  else    return 1;
}

int option::Compare(const char* n) const
{
  return strcmp(Name(), n);
}

bool option::isApropos(const doc_formatter* df, const char* keyword) const
{
  if (0==df)  return false;
  return      df->Matches(Name(), keyword);
}

void option::PrintDocs(doc_formatter* df, const char* keyword) const
{
  if (0==df)  return;
#ifndef DEVELOPMENT_CODE
  if (IsUndocumented())  return;
#endif
  df->begin_heading();
  ShowHeader(df->Out());
  if (IsUndocumented())  df->Out() << " (undocumented)";
  df->end_heading();
  df->begin_indent();
  df->Out() << documentation;
  df->Out() << "\n";
  ShowRange(df);
  RecurseDocs(df, keyword);
  df->end_indent();
}

void option::RecurseDocs(doc_formatter* df, const char* keyword) const
{
}

void option::registerWatcher(watcher* w)
{
    w->next = watchlist;
    watchlist = w;
}

option::error option::notifyWatchers() const
{
    for (watcher* w = watchlist; w; w=w->next) {
        w->notify(this);
    }
    return Success;
}

// **************************************************************************
// *                         custom_option  methods                         *
// **************************************************************************

custom_option::custom_option(type t, const char* name, const char* doc, const char* r) : option(t, name, doc)
{
  DCASSERT(t!=RadioButton);
  range = r;
}

custom_option::~custom_option()
{
}

void custom_option::ShowHeader(OutputStream &s) const
{
  show(s);
  s.Put(' ');
  switch (Type()) {
    case Boolean: {
        bool value;
        GetValue(value);
        s.Put(value);
        return;
    }
    case Integer: {
        long value;
        GetValue(value);
        s.Put(value);
        return;
    }
    case Real: {
        double value;
        GetValue(value);
        s.Put(value);
        return;
    }
    case String: {
        const char* value;
        GetValue(value);
        if (value)  s << '"' << value << '"';
        else        s << "null";
        return;
    }
    default:
        DCASSERT(0);
  }
}

void custom_option::ShowRange(doc_formatter* df) const
{
  DCASSERT(df);
  DCASSERT(range);
  df->Out() << "Legal values: " << range;
  return;
}

bool custom_option::isApropos(const doc_formatter* df, const char* keyword) const
{
  if (0==df)                        return false;
  if (df->Matches(Name(), keyword)) return true;
  return false;
}


// **************************************************************************
// *                              empty option                              *
// **************************************************************************

/// Used for searching for options only.
class empty_opt : public option {
public:
  empty_opt(const char* n) : option(NoType, n, 0) { }
  virtual void ShowHeader(OutputStream &s) const { }
  virtual void ShowRange(doc_formatter* df) const { }
};

// **************************************************************************
// *                            boolean  options                            *
// **************************************************************************

class bool_opt : public option {
  bool& value;
public:
  bool_opt(const char* n, const char* d, bool &v)
   : option(Boolean, n, d), value(v) { }
  virtual ~bool_opt() { }
  virtual error SetValue(bool b) {
    value = b;
    return notifyWatchers();
  }
  virtual error GetValue(bool &b) const {
    b = value;
    return Success;
  }
  virtual void ShowHeader(OutputStream &s) const {
    show(s);
    s << " " << value;
  }
  virtual void ShowRange(doc_formatter* df) const {
    DCASSERT(df);
    df->Out() << "Legal values: [false, true]";
  }
};

option* MakeBoolOption(const char* name, const char* doc, bool &link)
{
  return new bool_opt(name, doc, link);
}

// **************************************************************************
// *                                                                        *
// *                            integer  options                            *
// *                                                                        *
// **************************************************************************

class int_opt : public option {
  long max;
  long min;
  long &value;
public:
  int_opt(const char* n, const char* d, long &v, long mn, long mx)
   : option(Integer, n ,d), value(v) { max = mx; min = mn; }
  virtual ~int_opt() { }
  virtual error SetValue(long v);
  virtual error GetValue(long &v) const {
    v = value;
    return Success;
  }
  virtual void ShowHeader(OutputStream &s) const {
    show(s);
    s << " " << value;
  }
  virtual void ShowRange(doc_formatter* df) const {
    DCASSERT(df);
    df->Out() << "Legal values: ";
    if (min<max)
      df->Out() << "integers in [" << min << ", ..., " << max << "]\n";
    else
      df->Out() << "any integer";
  }
};

option::error int_opt::SetValue(long b)
{
  if (0==value)  return NullFunction;
  if (min<max) {
    if ((b<min) || (b>max)) return RangeError;
  }
  value = b;
  return notifyWatchers();
}

option* MakeIntOption(const char* name, const char* doc,
      long& v, long min, long max)
{
  return new int_opt(name, doc, v, min, max);
}


// **************************************************************************
// *                                                                        *
// *                              real options                              *
// *                                                                        *
// **************************************************************************


class real_opt : public option {
  bool has_min;
  bool includes_min;
  double min;
  bool has_max;
  bool includes_max;
  double max;
  double &value;
public:
  real_opt(const char* n, const char* d, double &v,
     bool hl, bool il, double l,
     bool hu, bool iu, double u);
  ~real_opt() { }
  virtual error SetValue(double b);
  virtual error GetValue(double &v) const {
    v = value;
    return Success;
  }
  virtual void ShowHeader(OutputStream &s) const {
    show(s);
    s << " " << value;
  }
  virtual void ShowRange(doc_formatter* df) const {
    DCASSERT(df);
    df->Out() << "Legal values: reals in ";
    if (has_min) {
      if (includes_min)  df->Out().Put('[');
      else     df->Out().Put('(');
      df->Out() << min;
    } else {
      df->Out() << "(-oo";
    }
    df->Out().Put(',');
    df->Out().Put(' ');
    if (has_max) {
      df->Out() << max;
      if (includes_max) df->Out().Put(']');
      else df->Out().Put(')');
    } else {
      df->Out() << "oo)";
    }
  }
};

real_opt::real_opt(const char* n, const char* d, double &v,
       bool hl, bool il, double l, bool hu, bool iu, double u)
 : option(Real, n, d), value(v)
{
  has_min = hl;
  includes_min = il;
  min = l;
  has_max = hu;
  includes_max = iu;
  max = u;
}

option::error real_opt::SetValue(double b)
{
  if (0==value) return NullFunction;
  bool bad = false;
  if (has_min) {
    if (includes_min) {
      bad = b < min;
    } else {
      bad = b <= min;
    }
    if (bad) return RangeError;
  }

  if (has_max) {
    if (includes_max) {
      bad = b > max;
    } else {
      bad = b >= max;
    }
    if (bad) return RangeError;
  }

  value = b;
  return notifyWatchers();
}

option* MakeRealOption(const char* name, const char* doc, double &v,
      bool hasmin, bool incmin, double min,
      bool hasmax, bool incmax, double max)
{
  return new real_opt(name, doc, v,
      hasmin, incmin, min,
      hasmax, incmax, max);
}


// **************************************************************************
// *                             string options                             *
// **************************************************************************

class string_opt : public option {
  char* &value;
public:
  string_opt(const char* n, const char* d, char* &v)
   : option(String, n, d), value(v) { }
  virtual ~string_opt() { delete[] value; }
  virtual error SetValue(char *v) {
    if (0==v)      return RangeError;
    delete[] value;
    value = v;
    return notifyWatchers();
  }
  virtual error GetValue(const char* &v) const {
    v = value;
    return Success;
  }
  virtual void ShowHeader(OutputStream &s) const {
    show(s);
    s.Put(' ');
    if (value) s << '"' << value << '"';
    else s << "null";
  }
  virtual void ShowRange(doc_formatter* df) const {
    DCASSERT(df);
    df->Out() << "Legal values: any string";
  }
};

option* MakeStringOption(const char* name, const char* doc, char* &v)
{
  return new string_opt(name, doc, v);
}



// **************************************************************************
// *                            radio_opt  class                            *
// **************************************************************************

class radio_opt : public option {
    unsigned& which;
    radio_button** possible;
    unsigned numpossible;
    unsigned numadded;
public:
    radio_opt(const char* n, const char* d, unsigned np, unsigned& w);
    virtual ~radio_opt();
    virtual error SetValue(option_enum* v);
    virtual option_enum* FindConstant(const char* name) const;
    virtual int NumConstants() const;
    virtual option_enum* GetConstant(long i) const;

    virtual error AddRadioButton(option_enum* v);
    virtual void Finish();

    virtual void ShowHeader(OutputStream &s) const;
    virtual void ShowRange(doc_formatter* df) const;
    virtual bool isApropos(const doc_formatter* df, const char* keyword) const;
    virtual void RecurseDocs(doc_formatter* df, const char* keyword) const;
};

// **************************************************************************
// *                           radio_opt  methods                           *
// **************************************************************************

radio_opt::radio_opt(const char* n, const char* d, unsigned np, unsigned &w)
    : option(RadioButton, n, d), which(w)
{
    possible = new radio_button* [np];
    for (unsigned i=0; i<np; i++) {
        possible[i] = 0;
    }
    numpossible = np;
    numadded = 0;
}

radio_opt::~radio_opt()
{
    for (int i=0; i<numpossible; i++) {
        delete possible[i];
    }
    delete[] possible;
}

option::error radio_opt::SetValue(option_enum* v)
{
    radio_button* rb = smart_cast <radio_button*> (v);
    if (0==rb) return WrongType;
    if (rb->getIndex() >= numpossible) return WrongType;
    if (v != possible[rb->getIndex()])  return WrongType;
    if (! rb->AssignToMe()) return WrongType;
    which = rb->getIndex();
    return notifyWatchers();
}

int radio_opt::NumConstants() const
{
    return numpossible;
}

option_enum* radio_opt::GetConstant(long i) const
{
    if (i>=numpossible) return 0;
    return possible[i];
}

option_enum* radio_opt::FindConstant(const char* name) const
{
  // binary search
  unsigned low = 0;
  unsigned high = numpossible;
  while (low < high) {
    unsigned mid = (low+high)/2;
    int cmp = strcmp(possible[mid]->Name(), name);
    if (0==cmp) return possible[mid];
    if (cmp>0) {
      high = mid;
    } else {
      low = mid+1;
    }
  }
  // not found
  return 0;
}

option::error radio_opt::AddRadioButton(option_enum* v)
{
    if (0==v) return NullFunction;
    radio_button* rb = smart_cast <radio_button*> (v);
    if (0==rb) return WrongType;

    if (numadded >= numpossible) return RangeError;

    // Insertion sort; for now we don't have any radio options
    // with more than a dozen choices so no worries about inefficiency.

    for (unsigned i=numadded; i; i--) {
        int cmp = strcmp(possible[i-1]->Name(), v->Name());
        if (0==cmp) {
            return Duplicate;
        }
        if (cmp<0) {
            // element i-1 is less than this one, so it can go in slot i.
            possible[i] = rb;
            ++numadded;
            return Success;
        }
        // element i-1 is greater than this one, move it and keep looking
        possible[i] = possible[i-1];
        possible[i-1] = 0;
    }
    // New element is the smallest, add it to the front.
    possible[0] = rb;
    ++numadded;
    return Success;
}

void radio_opt::Finish()
{
    DCASSERT(numadded == numpossible);

    for (unsigned i=0; i<numpossible; i++) {
        if (possible[i]->getIndex() == which) {
            possible[i]->AssignToMe();
        }
    }
}

void radio_opt::ShowHeader(OutputStream &s) const
{
    show(s);
    s.Put(' ');
    s.Put(possible[which]->Name());
}

void radio_opt::ShowRange(doc_formatter* df) const
{
    DCASSERT(numpossible);
    df->Out() << "Legal values:";
    unsigned i;
    int maxenum = 0;
    for (i=0; i<numpossible; i++)  {
        int l = strlen(possible[i]->Name());
        maxenum = MAX(maxenum, l);
    }
    df->begin_description(maxenum);
    for (i=0; i<numpossible; i++) {
        df->item(possible[i]->Name());
        df->Out() << possible[i]->Documentation();
    }
    df->end_description();
}

bool radio_opt::isApropos(const doc_formatter* df, const char* keyword) const
{
    if (0==df)                          return false;
    if (df->Matches(Name(), keyword))   return true;
    for (unsigned i=0; i<numpossible; i++) {
        if (possible[i]->isApropos(df, keyword)) return true;
    }
    return false;
}

void radio_opt::RecurseDocs(doc_formatter* df, const char* keyword) const
{
    if (0==df) return;
    for (unsigned i=0; i<numpossible; i++) {
        if (0==possible[i]->readSettings()) continue;

        if (df->Matches(possible[i]->Name(), keyword)) {
            // Print all options
            df->Out() << "\nAll settings for " << possible[i]->Name() << ":\n";
            df->begin_indent();
            possible[i]->readSettings()->DocumentOptions(df, 0);
            df->end_indent();
            continue;
        }

        // Ok, just print matching settings, if any
        df->Out() << "\nMatching settings for " << possible[i]->Name() << ":\n";
        df->begin_indent();
        possible[i]->readSettings()->DocumentOptions(df, keyword);
        df->end_indent();
    }
}

option* MakeRadioOption(const char* name, const char* doc,
            unsigned numv, unsigned &link)
{
    return new radio_opt(name, doc, numv, link);
}


option* MakeRadioOption(const char* name, const char* doc,
            radio_button** values, unsigned numv, unsigned &link)
{
    radio_opt* ro = new radio_opt(name, doc, numv, link);
    for (unsigned i=0; i<numv; i++) {
        ro->AddRadioButton(values[i]);
    }
    return ro;
}


// **************************************************************************
// *                           checklist  options                           *
// **************************************************************************

class checklist_opt : public option {
  SplayOfPointers <checklist_enum>* itemlist;
  checklist_enum** possible;
  int numpossible;
public:
  checklist_opt(const char* n, const char* d);
  virtual ~checklist_opt();
  virtual int NumConstants() const { return numpossible; }
  virtual option_enum* GetConstant(long i) const {
    if (i>=numpossible) return 0;
    return possible[i];
  }

  virtual void ShowHeader(OutputStream &s) const {
    show(s);
    s << " +/- values";
  }
  virtual void ShowCurrent(OutputStream &s) const;
  // These are a bit more interesting...
  virtual option_enum* FindConstant(const char* name) const;
  virtual void ShowRange(doc_formatter* df) const;
  virtual error AddCheckItem(option_enum* v);
  virtual void Finish();
  virtual bool isApropos(const doc_formatter* df, const char* keyword) const;
};

checklist_opt::checklist_opt(const char* n, const char* d)
: option(Checklist, n, d)
{
  possible = 0;
  numpossible = 0;
  itemlist = new SplayOfPointers <checklist_enum> (10, 0);

  itemlist->Insert(
    new checkall("ALL", "Alias for all possible items", this)
  );
}

checklist_opt::~checklist_opt()
{
  delete itemlist;
  delete[] possible;
}

void checklist_opt::ShowCurrent(OutputStream &s) const
{
  show(s);
  s << " {";
  bool printed = false;
  for (int i=0; i<numpossible; i++) if (possible[i]->IsChecked()) {
    if (printed) s << ", ";
    s.Put(possible[i]->Name());
    printed = true;
  }
  s << "}";
}

/** Find the appropriate value for this name.
    If the name is bad (i.e., not found), we return NULL.
*/
option_enum* checklist_opt::FindConstant(const char* name) const
{
  if (itemlist) {
    option_enum* find = itemlist->Find(name);
    return find;
  }
  // binary search
  int low = 0;
  int high = numpossible;
  while (low < high) {
    int mid = (low+high)/2;
    int cmp = strcmp(possible[mid]->Name(), name);
    if (0==cmp) return possible[mid];
    if (cmp>0) {
      high = mid;
    } else {
      low = mid+1;
    }
  }
  // not found
  return 0;
}

void checklist_opt::ShowRange(doc_formatter* df) const
{
  DCASSERT(df);
  df->Out() << "Legal values to be set or unset:";
  int i;
  int maxenum = 0;
  for (i=0; i<numpossible; i++)  {
    int l = strlen(possible[i]->Name());
    maxenum = MAX(maxenum, l);
  }
  df->begin_description(maxenum);
  for (i=0; i<numpossible; i++) {
    df->item(possible[i]->Name());
    df->Out() << possible[i]->Documentation();
  }
  df->end_description();
}

option::error checklist_opt::AddCheckItem(option_enum* v)
{
  if (0==itemlist)  return Finalized;
  checklist_enum* cv = smart_cast <checklist_enum*> (v);
  if (0==cv) return WrongType;
  option_enum* foo = itemlist->Insert(cv);
  if (cv != foo)     return Duplicate;
  return Success;
}

void checklist_opt::Finish()
{
  if (0==itemlist) return;
  numpossible = itemlist->NumElements();
  possible = new checklist_enum* [numpossible];
  itemlist->CopyToArray(possible);
  delete itemlist;
  itemlist = 0;
}

bool checklist_opt::isApropos(const doc_formatter* df, const char* keyword) const
{
  if (0==df)                          return false;
  if (df->Matches(Name(), keyword))   return true;
  for (int i=0; i<numpossible; i++)
  if (df->Matches(possible[i]->Name(), keyword))  return true;
  return false;
}


option* MakeChecklistOption(const char* name, const char* doc)
{
  return new checklist_opt(name, doc);
}

