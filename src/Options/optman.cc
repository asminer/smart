
#include "../include/defines.h"
#include "../Utils/splay.h"
#include "../Utils/strings.h"
#include "../Utils/textfmt.h"

#include "options.h"
#include "optman.h"
#include "boolopt.h"
#include "intopt.h"
#include "realopt.h"
#include "stropt.h"
#include "radio_opt.h"
#include "checklist.h"


//#define DEBUG_SORT

option_manager* option_manager::_global = nullptr;

// **************************************************************************
// *                         option_manager methods                         *
// **************************************************************************

option_manager::option_manager()
{
    optlist = new splayOfShared(16, 0);
    sortedOptions = nullptr;
    numOptions = 0;
}

option_manager::~option_manager()
{
    delete optlist;
    for (unsigned i=0; i<numOptions; i++) {
        Delete(sortedOptions[i]);
    }
    delete[] sortedOptions;
}

bool option_manager::Print(std::ostream &s, int width) const
{
    s << "option_manager";
    return true;
}

void option_manager::DoneAddingOptions()
{
    DCASSERT(!sortedOptions);
    numOptions = optlist->numElements();
    sortedOptions = new option*[numOptions];
    copy_traversal <option> T(sortedOptions, numOptions);
    optlist->traverse(T);
    delete optlist;
    optlist = nullptr;
    for (unsigned i=0; i<numOptions; i++) sortedOptions[i]->Finish();
}

option* option_manager::FindOption(const char* name) const
{
    const_string CS(name);

    if (optlist) {
        option* find = smart_cast <option*> (optlist->find(&CS));
        return find;
    }

    DCASSERT(sortedOptions);
    // binary search
    unsigned low = 0;
    unsigned high = numOptions;
    while (low < high) {
        unsigned mid = (low+high)/2;
        int cmp = sortedOptions[mid]->Compare(&CS);
        if (0==cmp) return sortedOptions[mid];
        if (cmp>0) {
            high = mid;
        } else {
            low = mid+1;
        }
    }
    // not found
    return nullptr;
}

unsigned option_manager::NumOptions() const
{
    return numOptions;
}

option* option_manager::GetOptionNumber(unsigned i) const
{
    if (i>=numOptions) return nullptr;
    if (!sortedOptions) return nullptr;
    return sortedOptions[i];
}

void option_manager::DocumentOptions(doc_formatter &df,
        const char* keyword) const
{
    DCASSERT(sortedOptions);
    for (unsigned i=0; i<numOptions; i++) {
        if (sortedOptions[i]->isApropos(df, keyword)) {
            df.Out() << "\n";
            sortedOptions[i]->PrintDocs(df, keyword);
        }
    }
}

void option_manager::ListOptions(doc_formatter &df) const
{
    DCASSERT(sortedOptions);
    for (unsigned i=0; i<numOptions; i++) {
#ifndef DEVELOPMENT_CODE
        if (sortedOptions[i]->IsUndocumented())  continue;
#endif
        sortedOptions[i]->ShowCurrent(df.Out());
        df.Out() << "\n";
    }
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
        const char* &v)
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


option* option_manager::addOption(option *o)
{
    DCASSERT(!sortedOptions);
    DCASSERT(optlist);
    optlist->insert(o);
    return o;
}

