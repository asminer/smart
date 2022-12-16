
#include "init_opts.h"
#include "../Options/optman.h"

static option_manager* OM;

optman_initializer::optman_initializer()
    : initializer("optman_initializer", 1, 0)
{
    handle = builds_resource("OM");
    OM = nullptr;
}

void optman_initializer::execute()
{
    OM = MakeOptionManager();
    if (!OM) {
        internal_error E(__FILE__, __LINE__);
        E << "MakeOptionManager() returned null pointer";
    }
    set_build_object(handle, OM);
}

static optman_initializer _omi;

//



