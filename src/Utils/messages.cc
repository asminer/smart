
#include "messages.h"

// TBD: move options out!
// transform initialize into a (friend?) function
#include "../Options/optman.h"
#include "../Options/options.h"

#include "location.h"
#include "sigman.h"

#include <cstdio>

// #define DEBUG_FILE

// io_environ* switchable_msg::io = 0;

// ******************************************************************
// *                     switchable_msg methods                     *
// ******************************************************************

void switchable_msg::setName(const char* n)
{
    // some messages don't need the name
}

switchable_msg::switchable_msg(const char* optname)
{
    option_name = optname;
}

bool switchable_msg::initialize(const option_manager* om, checklist_enum* grp,
        const char* name, const char* doc)
{
    setName(name);
    if (0==om) return false;
    option* opt = om->FindOption(option_name);
    if (0==opt) return false;
    return opt->addChecklistItem(grp, name, doc, active);
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

char named_msg::prefix[256];

void named_msg::setName(const char* n)
{
    DCASSERT(0==name);
    name = n;
}

const char* named_msg::setPrefix(char x) const
{
    prefix[0] = 'x';
    unsigned i;
    for (i=0; i<250; i++) {
        if (0 == name[i]) break;
        prefix[i+1] = name[i];
    }
    prefix[i++] = ':';
    prefix[i++] = ' ';
    prefix[i++] = 0;
    return prefix;
}

named_msg::named_msg(const char* optname) : switchable_msg(optname)
{
    name = 0;
}


// ******************************************************************
// *                     reporting_msg  methods                     *
// ******************************************************************

outputStream reporting_msg::Out(std::cout);

reporting_msg::reporting_msg() : named_msg("Report")
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

debugging_msg::debugging_msg() : named_msg("Debug")
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

