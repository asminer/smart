
#ifndef INITIALIZER_H
#define INITIALIZER_H

#include "messages.h"

class shared_object;

// ******************************************************************
// *                                                                *
// *                       initializer  class                       *
// *                                                                *
// ******************************************************************

/**
    Non-trivial initializations with (acyclic) dependencies.

    To automatically initialize something (typically, options or
    static members or whatnot) that depends on something else
    (e.g., don't initialize options until an option manager
    to hold them has been set up),
    derive a class from this one and override the execute method.

    Inside the constructor, call builds_resource() and needs_resource()
    to set up dependencies.
    Your execute() method can then safely assume that the required
    resources have been constructed already.

    Convention is to use the variable name as the resource
    we build or need.
*/
class initializer {
        struct node;
        class resource;
        friend class resource;
        class res_info;
    private:
        /// States of an initializer.
        enum status {
            init,       // initial state after construction
            waiting,    // waiting on resources
            running,    // currently executing
            complete    // finished executing
        };
    protected:
        /// Our name (for debugging).
        const char* name;
    private:
        /// Our state
        status state;

        /// Resource info (built/needed)
        res_info* res_list;
        /// Max size of resource list
        unsigned max_resources;
        /// Current size of resource list
        unsigned used_resources;

        /// Number of resources we're waiting on.
        unsigned wait_count;

        /// List of initializers that may need to run
        static initializer* Waiting;

        /// Debugging messages?
        static bool debug;

        /// Next initializer in our list
        initializer* next;

    public:
        /**
            Build an initializer.
                @param  _name       The initializer name (for debugging).
                                    Normally the source file name.
                @param  max_res     Max (typically, exact) number of
                                    resources this initializer needs or builds.
        */
        initializer(const char* _name, unsigned max_res);

        /**
            Execute all initializers.
            Order is arbitrary, except we guarantee that
            all "builders" of a resource are executed
            before "users" of a resource.

            Will cause a detailed, internal error message
            if not all initializers are able to execute
            due to cyclic dependencies.

            Dependency information will be destroyed upon completion.

            @param  debug       If true, debugging messages
                                will be displayed.
        */
        static void execute_all(bool debug = false);

    protected:
        /**
            Provided by derived classes.
            Perform necessary initializations.
        */
        virtual void execute() = 0;

        /**
            Cleanup any memory that is no longer needed,
            after everything has been initialized.
            If overridden in derived classes,
            be sure to call initializer::cleanup().
        */
        virtual void cleanup();

        /**
            Indicate that this initializer
            is a builder for the named resource.
                @param  name    Resource name.  Ignored if null.
        */
        void builds_resource(const char* name);

        /**
            Indicate that this initializer
            requires the named resource to be initialized,
            before it can execute.
                @param  name    Resource name.  Ignored if null.
        */
        void needs_resource(const char* name);

        /**
            Set an object for a resource (that we build).
                @param  name    Name of the resource.
                @param  o       Object to set for the resource.
        */
        void set_object(const char* name, shared_object* o);

        /**
            Get an object for a resource.
                @param  name    Name of the resource.
                @return         Object associated with the resource.
        */
        shared_object* get_object(const char* name);


        /**
            Indicate that the initializer should run right away,
            if possible; otherwise it won't run until the next
            call to execute_all().
            Use this for any initializers that may be created
            after main() is started.

            This should be called at the very end of the constructor.
        */
        void try_immediately();

    private:
        /**
            Find a resource we build/use, by name.
            If not present, returns max_resources+1.
        */
        unsigned res_list_find(const char* n) const;

        /**
            If the initializer is ready to run,
            then run it; otherwise make it wait.
        */
        void run_or_wait();

        /**
            Tell this initializer that one of the
            resources it needs, is now ready.
        */
        void notify(resource *r);

        /**
            Display, when there's an error
        */
        void show(error_msg &E) const;

        /**
            For debugging; convert state to its name.
        */
        const char* stateName() const;
};

#endif

