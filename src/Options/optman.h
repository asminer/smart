
#ifndef OPTMAN_H
#define OPTMAN_H

#include <iostream>

class doc_formatter;   // defined in streams.h
class option;
class shared_string;

// **************************************************************************
// *                        option_manager interface                        *
// **************************************************************************

/** Option manager (abstract) class.
    A centralized collection of options, with operations for
    addition of options (during initialization) and
    retrieval of optiions (during computation).
*/
class option_manager {
    public:
        option_manager();
        virtual ~option_manager();

        /** Called when initialization is complete.
            After this is called, no new options may be added.
        */
        virtual void DoneAddingOptions() = 0;

        /** Find an option with matching name.
            @param  name  Name of the desired option.
            @return The matching option, or NULL if none present with \a name.
        */
        virtual option* FindOption(const char* name) const = 0;

        /// Total number of options.
        virtual unsigned NumOptions() const = 0;

        /** Retrieve an option by index.
            This is useful for enumerating all options.
            @param  i  Index of option to retrieve.
            @return The ith option.
        */
        virtual option* GetOptionNumber(unsigned i) const = 0;

        /** For online help and documentation.
            Show documentation of all options matching the given keyword.
        */
        virtual void DocumentOptions(doc_formatter* df, const char* keyword)
            const = 0;

        /** For online help and documentation.
            List all options, with their current settings.
        */
        virtual void ListOptions(doc_formatter* df) const = 0;


        /** Make, add, and return a new option of type boolean.
        *
        *       @param  name  The option name
        *       @param  doc   Documentation for the option.
        *       @param  link  Link to the boolean value
        *
        *       @return A new option, or NULL on error.
        */
        option* addBoolOption(const char* name, const char* doc, bool &link);


        /** Make, add, and return a new option of type integer.
        *
        *       @param  name  The option name
        *       @param  doc   Documentation for the option.
        *       @param  link  Link to the value
        *       @param  min   The minimum allowed value.
        *       @param  max   The maximum allowed value.
        *
        *       @return A new option, or NULL on error.
        */
        option* addIntOption(const char* name, const char* doc,
            long &link, long min, long max);


        /** Make, add, and return a new option of type real.
        *
        *       @param  name            The option name
        *       @param  doc             Documentation for the option.
        *       @param  link            Link to the value
        *       @param  has_lower       Does the option have a lower bound? If
        *                               not, ignore the lower bound parameters.
        *       @param  includes_lower  Is the lower bound included in the
        *                               set of possible values?
        *       @param  lower           Lower bound.
        *       @param  has_upper       Does the option have an upper bound? If
        *                               not, ignore the upper bound parameters.
        *       @param  includes_upper  Is the upper bound included in the
        *                               set of possible values?
        *       @param  upper           Upper bound.
        *
        *       @return A new option, or NULL on error.
        */
        option* addRealOption(const char* name, const char* doc, double &link,
            bool has_lower, bool includes_lower, double lower,
            bool has_upper, bool includes_upper, double upper);


        /** Make, add, and return a new option of type string.
        *
        *       @param  name  The option name.
        *       @param  doc   Documentation for the option.
        *       @param  link  Link to the value.
        *
        *       @return  A new option, or NULL on error.
        */
        option* addStringOption(const char* name, const char* doc,
                    shared_string* &link);


        /** Make, add, and return a new option of type "radio button",
        *  where radio buttons are added later.
        *
        *       @param  name    The option name
        *       @param  doc     Documentation for the option.
        *       @param  numbuts The number of radio buttons to be added.
        *       @param  link    Link to the index() of the current selection.
        *
        *       @return A new option, or NULL on error.
        */
        option* addRadioOption(const char* name, const char* doc,
                        unsigned numbuts, unsigned& link);


        /** Make, add, and return a new option of type "checklist".
        *   Possible items on the list are added dynamically.
        *
        *       @param  name    The option name
        *       @param  doc     Documentation for the option.
        *
        *       @return A new option, or NULL on error.
        */
        option* addChecklistOption(const char* name, const char* doc);

    protected:
        virtual option* addOption(option*) = 0;
};

// **************************************************************************
// *                            Global interface                            *
// **************************************************************************

/** Make a new option manager.
      @return A new manager, or NULL on error.
*/
option_manager* MakeOptionManager();



#endif

