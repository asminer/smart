
#include "initializer.h"
#include "../include/defines.h"

#include <cstring>

// #define DEBUG
// #define DEBUG_FIND


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
        initializer::node* wait_builders;
        initializer::node* done_builders;
        initializer::node* subscribers;
        shared_object* data;
        resource* next;
        bool done_building;

        // TBD: switch to splay tree?
        static resource* RLIST;
    private:
        resource(const char* n, resource* nxt);

    public:
        const char* name;
        static bool debug;

        ~resource();

        inline bool is_built() const { return done_building; }

        /// Indicate that IN is a builder for this resource
        inline void add_builder(initializer* IN) {
            DCASSERT(!done_building);
            wait_builders = new initializer::node(IN, wait_builders);
        }

        /// Indicate that IN needs this resource
        inline void add_subscriber(initializer* IN) {
            DCASSERT(!done_building);
            subscribers = new initializer::node(IN, subscribers);
        }

        /// Get the data for this resource
        inline shared_object* get_object() {
            return data;
        }

        /// Set the data for this resource
        inline void set_object(shared_object* d) {
            data = d;
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

        /// Delete all resources.
        static void delete_all();

        //
        // For debugging.
        //
        void show(std::ostream &s) const;
        static void show_all(std::ostream &s, bool names_only = false);
    private:
        static void delete_list(initializer::node* L);
        static void show_list(std::ostream &s, const initializer::node* L);
};

// ******************************************************************
// *                 initializer::resource  methods                 *
// ******************************************************************

initializer::resource* initializer::resource::RLIST = nullptr;
bool initializer::resource::debug = false;

initializer::resource::resource(const char* n, resource* nxt)
{
    name = n;
    next = nxt;
    wait_builders = nullptr;
    done_builders = nullptr;
    subscribers = nullptr;
    data = nullptr;
    done_building = false;
#ifdef DEBUG
    std::cerr << "\tBuilt resource: " << name << "\n";
#endif
}

initializer::resource::~resource()
{
    delete_list(wait_builders);
    delete_list(done_builders);
    delete_list(subscribers);
    // Eventually: Delete(data);
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
        // Is our waiting list now empty?
        if (wait_builders) return;  // nope, we're done

        done_building = true;
        // Waiting list just became empty.
        // That means the resource is ready.
        if (debug) {
            std::cerr << "Resource " << name << " is ready.\n";
        }
        // Notify all our subscribers
        notify_subscribers();
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
#ifdef DEBUG_FIND
    std::cerr << "Looking for " << n << ":\n";
#endif
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
        if (prev) {
            // Not already in front; move it there
            prev->next = curr->next;
            curr->next = RLIST;
            RLIST = curr;
        }
#ifdef DEBUG_FIND
        std::cerr << "Found; new list: ";
        show_all(std::cerr, true);
#endif
        return RLIST;
    }
    // Not found; create
    curr = new resource(n, RLIST);
    RLIST = curr;
#ifdef DEBUG_FIND
    std::cerr << "Not found; new list: ";
    show_all(std::cerr, true);
#endif
    return RLIST;
}

void initializer::resource::delete_all()
{
    while (RLIST) {
        resource* curr = RLIST;
        RLIST = RLIST->next;
        delete curr;
    }
}

void initializer::resource::show(std::ostream &s) const
{
    s << "    " << name << "\n";
    s << "        waiting  builders: ";
    show_list(s, wait_builders);
    s << "        finished builders: ";
    show_list(s, done_builders);
    s << "        subscribers      : ";
    show_list(s, subscribers);
}

void initializer::resource::show_all(std::ostream &s, bool names_only)
{
    for (const resource* curr = RLIST; curr; curr = curr->next) {
        if (names_only) {
            s << curr->name << " -> ";
        } else {
            curr->show(s);
        }
    }
    if (names_only) s << "null\n";
}

void initializer::resource::delete_list(initializer::node* L)
{
    while (L) {
        initializer::node* curr = L;
        L = L->next;
        delete curr;
    }
}

void initializer::resource::show_list(std::ostream &s,
        const initializer::node* L)
{
    for (const initializer::node* curr = L; curr; curr=curr->next) {
        if (curr != L) {
            s << ", ";
        }
        s << curr->item->name;
    }
    s << "\n";
}

// ******************************************************************
// *                                                                *
// *                  initializer::res_info  class                  *
// *                                                                *
// ******************************************************************

/*
    Information about a resource that is built or needed.
*/
class initializer::res_info {
        initializer::resource* res;
        bool in_use;
        bool builds;
        bool needs;
        bool notified;
    public:
        res_info();

