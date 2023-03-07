
#include "messages.h"

#include "location.h"
#include "sigman.h"

#include <cstdio>

// #define DEBUG_FILE

// io_environ* switchable_msg::io = 0;

// ******************************************************************
// *                     switchable_msg methods                     *
// ******************************************************************

switchable_msg::switchable_msg(const char* optname)
{
    option_name = optname;
    my_name = nullptr;
}

// ******************************************************************
// *                      warning_msg  methods                      *
// ******************************************************************

outputStream warning_msg::Out(std::cerr);

warning_msg::warning_msg() : switchable_msg("Warning")
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

named_msg::named_msg(const char* optn) : switchable_msg(optn)
{
}

bool named_msg::start(outputStream &out) const
{
    if (!isActive()) return false;
    out.activate();
    out.clearIndent();
    out << "***** " << getName() << " " << optName() << " *****\n";
    out.incIndent();
    return true;
}

void named_msg::stop(outputStream &out, bool newline) const
{
    if (!isActive()) return;
    if (newline) out.stream() << std::endl;
    out.deactivate();
}

// ******************************************************************
// *                     reporting_msg  methods                     *
// ******************************************************************

outputStream reporting_msg::Out(std::cout);

reporting_msg::reporting_msg() : named_msg("Report")
{
    Deactivate();
}

// ******************************************************************
// *                     debugging_msg  methods                     *
// ******************************************************************

outputStream debugging_msg::Out(std::cerr);

debugging_msg::debugging_msg() : named_msg("Debug")
{
    Deactivate();
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

