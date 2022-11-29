
#ifndef MESSAGES_H
#define MESSAGES_H

#include "../include/defines.h"
#include "outstream.h"

class option_manager;
class checklist_enum;

class location;

/**
 * Base class for named messages.
 * I.e., warning messages, reporting messages, debugging messages,
 * that may be switched on and off.
 *
*/
class abstract_msg {
    friend class checklist_opt;
    const char* option_name;
    const char* name;
    bool active;
protected:
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
    inline bool isActive() const { return active; }
};


/*
 * Warning messages.  Default to "active".
 */
class warning_msg : public abstract_msg {
    public:
        static outputStream Out;
    public:
        warning_msg();

        inline static bool switchOutput(const char* outfile) {
            return Out.switchOutput(outfile);
        }
        inline static void defaultOutput() {
            Out.defaultOutput();
        }

        bool start(const location &L) const;

        static inline std::ostream& stream() {
            return Out.stream();
        }
        static inline void indentMore() {   Out.indentMore();       }
        static inline void indentLess() {   Out.indentLess();       }
        static inline void newLine()    {   Out.newLine();          }
        static inline void stop()       {   stream() << std::endl;  }
};

/*
 * Reporting messages.  Default to "inactive".
 */
class reporting_msg : public abstract_msg {
    public:
        static outputStream Out;
    public:
        reporting_msg();

        inline static bool switchOutput(const char* outfile) {
            return Out.switchOutput(outfile);
        }
        inline static void defaultOutput() {
            Out.defaultOutput();
        }

        bool start() const;

        static inline std::ostream& stream() { return Out.stream(); }
        static inline void indentMore() {   Out.indentMore();       }
        static inline void indentLess() {   Out.indentLess();       }
        inline void newLine() const     {   Out.newLine(prefix);    }
        static inline void stop()       {   stream() << std::endl;  }
    private:
        static char prefix[256];
};

/*
 * Debugging messages.  Default to "inactive".
 */
class debugging_msg : public abstract_msg {
    public:
        static outputStream Out;
    public:
        debugging_msg();

        inline static bool switchOutput(const char* outfile) {
            return Out.switchOutput(outfile);
        }
        inline static void defaultOutput() {
            Out.defaultOutput();
        }

        bool start() const;

        static inline std::ostream& stream() { return Out.stream(); }
        static inline void indentMore() {   Out.indentMore();       }
        static inline void indentLess() {   Out.indentLess();       }
        inline void newLine() const     {   Out.newLine(prefix);    }
        static inline void stop()       {   stream() << std::endl;  }
    private:
        static char prefix[256];
};



/*
 *  Abstract base class for error messages.
 *
 *  Derived classes have different start of error
 *  and end of error text and possibly actions.
 *
 *  Start of error is handled in derived constructors.
 *  Stop of error is handled in derived destructors.
 *
 */
class error_msg {
    public:
        static outputStream Out;
    public:
        error_msg(const char* prefix);
        ~error_msg();

        static inline bool switchOutput(const char* outfile) {
            return Out.switchOutput(outfile);
        }
        static inline void defaultOutput() {
            Out.defaultOutput();
        }
        static inline std::ostream& stream() { return Out.stream(); }
        static inline void indentMore() {   Out.indentMore();       }
        static inline void indentLess() {   Out.indentLess();       }
        static inline void newLine()    {   Out.newLine();          }
};

class internal_error : public error_msg {
    public:
        internal_error(const char* sfile, unsigned sline);
        internal_error(const char* sfile, unsigned sline, const location &cause);
        ~internal_error();  // terminates!
};

/*
class parser_error : public error_msg {
    public:
        parser_error(location& W, const char* text);
};
*/

class typechecking_error : public error_msg {
    public:
        typechecking_error(const location& W);
};


/*
 *  Let us use all of these messages on lhs of <<.
 *
 */

template <class TYPE>
inline const warning_msg& operator<<(const warning_msg &W, const TYPE& t)
{
    W.stream() << t;
    return W;
}

template <class TYPE>
inline const reporting_msg& operator<<(const reporting_msg &R, const TYPE& t)
{
    R.stream() << t;
    return R;
}

template <class TYPE>
inline const debugging_msg& operator<<(const debugging_msg &D, const TYPE& t)
{
    D.stream() << t;
    return D;
}

template <class TYPE>
inline const error_msg& operator<<(const error_msg &E, const TYPE& t)
{
    E.stream() << t;
    return E;
}

#endif

