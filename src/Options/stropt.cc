
#include "../include/defines.h"
#include "../Streams/streams.h"
#include "../Utils/strings.h"
#include "stropt.h"

string_opt::string_opt(const char* n, const char* d, shared_string* &v)
    : option(String, n, d), value(v)
{
}

string_opt::~string_opt()
{
    Nullify(value);
}

option::error string_opt::SetValue(shared_string* v)
{
    if (0==v)      return RangeError;
    Delete(value);
    value = Share(v);
    return notifyWatchers();
}

option::error string_opt::GetValue(shared_string* &v) const
{
    Delete(v);
    v = Share(value);
    return Success;
}

void string_opt::ShowHeader(OutputStream &s) const
{
    show(s);
    s.Put(' ');
    if (value) s << '"' << value->getStr() << '"';
    else s << "null";
}

void string_opt::ShowRange(doc_formatter* df) const
{
    DCASSERT(df);
    df->Out() << "Legal values: any string";
}

