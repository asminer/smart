
#include "initializer.h"


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
        void done_builder(initilizer* IN);

        /// Notify all subscribers that we're ready.
        void notify_subscribers();

        /**
            Find (and build a new one of needed)
            a resource with the given name.
        */
        static resource* findResource(const char* n);
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
        curr->notify(this);
    }
}

void resource* initializer::resource::findResource(const char* n)
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
