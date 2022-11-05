
/** \file options.h

    New option interface.
*/

#ifndef OPTIONS_H
#define OPTIONS_H

class OutputStream;  // defined in streams.h
class doc_formatter;   // defined in streams.h
class option_manager;
class shared_string;

#include "opt_enum.h"
#include "optman.h"

// **************************************************************************
// *                            option interface                            *
// **************************************************************************

/**  Base class for options.
     Some derived classes are "hidden" in options.cc.
*/
class option {
    public:
        /// Errors for options.
        enum error {
            /// The operation was successful.
            Success = 0,
            /// Type mismatch error.
            WrongType,
            /// Tried to set a value out of range.
            RangeError,
            /// Null action set/get function or pointer.
            NullFunction,
            /// Option is already finalized.
            Finalized,
            /// Duplicate checklist item.
            Duplicate
        };
        /// Types for options.
        enum type {
            /// Dummy type
            NoType = 0,
            /// Boolean options.
            Boolean,
            /// Integer options.
            Integer,
            /// Real options.
            Real,
            /// String options.
            String,
            /// Radio button options.
            RadioButton,
            /// Checklist options.
            Checklist
        };
        /// Subscribers: notify when option is changed
        class watcher {
            public:
                watcher();
                virtual ~watcher();
                virtual void notify(const option* opt) = 0;
            private:
                watcher* next;  // we'll have a list of these
                friend class option;
        };
    private:
        type mytype;
        const char* name;
        const char* documentation;
        bool hidden;
        watcher* watchlist;

    public:
        /** Constructor.
                @param  t  The type of option.
                @param  n  The option name.
                @param  d  Documentation for the option.
        */
        option(type t, const char* n, const char* d);
        virtual ~option();

        inline type Type() const { return mytype; }
        inline const char* Name() const { return name; }
        inline const char* GetDocumentation() const { return documentation; }
        inline bool IsUndocumented() const { return hidden; }
        inline void Hide() { hidden = true; }

        void show(OutputStream &s) const;

        /** Set the value for a boolean option.
                @param  b  Value to set.
                @return Appropriate error code.
        */
        virtual error SetValue(bool b);

        /** Set the value for an integer option.
                @param  n  Value to set.
                @return Appropriate error code.
        */
        virtual error SetValue(long n);

        /** Set the value for a real option.
                @param  r  Value to set.
                @return Appropriate error code.
        */
        virtual error SetValue(double r);

        /** Set the value for a string option.
                @param  c  Value to set.
                @return Appropriate error code.
        */
        virtual error SetValue(shared_string *c);

        /** Set the value for a radio button option.
                @param  c  Value to set.
                @return Appropriate error code.
        */
        virtual error SetValue(option_enum* c);

        /** Find the option constant for this option,
            with the specified name.
            If this is not a RadioButton or CheckList option,
            we always return 0.
                @param  name  Name of option constant to find.
                @return An option constant "owned" by this option
                        with the given name, if it exists; 0 otherwise.
        */
        virtual option_enum* FindConstant(const char* name) const;

        /** Get the value for a boolean option.
                @param  v  Value stored here.
                @return Appropriate error code.
        */
        virtual error GetValue(bool &v) const;

        /** Get the value for an integer option.
                @param  v  Value stored here.
                @return Appropriate error code.
        */
        virtual error GetValue(long &v) const;

        /** Get the value for a real option.
                @param  v  Value stored here.
                @return Appropriate error code.
        */
        virtual error GetValue(double &v) const;

        /** Get the value for a string option.
                @param  v  Value stored here.
                @return Appropriate error code.
        */
        virtual error GetValue(shared_string* &v) const;

        virtual int NumConstants() const;
        virtual option_enum* GetConstant(long i) const;

        virtual void ShowHeader(OutputStream &s) const = 0;
        virtual void ShowCurrent(OutputStream &s) const;
        virtual void ShowRange(doc_formatter* df) const = 0;

        /** Add a new item to a checklist option.
                @param  v  The new checklist item.
                @return Appropriate error code.
        */
        virtual error AddCheckItem(option_enum* v);

        /** Add a radio button while building a radio button option.
                @param  v  The new radio button item.
                @return Appropriate error code.
        */
        virtual error AddRadioButton(option_enum* v);

        /// Will be called when the option list is finalized.
        virtual void Finish();

        int Compare(const option* b) const;
        int Compare(const char* name) const;

        /// Determine if this option matches the given keyword.
        virtual bool isApropos(const doc_formatter* df, const char* keyword) const;

