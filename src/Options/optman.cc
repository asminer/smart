
#include "../include/defines.h"
#include "../Streams/streams.h"
#include "../include/splay.h"

#include "options.h"
#include "optman.h"
#include "boolopt.h"
#include "intopt.h"
#include "realopt.h"
#include "stropt.h"
#include "radio_opt.h"
#include "checklist.h"

//#define DEBUG_SORT


// **************************************************************************
// *                         option_manager methods                         *
// **************************************************************************

option_manager::option_manager()
{
}

option_manager::~option_manager()
{
}

option* option_manager::addBoolOption(const char* name, const char* doc,
        bool &link)
{
    return addOption( new bool_opt(name, doc, link) );
}


option* option_manager::addIntOption(const char* name, const char* doc,
      long& v, long min, long max)
{
    return addOption( new int_opt(name, doc, v, min, max) );
}


option* option_manager::addRealOption(const char* n, const char* d, double &v,
      bool hasmin, bool incmin, double min,
      bool hasmax, bool incmax, double max)
{
    return addOption( new real_opt(n, d, v,
        hasmin, incmin, min,
        hasmax, incmax, max)
    );
}


option* option_manager::addStringOption(const char* name, const char* doc,
        shared_string* &v)
{
    return addOption( new string_opt(name, doc, v) );
}


option* option_manager::addRadioOption(const char* name, const char* doc,
            unsigned numv, unsigned &link)
{
    return addOption( new radio_opt(name, doc, numv, link) );
}

option* option_manager::addChecklistOption(const char* name, const char* doc)
{
    return addOption( new checklist_opt(name, doc) );
}


// **************************************************************************
// *                                                                        *
// *                         Option list management                         *
// *                                                                        *
// **************************************************************************

class option_heap : public option_manager {
  SplayOfPointers <option> *optlist;
  option** SortedOptions;
  long NumSortedOptions;
public:
  option_heap();
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
protected:
  virtual option* addOption(option *);
};


option_heap::option_heap() : option_manager()
{
  optlist = new SplayOfPointers <option> (16, 0);
  SortedOptions = 0;
  NumSortedOptions = 0;
}

option* option_heap::addOption(option *o)
{
  DCASSERT(0==SortedOptions);
  optlist->Insert(o);
  return o;
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
    int cmp = SortedOptions[mid]->Compare(name);
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

