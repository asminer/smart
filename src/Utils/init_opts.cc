
#include "init_opts.h"
#include "../Options/optman.h"

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
        unsigned handle;
    public:
        optman_initializer();
    protected:
        virtual void execute();
};

optman_initializer::optman_initializer()
    : initializer("optman_initializer", 1, 0)
{
    handle = builds_resource("OM");
    OM = nullptr;
}

void optman_initializer::execute()
{
    OM = MakeOptionManager();
    if (!OM) {
        internal_error E(__FILE__, __LINE__);
        E << "MakeOptionManager() returned null pointer";
    }
    set_build_object(handle, OM);
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

        unsigned om_handle;
        unsigned option_handle;
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

    om_handle = needs_resource("OM");
    option_handle = builds_resource(name);
}

void checklist_initializer::execute()
{
    option_manager* OM = dynamic_cast <option_manager*>
        (get_needed_resource(om_handle));

    if (0==OM) return;      // Error out here?

    set_built_resource(option_handle, OM->addChecklistOption(optname, optdoc));
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
    const char* name, const char* doc)
    : initializer("checklistgroup_initializer", 1, 1)
{
    mainopt = main;
    groupname = name;
    groupdoc = doc;

    main_handle = needs_resource(main);
    group_nahdle = builds_resource(name);
}

void checklistgroup_initializer::execute()
{
    option* main = dynamic_cast <option*> (get_needed_resource(main_handle));

    if (0==main) return;      // Error out here?

    set_built_resource(group_handle, main->addChecklistGroup(groupname, groupdoc));
}

