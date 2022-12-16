#ifndef INIT_OPTS_H
#define INIT_OPTS_H

#include "initializer.h"
#include "messages.h"

/**
    Initializer for a group of items, in a checklist option.
*/
class checklistgroup_initializer : public initializer {
        const char* mainopt;
        const char* groupname;
        const char* groupdoc;

        unsigned main_handle;
        unsigned group_handle;
    public:
        /**
            Set up an initializer.
                @param  main    Main checklist option name, e.g., "Warning"
                @param  name    Group name, e.g., "pn_ALL"
                @param  doc     Group documentation
        */
        checklistgroup_initializer(const char* main, const char* name, const char* doc);
    protected:
        virtual void execute();
};


/**
    Initializer for a switchable message.
*/

#endif
