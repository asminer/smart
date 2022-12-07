
#ifndef TEXTFMT_H
#define TEXTFMT_H

#include <iostream>
#include <sstream>
#include "outstream.h"

/** Centralized documentation formatting class.

    The main idea is to dump strings into the formatter,
    using the comfortable OutputStream interface,
    and it will automatically wrap lines around, and
    produce generally pleasing results.

*/
class doc_formatter {
//        unsigned pagewidth;
        unsigned left;
        unsigned right;
        outputStream &out;
        std::stringstream buffer;

        bool in_heading;
        unsigned indent_depth;
        unsigned desc_width;

    public:
        doc_formatter(unsigned _pagewidth, outputStream &_out);

        /// Write text here.
        inline std::ostream& Out() { return buffer; }

        /// Start a new section, with given name.
        void section(const char* name);

        /** Signifies the start of a heading.
            This should be called when starting to display
            an "entry", e.g., a function header.
        */
        void begin_heading();

        /// Signifies the end of a heading.
        void end_heading();

        /// Increase level of indentation.
        void begin_indent();

        /// Decrease level of indentation.
        void end_indent();

        /** Start a "description" environment.
            This is a list of the form:
                item_1  description of item 1
                item_2  description of item 2, possibly so long that
                        it is forced to wrap around.

                @param  width  Width (in chars) of the widest "item".
        */
        void begin_description(unsigned width);

        /** Set the next item in the list.
            Does nothing if we are not within a description environment.
                @param  str  The next item.
        */
        void item(const char* str);

        /// end a "description" environment.
        void end_description();

        /// eject the current "page" of text.
        void eject_page();

  // Not strictly documentation formatting, but it needs to go somewhere...

        /** Determine if a string matches a keyword (for searches).
            Default behavior: check if the string contains the keyword
            (ignoring case).
            Derive a class from this and override if you want other behavior.
        */
        static bool Matches(const char* str, const char* keyword);

    protected:
        void FlushText();
};



#endif
