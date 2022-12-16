#ifndef INIT_OPTS_H
#define INIT_OPTS_H

#include "initializer.h"
#include "messages.h"

/**
    Initialize a group of checklists,
    and build and return a checklist item for it.

        @param  main    Main option the checklist items are tied to
        @param  item    Number of items (max) in this group
        @param  name    Name of the group
        @param  doc     Documentation
*/
shared_object* initialize_group(shared_object* main, unsigned items,
        const char* name, const char* doc);


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
                @param  name    The message name
                @param  doc     Documentation
        */
        message_initializer(const char* group, switchable_msg &m,
                const char* name, const char* doc);

        /** Initialize a switchable message.
                @param  m       The message
                @param  name    The message name
                @param  doc     Documentation
        */
        message_initializer(switchable_msg &m,
                const char* name, const char* doc);
    protected:
        virtual void execute();
};

/**
    Initialize a switchable message,
    and build and return a checklist item for it.

    Called by message_initializer::execute().
*/
shared_object* initialize_msg(switchable_msg &msg, const char* name,
        const char* doc, shared_object* option, shared_object* group = nullptr);

#endif
