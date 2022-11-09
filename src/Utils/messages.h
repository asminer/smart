
#ifndef MESSAGES_H
#define MESSAGES_H

#include "../include/defines.h"
#include "../Streams/streams.h"

// For causedBy
#include "../ExprLib/expr.h"

/**
 * Base class for named messages.
 * I.e., warning messages, reporting messages, debugging messages,
 * that may be switched on and off.
 *
*/
class abstract_msg {
    friend class checklist_opt;
    const char* name;
protected:
    bool active;
    static io_environ* io;

    inline const char* getName() const { return name; }
public:
    abstract_msg();

    /// Initialize static members
    static void initStatic(io_environ* _io) { io = _io; }


  inline void Activate()    { active = true; }
  inline void Deactivate()  { active = false; }
  inline bool isActive() const { return active; }
  inline bool canWrite() const { return active && io; }

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
  inline bool caughtTerm() const {
    DCASSERT(io);
    return io->caughtTerm();
  }
};


/*
 * Warning messages.  Default to "active".
 */
class warning_msg : public abstract_msg {
    public:
        warning_msg();

        inline bool startWarning() const {
            if (!active) return false;
            if (!io)     return false;
            io->StartWarning();
            return true;
        }
        inline DisplayStream& warn() const {
            DCASSERT(io);
            return io->Warning;
        }
};

/*
 * Reporting messages.  Default to "inactive".
 */
class reporting_msg : public abstract_msg {
    public:
        reporting_msg();

        inline bool startReport() const {
            if (!active)  return false;
            if (!io)      return false;
            io->StartReport(getName());
            return true;
        }
        inline DisplayStream& report() const {
            DCASSERT(io);
            return io->Report;
        }
        inline FILE* Freport() const {
            if (io) return io->Report.getDisplay();
            return stdout;
        }
};

/*
 * Debugging messages.  Default to "inactive".
 */
class debugging_msg : public abstract_msg {
    public:
        debugging_msg();

        inline bool startReport() const {
            if (!active)  return false;
            if (!io)      return false;
            io->StartReport(getName());
            return true;
        }
        inline DisplayStream& report() const {
            DCASSERT(io);
            return io->Report;
        }
};


#endif

