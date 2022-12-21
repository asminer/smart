
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
        virtual int Compare(const shared_object* s) const;
};

/** Shared constant strings.
    Like shared strings, but these are const char* so
    they are not deleted.
*/
class const_string : public shared_object {
        const char* string;
    public:
        const_string(const char* s=nullptr);
        inline const char* getStr() const { return string; }
        inline void setStr(const char* s) { string = s;    }
        virtual bool Print(std::ostream &s, int indent=0) const;
        virtual int Compare(const shared_object* s) const;
};

inline std::ostream& operator<< (std::ostream& s, const shared_string &x)
{
    x.Print(s);
    return s;
}

inline std::ostream& operator<< (std::ostream& s, const const_string &x)
{
    x.Print(s);
    return s;
}

#endif
