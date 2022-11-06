
#include "../include/defines.h"
#include "../Streams/streams.h"
#include "boolopt.h"

bool_opt::bool_opt(const char* n, const char* d, bool &v)
        : option(Boolean, n, d), value(v)
{
}

bool_opt::~bool_opt()
{
}

option::error bool_opt::SetValue(bool b)
{
    if (b == value) return Success;
    value = b;
    return notifyWatchers();
}

option::error bool_opt::GetValue(bool &b) const
{
    b = value;
    return Success;
}

void bool_opt::ShowHeader(OutputStream &s) const
{
    show(s);
    s << " " << value;
}

void bool_opt::ShowRange(doc_formatter* df) const
{
    DCASSERT(df);
    df->Out() << "Legal values: [false, true]";
}

