
/**
     Implementation of system environment
*/

#include "../include/defines.h"
#include "sysenv.h"

system_environ::system_environ()
{
    errindex = 0;
    sigx = 0;
    wait_to_terminate = false;
}

system_environ::~system_environ()
{
    // Nothing to clean up (yet?)
}

void system_environ::resumeTermination()
{
    catchterm = false;
    if (0==sigx) return;
    /*
     * Actually terminate
     */

  Output.flush();
  Error.Activate();
  Error << "Caught signal " << sigx << ", terminating.\n";
  Error.flush();
  Error.Deactivate();
  clean_exit(3);
}

void system_environ::clean_exit(int code)
{
    // Actually, nothing special at the moment
    exit(code);
}

