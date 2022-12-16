
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

        /// Array of resources; built first.
        resource** res_list;
        /// Number of built resources (max).
        unsigned max_built;
        /// Number of needed resources (max).
        unsigned max_needed;
        /// Total number of resources (always max_built + max_needed).
        unsigned max_resources;
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
                @param  max_bld     Max (typically, exact) number of
                                    resources this initializer builds.
                @param  max_nds     Max (typically, exact) number of
                                    resources this initializer needs.
        */
        initializer(const char* _name, unsigned max_bld, unsigned max_nds);

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
                @param  slot    The build slot; must be in the range
                                [0, max_build)
                @param  name    Resource name.  Ignored if null.
        */
        void builds_resource(unsigned slot, const char* name);

        /**
            Indicate that this initializer
            requires the named resource to be initialized,
            before it can execute.
                @param  slot    The needed resource slot; must be in
                                the range [max_build, max_build + max_needs)
                @param  name    Resource name.  Ignored if null.
        */
        void needs_resource(unsigned slot, const char* name);

        /**
            Set an object for a resource (that we build).
                @param  slot    The build slot; must be in the range
                                [0, max_build).
                @param  o       Object to set for the resource.
        */
        void set_object(unsigned slot, shared_object* o);

        /**
            Get an object for a resource.
                @param  slot    The resource slot; must be in the range
                                [0, max_build + max_needs)
                @return         Object associated with the resource.
        */
        shared_object* get_object(unsigned slot);

    private:
        /**
            Tell this initializer that one of the
            resources it needs, is now ready.
        */
        void notify(resource *r);

        /**
            If the initializer is ready to run,
            then run it; otherwise make it wait.
        */
        void run_or_wait();

        /**
            Display, when there's an error
        */
        void show(error_msg &E) const;
};

#endif