        inline bool isEmpty()       const { return !in_use; }
        inline bool isBuilt()       const { return builds; }
        inline bool isNeeded()      const { return needs; }
        inline bool isNotified()    const { return notified; }

        void init_builds(initializer::resource* r);
        void init_needs (initializer::resource* r);

        inline bool matches(const char* name) const
        {
            if (!in_use) return false;
            DCASSERT(res);
            DCASSERT(res->name);
            DCASSERT(name);
            return (0 == strcmp(name, res->name));
        }

        /**
         * Indicate that the resource has finished.
         * Decrements num_waiting appropriately.
         */
        void done(const initializer::resource* r, unsigned &num_waiting);

        /*
         * Indicate that the initializer has finished.
         */
        void done(initializer* who);

        // Return true on success
        bool set_object(shared_object* o);

        shared_object* get_object() const;

        void show(error_msg &E) const;
};

// ******************************************************************
// *                 initializer::res_info  methods                 *
// ******************************************************************

initializer::res_info::res_info()
{
    res = nullptr;
    in_use = false;
    builds = false;
    needs = false;
    notified = false;
}

void initializer::res_info::init_builds(initializer::resource* r)
{
    DCASSERT(r);
    DCASSERT(!res);
    DCASSERT(!in_use);
    res = r;
    in_use = true;
    builds = true;
}

void initializer::res_info::init_needs(initializer::resource* r)
{
    DCASSERT(r);
    DCASSERT(!res);
    DCASSERT(!in_use);
    res = r;
    in_use = true;
    needs  = true;
}

void initializer::res_info::done(const initializer::resource *r,
        unsigned &num_waiting)
{
    if (!in_use) return;
    if (!needs)  return;
    if (notified) return;
    if (r == res) {
        notified = true;
        --num_waiting;
    }
}

void initializer::res_info::done(initializer* who)
{
    if (!in_use) return;
    if (!builds)  return;
    if (notified) return;
    notified = true;
    DCASSERT(res);
    res->done_builder(who);
}


bool initializer::res_info::set_object(shared_object* o)
{
    if (!in_use) return false;
    if (!builds) return false;
    DCASSERT(res);
    res->set_object(o);
    return true;
}

shared_object* initializer::res_info::get_object() const
{
    if (!in_use) return nullptr;
    DCASSERT(res);
    return res->get_object();
}

void initializer::res_info::show(error_msg &E) const
{
    if (!in_use) return;
    if (builds) E << "Builds ";
    if (needs)  E << "Needs  ";
    E << res->name;
    E << (notified ? " (done)" : " (waiting)");
    E.newLine();
}

// ******************************************************************
// *                                                                *
// *                      initializer  methods                      *
// *                                                                *
// ******************************************************************

bool initializer::debug = false;
initializer* initializer::Waiting = nullptr;


initializer::initializer(const char* _name, unsigned max_res)
{
    name = _name;
    max_resources = max_res;
    used_resources = 0;
    wait_count = 0;
    state = init;

#ifdef DEBUG
    std::cerr << "Building initializer: " << name << "\n";
#endif

    res_list = nullptr;
    if (max_resources) {
        res_list = new res_info [max_resources];
    }

    // Add us to the waiting list
    next = Waiting;
    Waiting = this;
}

void initializer::execute_all(bool _debug)
{
    resource::debug = _debug;
    debug = _debug;

    if (debug) {
        std::cerr << "Prepping to run initializers.\nResources:\n";
        resource::show_all(std::cerr);
        error_msg E("Initializers:");
        E.newLine();
        for (const initializer* I=Waiting; I; I=I->next) {
            I->show(E);
        }
    }

    //
    // One pass through the list, run what we can.
    // Notifications should get everyone else,
    // unless there is a deadlock.
    //

    for (initializer* I=Waiting; I; I=I->next) {
        I->run_or_wait();
    }

    //
    // Now, go through the list, and remove any completed items.
    // Whatever is left, is in a deadlock.
    //

    initializer* dead = nullptr;

    for (initializer* I=Waiting; I; ) {
        initializer* nxt = I->next;
        if (complete != I->state) {
            I->next = dead;
            dead = I;
        } else {
            // Clean up I, but don't delete it.
            I->cleanup();
        }
        I = nxt;
    }

    //
    // If deadlock, throw a major hissy fit
    //
    if (dead) {
        internal_error E(__FILE__, __LINE__);
        E << "Deadlock in initializer::execute_all";
        E.newLine();
        E << "Remaining (dead) initializers:";
        E.newLine('+');
        for (initializer* I = dead; I; I=I->next) {
            I->show(E);
        }
    }

    //
    // DON'T Destroy resources yet,
    // for late initializers.
    //
    // resource::delete_all();

    if (debug) {
        std::cerr << "Finished running initializers\n";
    }
}

