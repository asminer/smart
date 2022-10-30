
#include "startup.h"
#include <string.h>
#include <cstdio>

// ******************************************************************

initializer::resource* initializer::resource_list = 0;
initializer* initializer::waiting_list = 0;
initializer* initializer::completed_list = 0;
initializer* initializer::failed_list = 0;
bool initializer::debug = 0;
// bool initializer::debug = 1;

exprman* initializer::em = 0;
symbol_table* initializer::st = 0;
const char** initializer::env = 0;
const char* initializer::version = 0;
List <msr_func> initializer::CML;

// ******************************************************************

inline void DEBUG(const char* S)
{
    if (initializer::isDebugging()) {
        fputs(S, stderr);
    }
}

template <class T>
inline void DEBUG(const char* fmt, T t)
{
    if (initializer::isDebugging()) {
        fprintf(stderr, fmt, t);
    }
}

// ******************************************************************
// *                                                                *
// *                  initializer::resource  class                  *
// *                                                                *
// ******************************************************************

class initializer::resource {
        const char* name;
        List <initializer> builders;
        bool ready;
    public:
        resource* next;
    public:
        resource(const char* n);
        bool isReady();
        void addBuilder(initializer* b);

        friend class initializer;
};

// ******************************************************************
// *                 initializer::resource  methods                 *
// ******************************************************************

initializer::resource::resource(const char* n)
{
    name = n;
    next = 0;
    ready = false;
}

bool initializer::resource::isReady()
{
    if (ready) {
        DEBUG("\t\tResource %s is ready\n", name);
        return true;
    }
    DEBUG("\t\tChecking resource %s\n", name);
    for (int i=0; i<builders.Length(); i++) {
        if (builders.ReadItem(i)->executed) {
            DEBUG("\t\t\tBuilder %s has executed\n",
                builders.ReadItem(i)->name);
            continue;
        }
        DEBUG("\t\t\tBuilder %s has not executed\n",
            builders.ReadItem(i)->name);
        return false;
    }
    ready = true;
    DEBUG("\t\tResource %s is ready\n", name);
    return true;
}

void initializer::resource::addBuilder(initializer *b)
{
    builders.Append(b);
}

// ******************************************************************
// *                                                                *
// *                      initializer  methods                      *
// *                                                                *
// ******************************************************************

initializer::initializer(const char* n)
{
    name = n;
    next = waiting_list;
    waiting_list = this;
    executed = false;
}

initializer::~initializer()
{
}

bool initializer::executeAll()
{
    //
    // Giant debugging chunk here: show resource list
    //
    if (isDebugging()) {
        fprintf(stderr, "Initializer resource list:\n");

        for (resource* curr = resource_list; curr; curr=curr->next) {
            fprintf(stderr, "\t%s\n", curr->name);
        }
        fprintf(stderr, "End of resource list\n");
    }

    //
    // Actual code here
    //
    while (waiting_list) {
        int ran = executeWaiting();
        if (0==ran) return false; // STUCK!
    }
    return true;
}

// ******************************************************************

void initializer::buildsResource(const char* name)
{
    resource* r = findResource(name);
    DCASSERT(r);
    r->addBuilder(this);
}

void initializer::usesResource(const char* name)
{
    resources_used.Append(findResource(name));
}

// ******************************************************************

bool initializer::isReady()
{
    for (int i=0; i<resources_used.Length(); i++) {
        if (resources_used.Item(i)->isReady()) continue;
        return false;
    }
    return true;
}


int initializer::executeWaiting()
{
    int count = 0;
    initializer* run_list = waiting_list;
    waiting_list = 0;

    DEBUG("Running through waiting initializers\n");

    while (run_list) {
        DEBUG("\tChecking %s\n", run_list->name);
        initializer* next = run_list->next;
        if (run_list->isReady()) {
            DEBUG("\tExecuting %s\n", run_list->name);
            count++;
            bool ok = run_list->execute();
            run_list->executed = true;
            if (ok) {
                DEBUG("\tExecution of %s succeeded\n", run_list->name);
                add_to_completed(run_list);
            } else {
                DEBUG("\tExecution of %s failed\n", run_list->name);
                add_to_failed(run_list);
            }
        } else {
            DEBUG("\tInitializer %s not ready\n", run_list->name);
            add_to_waiting(run_list);
        }
        run_list = next;
    }
    return count;
}

initializer::resource* initializer::findResource(const char* name)
{
    // traverse list, find item with name, and move it to front
    // otherwise, if not present, create a new entry in front

    resource* prev = 0;
    resource* curr = resource_list;

    while (curr) {

        if (0==strcmp(name, curr->name)) {  // found!
            if (prev) {
                // Not at front; move it there
                prev->next = curr->next;
                curr->next = resource_list;
                resource_list = curr;
            }
            return resource_list;
        }
        prev = curr;
        curr = curr->next;
    }

    // still here?  Wasn't found

    curr = new resource(name);
    curr->next = resource_list;
    resource_list = curr;
    return resource_list;
}

