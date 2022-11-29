
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

outputStream warning_msg::Warning(std::cerr);

warning_msg::warning_msg() : abstract_msg("Warning")
{
    Activate();
}

bool warning_msg::startWarning(const location &L) const
{
    if (!isActive()) return false;
    Warning.out() << "WARNING";
    if (L) Warning.out() << ' ' << L;
    Warning.out() << ":\n    ";
    return true;
}

// ******************************************************************
// *                     reporting_msg  methods                     *
// ******************************************************************

outputStream reporting_msg::Report(std::cout);

reporting_msg::reporting_msg() : abstract_msg("Report")
{
    Deactivate();
}

bool reporting_msg::startReport() const
{
    if (!isActive()) return false;
    Report.out() << 'R' << getName() << ": ";
    return true;
}



// ******************************************************************
// *                     debugging_msg  methods                     *
// ******************************************************************

outputStream debugging_msg::Debug(std::cerr);

debugging_msg::debugging_msg() : abstract_msg("Debug")
{
    Deactivate();
}

bool debugging_msg::startDebug() const
{
    if (!isActive()) return false;
    Debug.out() << 'D' << getName() << ": ";
    return true;
}


// ******************************************************************
// *                       error_msg  methods                       *
// ******************************************************************

error_msg::error_msg(const char* prefix)
{
    if (prefix) Error.out() << prefix;
}

error_msg::~error_msg()
{
    Error.out() << std::endl;
}

// ******************************************************************
// *                     internal_error methods                     *
// ******************************************************************

internal_error::internal_error(const char* sfile, unsigned sline)
    : error_msg("INTERNAL in file ")
{
    err() << sfile << " on line " << sline << ":";
    newLine();
}

internal_error::internal_error(const char* sfile, unsigned sline,
        const location &w) : error_msg("INTERNAL in file ")
{
    err() << sfile << " on line " << sline;
    if (w) {
        newLine();
        err() << "caused " << w;
    }
    err() << ":";
    newLine();
}

internal_error::~internal_error()
{
    err() << std::endl;
    signal_manager::clean_exit(1);
}

// ******************************************************************
// *                   typechecking_error methods                   *
// ******************************************************************

typechecking_error::typechecking_error(const location &W)
    : error_msg("ERROR")
{
    if (W) {
        err() << ' ' << W;
    }
    err() << ':';
    newLine();
}