        /** Write documentation header and body for this option.
            @param  df  Document formatter; output is sent here.
        */
        void PrintDocs(doc_formatter* df, const char* keyword) const;

        /// Recursively document children as appropriate.
        virtual void RecurseDocs(doc_formatter* df, const char* keyword) const;

        void registerWatcher(watcher* w);
    protected:
        error notifyWatchers() const;
};

// **************************************************************************
// *                        custom_option  interface                        *
// **************************************************************************

/** If you want setting / getting option values to trigger an action,
    derive a class from this one and use it for your option.
    You must provide the appropriate SetValue() and GetValue() methods.
*/
class custom_option : public option {
  const char* range;
public:
  custom_option(type t, const char* name, const char* doc, const char* range);

  virtual ~custom_option();

  virtual void ShowHeader(OutputStream &s) const;
  virtual void ShowRange(doc_formatter* df) const;
  virtual bool isApropos(const doc_formatter* df, const char* keyword) const;
};


// **************************************************************************
// *                            Global interface                            *
// **************************************************************************


/** Make a new option of type "radio button",
    with radio buttons to be added later.
      @param  name      The option name
      @param  doc       Documentation for the option.
      @param  values    Radio buttons to add.
      @param  numvalues Number of radio buttons to be added.
      @param  link      Link to current selection (the value index).
      @return A new option, or NULL on error.
              An error will occur if the values are not sorted!
*/
option* MakeRadioOption(const char* name, const char* doc,
           radio_button** values, unsigned numvalues, unsigned& link);


/** Make a new option of type "radio button",
    with radio buttons to be added later.
      @param  name      The option name
      @param  doc       Documentation for the option.
      @param  numvalues Number of radio buttons to be added.
      @param  link      Link to current selection (the value index).
      @return A new option, or NULL on error.
              An error will occur if the values are not sorted!
*/
option* MakeRadioOption(const char* name, const char* doc,
           unsigned numvalues, unsigned& link);


/** Make a new option constant for a checklist.
      @param  name  The constant name
      @param  doc   Documentation for the option.
      @param  link  Link to "are we checked or not".
      @return A new constant, or NULL on error.
*/
checklist_enum* MakeChecklistConstant(const char* name, const char* doc, bool &link);

/** Make a new option constant for a group of checklist items.
      @param  name  The constant name
      @param  doc   Documentation for the option.
      @param  items Items that this group contains.
      @param  ni    Dimension of items array.
      @return A new constant, or NULL on error.
*/
checklist_enum* MakeChecklistGroup(const char* name, const char* doc, checklist_enum** items, int ni);

/** Make a new option of type "checklist".
    Possible items on the list are added dynamically.
      @param  name  The option name
      @param  doc   Documentation for the option.
      @return A new option, or NULL on error.
*/
option* MakeChecklistOption(const char* name, const char* doc);

/** Make a new option of type boolean.
      @param  name  The option name
      @param  doc   Documentation for the option.
      @param  link  Link to the boolean value; value can be
                    changed by the option or otherwise.
      @return A new option, or NULL on error.
*/
option* MakeBoolOption(const char* name, const char* doc, bool &link);

/** Make a new option of type integer.
      @param  name  The option name
      @param  doc   Documentation for the option.
      @param  link  Link to the value; value can be changed by
                    the option or otherwise.
      @param  min   The minimum allowed value.
      @param  max   The maximum allowed value.
                    Use min > max for all possible integers.
      @return A new option, or NULL on error.
*/
option* MakeIntOption(const char* name, const char* doc,
      long &link, long min, long max);


/** Make a new option of type real.
      @param  name            The option name
      @param  doc             Documentation for the option.
      @param  link            Link to the value; value can be changed by
                              the option or otherwise.
      @param  has_lower       Does the option have a lower bound?
                              If not, ignore the lower bound parameters.
      @param  includes_lower  Is the lower bound included in the
                              set of possible values?
      @param  lower           Lower bound.
      @param  has_upper       Does the option have an upper bound?
                              If not, ignore the upper bound parameters.
      @param  includes_upper  Is the upper bound included in the
                              set of possible values?
      @param  upper           Upper bound.
      @return A new option, or NULL on error.
*/
option* MakeRealOption(const char* name, const char* doc, double &link,
      bool has_lower, bool includes_lower, double lower,
      bool has_upper, bool includes_upper, double upper);


/** Make a new option of type string.
      @param  name  The option name.
      @param  doc   Documentation for the option.
      @param  link  Link to the value.
      @return  A new option, or NULL on error.
*/
option* MakeStringOption(const char* name, const char* doc, shared_string* &link);


#endif

