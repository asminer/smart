
/**
     Implementation of system environment
*/

#include "../include/defines.h"
#include "sigman.h"
#include "outstream.h"
#include "location.h"
#include "messages.h"

signal_manager::signal_manager()
{
    sigx = 0;
    wait_to_terminate = false;
}

signal_manager::~signal_manager()
{
    // Nothing to clean up (yet?)
}

void signal_manager::resumeTermination()
{
    if (0==sigx) return;
    /*
     * Actually terminate
     */

    // outputStream::Output().flush();
    // TBD: how to do ^ this

    error_msg E("ERROR");
    E << "Caught signal " << sigx << ", terminating.";
    E.stream() << std::endl;
    clean_exit(3);
}

void signal_manager::clean_exit(int code)
{
    // Actually, nothing special at the moment
    exit(code);
}

signal_manager& signal_manager::theSigMan()
{
    static signal_manager tsm;
    return tsm;
}
