
#include "strings.h"
#include "outstream.h"

#include <cstring>
#include <cstdlib>
#include <iomanip>

static char* build_printable(char* raw)
{
    if (nullptr == raw) return nullptr;
    unsigned rawlen = 0;
    unsigned special = 0;

    //
    // Check for special chars and determine
    // length of printable copy.
    //
    for (; raw[rawlen]; rawlen++) {
        if (raw[rawlen] != '\\' ) continue;
        // special char
        ++rawlen;
        if (0==raw[rawlen]) break;

        if (    'a' == raw[rawlen] ||
                'b' == raw[rawlen] ||
                'n' == raw[rawlen] ||
                'q' == raw[rawlen] ||
                't' == raw[rawlen] ||
                '\\' == raw[rawlen] )   ++special;
    }

    if (0==special) return raw;

    //
    // Build a version of the string
    // with special chars in place
    //

    char* print = new char[rawlen+1-special];

    unsigned p=0;
    for (unsigned i=0; raw[i]; i++) {
        if (raw[i] != '\\') {
            print[p++] = raw[i];
            continue;
        }
        ++i;
        if (0==raw[i]) {
            print[p++] = '\\';
            break;
        }
        switch (raw[i]) {
            case 'a'  :  print[p++] = '\a';     break;
            case 'b'  :  print[p++] = '\b';     break;
            case 'n'  :  print[p++] = '\n';     break;
            case 'q'  :  print[p++] = '"';      break;
            case 't'  :  print[p++] = '\t';     break;
            case '\\' :  print[p++] = '\\';     break;
        }
    }
    print[p] = 0;
    return print;
}

// ******************************************************************
// *                                                                *
// *                     shared_string  methods                     *
// *                                                                *
// ******************************************************************

shared_string::shared_string() : shared_object()
{
    string = nullptr;
    printable = nullptr;
}

shared_string::shared_string(const char* s) : shared_object()
{
    string = strdup(s);
    printable = build_printable(string);
}

shared_string::shared_string(const std::string &s) : shared_object()
{
    string = strdup(s.c_str());
    printable = build_printable(string);
}

shared_string::~shared_string()
{
    if (printable && printable != string) {
        delete[] printable;
    }
    free(string);
}

unsigned shared_string::length() const
{
    if (string) return strlen(string);
    return 0;
}

bool shared_string::Print(std::ostream &s, int indent) const
{
    DCASSERT(printable);
    if (indent < 0) {
        s << std::setw(-indent) << std::left << printable;
    } else {
        s << std::setw(indent) << std::right << printable;
    }

    return true;
}

bool shared_string::Equals(const shared_object* o) const
{
    if (o==this) return true;
    const shared_string* s = dynamic_cast <const shared_string*> (o);
    if (0==s) return false;
    if ( (0==string) && (0==s->string) ) return true;
    if ( (0==string) || (0==s->string) ) return false;
    return (0==strcmp(string, s->string));
}

int shared_string::Compare(const shared_string* s) const
{
    if (0==s) return 1;
    if ( (0==string) && (0==s->string) ) return 0;
    if (0==string) return -1;
    if (0==s->string) return 1;
    return strcmp(string, s->string);
}

int shared_string::Compare(const char* s) const
{
    if ( (0==string) && (0==s) ) return 0;
    if (0==string) return -1;
    if (0==s) return 1;
    return strcmp(string, s);
}