void initializer::cleanup()
{
    delete[] res_list;
    res_list = nullptr;
    max_resources = 0;
    used_resources = 0;
}

void initializer::builds_resource(const char* res)
{
    DCASSERT(init == state);
    if (0==res) return;
    CHECK_RANGE(__FILE__, __LINE__, 0, used_resources, max_resources);

    if (res_list_find(res) < max_resources) {
        internal_error E(__FILE__, __LINE__);
        E << "Intializer " << basename(name) << " already builds/needs " << res;
        return;
    }

#ifdef DEBUG
    std::cerr << "    builds " << res << "\n";
#endif

    resource* r = resource::find(res);
    r->add_builder(this);

    res_list[used_resources].init_builds(r);
    ++used_resources;

#ifdef DEBUG
    std::cerr << "        done\n";
#endif
}

void initializer::needs_resource(const char* res)
{
    DCASSERT(init == state);
    if (0==res) return;
    CHECK_RANGE(__FILE__, __LINE__, 0, used_resources, max_resources);

    if (res_list_find(res) < max_resources) {
        internal_error E(__FILE__, __LINE__);
        E << "Intializer " << basename(name) << " already builds/needs " << res;
        return;
    }

#ifdef DEBUG
    std::cerr << "    needs  " << res << "\n";
#endif
    resource* r = resource::find(res);

    res_list[used_resources].init_needs(r);
    ++wait_count;
    if (r->is_built()) {
        // Already built? ok
        res_list[used_resources].done(r, wait_count);
    } else {
        // Tell r to notify us when it's built
        r->add_subscriber(this);
    }
    ++used_resources;

#ifdef DEBUG
    std::cerr << "        done\n";
#endif
}

void initializer::set_object(const char* res, shared_object* o)
{
    unsigned slot = res_list_find(res);
    if (slot < max_resources) {
        if (res_list[slot].set_object(o)) return;
    }

    internal_error E(__FILE__, __LINE__);
    E   << "set_object(" << res << ") fail in initializer "
        << basename(name) << ":";
    E.newLine();
    if (slot > max_resources) {
        E << "resource not found";
    } else {
        E << "not a builder for resource";
    }
}

shared_object* initializer::get_object(const char* res)
{
    unsigned slot = res_list_find(res);
    if (slot > max_resources) {
        internal_error E(__FILE__, __LINE__);
        E   << "get_object(" << res << ") fail in initializer "
            << basename(name) << ":";
        E.newLine();
        E << "resource not found";
    }
    return res_list[slot].get_object();
}

void initializer::try_immediately()
{
    // We should be top on the waiting list.
    // (If not, we'll bail out.) Pull us off.

    if (Waiting != this) return;
    Waiting = next;
    next = nullptr;

    //
    // Run if possible.
    //
    run_or_wait();

    //
    // See if we need to go back on the waiting list.
    if (waiting == state) {
        next = Waiting;
        Waiting = this;
        return;
    }
    DCASSERT(complete == state);
    cleanup();
    // Don't delete
}

unsigned initializer::res_list_find(const char* n) const
{
    for (unsigned i=0; i<used_resources; i++) {
        if (res_list[i].matches(n)) return i;
    }
    return 1+max_resources;
}

void initializer::run_or_wait()
{
    if (complete == state) return;

    DCASSERT( running != state );

    if (wait_count) {
        state = waiting;
        return;
    }
    state = running;
    if (debug) {
        std::cerr << "Running initializer " << basename(name) << "\n";
    }
    execute();
    // Notify resources we build
    for (unsigned i=0; i<used_resources; i++) {
        res_list[i].done(this);
    }
    state = complete;
}

void initializer::notify(resource *r)
{
    if (debug) {
        std::cerr   << "    notifying " << basename(name) << "; status: "
                    << stateName() << "\n";
    }
    DCASSERT(running != state);
    DCASSERT(complete != state);
    DCASSERT(r);
    DCASSERT(r->is_built());

    // Update resource info list
    for (unsigned i=0; i<used_resources; i++) {
        res_list[i].done(r, wait_count);
    }

    run_or_wait();
}

void initializer::show(error_msg &E) const
{
    E << "Initializer '" << basename(name) << "'; status: " << stateName();
    E.newLine('+');
    for (unsigned i=0; i<used_resources; i++) {
        res_list[i].show(E);
    }
    E << "wait count: " << wait_count;
    E.newLine('-');
}

const char* initializer::stateName() const
{
    switch (state) {
        case init:      return "init";
        case waiting:   return "waiting";
        case running:   return "running";
        case complete:  return "complete";
        default:        return "?";
    }
}
