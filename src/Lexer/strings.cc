
#include "../Streams/streams.h"
#include "strings.h"
#include <string.h>
#include <stdlib.h>

// ******************************************************************
// *                                                                *
// *                     shared_string  methods                     *
// *                                                                *
// ******************************************************************

shared_string::shared_string() : shared_object()
{
    string = 0;
}

shared_string::shared_string(char* s) : shared_object()
{
    string = s;
}

shared_string::~shared_string()
{
    free(string);
}

void shared_string::CopyFrom(const char* s)
{
    free(string);
    string = strdup(s);
}

bool shared_string::Print(OutputStream &s, int width) const
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
        s.Put(string, width);
        return true;
    }

    // right justify
    if (width>0) s.Pad(' ', width-stlen+correction);

    // print the string, taking special chars into account
    for (int i=0; i<stlen; i++) {
        if (string[i] != '\\') {
            s.Put(string[i]);
            continue;
        }
        // special char.
        i++;
        if (i>=stlen) break;
        switch (string[i]) {
            case 'a'  :  s.Put('\a'); break;
            case 'b'  :  s.Put('\b'); break;
            case 'f'  :  s.flush();   break;  // does this work?
            case 'n'  :  s.Put('\n'); break;
            case 'q'  :  s.Put('"');  break;
            case 't'  :  s.Put('\t'); break;
            case '\\' :  s.Put('\\'); break;
        }
    }

    // left justify
    if (width<0) s.Pad(' ', correction-width-stlen);

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

