
#include "strings.h"
#include "outstream.h"

#include <cstring>
#include <cstdlib>
#include <iomanip>

// ******************************************************************
// *                                                                *
// *                     shared_string  methods                     *
// *                                                                *
// ******************************************************************

shared_string::shared_string() : shared_object()
{
    string = 0;
}

shared_string::shared_string(const char* s) : shared_object()
{
    string = strdup(s);
}

shared_string::shared_string(const std::string &s) : shared_object()
{
    string = strdup(s.c_str());
}

shared_string::~shared_string()
{
    free(string);
}

unsigned shared_string::length() const
{
    if (string) return strlen(string);
    return 0;
}

bool shared_string::Print(std::ostream &s, int indent) const
{
    DCASSERT(string);

    s << std::setw(indent) << "";

    unsigned i;
    for (i=0; string[i]; i++) {
        if (string[i] != '\\') {
            s << string[i];
            continue;
        }
        // special char.
        i++;
        if (0==string[i]) break;
        switch (string[i]) {
            case 'a'  :  s << '\a'; break;
            case 'b'  :  s << '\b'; break;
            case 'n'  :  s << '\n'; break;
            case 'q'  :  s << '"';  break;
            case 't'  :  s << '\t'; break;
            case '\\' :  s << '\\'; break;
        }
    }


    /*
     *  OLD IMPLEMENTATION
     *
    DCASSERT(string);

    int stlen = strlen(string);
    bool has_special = false;
    int correction = 0;

    // check if there are any special characters
    for (int i=0; i<stlen; i++) if ('\\' == string[i]) {
        // handle a "\x" sequence for some x.
        i++;
        if (i<stlen) switch (string[i]) {
            case 'a'  :
            case 'b'  :
            case 'f'  :
            case 'n'  :
            case 't'  :
                has_special = true;
                break;
            default:
                correction++;
        }
    }
    if (has_special) width = 0;  // don't try to line it up

    // nice trick: if no special chars, just print it!
    if (0==correction && 0==has_special) {
        s << std::setw(width) << string;
        return true;
    }

    // right justify
    if (width>0) {
        Pad(s, ' ', width-stlen+correction);
    }

    // print the string, taking special chars into account
    for (int i=0; i<stlen; i++) {
        if (string[i] != '\\') {
            s << string[i];
            continue;
        }
        // special char.
        i++;
        if (i>=stlen) break;
        switch (string[i]) {
            case 'a'  :  s << '\a'; break;
            case 'b'  :  s << '\b'; break;
            case 'f'  :  s.flush(); break;  // does this work?
            case 'n'  :  s << '\n'; break;
            case 'q'  :  s << '"';  break;
            case 't'  :  s << '\t'; break;
            case '\\' :  s << '\\'; break;
        }
    }

    // left justify
    if (width<0) {
        Pad(s, ' ', correction-width-stlen);
    }
    */

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

