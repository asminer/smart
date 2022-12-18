
#include "../include/defines.h"
#include "../Utils/textfmt.h"
#include "../Utils/messages.h"
#include "radio_opt.h"
#include "optman.h"
#include <cstring>

// **************************************************************************
// *                          radio_button methods                          *
// **************************************************************************

radio_button::radio_button(const char* n, const char* d, unsigned ndx)
    : option_enum(n, d)
{
    index = ndx;
}

bool radio_button::AssignToMe()
{
    return true;
}

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

option::error radio_opt::SetValue(option_enum* x)
{
    radio_button* rb = smart_cast <radio_button*> (x);
    if (0==rb) return WrongType;
    if (rb->getIndex() >= numpossible) return WrongType;
    if (rb->getIndex() == which) return Success;
    if (! rb->AssignToMe()) return WrongType;
    which = rb->getIndex();
    return notifyWatchers();
}

unsigned radio_opt::NumConstants() const
{
    return numpossible;
}

option_enum* radio_opt::GetConstant(unsigned i) const
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

void radio_opt::Finish()
{
    DCASSERT(numadded == numpossible);

    //
    // Sanity check: indexes are unique
    //
    for (unsigned i=0; i<numpossible; i++) {
        unsigned c = countButtonsWithIndex(possible[i]->getIndex());
        if (c!=1) {
            internal_error E(__FILE__, __LINE__);
            E << "Option " << *this << " has " << c << " buttons with the same index";
            E.newLine();
            for (unsigned j=0; j<numpossible; j++) {
                if (possible[j]->getIndex() == possible[i]->getIndex()) {
                    E << "    " << *possible[j];
                    E.newLine();
                }
            }
        }
    }

    //
    // Press the current button
    //
    unsigned i = findButtonWithIndex(which);
    if (i >= numpossible) {
        internal_error E(__FILE__, __LINE__);
        E << "Option " << *this << " set to value " << which;
        E << " with no matching button";
    }

    possible[i]->AssignToMe();
}

void radio_opt::ShowHeader(std::ostream &s) const
{
    unsigned i = findButtonWithIndex(which);
    s << *this << ' ' << possible[i]->Name();
}

void radio_opt::ShowRange(doc_formatter &df) const
{
    DCASSERT(numpossible);
    df.Out() << "Legal values:";
    unsigned i;
    unsigned maxenum = 0;
    for (i=0; i<numpossible; i++)  {
        unsigned l = strlen(possible[i]->Name());
        maxenum = MAX(maxenum, l);
    }
    df.begin_description(maxenum);
    for (i=0; i<numpossible; i++) {
        df.item(possible[i]->Name());
        df.Out() << possible[i]->Documentation();
    }
    df.end_description();
}

bool radio_opt::isApropos(const doc_formatter &df, const char* keyword) const
{
    if (df.Matches(Name(), keyword))   return true;
    for (unsigned i=0; i<numpossible; i++) {
        if (possible[i]->isApropos(df, keyword)) return true;
    }
    return false;
}

void radio_opt::RecurseDocs(doc_formatter &df, const char* keyword) const
{
    for (unsigned i=0; i<numpossible; i++) {
        if (0==possible[i]->readSettings()) continue;

        if (df.Matches(possible[i]->Name(), keyword)) {
            // Print all options
            df.Out() << "\nAll settings for " << possible[i]->Name() << ":\n";
            df.begin_indent();
            possible[i]->readSettings()->DocumentOptions(df, 0);
            df.end_indent();
            continue;
        }

        // Ok, just print matching settings, if any
        df.Out() << "\nMatching settings for " << possible[i]->Name() << ":\n";
        df.begin_indent();
        possible[i]->readSettings()->DocumentOptions(df, keyword);
        df.end_indent();
    }
}

radio_button* radio_opt::
addRadioButton(const char* n, const char* d, unsigned ndx)
{
    if (numadded >= numpossible) return 0;

    // Insertion sort; for now we don't have any radio options
    // with more than a dozen choices so no worries about inefficiency.

    for (unsigned i=numadded; i; i--) {
        int cmp = strcmp(possible[i-1]->Name(), n);
        if (0==cmp) {
            return 0;
        }
        if (cmp<0) {
            // element i-1 is less than this one, so it can go in slot i.
            possible[i] = new radio_button(n, d, ndx);
            ++numadded;
            return possible[i];
        }
        // element i-1 is greater than this one, move it and keep looking
        possible[i] = possible[i-1];
        possible[i-1] = 0;
    }
    // New element is the smallest, add it to the front.
    possible[0] = new radio_button(n, d, ndx);
    ++numadded;
    return possible[0];
}

unsigned radio_opt::findButtonWithIndex(unsigned ndx) const
{
    for (unsigned i=0; i<numpossible; i++) {
        if (possible[i]->getIndex() == ndx) return i;
    }
    return numpossible;
}

unsigned radio_opt::countButtonsWithIndex(unsigned ndx) const
{
    unsigned count = 0;
    for (unsigned i=0; i<numpossible; i++) {
        if (possible[i]->getIndex() == ndx) ++count;
    }
    return count;
}

