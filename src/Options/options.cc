
#include "../include/defines.h"
#include "options.h"
#include "opt_enum.h"
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

option::option(type t, const char *n, const char* d)
{
  mytype = t;
  name = n;
  documentation = d;
  hidden = false;
}

option::~option()
{
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

option::error option::SetValue(radio_button*)
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

option::error option::GetValue(const radio_button* &v) const
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

option::error option::AddCheckItem(checklist_enum* v)
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
    return Success;
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
  return Success;
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
  return Success;
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
    delete[] value; value = v;   return Success;
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
  int& which;
  radio_button** possible;
  int numpossible;
public:
  radio_opt(const char* n, const char* d, radio_button** p, long np, int &w);
  virtual ~radio_opt();
  virtual error SetValue(radio_button* v) {
    which = v->getIndex();
    return v->AssignToMe() ? Success : WrongType;
  }
  virtual option::error GetValue(const radio_button* &v) const {
    v = possible[which];
    return Success;
  }
  virtual option_enum* FindConstant(const char* name) const;
  virtual int NumConstants() const;
  virtual option_enum* GetConstant(long i) const;

  virtual void ShowHeader(OutputStream &s) const;
  virtual void ShowRange(doc_formatter* df) const;
  virtual bool isApropos(const doc_formatter* df, const char* keyword) const;
  virtual void RecurseDocs(doc_formatter* df, const char* keyword) const;
};

// **************************************************************************
// *                           radio_opt  methods                           *
// **************************************************************************

radio_opt::radio_opt(const char* n, const char* d, radio_button** p,
  long np, int &w) : option(RadioButton, n, d), which(w)
{
  possible = p;
  numpossible = np;
  possible[which]->AssignToMe();
}

radio_opt::~radio_opt()
{
  for (int i=0; i<numpossible; i++) {
    delete possible[i];
  }
  delete[] possible;
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

bool radio_opt::isApropos(const doc_formatter* df, const char* keyword) const
{
  if (0==df)                          return false;
  if (df->Matches(Name(), keyword))   return true;
  for (int i=0; i<numpossible; i++) {
    if (possible[i]->isApropos(df, keyword)) return true;
  }
  return false;
}

void radio_opt::RecurseDocs(doc_formatter* df, const char* keyword) const
{
  if (0==df) return;
  for (int i=0; i<numpossible; i++) {
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
                       radio_button** values, int numv,
           int &link)
{
  DCASSERT(values);
  // Check that the values are sorted and indexed properly
  DCASSERT(values[0]);
  if (values[0]->getIndex() != 0)  return 0;
  int i;
  for (i=1; i<numv; i++) {
    DCASSERT(values[i]);
    int cmp = strcmp(values[i]->Name(), values[i-1]->Name());
    if (cmp<=0) return 0;  // NOT SORTED, BAIL OUT!!!!
    if (values[i]->getIndex() != i)  return 0;
  }
  return new radio_opt(name, doc, values, numv, link);
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
  virtual error AddCheckItem(checklist_enum* v);
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

option::error checklist_opt::AddCheckItem(checklist_enum* v)
{
  if (0==itemlist)  return Finalized;
  option_enum* foo = itemlist->Insert(v);
  if (v != foo)     return Duplicate;
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

// **************************************************************************
// *                                                                        *
// *                         Option list management                         *
// *                                                                        *
// **************************************************************************

option_manager::option_manager() { }
option_manager::~option_manager() { }

class option_heap : public option_manager {
  SplayOfPointers <option> *optlist;
  option** SortedOptions;
  long NumSortedOptions;
public:
  option_heap();
  virtual void AddOption(option *);
  virtual void DoneAddingOptions();
  virtual option* FindOption(const char* name) const;
  virtual long NumOptions() const {
    return NumSortedOptions;
  }
  virtual option* GetOptionNumber(long i) const {
    if (i>=NumSortedOptions) return 0;
    if (0==SortedOptions) return 0;
    return SortedOptions[i];
  }
  virtual void DocumentOptions(doc_formatter* df, const char* keyword) const;
  virtual void ListOptions(doc_formatter* df) const;
};


option_heap::option_heap() : option_manager()
{
  optlist = new SplayOfPointers <option> (16, 0);
  SortedOptions = 0;
  NumSortedOptions = 0;
}

void option_heap::AddOption(option *o)
{
  DCASSERT(0==SortedOptions);
  optlist->Insert(o);
}

void option_heap::DoneAddingOptions()
{
  DCASSERT(0==SortedOptions);
  NumSortedOptions = optlist->NumElements();
  SortedOptions = new option*[NumSortedOptions];
  optlist->CopyToArray(SortedOptions);
  delete optlist;
  optlist = 0;
  for (int i=0; i<NumSortedOptions; i++) SortedOptions[i]->Finish();
}

option* option_heap::FindOption(const char* name) const
{
  if (optlist) {
    option* find = optlist->Find(name);
    return find;
  }
  DCASSERT(SortedOptions);
  // binary search
  int low = 0;
  int high = NumSortedOptions;
  while (low < high) {
    int mid = (low+high)/2;
    int cmp = strcmp(SortedOptions[mid]->Name(), name);
    if (0==cmp) return SortedOptions[mid];
    if (cmp>0) {
      high = mid;
    } else {
      low = mid+1;
    }
  }
  // not found
  return 0;
}

void option_heap::DocumentOptions(doc_formatter* df, const char* keyword) const
{
  if (0==df)  return;
  DCASSERT(SortedOptions);
  for (int i=0; i<NumSortedOptions; i++)
    if (SortedOptions[i]->isApropos(df, keyword)) {
      df->Out() << "\n";
      SortedOptions[i]->PrintDocs(df, keyword);
    }
}

void option_heap::ListOptions(doc_formatter* df) const
{
  if (0==df)  return;
  DCASSERT(SortedOptions);
  for (int i=0; i<NumSortedOptions; i++) {
#ifndef DEVELOPMENT_CODE
    if (SortedOptions[i]->IsUndocumented())  continue;
#endif
    SortedOptions[i]->ShowCurrent(df->Out());
    df->Out() << "\n";
  }
}


option_manager* MakeOptionManager()
{
  return new option_heap;
}

