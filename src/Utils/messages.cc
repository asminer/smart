
#include "messages.h"

#include "../Options/optman.h"
#include "../Options/options.h"

#include "location.h"
#include "sigman.h"

#include <cstdio>

// #define DEBUG_FILE

// io_environ* abstract_msg::io = 0;

// ******************************************************************
// *                      abstract_msg methods                      *
// ******************************************************************

abstract_msg::abstract_msg(const char* optname)
{
    name = 0;
    option_name = optname;
}

bool abstract_msg::initialize(const option_manager* om, checklist_enum* grp,
        const char* n, const char* doc)
{
    DCASSERT(0==name);
    name = n;
    if (0==om) return false;
    option* opt = om->FindOption(option_name);
    if (0==opt) return false;
    return opt->addChecklistItem(grp, name, doc, active);
}

// ******************************************************************
// *                      warning_msg  methods                      *
// ******************************************************************

outputStream warning_msg::Out(std::cerr);

warning_msg::warning_msg() : abstract_msg("Warning")
{
    Activate();
}

bool warning_msg::start(const location &L) const
{
    if (!isActive()) return false;
    Out << "WARNING";
    if (L) Out << ' ' << L;
    Out << ":\n    ";
    return true;
}

// ******************************************************************
// *                     reporting_msg  methods                     *
// ******************************************************************

outputStream reporting_msg::Out(std::cout);

reporting_msg::reporting_msg() : abstract_msg("Report")
{
    Deactivate();
}

bool reporting_msg::start() const
{
    if (!isActive()) return false;
    // Build line prefix string
    const char* n = getName();
    prefix[0] = 'R';
    unsigned i;
    for (i=0; i<250; i++) {
        if (0 == n[i]) break;
        prefix[i+1] = n[i];
    }
    prefix[i++] = ':';
    prefix[i++] = ' ';
    prefix[i++] = 0;

    Out << prefix;
    return true;
}



// ******************************************************************
// *                     debugging_msg  methods                     *
// ******************************************************************

outputStream debugging_msg::Out(std::cerr);

debugging_msg::debugging_msg() : abstract_msg("Debug")
{
    Deactivate();
}

bool debugging_msg::start() const
{
    if (!isActive()) return false;
    // Build line prefix string
    const char* n = getName();
    prefix[0] = 'D';
    unsigned i;
    for (i=0; i<250; i++) {
        if (0 == n[i]) break;
        prefix[i+1] = n[i];
    }
    prefix[i++] = ':';
    prefix[i++] = ' ';
    prefix[i++] = 0;

    Out << prefix;
    return true;
}


// ******************************************************************
// *                       error_msg  methods                       *
// ******************************************************************

error_msg::error_msg(const char* prefix)
{
    Out.indentMore();
    if (prefix) Out << prefix;
}

error_msg::~error_msg()
{
    Out.stream() << std::endl;
    Out.clearIndent();
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
// *                   typechecking_error methods                   *
// ******************************************************************

typechecking_error::typechecking_error(const location &W)
    : error_msg("ERROR")
{
    if (W) {
        Out << ' ' << W;
    }
    Out << ':';
    newLine();
}

