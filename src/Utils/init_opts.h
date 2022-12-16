#ifndef INIT_OPTS_H
#define INIT_OPTS_H

#include "initializer.h"
#include "messages.h"

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


/**
    Initializer for a switchable message.
*/
class message_initializer : public initializer {
        switchable_msg &msg;
        const char* doc;
    public:
        /** Initialize a switchable message.
                @param  group   Group the message belongs to
                @param  m       The message
                @param  doc     Documentation
        */
        message_initializer(const char* group, switchable_msg &m, const char* doc);

        /** Initialize a switchable message.
                @param  m       The message
                @param  doc     Documentation
        */
        message_initializer(switchable_msg &m, const char* doc);
    protected:
        virtual void execute();
    public:
        // This is what execute() does,
        // but a parameterized version in case we can't use
        // this object directly.
        static shared_object* exec(switchable_msg &msg, const char* doc,
                shared_object* option, shared_object* group = nullptr);
};

#endif
