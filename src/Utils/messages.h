
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
        static outputStream Warning;
    public:
        warning_msg();

        inline static bool switchOutput(const char* outfile) {
            return Warning.switchOutput(outfile);
        }
        inline static void defaultOutput() {
            Warning.defaultOutput();
        }

        bool startWarning(const location &L) const;

        inline std::ostream& warn() const {
            return Warning.out();
        }
        inline void newLine() const {
            Warning.out() << "\n    ";
        }
        inline void stopIO() const {
            Warning.out() << std::endl;
        }

        static inline outputStream& getStream() {
            return Warning;
        }
};

/*
 * Reporting messages.  Default to "inactive".
 */
class reporting_msg : public abstract_msg {
        static outputStream Report;
    public:
        reporting_msg();

        inline static bool switchOutput(const char* outfile) {
            return Report.switchOutput(outfile);
        }
        inline static void defaultOutput() {
            Report.defaultOutput();
        }

        bool startReport() const;

        inline std::ostream& report() const {
            return Report.out();
        }
        inline void newLine() const {
            Report.out() << "\nR" << getName() << ": ";
        }
        inline void stopIO() const {
            Report.out() << std::endl;
        }
        static inline outputStream& getStream() {
            return Report;
        }
};

/*
 * Debugging messages.  Default to "inactive".
 */
class debugging_msg : public abstract_msg {
        static outputStream Debug;
    public:
        debugging_msg();

        inline static bool switchOutput(const char* outfile) {
            return Debug.switchOutput(outfile);
        }
        inline static void defaultOutput() {
            Debug.defaultOutput();
        }

        bool startDebug() const;

        inline std::ostream& debug() const {
            return Debug.out();
        }
        inline void newLine() const {
            Debug.out() << "\nD" << getName() << ": ";
        }
        inline void stopIO() const {
            Debug.out() << std::endl;
        }
        static inline outputStream& getStream() {
            return Debug;
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
        static outputStream Error;
    public:
        error_msg(const char* prefix);
        ~error_msg();

        inline std::ostream& err() const {
            return Error.out();
        }
        inline void newLine() const {
            Error.out() << "\n    ";
        }

        inline static bool switchOutput(const char* outfile) {
            return Error.switchOutput(outfile);
        }
        inline static void defaultOutput() {
            Error.defaultOutput();
        }
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
inline std::ostream& operator<<(warning_msg &W, const TYPE& t)
{
    return W.warn() << t;
}

template <class TYPE>
inline std::ostream& operator<<(reporting_msg &R, const TYPE& t)
{
    return R.report() << t;
}

template <class TYPE>
inline std::ostream& operator<<(debugging_msg &D, const TYPE& t)
{
    return D.debug() << t;
}

template <class TYPE>
inline std::ostream& operator<<(error_msg &E, const TYPE& t)
{
    return E.err() << t;
}

#endif

