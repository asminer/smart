
/* $Id$

   Main module of smart (i.e., compiler)

*/

#ifndef MAIN_API
#define MAIN_API

int smart_main(int argc, char** argv, char** env);

/** Should be called before exiting, either normally or due to an error.
    Does critical cleanup, like flushing output.
*/
void smart_exit();

#endif
