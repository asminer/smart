
#ifndef MESSAGES_H
#define MESSAGES_H

#include "../include/defines.h"
#include "outstream.h"

class location;

/**
 * Base class for switchable.
 * I.e., warning messages, reporting messages, debugging messages,
 * that may be switched on and off.
 *
*/
class switchable_msg {
//    friend class checklist_opt;
    const char* option_name;
    const char* my_name;
    bool active;
public:
    switchable_msg(const char* optname, const char* myname);

    inline void Activate()    { active = true; }
    inline void Deactivate()  { active = false; }
    inline bool isActive() const { return active; }

    inline const char* optName() const  { return option_name; }
    inline const char* getName() const  { return my_name; }
    inline bool& Active()               { return active; }
};


/*
 * Warning messages.  Default to "active".
 */
class warning_msg : public switchable_msg {
    public:
        static outputStream Out;
    public:
        warning_msg(const char* myname);

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
        static inline void newLine(char tab=0) {
            if ('+' == tab) Out.incIndent();
            if ('-' == tab) Out.decIndent();
            Out.newLine();
        }
        static inline void stop(bool nl=true) {
            if (nl) stream() << std::endl;
            Out.deactivate();
        }
};

/*
 * Named messages.
 * The option name is part of the message prefix.
 */
class named_msg : public switchable_msg {
        static char prefix[256];
    protected:
        const char* setPrefix(char x) const;
        const char* getPrefix() const { return prefix; }
    public:
        named_msg(const char* optname, const char* mn);
};

/*
 * Reporting messages.  Default to "inactive".
 */
class reporting_msg : public named_msg {
    public:
        static outputStream Out;
    public:
        reporting_msg(const char* myname);

        inline static bool switchOutput(const char* outfile) {
            return Out.switchOutput(outfile);
        }
        inline static void defaultOutput() {
            Out.defaultOutput();
        }

        bool start() const;

        static inline std::ostream& stream() {
            return Out.stream();
        }
        inline void newLine(char tab=0) const {
            if ('+' == tab) Out.incIndent();
            if ('-' == tab) Out.decIndent();
            Out.newLine(getPrefix());
        }
        static inline void stop(bool nl=true) {
            if (nl) stream() << std::endl;
            Out.deactivate();
        }
};

/*
 * Debugging messages.  Default to "inactive".
 */
class debugging_msg : public named_msg {
    public:
        static outputStream Out;
    public:
        debugging_msg(const char* myname);

        inline static bool switchOutput(const char* outfile) {
            return Out.switchOutput(outfile);
        }
        inline static void defaultOutput() {
            Out.defaultOutput();
        }

        bool start() const;

        static inline std::ostream& stream() {
            return Out.stream();
        }
        inline void newLine(char tab=0) const {
            if ('+' == tab) Out.incIndent();
            if ('-' == tab) Out.decIndent();
            Out.newLine(getPrefix());
        }
        static inline void stop(bool nl=true) {
            if (nl) stream() << std::endl;
            Out.deactivate();
        }
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
        static inline std::ostream& stream() {
            return Out.stream();
        }
        inline void newLine(char tab=0) const {
            if ('+' == tab) Out.incIndent();
            if ('-' == tab) Out.decIndent();
            Out.newLine();
        }
};

/*
 * Fatal, internal errors.
 * Displays info to help developers debug.
 */
class internal_error : public error_msg {
    public:
        internal_error(const char* sfile, unsigned sline);
        internal_error(const char* sfile, unsigned sline, const location &cause);
        ~internal_error();  // terminates!
};

/*
 * Like warning_msg, but not part of the Warning option.
 * Long term: replace these instances with warning_msgs.
 */
class unnamed_warning : public error_msg {
    public:
        unnamed_warning();
        unnamed_warning(const location &cause);
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

