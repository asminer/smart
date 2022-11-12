
#include "../Streams/streams.h"
#include "strings.h"
#include <cstring>
#include <cstdlib>

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

shared_string::~shared_string()
{
    free(string);
}

unsigned shared_string::length() const
{
    if (string) return strlen(string);
    return 0;
}

/*
void shared_string::CopyFrom(const char* s)
{
    free(string);
    string = strdup(s);
}
*/

#ifdef OLD_STREAMS
bool shared_string::Print(OutputStream &s, int width) const
#else
bool shared_string::Print(std::ostream &s, int width) const
#endif
{
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
#ifdef OLD_STREAMS
        s.Put(string, width);
#else
        s << std::setw(width) << string;
#endif
        return true;
    }

    // right justify
    if (width>0) {
#ifdef OLD_STREAMS
        s.Pad(' ', width-stlen+correction);
#else
        Pad(s, ' ', width-stlen+correction);
#endif
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
#ifdef OLD_STREAMS
        s.Pad(' ', correction-width-stlen);
#else
        Pad(s, ' ', correction-width-stlen);
#endif
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

