
#include "messages.h"

#include "../Options/optman.h"
#include "../Options/options.h"

io_environ* abstract_msg::io = 0;

// ******************************************************************
// *                      abstract_msg methods                      *
// ******************************************************************

abstract_msg::abstract_msg()
{
    name = 0;
}

// ******************************************************************
// *                      warning_msg  methods                      *
// ******************************************************************

warning_msg::warning_msg()
{
    active = true;
}

bool warning_msg::initialize(const option_manager* om, checklist_enum* grp,
                const char* name, const char* doc)
{
    setName(name);
    if (0==om) return false;
    option* warning = om->FindOption("Warning");
    if (0==warning) return false;
    return warning->addChecklistItem(grp, name, doc, active);
}

// ******************************************************************
// *                     reporting_msg  methods                     *
// ******************************************************************

reporting_msg::reporting_msg()
{
    active = false;
}

bool reporting_msg::initialize(const option_manager* om, checklist_enum* grp,
                const char* name, const char* doc)
{
    setName(name);
    if (0==om) return false;
    option* report = om->FindOption("Report");
    if (0==report) return false;
    return report->addChecklistItem(grp, name, doc, active);
}

// ******************************************************************
// *                     debugging_msg  methods                     *
// ******************************************************************

debugging_msg::debugging_msg()
{
    active = false;
}

bool debugging_msg::initialize(const option_manager* om, checklist_enum* grp,
                const char* name, const char* doc)
{
    setName(name);
    if (0==om) return false;
    option* debug = om->FindOption("Debug");
    if (0==debug) return false;
    return debug->addChecklistItem(grp, name, doc, active);
}

