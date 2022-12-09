
#include "initializer.h"
#include "../include/defines.h"
#include <iostream>
#include "messages.h"

// ******************************************************************
// *                                                                *
// *                    initializer::node struct                    *
// *                                                                *
// ******************************************************************

struct initializer::node {
        initializer* item;
        initializer::node* next;
    public:
        node(initializer* _item, node* _next) {
            DCASSERT(_item);
            item = _item;
            next = _next;
        }
};

// ******************************************************************
// *                                                                *
// *                  initializer::resource  class                  *
// *                                                                *
// ******************************************************************

/*
    Note: the number of builders for a resource
    is assumed to be relatively small.

    The number of subscribers can be huge.
*/
class initializer::resource {
        const char* name;
        initializer::node* wait_builders;
        initializer::node* done_builders;
        initializer::node* subscribers;
        resource* next;

        static resource* RLIST;
    private:
        resource(const char* n, resource* nxt);

    public:
        ~resource();

        inline bool is_ready() const { return 0==wait_builders; }

        /// Indicate that IN is a builder for this resource
        inline void add_builder(initializer* IN) {
            wait_builders = new initializer::node(IN, wait_builders);
        }

        /// Indicate that IN needs this resource
        inline void add_subscriber(initializer* IN) {
            subscribers = new initializer::node(IN, subscribers);
        }

        /// Indicate that builder IN has executed
        void done_builder(initializer* IN);

        /// Notify all subscribers that we're ready.
        void notify_subscribers();

        /**
            Find (and build a new one of needed)
            a resource with the given name.
        */
        static resource* find(const char* n);
    private:
        static void delete_list(initializer::node* L);
};

// ******************************************************************
// *                 initializer::resource  methods                 *
// ******************************************************************

initializer::resource* initializer::resource::RLIST = nullptr;

initializer::resource::resource(const char* n, resource* nxt)
{
    name = n;
    next = nxt;
    wait_builders = nullptr;
    done_builders = nullptr;
    subscribers = nullptr;
}

initializer::resource::~resource()
{
    delete_list(wait_builders);
    delete_list(done_builders);
    delete_list(subscribers);
}

void initializer::resource::done_builder(initializer* IN)
{
    // Find and remove the node containing IN.
    // If none, then do nothing.
    initializer::node* prev = nullptr;
    initializer::node* find = wait_builders;
    while (find) {
        if (find->item != IN) {
            prev = find;
            find = find->next;
            continue;
        }
        // Found it
        //
        // Remove from current list
        //
        if (prev) {
            prev->next = find->next;
        } else {
            wait_builders = find->next;
        }
        // Add to front of done list
        find->next = done_builders;
        done_builders = find;
        // done!
        return;
    }
}

void initializer::resource::notify_subscribers()
{
    for (initializer::node* curr = subscribers; curr; curr=curr->next) {
        curr->item->notify(this);
    }
}

initializer::resource* initializer::resource::find(const char* n)
{
    //
    // Move the resource to the front of the list if present;
    // if not, create it at the front of the list
    //
    resource* prev = nullptr;
    resource* curr = RLIST;
    while (curr) {
        if (strcmp(curr->name, n)) {
            prev = curr;
            curr = curr->next;
            continue;
        }
        // found
        if (prev) prev->next = curr->next;
        curr->next = RLIST;
        RLIST = curr;
        break;
    }
    // Not found; create
    if (!curr) {
        RLIST = new resource(n, RLIST);
    }
    return RLIST;
}

void initializer::resource::delete_list(initializer::node* L)
{
    while (L) {
        initializer::node* curr = L;
        L = L->next;
        delete curr;
    }
}

// ******************************************************************
// *                                                                *
// *                      initializer  methods                      *
// *                                                                *
// ******************************************************************

initializer* initializer::init_list = nullptr;
initializer* initializer::waiting_list = nullptr;
initializer* initializer::finished_list = nullptr;


initializer::initializer(const char* _name, unsigned maxbld, unsigned maxnds)
{
    name = _name;
    max_build = maxbld;
    max_needs = maxnds;

    state = init;

    if (max_build) {
        build_list = new resource* [max_build];
        for (unsigned i=0; i<max_build; i++) {
            build_list[i] = nullptr;
        }
    }
    next_build = 0;

    if (max_needs) {
        need_list = new resource* [max_needs];
        for (unsigned i=0; i<max_needs; i++) {
            need_list[i] = nullptr;
        }
    }
    next_needs = 0;

    // Add us to init_list
    next = init_list;
    init_list = this;
}

void initializer::execute_all(bool debug)
{
    // TBD
    //

    // Go through init_list.
    // First pass: add everything with one or more needs
    // to the waiting list.
    // Second pass: execute everything in this list
}

initializer::~initializer()
{
    delete[] build_list;
    delete[] need_list;
}

void initializer::builds_resource(const char* res)
{
    DCASSERT(init == state);

    if (0==res) return;

    if (next_build >= max_build) {
        internal_error E(__FILE__, __LINE__);
        E << "Initializer " << name
          << " build overflow: more than " << max_build;
        return;
    }
    build_list[next_build++] = resource::find(res);
}

void initializer::needs_resource(const char* res)
{
    DCASSERT(init == state);

    if (0==res) return;

    if (next_needs >= max_needs) {
        internal_error E(__FILE__, __LINE__);
        E << "Initializer " << name
          << " needs overflow: more than " << max_needs;
        return;
    }
    need_list[next_needs++] = resource::find(res);
}

void initializer::notify(resource *r)
{
    DCASSERT(waiting == state);

    // Resource r is now ready
    // find it in our needs array, move it to the end, decrement next_needs
    // when next_needs becomes 0, we can execute

    for (unsigned i=0; i<next_needs; i++) {
        if (need_list[i] != r) continue;
        // found it

        if (i+1 != next_needs) {
            // Not the last element.
            // Make it so by swapping.
            need_list[i] = need_list[next_needs-1];
            need_list[next_needs-1] = r;
        }
        --next_needs;

        if (0==next_needs) {
            state = running;
            execute();
            post_execute();
        }
        return;
    }

    // Not found; don't change anything
}

void initializer::post_execute()
{
    // Notify resources we build
    for (unsigned i=0; i<max_build; i++) {
        build_list[i]->done_builder(this);
    }
    state = complete;
}


// ******************************************************************
// ******************************************************************
// ******************************************************************
// ******************************************************************
//
// OLD IMPLEMENTATION BELOW HERE
//
// ******************************************************************
// ******************************************************************
// ******************************************************************
// ******************************************************************

#if 0
initializer::resource* initializer::resource_list = 0;
bool initializer::debug = 0;
// bool initializer::debug = 1;

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

#endif
