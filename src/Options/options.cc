
#include "../include/defines.h"
#include "../Utils/strings.h"
#include "../Streams/textfmt.h"
#include "options.h"
#include "opt_enum.h"
#include <stdlib.h>
#include "../include/splay.h"

#include <cstring>

//#define DEBUG_SORT


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
    s << '#' << name;
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

option::error option::SetValue(shared_string *)
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

unsigned option::NumConstants() const
{
    return 0;
}

option_enum* option::GetConstant(unsigned i) const
{
    return 0;
}

void option::ShowCurrent(OutputStream &s) const
{
    ShowHeader(s);
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

radio_button* option::addRadioButton(const char* name, const char* doc,
        unsigned ndx)
{
    return 0;
}

checklist_enum* option::addChecklistItem(checklist_enum* grp,
                const char* name, const char* doc, bool &link)
{
    return 0;
}

checklist_enum* option::addChecklistGroup(const char* name, const char* doc,
        unsigned ni)
{
    return 0;
}

option::error option::notifyWatchers() const
{
    for (watcher* w = watchlist; w; w=w->next) {
        w->notify(this);
    }
    return Success;
}

