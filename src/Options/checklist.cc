
#include "../include/defines.h"
#include "../Streams/textfmt.h"
#include "../Utils/messages.h"

#include "checklist.h"

#include <cstring>

// **************************************************************************
// *                        checklist_enum  methods                        *
// **************************************************************************

checklist_enum::checklist_enum(const char* n, const char* d)
    : option_enum(n, d)
{
}

// **************************************************************************
// *                            checklist  items                            *
// **************************************************************************

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

// **************************************************************************
// *                            checklist groups                            *
// **************************************************************************

checklist_group
::checklist_group(const char* n, const char* d, unsigned mi)
 : checklist_enum(n, d)
{
  max_items = mi;
  num_items = 0;
  items = new checklist_enum* [max_items];
  for (unsigned i=0; i<max_items; i++) {
      items[i] = 0;
  }
}

checklist_group::~checklist_group()
{
  delete[] items;
}

bool checklist_group::CheckMe()
{
  for (unsigned i=0; i<num_items; i++) {
    if (!items[i]->CheckMe())  return false;
  }
  return true;
}

bool checklist_group::UncheckMe()
{
  for (unsigned i=0; i<num_items; i++) {
    if (!items[i]->UncheckMe())  return false;
  }
  return true;
}

bool checklist_group::IsChecked() const
{
  return false;
}

// **************************************************************************
// *                             checkall class                             *
// **************************************************************************

checkall::checkall(const char* n, const char* d, option* p)
 : checklist_enum(n,d)
{
  parent = p;
}

bool checkall::CheckMe()
{
  const unsigned numconsts = parent->NumConstants();
  for (unsigned i=0; i<numconsts; i++) {
    checklist_enum* item
        = smart_cast <checklist_enum*> (parent->GetConstant(i));
    DCASSERT(item);
    if (item != this) {
      if (!item->CheckMe())  return false;
    }
  }
  return true;
}

bool checkall::UncheckMe()
{
  const unsigned numconsts = parent->NumConstants();
  for (unsigned i=0; i<numconsts; i++) {
    checklist_enum* item
        = smart_cast <checklist_enum*> (parent->GetConstant(i));
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
// *                           checklist  options                           *
// **************************************************************************

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

unsigned checklist_opt:: NumConstants() const
{
    return numpossible;
}

option_enum* checklist_opt::GetConstant(unsigned i) const
{
    if (i>=numpossible) return 0;
    return possible[i];
}

void checklist_opt::ShowHeader(OutputStream &s) const
{
    show(s);
    s << " +/- values";
}

void checklist_opt::ShowCurrent(OutputStream &s) const
{
  show(s);
  s << " {";
  bool printed = false;
  for (unsigned i=0; i<numpossible; i++) if (possible[i]->IsChecked()) {
    if (printed) s << ", ";
    s << possible[i]->Name();
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

void checklist_opt::ShowRange(doc_formatter* df) const
{
  DCASSERT(df);
  df->Out() << "Legal values to be set or unset:";
  unsigned i;
  unsigned maxenum = 0;
  for (i=0; i<numpossible; i++)  {
    unsigned l = strlen(possible[i]->Name());
    maxenum = MAX(maxenum, l);
  }
  df->begin_description(maxenum);
  for (i=0; i<numpossible; i++) {
    df->item(possible[i]->Name());
    df->Out() << possible[i]->Documentation();
  }
  df->end_description();
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
  for (unsigned i=0; i<numpossible; i++) {
    if (df->Matches(possible[i]->Name(), keyword))  return true;
  }
  return false;
}


checklist_enum* checklist_opt::addChecklistItem(checklist_enum* grp,
                const char* name, const char* doc, bool &link)
{
    if (0==itemlist) return 0;
    if (itemlist->Find(name)) return 0; // duplicate

    checklist_enum* item = new checklist_item(name, doc, link);

    checklist_group* clg = smart_cast <checklist_group*>(grp);
    if (clg) clg->addItem(item);
    return itemlist->Insert(item);
}

checklist_enum* checklist_opt::addChecklistItem(checklist_enum* grp,
                const char* name, const char* doc, abstract_msg &m, bool act)
{
    if (0==itemlist) return 0;
    if (itemlist->Find(name)) return 0; // duplicate

    checklist_enum* item = new checklist_item(name, doc, m.active);
    m.name = name;
    m.active = act;

    checklist_group* clg = smart_cast <checklist_group*>(grp);
    if (clg) clg->addItem(item);
    return itemlist->Insert(item);
}



checklist_enum* checklist_opt::addChecklistGroup(const char* name,
                const char* doc, unsigned ni)
{
    if (0==itemlist) return 0;
    if (itemlist->Find(name)) return 0; // duplicate

    return itemlist->Insert(
            new checklist_group(name, doc, ni)
    );
}

