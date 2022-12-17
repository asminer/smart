#ifndef LIBRARY_H
#define LIBRARY_H

#include "../include/defines.h"
#include <iostream>
class doc_formatter;

/** Abstract base class for external libraries.
    Right now, this is used only for "credits".

    To register an external library, derive a class from this one
    and implement the virtual functions.
*/
class library {
    bool has_copyright;
    bool has_release_date;
public:
    library(bool has_cr, bool has_date);
    virtual ~library();

    virtual void printVersion(std::ostream &s) const = 0;

    /// Does the library have copyright info.
    inline bool hasCopyright() const { return has_copyright; }

    /** Print copyright info for a library.
        Default is an assertion violation:
        assuming there is no copyright info then this should not be called.
        Otherwise, if there is copyright info, then this must be overridden
        in the derived class.
    */
    virtual void printCopyright(doc_formatter &df) const;

    /// Does the library have a release date.
    inline bool hasReleaseDate() const { return has_release_date; }

    /** Print release date for a library.
        Default is an assertion violation.
    */
    virtual void printReleaseDate(std::ostream &s) const;

// These methods used to be in exprman, now they're here
public:
    static void printLibraryVersions(std::ostream &s);
    static void printLibraryCopyrights(doc_formatter &df);

protected:
    /**
        Check for duplicates.
        Default behavior is to return false;
        if there is a situation where the same library
        could be added twice, then override this method.
    */
    virtual bool is_duplicate(const library* lib) const;

    // Should be called in derived class constructor
    static void registerLibrary(const library* lib);

private:
    static void unregisterLibrary(const library* lib);

private:
    // List of registered libraries.
    static const library** extlibs;
    static unsigned num_libs;
    static unsigned max_libs;
};

#endif
