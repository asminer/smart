
#ifndef STRINGS_H
#define STRINGS_H

#include "../include/shared.h"

/** Shared strings.
    Used so we can "share" strings without copying them.
    Plus, any string result will be stored using this class.
*/
class shared_string : public shared_object {
        /// The 'raw' string
        char* string;
        /// The string with special chars replaced.
        /// Will point to the raw string if no special chars.
        char* printable;
    public:
        /// Constructor, sets string to null.
        shared_string();
        /** Constructor.
            @param  s   String to fill from.  Will be copied.
        */
        shared_string(const char* s);
        /** Constructor.
            @param  s   String to fill from.  Will be copied.
        */
        shared_string(const std::string &s);
        unsigned length() const;
    protected:
        virtual ~shared_string();
    public:
        inline const char* getStr() const { return string; }
        virtual bool Print(std::ostream &s, int indent=0) const;
        virtual bool Equals(const shared_object *o) const;
        int Compare(const shared_string* s) const;
        int Compare(const char* x) const;
};

#endif
