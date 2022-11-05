
#ifndef STRINGS_H
#define STRINGS_H

#include "../include/shared.h"

/** Shared strings.
    Used so we can "share" strings without copying them.
    Plus, any string result will be stored using this class.
*/
class shared_string : public shared_object {
        char* string;
    public:
        /// Constructor, sets string to null.
        shared_string();
        /** Constructor.
            @param  s   String to fill from.
                        Will be taken (i.e., owned)
                        by the object created.
        */
        shared_string(char* s);
        unsigned length() const;
    protected:
        virtual ~shared_string();
    public:
        void CopyFrom(const char* s);
        inline const char* getStr() const { return string; }
        virtual bool Print(OutputStream &s, int width) const;
        virtual bool Equals(const shared_object *o) const;
        int Compare(const shared_string* s) const;
};

#endif
