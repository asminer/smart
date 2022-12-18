
#include "init_opts.h"
#include "../Options/options.h"
#include "../Options/optman.h"
#include "../Options/checklist.h"

// **********************************************************************
// *                                                                    *
// *                optman_initializer class and methods                *
// *                                                                    *
// **********************************************************************

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
static optman_initializer the_optman_initializer;

optman_initializer::optman_initializer()
    : initializer("optman_initializer", 1, 0)
{
    builds_resource(0, "OM");
}

void optman_initializer::execute()
{
    option_manager* OM = getGlobalOptionManager();
    if (!OM) {
        internal_error E(__FILE__, __LINE__);
        E << "getGlobalOptionManager() returned null pointer";
    }
    set_object(0, OM, "OM");
}


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
    option_manager* OM = dynamic_cast <option_manager*> (get_object(1, "OM"));

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

#if 0

/**
    Initializer for a group of items, in a checklist option.
*/
class checklistgroup_initializer : public initializer {
        const char* gname;
        const char* gdoc;
        unsigned gitems;
    public:
        /** Initialize a checklist group.
                @param  main    Main checklist option name, e.g., "Warning"
                @param  ni      Max number of items in the group
                @param  name    Group name, e.g., "pn_ALL"
                @param  doc     Group documentation
        */
        checklistgroup_initializer(const char* main, unsigned ni,
                const char* name, const char* doc);
    protected:
        virtual void execute();
    public:
        // This is what execute() does,
        // but a parameterized version in case we can't use
        // this object directly.
        static shared_object* exec(shared_object* main, unsigned items,
                const char* name, const char* doc);
};


checklistgroup_initializer::checklistgroup_initializer(const char* main,
    unsigned ni, const char* name, const char* doc)
    : initializer("checklistgroup_initializer", 1, 1)
{
    gitems = ni;
    gname = name;
    gdoc = doc;

    builds_resource(0, gname);
    needs_resource(1, main);
}

void checklistgroup_initializer::execute()
{
    shared_object* obj = exec(get_object(1), gitems, gname, gdoc);
    DCASSERT(obj);
    set_object(0, obj);
}

shared_object* checklistgroup_initializer::exec(shared_object* _main,
    unsigned items, const char* name, const char* doc)
{
    option* main = dynamic_cast <option*> (_main);
    if (!main) return nullptr;
    return main->addChecklistGroup(name, doc, items);
}

#endif

shared_object* initialize_group(shared_object* _main, unsigned items,
        const char* name, const char* doc)
{
    option* main = dynamic_cast <option*> (_main);
    if (!main) return nullptr;
    return main->addChecklistGroup(name, doc, items);
}


// **********************************************************************
// *                                                                    *
// *                    message_initializer  methods                    *
// *                                                                    *
// **********************************************************************

message_initializer::message_initializer(const char* g, switchable_msg &m,
    const char* name, const char* d)
    : initializer("message_initializer", 1, 2), msg(m)
{
    m.setName(name);
    doc = d;

    builds_resource(0, m.getName());
    needs_resource(1, m.optName());
    needs_resource(2, g);
}

message_initializer::message_initializer(switchable_msg &m,
    const char* name, const char* d)
    : initializer("message_initializer", 1, 2), msg(m)
{
    m.setName(name);
    doc = d;

    builds_resource(0, m.getName());
    needs_resource(1, m.optName());
}

void message_initializer::execute()
{
    shared_object* obj =
        initialize_msg(msg, nullptr, doc, get_object(1), get_object(2));
    DCASSERT(obj);
    set_object(0, obj);
}

shared_object* initialize_msg(switchable_msg &msg, const char* name,
        const char* doc, shared_object* _opt, shared_object* _grp)
{
    if (name) msg.setName(name);
    option* opt = dynamic_cast<option*> (_opt);
    if (!opt) return nullptr;
    checklist_enum* grp = dynamic_cast<checklist_enum*> (_grp);
    // No problem if grp is null
    return opt->addChecklistItem(grp, msg.getName(), doc, msg.Active());
}

