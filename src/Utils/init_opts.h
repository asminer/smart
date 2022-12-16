#ifndef INIT_OPTS_H
#define INIT_OPTS_H

#include "initializer.h"
#include "messages.h"

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

/**
    Build a group of items, in a checklist option.

    Requires resource OM.
    Requires resource with name = mainopt.
    Builds resource with name = groupname.
*/
class checklistgroup_initializer : public initializer {
        const char* mainopt;
        const char* groupname;
        const char* groupdoc;
    public:
        checklistgroup_initializer(const char* main, const char* name, const char* doc);
    protected:
        virtual void execute();
};

/**
    Build and add the appropriate initializer(s) for a switchable message.
*/
void addInitializer(switchable_msg &M);

#endif
