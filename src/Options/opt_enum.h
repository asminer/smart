
/*
 *  Names for option settings.
 *  I.e., the enumerated-type kind of option names that
 *  can appear as an option setting, such as
 *  #MCSolver JACOBI
 */

#ifndef OPT_ENUM_H
#define OPT_ENUM_H

class OutputStream;
class doc_formatter;   // defined in streams.h
class option_manager;

// **************************************************************************
// *                                                                        *
// *                         option_enum  interface                         *
// *                                                                        *
// **************************************************************************

/** Abstract base class for option enumerated values.
    Used by Radio button and checkbox type options.
    Neat trick!  We can have suboptions for these now!
*/
class option_enum {
    public:
        option_enum(const char* n, const char* d);
        virtual ~option_enum();

        inline const char* Name() const { return name; }
        inline const char* Documentation() const { return doc; }

        void show(OutputStream &s) const;

        int Compare(const option_enum* b) const;
        int Compare(const char* name) const;

        inline const option_manager* readSettings() const { return settings; }
        inline void makeSettings(const option_manager* s) { settings = s; }

        bool isApropos(const doc_formatter* df, const char* keyword) const;

    private:
        /// Name of the constant.
        const char* name;
        /// Documentation.
        const char* doc;
    protected:
        /// Settings (suboptions) for this button; null for none.
        const option_manager* settings;
};


#endif
