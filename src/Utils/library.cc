
#include "library.h"
#include "textfmt.h"

const library** library::extlibs = nullptr;
unsigned library::num_libs = 0;
unsigned library::max_libs = 0;

// ******************************************************************
// *                        library  methods                        *
// ******************************************************************

library::library(bool has_cr, bool has_date)
{
    has_copyright = has_cr;
    has_release_date = has_date;
}

library::~library()
{
    unregisterLibrary(this);
}

void library::printCopyright(doc_formatter&) const
{
    DCASSERT(0);
}

void library::printReleaseDate(std::ostream &) const
{
    DCASSERT(0);
}

//
// Statics
//

void library::registerLibrary(const library* lib)
{
    if (0==lib) return;

    // Check for duplicates

    for (unsigned i=0; i<num_libs; i++) {
        if (lib == extlibs[i]) return;
        if (lib->is_duplicate(extlibs[i])) return;
    }

    // Not a duplicate.
    // First, check for any holes in the list
    // (can happen if a library is un-registered).

    for (unsigned i=0; i<num_libs; i++) {
        if (extlibs[i]) continue;
        extlibs[i] = lib;
        return;
    }

    // No holes. Add to the end.
    // But first, check if we need to expand the array

    if (num_libs >= max_libs) {
        // Note: implicitly assuming there won't be very many libraries
        max_libs += 16;
        extlibs = (const library**) realloc(extlibs, max_libs * sizeof(void*));
        DCASSERT(extlibs);
    }

    extlibs[num_libs++] = lib;
}

void library::unregisterLibrary(const library* lib)
{
    if (0==lib) return;

    for (unsigned i=0; i<num_libs; i++) {
        if (extlibs[i] != lib) continue;
        extlibs[i] = nullptr;
    }
}

void library::printLibraryVersions(std::ostream &s)
{
    for (unsigned i=0; i<num_libs; i++) {
        if (!extlibs[i]) continue;
        s << '\t';
        extlibs[i]->printVersion(s);
        s << '\n';
    }
}

void library::printLibraryCopyrights(doc_formatter &df)
{
    for (unsigned i=0; i<num_libs; i++) {
        if (!extlibs[i]) continue;
        if (!extlibs[i]->hasCopyright()) continue;
        df.Out() << "\n";
        df.begin_heading();
        extlibs[i]->printVersion(df.Out());
        if (extlibs[i]->hasReleaseDate()) {
            df.Out() << ", released ";
            extlibs[i]->printReleaseDate(df.Out());
        }
        df.end_heading();
        extlibs[i]->printCopyright(df);
    }
}




