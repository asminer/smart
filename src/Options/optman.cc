
#include "../include/defines.h"
#include "options.h"
#include "optman.h"
#include "../Streams/streams.h"
#include "../include/splay.h"

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

