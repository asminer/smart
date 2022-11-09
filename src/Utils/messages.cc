
#include "messages.h"

#include "../Options/options.h"

io_environ* named_msg::io = 0;

// ******************************************************************
// *                       named_msg  methods                       *
// ******************************************************************

named_msg::named_msg()
{
    active = false;
    name = 0;
}

/*
void named_msg::Initialize(option* owner, checklist_enum* grp,
        const char* n, const char* docs, bool act)
{
    name = n;
    active = act;
    if (owner) {
        owner->addChecklistItem(grp, n, docs, active);
    }
}
*/
