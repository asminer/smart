#include "lexer.h"
#include "tokens.h"

#include <cstring>

#define BUFSIZE 1024

lexer::buffer::buffer(const char* fn, buffer* nxt)
{
    next = nxt;
    inchars = new char[BUFSIZE+1];

    L.start(fn);

    if ( strcmp("-", fn) ) {
        infile = fopen(fn, "r");
    } else {
        infile = stdin;
    }
    if (infile) refill();
}

lexer::buffer::~buffer()
{
    delete[] inchars;
}

void lexer::buffer::refill()
{
    DCASSERT(infile);
    ptr = inchars;
    unsigned long got = fread(inchars, 1, BUFSIZE, infile);
    inchars[got] = 0;
    if (feof(infile)) {
        if (stdin != infile) fclose(infile);
        infile = 0;
    }
}

//
// ======================================================================
//

lexer::lexer(const char** fns, unsigned nfs)
{
    topfile = 0;
    filenames = fns;
    numfiles = nfs;
    fileindex = 0;


}
