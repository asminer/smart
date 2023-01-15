
#include "location.h"
#include <cstring>

location::location()
{
    filename = 0;
    linenumber = 0;
    ltype = ' ';
}

location::location(shared_string* fn, unsigned ln)
{
    filename = Share(fn);
    linenumber = ln;
    ltype = 'f';
}

location::location(const location& L)
{
    filename = Share(L.filename);
    linenumber = L.linenumber;
    ltype = L.ltype;
}

void location::reset(char lt, shared_string* fn, unsigned ln)
{
    if (filename != fn) {
      Delete(filename);
      filename = Share(fn);
    }
    linenumber = ln;
    ltype = lt;
}

location::~location()
{
    Delete(filename);
}

void location::show(std::ostream &s) const
{
    switch (ltype) {
        case 'c':   s << "on command line";     return;
        case 'i':   s << "internally";          return;
        case '$':   s << "at end of input";     return;
        case 'f':   break;
        default :   return; // includes "nowhere" case
    };

    if (filename) s << "in file " << filename->getStr();
    else          s << "in standard input";

    if (linenumber) {
        s << " near line " << long(linenumber);
    }
}

void location::start(const char* file)
{
    Delete(filename);
    filename = file ? new shared_string(file) : 0;
    linenumber = 1;
    ltype = 'f';
}

void location::clear()
{
    Delete(filename);
    filename = 0;
    linenumber = 0;
    ltype = ' ';
}

const location& location::CMDLINE()
{
    static location L;
    L.ltype = 'c';
    return L;
}

const location& location::EOINPUT()
{
    static location L;
    L.ltype = '$';
    return L;
}

const location& location::NOWHERE()
{
    static location L;
    L.ltype = ' ';
    return L;
}

const location& location::INTERNALLY()
{
    static location L;
    L.ltype = 'i';
    return L;
}

