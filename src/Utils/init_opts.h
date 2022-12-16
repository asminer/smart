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
        /**
            Set up an initializer.
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

#endif
