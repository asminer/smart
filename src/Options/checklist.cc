
#include "../include/defines.h"
#include "../Utils/textfmt.h"
#include "../Utils/strings.h"

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
    itemlist = new splayOfShared (10, 0);
    itemarray = nullptr;

    itemlist->insert(
        new checkall("ALL", "Alias for all possible items", this)
    );
}

checklist_opt::~checklist_opt()
{
    delete itemlist;
    delete itemarray;
}

unsigned checklist_opt:: NumConstants() const
{
    if (itemarray) return itemarray->numElements();
    if (itemlist)  return itemlist->numElements();
    return 0;
}

option_enum* checklist_opt::GetConstant(unsigned i) const
{
    if (itemarray) return itemarray->get(i);
    return nullptr;
}

void checklist_opt::ShowHeader(std::ostream &s) const
{
    s << *this << " +/- values";
}

void checklist_opt::ShowCurrent(std::ostream &s) const
{
    s << *this << " {";
    if (itemarray) {
        bool printed = false;
        for (unsigned i=0; i<itemarray->numElements(); i++) {
            const checklist_enum* c = itemarray->get(i);
            if (!c) continue;
            if (!c->IsChecked()) continue;

            if (printed) s << ", ";
            c->Print(s);
            printed = true;
        }
    }
    s << "}";
}

/** Find the appropriate value for this name.
    If the name is bad (i.e., not found), we return NULL.
*/
option_enum* checklist_opt::FindConstant(const char* name) const
{
    const_string CS(name);
    if (itemlist) {
        return smart_cast <option_enum*> (itemlist->find(&CS));
    }

    if (itemarray) {
        return itemarray->find(&CS);
    }

    return nullptr;
}

void checklist_opt::ShowRange(doc_formatter &df) const
{
    DCASSERT(itemarray);
    df.Out() << "Legal values to be set or unset:";
    unsigned maxenum = 0;
    for (unsigned i=0; i<itemarray->numElements(); i++)  {
        const checklist_enum* c = itemarray->get(i);
        if (!c) continue;
        unsigned l = strlen(c->Name());
        maxenum = MAX(maxenum, l);
    }
    df.begin_description(maxenum);
    for (unsigned i=0; i<itemarray->numElements(); i++) {
        const checklist_enum* c = itemarray->get(i);
        if (!c) continue;
        df.item(c->Name());
        df.Out() << c->Documentation();
    }
    df.end_description();
}

void checklist_opt::Finish()
{
    if (itemlist) {
        itemarray = new orderedArray<checklist_enum> (*itemlist);
        delete itemlist;
        itemlist = nullptr;
    }
}

bool checklist_opt::isApropos(const doc_formatter &df, const char* keyword) const
{
    if (df.Matches(Name(), keyword))   return true;
    if (!itemarray) return false;
    for (unsigned i=0; i<itemarray->numElements(); i++) {
        const checklist_enum* c = itemarray->get(i);
        if (!c) continue;
        if (df.Matches(c->Name(), keyword))  return true;
    }
    return false;
}


checklist_enum* checklist_opt::addChecklistItem(checklist_enum* grp,
                const char* name, const char* doc, bool &link)
{
    const_string CS(name);
    if (!itemlist) return nullptr;
    if (itemlist->find(&CS)) return nullptr; // duplicate

    checklist_enum* item = new checklist_item(name, doc, link);

    checklist_group* clg = smart_cast <checklist_group*>(grp);
    if (clg) clg->addItem(item);
    return smart_cast <checklist_enum*> (itemlist->insert(item));
}

checklist_enum* checklist_opt::addChecklistGroup(const char* name,
                const char* doc, unsigned ni)
{
    const_string CS(name);
    if (!itemlist) return nullptr;
    if (itemlist->find(&CS)) return nullptr; // duplicate

    checklist_enum* grp = new checklist_group(name, doc, ni);
    return smart_cast <checklist_enum*> (itemlist->insert(grp));
}

