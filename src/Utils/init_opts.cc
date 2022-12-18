
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

// **********************************************************************
// *                                                                    *
// *                    Stream option initialization                    *
// *                                                                    *
// **********************************************************************

/*
class real_format_watch : public option::watcher {
        outputStream& stream;
    public:
        unsigned link;
    public:
        real_format_watch(outputStream& s) : stream(s) {
            link = stream.get_real_format();
        }
        virtual void notify(const option*) {
            stream.set_real_format(link);
        }
};

class stream_option_init : public initializer {
    public:
        stream_option_init();
    protected:
        virtual void execute();

        void buildRealOption(option_manager* om, outputStream &out,
                const char* name, const char* doc);
};
static stream_option_init the_stream_option_init;

stream_option_init::stream_option_init()
    : initializer("stream_option_init", 1, 1)
{
    builds_resource(0, "stream_options");
    needs_resource(1, "OM");
}

void stream_option_init::execute()
{
    option_manager* om = dynamic_cast <option_manager*> (get_object(1, "OM"));
    if (!om) return;

    //
    // Real format option
    //

    buildRealOption(om, outputStream::globalOut(), "OutputRealFormat",
            "Format to use for writing reals to the output stream");
    buildRealOption(om, reporting_msg::Out, "ReportRealFormat",
            "Format to use for writing reals to the reporting stream");

    //
    // Thousands separator option
    //

    om->addStringOption(
        "OutputThousandSeparator",
        "Thousands separator to use for the output stream",
        outputStream::globalOut().comma
    );
    om->addStringOption(
        "ReportThousandSeparator",
        "Thousands separator to use for the reporting stream",
        reporting_msg::Out.comma
    );
}

void stream_option_init::buildRealOption(option_manager* om, outputStream &out,
        const char* name, const char* doc)
{
    real_format_watch *w = new real_format_watch(out);
    option* rbo = om->addRadioOption(name, doc, 3, w->link);
    rbo->registerWatcher(w);

    rbo->addRadioButton("FIXED", "Same as printf(%f)", outputStream::FIXED);
    rbo->addRadioButton("GENERAL", "Same as printf(%g)", outputStream::GENERAL);
    rbo->addRadioButton("SCIENTIFIC", "Same as printf(%e)", outputStream::SCIENTIFIC);
    rbo->Finish();
}
*/
