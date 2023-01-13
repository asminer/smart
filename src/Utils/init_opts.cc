
#include "init_opts.h"
#include "../Options/options.h"
#include "../Options/optman.h"
#include "../Options/checklist.h"

// **********************************************************************
// *                                                                    *
// *              checklist_initializer  class and methods              *
// *                                                                    *
// **********************************************************************

/**
    Build a checklist option for switchable messages.
    These are options like "Report" and "Debug"
    with lots of suboptions.

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
    : initializer("checklist_initializer", 1)
{
    optname = name;
    optdoc = doc;

    builds_resource(optname);
}

void checklist_initializer::execute()
{
    set_object(optname,
        option_manager::global().addChecklistOption(optname, optdoc)
    );
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
    const char* n, const char* d)
    : initializer("message_initializer", 3), msg(m)
{
    msg.setName(n);
    doc = d;

    builds_resource(msg.getName());
    needs_resource(msg.optName());
    needs_resource(g);

    group = g;
}

message_initializer::message_initializer(switchable_msg &m,
    const char* n, const char* d)
    : initializer("message_initializer", 2), msg(m)
{
    msg.setName(n);
    doc = d;

    builds_resource(msg.getName());
    needs_resource(msg.optName());

    group = nullptr;
}

void message_initializer::execute()
{
    shared_object* obj = initialize_msg(
        msg, nullptr, doc, get_object(msg.optName()), get_object(group)
    );
    DCASSERT(obj);
    set_object(msg.getName(), obj);
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

