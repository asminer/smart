
// $Id$

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
  public:
    initializer(const char* n);
  protected:
    virtual ~initializer();
  public:
    // Public, but hidden?  Ok
    class resource;

    /**
      Returns true on success, false on failure.
    */
    virtual bool execute() = 0;
    bool isReady();
    inline bool hasExecuted() const { return executed; }

    /**
      Execute all initializers.
      Order is arbitrary, except we guarantee that 
      all "builders" of a resource are executed 
      before "users" of a resource.

      Returns true if all initializers had a chance to execute,
      false otherwise (happens if "deadlock" occurs).
    */
    static bool executeAll();

    inline static void setDebugging() {
      debug = true;
    }

    inline static bool isDebugging() { 
      return debug;
    }

    inline const char* Name() const {
      return name;
    }

  private:
    // helper for executeAll
    static int executeWaiting();
    resource* findResource(const char* name);

  private:
    const char* name;
    initializer* next;
    bool executed;
    List <resource> resources_used;

    // Current implementation assumes not many resources
    static resource* resource_list;

    static initializer* waiting_list;
    static initializer* completed_list;
    static initializer* failed_list;

    static bool debug;

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
};

#endif

