
#include "messages.h"

#include "../Options/optman.h"
#include "../Options/options.h"

#include <cstdio>

// #define DEBUG_FILE

io_environ* abstract_msg::io = 0;

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

void abstract_msg::initStatic(io_environ* _io)
{
    io = _io;
}

// ******************************************************************
// *                      warning_msg  methods                      *
// ******************************************************************

warning_msg::warning_msg() : abstract_msg("Warning")
{
    Activate();
}

// ******************************************************************
// *                     reporting_msg  methods                     *
// ******************************************************************

reporting_msg::reporting_msg() : abstract_msg("Report")
{
    Deactivate();
}


// ******************************************************************
// *                     debugging_msg  methods                     *
// ******************************************************************

debugging_msg::debugging_msg() : abstract_msg("Debug")
{
    Deactivate();
}

