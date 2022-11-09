
#ifndef MESSAGES_H
#define MESSAGES_H

#include "../include/defines.h"
#include "../Streams/streams.h"

// For causedBy
#include "../ExprLib/expr.h"

/*
class option;
class checklist_enum;
*/

/**
 * Base class for named messages.
 * I.e., warning messages, reporting messages, debugging messages,
 * that may be switched on and off.
 *
*/
class named_msg {
    friend class checklist_opt;
    bool active;
    const char* name;
protected:
    static io_environ* io;
public:
    named_msg();

    /// Initialize static members
    static void initStatic(io_environ* _io) { io = _io; }

  /** Initialize.
        @param  owner If nonzero, a new checklist constant for this
                      item will be created and added to owner.
        @param  grp   If nonzero, the checklist const will also be
                      added to the specified group.
        @param  n     Name of the item.
        @param  d     Documentation (not required if \a owner is 0).
        @param  act   Are we initially active or not.
  */
  // void Initialize(option* owner, checklist_enum* grp, const char* n, const char* docs, bool act);

  inline void Activate()    { active = true; }
  inline void Deactivate()  { active = false; }
  inline bool isActive() const { return active; }
  inline bool canWrite() const { return active && io; }

  inline bool startWarning() const {
    if (!active) return false;
    if (!io)     return false;
    io->StartWarning();
    return true;
  }
  inline bool startReport() const {
    if (!active)  return false;
    if (!io)      return false;
    io->StartReport(name);
    return true;
  }
  inline void causedBy(const expr* x) const {
    DCASSERT(io);
    if (x)
      io->CausedBy(x->Filename(), x->Linenumber());
    else
      io->NoCause();
  }
  inline void causedBy(const char* fn, int ln) const {
    DCASSERT(io);
    io->CausedBy(fn, ln);
  }
  inline void noCause() const {
    DCASSERT(io);
    io->NoCause();
  }
  inline void newLine() const {
    DCASSERT(io);
    io->NewLine(name);
  }
  inline void stopIO() const {
    DCASSERT(io);
    io->Stop();
  }
  inline DisplayStream& report() const {
    DCASSERT(io);
    return io->Report;
  }
  inline FILE* Freport() const {
    if (io) return io->Report.getDisplay();
    return stdout;
  }
  inline DisplayStream& warn() const {
    DCASSERT(io);
    return io->Warning;
  }
  inline bool caughtTerm() const {
    DCASSERT(io);
    return io->caughtTerm();
  }
};


#endif

