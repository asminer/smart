
#include "initializer.h"
#include "../include/defines.h"

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
        resource* next;

        static resource* RLIST;
    private:
        resource(const char* n, resource* nxt);

    public:
        const char* name;
        static bool debug;

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
#ifdef DEBUG
    std::cerr << "\tBuilt resource: " << name << "\n";
#endif
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
        // Is our waiting list now empty?
        if (wait_builders) return;  // nope, we're done

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
// *                      initializer  methods                      *
// *                                                                *
// ******************************************************************

bool initializer::debug = false;
initializer* initializer::Waiting = nullptr;


initializer::initializer(const char* _name, unsigned maxbld, unsigned maxnds)
{
    name = _name;
    max_build = maxbld;
    max_needs = maxnds;
    state = init;

#ifdef DEBUG
    std::cerr << "Building initializer: " << name << "\n";
#endif

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
    // Destroy resources
    //
    resource::delete_all();
}

void initializer::cleanup()
{
    delete[] build_list;
    build_list = nullptr;
    next_build = 0;

    delete[] need_list;
    need_list = nullptr;
    next_needs = 0;
}


void initializer::builds_resource(const char* res)
{
    DCASSERT(init == state);

    if (0==res) return;

#ifdef DEBUG
    std::cerr << "    builds " << res << "\n";
#endif

    if (next_build >= max_build) {
        internal_error E(__FILE__, __LINE__);
        E << "Initializer " << name
          << " build overflow: more than " << max_build;
        return;
    }
    resource* r = resource::find(res);
    r->add_builder(this);
    build_list[next_build++] = r;
#ifdef DEBUG
    std::cerr << "        done\n";
#endif
}

void initializer::needs_resource(const char* res)
{
    DCASSERT(init == state);

    if (0==res) return;

#ifdef DEBUG
    std::cerr << "    needs  " << res << "\n";
#endif

    if (next_needs >= max_needs) {
        internal_error E(__FILE__, __LINE__);
        E << "Initializer " << name
          << " needs overflow: more than " << max_needs;
        return;
    }
    resource* r = resource::find(res);
    r->add_subscriber(this);
    need_list[next_needs++] = r;
#ifdef DEBUG
    std::cerr << "        done\n";
#endif
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

        run_or_wait();
        return;
    }

    // Not found; don't change anything
}

void initializer::run_or_wait()
{
    if (complete == state) return;

    DCASSERT( running != state );

    if (next_needs) {
        state = waiting;
        return;
    }
    state = running;
    if (debug) {
        std::cerr << "Running initializer " << name << "\n";
    }
    execute();
    // Notify resources we build
    for (unsigned i=0; i<max_build; i++) {
        build_list[i]->done_builder(this);
    }
    state = complete;
}

void initializer::show(error_msg &E) const
{
    E << "Initializer '" << name << "'";
    E.newLine('+');
    E << "Needs : ";
    for (unsigned i=0; i<next_needs; i++) {
        if (i) E << ", ";
        E << need_list[i]->name;
    }
    E.newLine();
    E << "Builds: ";
    for (unsigned i=0; i<next_build; i++) {
        if (i) E << ", ";
        E << build_list[i]->name;
    }
    E.newLine('-');
}

