
#ifndef INITIALIZER_H
#define INITIALIZER_H

// ******************************************************************
// *                                                                *
// *                       initializer  class                       *
// *                                                                *
// ******************************************************************

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
    private:
        /// Our name (for debugging).
        const char* name;
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

        /// List of initializers in init state
        static initializer* init_list;
        /// List of initializers in waiting state
        static initializer* waiting_list;
        /// List of completed initializers
        static initializer* finished_list;

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

            @param  debug       If true, debugging messages
                                will be displayed.
        */
        static void execute_all(bool debug = false);

    protected:
        virtual ~initializer();

        /**
            Provided by derived classes.
            Perform necessary initializations.
        */
        virtual void execute() = 0;

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
            Book keeping after calling execute().
        */
        void post_execute();
};

#endif

