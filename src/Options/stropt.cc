
#include "../include/defines.h"
#include "../Utils/textfmt.h"
#include "../Utils/strings.h"
#include "stropt.h"

string_opt::string_opt(const char* n, const char* d, const char* &L)
    : option(String, n, d), link(L)
{
    if (link)   value = new shared_string(link);
    else        value = nullptr;
}

string_opt::~string_opt()
{
    Delete(value);
}

option::error string_opt::SetValue(shared_string* v)
{
    if (0==v)      return RangeError;
    Delete(value);
    value = Share(v);
    if (value)  link = value->getStr();
    else        link = nullptr;
    return notifyWatchers();
}

void string_opt::ShowHeader(std::ostream &s) const
{
    s << *this << ' ';
    if (value) s << '"' << value->getStr() << '"';
    else s << "null";
}

void string_opt::ShowRange(doc_formatter &df) const
{
    df.Out() << "Legal values: any string";
}

