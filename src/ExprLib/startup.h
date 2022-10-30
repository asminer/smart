
#ifndef STARTUP_H
#define STARTUP_H

#include "../include/list.h"

class exprman;
class symbol_table;
class msr_func;

// ******************************************************************
// *                                                                *
// *                       initializer  class                       *
// *                                                                *
// ******************************************************************

class initializer {
        class resource;
    public:
        initializer(const char* n);
    protected:
        virtual ~initializer();
    public:

        /**
            Provided by derived classes.
            Returns true on success, false on failure.
        */
        virtual bool execute() = 0;

        inline static void setDebugging() {
            debug = true;
        }

        inline static bool isDebugging() {
            return debug;
        }

        /**
            Execute all initializers.
            Order is arbitrary, except we guarantee that
            all "builders" of a resource are executed
            before "users" of a resource.

            Returns true if all initializers had a chance to execute,
            false otherwise (happens if "deadlock" occurs).
        */
        static bool executeAll();

    protected:
        void buildsResource(const char* name);
        void usesResource(const char* name);

    protected:
        // stuff that our initializers will want to use.
        // convention: these member names are also resource names.

        static exprman* em;
        static symbol_table* st;
        static const char** env;
        static const char* version;
        static List <msr_func> CML;


    private:
        /// Checks if all required resources are ready.
        bool isReady();

        /// Execute all waiting initializers;
        /// put them in the appropriate list.
        static int executeWaiting();

        resource* findResource(const char* name);

        static inline void add_to_waiting(initializer* r) {
            r->next = waiting_list;
            waiting_list = r;
        }
        static inline void add_to_completed(initializer* r) {
            r->next = completed_list;
            completed_list = r;
        }
        static inline void add_to_failed(initializer* r) {
            r->next = failed_list;
            failed_list = r;
        }

    private:
        const char* name;
        initializer* next;
        bool executed;
        List <resource> resources_used;

        // Current implementation assumes not many resources
        // TBD: use a splay tree instead
        static resource* resource_list;

        static initializer* waiting_list;
        static initializer* completed_list;
        static initializer* failed_list;

        static bool debug;
};

#endif

