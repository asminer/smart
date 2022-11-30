
#include "../include/defines.h"
#include "textfmt.h"

#include <iomanip>
#include <cstring>

const int item_sep=4;  // separation distance in descriptions

inline bool EndOfWord(char c)
{
    return (c==0) || (c==' ') || (c=='\n');
}

// ==================================================================
// |                                                                |
// |                     doc_formatter  methods                     |
// |                                                                |
// ==================================================================

doc_formatter::doc_formatter(unsigned width, std::ostream& o)
 : out(o)
{
    // pagewidth = width;
    left = 0;
    right = (width ? width-1 : 0);
    in_heading = false;
    indent_depth = 0;
    desc_width = 0;
}

void doc_formatter::section(const char* name)
{
    FlushText();
    buffer << name;
    FlushText();
}

void doc_formatter::begin_heading()
{
    FlushText();
    in_heading = true;
}

void doc_formatter::end_heading()
{
    FlushText();
    in_heading = false;
}

void doc_formatter::begin_indent()
{
    FlushText();
    indent_depth++;
    left += 4;
    DCASSERT(right>=4);
    right -= 4;
}

void doc_formatter::end_indent()
{
    FlushText();
    if (0==indent_depth)  return;
    indent_depth--;
    DCASSERT(left>=4);
    left -= 4;
    right += 4;
}

void doc_formatter::begin_description(unsigned w)
{
    FlushText();
    desc_width = w;
}

void doc_formatter::item(const char* s)
{
    unsigned old_left = left;
    left += desc_width+item_sep;
    FlushText();
    left = old_left;
    out << std::setw(left) << "";
    out << std::setw(desc_width) << std::left << s << std::right;
    out << std::setw(item_sep) << "";
}

void doc_formatter::end_description()
{
    unsigned old_left = left;
    left += desc_width+item_sep;
    FlushText();
    left = old_left;
    desc_width = 0;
}

void doc_formatter::eject_page()
{
    FlushText();
}

bool doc_formatter::Matches(const char* item, const char* keyword)
{
    if (NULL==keyword) return true;
    unsigned slen = strlen(keyword);
    unsigned last = strlen(item) - slen;
    for (unsigned i=0; i<=last; i++) {
        if (0==strncasecmp(item+i, keyword, slen)) return true;
    }
    return false;
}

void doc_formatter::FlushText()
{
    std::string doc = buffer.str();
    unsigned ptr = 0;
    unsigned linewidth = (right > left) ? (right - left) : 0;
    bool ignore_marg = desc_width;
    while (doc[ptr]) {
        // We are at the start of a line, skip whitespace (but not newlines)
        if (' ' == doc[ptr]) {
            ptr++;
            continue;
        }
        // margin, except the first line of a description
        if (ignore_marg) {
            ignore_marg = false;
        } else {
            out << std::setw(left) << "";
        }
        //
        // write first word, regardless of length
        unsigned written = 0;
        while (1) {
            if (EndOfWord(doc[ptr]))  break;
            if ('~'==doc[ptr])  out << ' ';
            else                out << doc[ptr];
            written++;
            ptr++;
        }
        //
        // continue writing, one word at a time, until either:
        // () we exceed the line width, or
        // () we hit a newline, or
        // () we reach the end of input.
        while (written < linewidth) {
            if (0==doc[ptr])    break;
            if ('\n'==doc[ptr]) {
                ptr++;
                break;
            }
            // print inter-word space
            out << ' ';
            written++;
            // advance to start of next word
            while (doc[ptr]==' ') ptr++;
            // count spaces of next word
            unsigned ns = ptr;
            while (1) {
                if (EndOfWord(doc[ns])) break;
                ns++;
                written++;
            }
            if (written < linewidth) {
                // write the next word
                while (1) {
                    if (EndOfWord(doc[ptr])) break;
                    if ('~'==doc[ptr]) out << ' ';
                    else out << doc[ptr];
                    ptr++;
                }
            }
        } // while written
        // That's all we can fit on this line, or it is the last line.
        out << '\n';
    } // while doc[ptr]
    out.flush();
    // Clear the buffer
    buffer.str(std::string());
}

