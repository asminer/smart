
#ifndef MESSAGES_H
#define MESSAGES_H

#include "../include/defines.h"
#include "../Streams/streams.h"

// For causedBy
#include "../ExprLib/expr.h"

class option_manager;
class checklist_enum;

/**
 * Base class for named messages.
 * I.e., warning messages, reporting messages, debugging messages,
 * that may be switched on and off.
 *
*/
class abstract_msg {
    friend class checklist_opt;
    const char* name;
    bool active;

    const char* option_name;
protected:
    static io_environ* io;

    inline const char* getName() const { return name; }

public:
    abstract_msg(const char* optname);

    /**
     *  Initialize checklist item for an option.
     *      @param  om      Option manager that owns the option.
     *      @param  grp     Group, or null, to add the item to also.
     *      @param  name    Name of the checklist item.
     *      @param  doc     Documentation for the checklist item.
     *
     *      @return true    Iff we were able to add the checklist item.
     */
    bool initialize(const option_manager* om, checklist_enum* grp,
            const char* name, const char* doc);

    inline bool initialize(const option_manager* om,
            const char* name, const char* doc)
    {
        return initialize(om, 0, name, doc);
    }


    inline void Activate()    { active = true; }
    inline void Deactivate()  { active = false; }
    inline bool isActive() const { return active && io; }

    // TBD below here


  // TBD: this should be handled elsewhere...
  inline bool caughtTerm() const {
    DCASSERT(io);
    return io->caughtTerm();
  }

    /// Initialize static members
    static void initStatic(io_environ* _io);

};


/*
 * Warning messages.  Default to "active".
 */
class warning_msg : public abstract_msg {
    public:
        warning_msg();

        inline static bool switchOutput(const char* outfile) {
            FILE* outf = fopen(outfile, "a");
            if (outfile) {
                io->Warning.SwitchDisplay(outf);
                return true;
            }
            return false;
        }
        inline static void defaultOutput() {
            io->Warning.SwitchDisplay(0);
        }

        inline bool startWarning(const location &L) const {
            if (!isActive()) return false;
            io->StartWarning();
            io->CausedBy(L);
            return true;
        }
        inline DisplayStream& warn() const {
            DCASSERT(io);
            return io->Warning;
        }

  inline void newLine() const {
    DCASSERT(io);
    io->NewLine(getName());
  }
  inline void stopIO() const {
    DCASSERT(io);
    io->Stop();
  }

};

/*
 * Reporting messages.  Default to "inactive".
 */
class reporting_msg : public abstract_msg {
    public:
        reporting_msg();

        inline static bool switchOutput(const char* outfile) {
            FILE* outf = fopen(outfile, "a");
            if (outfile) {
                io->Report.SwitchDisplay(outf);
                return true;
            }
            return false;
        }
        inline static void defaultOutput() {
            io->Report.SwitchDisplay(0);
        }

        inline bool startReport() const {
            if (!isActive()) return false;
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

  inline void causedBy(const location &L) const {
    DCASSERT(io);
    io->CausedBy(L);
  }
  inline void newLine() const {
    DCASSERT(io);
    io->NewLine(getName());
  }
  inline void stopIO() const {
    DCASSERT(io);
    io->Stop();
  }

};

/*
 * Debugging messages.  Default to "inactive".
 */
class debugging_msg : public abstract_msg {
    public:
        debugging_msg();

        inline bool startReport() const {
            if (!isActive()) return false;
            io->StartReport(getName());
            return true;
        }
        inline DisplayStream& report() const {
            DCASSERT(io);
            return io->Report;
        }

  inline void causedBy(const location &L) const {
    DCASSERT(io);
    io->CausedBy(L);
  }
  inline void newLine() const {
    DCASSERT(io);
    io->NewLine(getName());
  }
  inline void stopIO() const {
    DCASSERT(io);
    io->Stop();
  }

};


#endif

