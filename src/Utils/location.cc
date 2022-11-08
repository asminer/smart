
#include "location.h"
#include "../Streams/streams.h"

location::location()
{
    filename = 0;
    linenumber = 0;
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

#ifdef OLD_STREAMS
void location::show(OutputStream &s) const
#else
void location::show(std::ostream &s) const
#endif
{
    if (0==filename) return;

    const char* fn = filename->getStr();
    unsigned long ln = linenumber;

    if (0==fn[1]) {
        // special files
        switch (fn[0]) {
            case '-':
                s << "in standard input";
                break;

            case '>':
                s << "on command line";
				ln = 0;
                break;

            case '<':
                s << "at end of input";
				ln = 0;
                break;

            default:
                s << "in file " << fn;
        } // switch

    } else if (' ' == fn[0]) {
        s << fn+1;
    } else {
        s << "in file " << fn;
    }
    if (ln) {
        s << " near line " << ln;
    }
}

void location::start(const char* file)
{
    Delete(filename);
    filename = new shared_string;
    filename->CopyFrom(file);
    linenumber = 1;
}

void location::clear()
{
    Delete(filename);
    filename = 0;
    linenumber = 0;
}

const location& location::NOWHERE()
{
    static location L;
    return L;
}
