
#include "help.h"
#include "functions.h"

#include "../Utils/textfmt.h"
#include "../Options/options.h"

// ******************************************************************
// *                                                                *
// *                       help_topic methods                       *
// *                                                                *
// ******************************************************************

help_topic::help_topic(const char* n, const char* s)
    : symbol(location::NOWHERE(), (typelist*) 0, strdup(n))
{
    summary = s;
}

help_topic::help_topic()
    : symbol(location::NOWHERE(), (typelist*) 0, 0)
{
    summary = 0;
}

help_topic::~help_topic()
{
}

void help_topic::setName(char* n)
{
    Rename(new shared_string(n));
}

void help_topic::setName(const std::string &n)
{
    Rename(new shared_string(n));
}


bool help_topic::DocumentHeader(doc_formatter &df) const
{
    df.begin_heading();
    df.Out() << "Help topic: " << Name();
    df.end_heading();
    return true;
}

// ******************************************************************
// *                                                                *
// *                       help_group methods                       *
// *                                                                *
// ******************************************************************

help_group::help_group(const char* n, const char* s, const char* d)
    : help_topic(n, s)
{
    DCASSERT(d);
    docs = d;
}

help_group::~help_group()
{
}

void help_group::PrintDocs(doc_formatter &df, const char* keyword) const
{
    if (!DocumentHeader(df)) return;
    df.begin_indent();
    df.Out() << docs << "\n";
    if (funcs.Length()) {
        df.Out() << "\nRelevant functions:\n";
        df.begin_indent();
        for (int i=0; i<funcs.Length(); i++) {
            const function* f = funcs.ReadItem(i);
            f->PrintHeader(df.Out(), true);
            df.Out() << "\n";
        }
        df.end_indent();
    }
    if (options.Length()) {
        df.Out() << "\nRelevant options:\n";
        df.begin_indent();
        for (int i=0; i<options.Length(); i++) {
            const option* o = options.ReadItem(i);
            o->PrintDocs(df, keyword);
        }
        df.end_indent();
    }
    df.end_indent();
}

