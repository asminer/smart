
#include "../include/defines.h"
#include "opt_enum.h"
#include "options.h"
#include "optman.h"
#include "../Streams/textfmt.h"

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

void option_enum::show(OutputStream &s) const
{
    if (name)   s << name;
    else        s << "(no name)";
}

int option_enum::Compare(const option_enum* b) const
{
    if (b)  return strcmp(Name(), b->Name());
    else    return 1;
}

int option_enum::Compare(const char* n) const
{
    return strcmp(Name(), n);
}

bool option_enum::isApropos(const doc_formatter* df, const char* key) const
{
    if (df->Matches(name, key))  return true;
    if (0==settings) return false;
    for (unsigned n=0; n<settings->NumOptions(); n++) {
        option* rec = settings->GetOptionNumber(n);
        if (rec->isApropos(df, key)) return true;
    }
    return false;
}

