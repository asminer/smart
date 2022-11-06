
#include "../include/defines.h"
#include "../Streams/streams.h"
#include "intopt.h"

int_opt::int_opt(const char* n, const char* d, long &v, long mn, long mx)
    : option(Integer, n ,d), value(v)
{
    max = mx;
    min = mn;
}

int_opt::~int_opt()
{
}

option::error int_opt::SetValue(long b)
{
    if (b == value) return Success;
    if ((b<min) || (b>max)) return RangeError;
    value = b;
    return notifyWatchers();
}

void int_opt::ShowHeader(OutputStream &s) const
{
    show(s);
    s << " " << value;
}

void int_opt::ShowRange(doc_formatter* df) const
{
    DCASSERT(df);
    df->Out() << "Legal values: ";
    df->Out() << "integers in [" << min << ", ..., " << max << "]\n";
}

