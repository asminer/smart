
/** \file options.h

    New option interface.
*/

#ifndef OPTIONS_H
#define OPTIONS_H

class OutputStream;  // defined in streams.h
class doc_formatter;   // defined in streams.h
class shared_string;

class option_enum;
class radio_button;
class checklist_enum;

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

        virtual unsigned NumConstants() const;
        virtual option_enum* GetConstant(unsigned i) const;

        virtual void ShowHeader(OutputStream &s) const = 0;
        virtual void ShowCurrent(OutputStream &s) const;
        virtual void ShowRange(doc_formatter* df) const = 0;

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

        /*
         * Add a subscriber to the list.
         * Any time the option changes, all subscribers are notified.
         */
        void registerWatcher(watcher* w);

        /**
            Build and add a radio button to this option.
                @param  name    Button name.
                @param  doc     Button documentation.
                @param  ndx     Button index.

                @return NULL, if this is not a radio button option,
                              or some other error occurred.
                        The radio button (as an option_enum), otherwise.
        */
        virtual radio_button* addRadioButton(const char* name,
                                const char* doc, unsigned ndx);

        /**
            Build and add an item to a checklist.
                @param  grp   Group, or null for none.
                @param  name  The item name
                @param  doc   Documentation for the item.
                @param  link  Link to "are we checked or not".

                @return A new item, or NULL on error.
        */
        virtual checklist_enum* addChecklistItem(checklist_enum* grp,
                const char* name, const char* doc, bool &link);

        /**
            Build and add an item that's a group of items to a checklist.
                @param  name    The item name
                @param  doc     Documentation for the item.
                @param  ni      Max items to add to the group.

                @return A new item, or NULL on error.
        */
        virtual checklist_enum* addChecklistGroup(const char* name,
                const char* doc, unsigned ni);

    protected:
        error notifyWatchers() const;
};

#endif

