
#include "../include/defines.h"
#include "opt_enum.h"
#include "options.h"
#include "optman.h"
#include "../Utils/textfmt.h"
#include "../Utils/strings.h"

#include <cstring>

// **************************************************************************
// *                                                                        *
// *                          option_const methods                          *
// *                                                                        *
// **************************************************************************

option_enum::option_enum(const char* n, const char* d)
{
    name = n;
    doc = d;
    settings = 0;
}

option_enum::~option_enum()
{
    // don't delete the name or the documentation
    // for now, don't delete the settings
}

bool option_enum::Print(std::ostream &s, int width) const
{
    if (name)   s << name;
    else        s << "(no name)";
    return true;
}

int option_enum::Compare(const shared_object* b) const
{
    const shared_string* ss = dynamic_cast <const shared_string*> (b);
    if (ss) {
        return strcmp(Name(), ss->getStr());
    }
    const const_string* cs = dynamic_cast <const const_string*> (b);
    if (cs) {
        return strcmp(Name(), cs->getStr());
    }
    const option_enum* oe = dynamic_cast <const option_enum*> (b);
    if (oe)  {
        return strcmp(Name(), oe->Name());
    } else {
        return 1;
    }
}

bool option_enum::isApropos(const doc_formatter &df, const char* key) const
{
    if (df.Matches(name, key))  return true;
    if (0==settings) return false;
    for (unsigned n=0; n<settings->NumOptions(); n++) {
        option* rec = settings->GetOptionNumber(n);
        if (rec->isApropos(df, key)) return true;
    }
    return false;
}

