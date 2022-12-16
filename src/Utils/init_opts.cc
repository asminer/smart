
#include "init_opts.h"
#include "../Options/options.h"
#include "../Options/optman.h"
#include "../Options/checklist.h"

// **********************************************************************
// *                                                                    *
// *                optman_initializer class and methods                *
// *                                                                    *
// **********************************************************************

static option_manager* OM;

/**
    Build the global option manager.
    Requires nothing.
    Builds resource OM.
*/
class optman_initializer : public initializer {
    public:
        optman_initializer();
    protected:
        virtual void execute();
};

optman_initializer::optman_initializer()
    : initializer("optman_initializer", 1, 0)
{
    builds_resource(0, "OM");
    OM = nullptr;
}

void optman_initializer::execute()
{
    OM = MakeOptionManager();
    if (!OM) {
        internal_error E(__FILE__, __LINE__);
        E << "MakeOptionManager() returned null pointer";
    }
    set_object(0, OM);
}

static optman_initializer _omi;

// **********************************************************************
// *                                                                    *
// *              checklist_initializer  class and methods              *
// *                                                                    *
// **********************************************************************

/**
    Build a checklist option for switchable messages.
    These are options like "Report" and "Debug"
    with lots of suboptions.

    Requires resource OM.
    Builds resource with name = optname.
*/
class checklist_initializer : public initializer {
        const char* optname;
        const char* optdoc;
    public:
        checklist_initializer(const char* name, const char* doc);
    protected:
        virtual void execute();
};

checklist_initializer::checklist_initializer(const char* name, const char* doc)
    : initializer("checklist_initializer", 1, 1)
{
    optname = name;
    optdoc = doc;

    builds_resource(0, name);
    needs_resource(1, "OM");
}

void checklist_initializer::execute()
{
    option_manager* OM = dynamic_cast <option_manager*> (get_object(1));

    if (0==OM) return;      // Error out here?

    set_object(0, OM->addChecklistOption(optname, optdoc));
}

static checklist_initializer _report_init(
    "Report",
    "Switches to control what reports, if any, are written to the report stream."
);

static checklist_initializer _debug_init(
    "Debug",
    "Switches to control what low-level debugging information, if any, is written to the report stream."
);

static checklist_initializer _warning_init(
    "Warning",
    "Switches to control which warning messages are displayed and which are suppressed."
);


// **********************************************************************
// *                                                                    *
// *                 checklistgroup_initializer methods                 *
// *                                                                    *
// **********************************************************************

checklistgroup_initializer::checklistgroup_initializer(const char* main,
    const char* name, const char* doc, unsigned ni)
    : initializer("checklistgroup_initializer", 1, 1)
{
    gname = name;
    gdoc = doc;
    gitems = ni;

    builds_resource(0, gname);
    needs_resource(1, main);
}

void checklistgroup_initializer::execute()
{
    option* main = dynamic_cast <option*> (get_object(1));

    if (0==main) return;      // Error out here?

    set_object(0, main->addChecklistGroup(gname, gdoc, gitems));
}

// **********************************************************************
// *                                                                    *
// *                    message_initializer  methods                    *
// *                                                                    *
// **********************************************************************

message_initializer::message_initializer(const char* g, switchable_msg &m,
    const char* d) : initializer("message_initializer", 1, 2), msg(m)
{
    doc = d;

    builds_resource(0, m.getName());
    needs_resource(1, m.optName());
    needs_resource(2, g);
}

message_initializer::message_initializer(switchable_msg &m,
    const char* d) : initializer("message_initializer", 1, 2), msg(m)
{
    doc = d;

    builds_resource(0, m.getName());
    needs_resource(1, m.optName());
}

void message_initializer::execute()
{
    option* opt = dynamic_cast<option*> (get_object(1));
    DCASSERT(opt);
    checklist_enum* grp = dynamic_cast<checklist_enum*> (get_object(2));
    // grp may be null, that's ok
    set_object(0, opt->addChecklistItem(grp, msg.getName(), doc, msg.Active()));
}

