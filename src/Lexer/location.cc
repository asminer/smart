
#include "location.h"
#include "../Streams/streams.h"

location::location()
{
    filename = 0;
    linenumber = 0;
}

location::location(shared_string* fn,  unsigned ln)
{
    filename = Share(fn);
    linenumber = ln;
}

location::location(const location& L)
{
    filename = Share(L.filename);
    linenumber = L.linenumber;
}

void location::operator=(const location& L)
{
    if (filename != L.filename) {
	    Delete(filename);
	    filename = Share(L.filename);
    }
    linenumber = L.linenumber;
}

location::~location()
{
    Delete(filename);
}

void location::show(OutputStream &s) const
{
    if (0==filename) return;
    const char* fn = filename->getStr();

    unsigned long ln = linenumber;

    if (0==fn[1]) {
        // special files
        switch (fn[0]) {
            case '-':
                s.Put("in standard input");
                break;

            case '>':
                s.Put("on command line");
				ln = 0;
                break;

            case '<':
                s.Put("at end of input");
				ln = 0;
                break;

            default:
                s.Put("in file");
                s.Put(fn);
        } // switch
    } else if (' ' == fn[0]) {
        s.Put(fn+1);
    } else {
        s.Put("in file ");
        s.Put(fn);
    }
    if (ln) {
        s.Put(" near line ");
        s.Put(ln);
    }
}
