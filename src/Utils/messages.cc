
#include "messages.h"

#include "location.h"
#include "sigman.h"

#include <cstdio>

// #define DEBUG_FILE

// io_environ* switchable_msg::io = 0;

// ******************************************************************
// *                     switchable_msg methods                     *
// ******************************************************************

switchable_msg::switchable_msg(const char* optname, const char* myname)
{
    option_name = optname;
    my_name = myname;
}

// ******************************************************************
// *                      warning_msg  methods                      *
// ******************************************************************

outputStream warning_msg::Out(std::cerr);

warning_msg::warning_msg(const char* myname)
    : switchable_msg("Warning", myname)
{
    Activate();
}

bool warning_msg::start(const location &L) const
{
    if (!isActive()) return false;
    outputStream::globalOut().flush();
    Out.activate();
    Out.clearIndent(1);
    Out << "WARNING";
    if (L) Out << ' ' << L;
    Out << ':';
    Out.newLine();
    return true;
}

// ******************************************************************
// *                       named_msg  methods                       *
// ******************************************************************

char named_msg::prefix[256];

const char* named_msg::setPrefix(char x) const
{
    prefix[0] = 'x';
    unsigned i;
    for (i=0; i<250; i++) {
        if (0 == getName()[i]) break;
        prefix[i+1] = getName()[i];
    }
    prefix[i++] = ':';
    prefix[i++] = ' ';
    prefix[i++] = 0;
    return prefix;
}

named_msg::named_msg(const char* optn, const char* myn)
    : switchable_msg(optn, myn)
{
}


// ******************************************************************
// *                     reporting_msg  methods                     *
// ******************************************************************

outputStream reporting_msg::Out(std::cout);

reporting_msg::reporting_msg(const char* myname)
    : named_msg("Report", myname)
{
    Deactivate();
}

bool reporting_msg::start() const
{
    if (!isActive()) return false;
    Out.activate();
    Out.clearIndent();
    Out << setPrefix('R');
    return true;
}



// ******************************************************************
// *                     debugging_msg  methods                     *
// ******************************************************************

outputStream debugging_msg::Out(std::cerr);

debugging_msg::debugging_msg(const char* myname)
    : named_msg("Debug", myname)
{
    Deactivate();
}

bool debugging_msg::start() const
{
    if (!isActive()) return false;
    Out.activate();
    Out.clearIndent();
    Out << setPrefix('D');
    return true;
}


// ******************************************************************
// *                       error_msg  methods                       *
// ******************************************************************

outputStream error_msg::Out(std::cerr);

error_msg::error_msg(const char* prefix)
{
    outputStream::globalOut().flush();
    Out.activate();
    Out.clearIndent(1);
    if (prefix) Out << prefix;
}

error_msg::~error_msg()
{
    Out.stream() << std::endl;
    Out.deactivate();
}

// ******************************************************************
// *                     internal_error methods                     *
// ******************************************************************

internal_error::internal_error(const char* sfile, unsigned sline)
    : error_msg("INTERNAL in file ")
{
    Out << sfile << " on line " << sline << ":";
    newLine();
}

internal_error::internal_error(const char* sfile, unsigned sline,
        const location &w) : error_msg("INTERNAL in file ")
{
    Out << sfile << " on line " << sline;
    if (w) {
        newLine();
        Out << "caused " << w;
    }
    Out << ":";
    newLine();
}

internal_error::~internal_error()
{
    Out.stream() << std::endl;
    signal_manager::clean_exit(1);
}

// ******************************************************************
// *                    unnamed_warning  methods                    *
// ******************************************************************

unnamed_warning::unnamed_warning() : error_msg("WARNING:")
{
    newLine();
}

unnamed_warning::unnamed_warning(const location &L) : error_msg("WARNING ")
{
    Out << L << ':';
    newLine();
}

