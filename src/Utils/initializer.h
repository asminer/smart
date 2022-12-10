
#ifndef INITIALIZER_H
#define INITIALIZER_H

#include "messages.h"

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

        /// Array of needed resources.
        resource** need_list;
        /// Dimension of need_list array.
        unsigned max_needs;
        /// Index of next needed resource
        unsigned next_needs;

        /// Array of built resources
        resource** build_list;
        /// Dimension of build_list array.
        unsigned max_build;
        /// Index of next built resource
        unsigned next_build;

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
            Cannot be called more than max_bld times.
        */
        void builds_resource(const char* name);

        /**
            Indicate that this initializer
            requires the named resource to be initialized,
            before it can execute.
            Cannot be called more than max_nds times.
        */
        void needs_resource(const char* name);

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

