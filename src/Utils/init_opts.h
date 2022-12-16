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
                @param  name    Group name, e.g., "pn_ALL"
                @param  doc     Group documentation
                @param  ni      Max number of items in the group
        */
        checklistgroup_initializer(const char* main, const char* name,
                const char* doc, unsigned ni);
    protected:
        virtual void execute();
};


/**
    Initializer for a switchable message.
*/
class message_initializer : public initializer {
        switchable_msg &msg;
        const char* doc;
        const char* group;
    public:
        /** Initialize a switchable message.
                @param  m       The message
                @param  doc     Documentation
                @param  group   Group it belongs to, or null
        */
        message_initializer(switchable_msg &m, const char* doc,
                const char* gr=nullptr);
    protected:
        virtual void execute();
};

#endif
