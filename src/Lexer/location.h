#ifndef LOCATION_H
#define LOCATION_H

#include "strings.h"

class location {
        shared_string* filename;
        unsigned linenumber;
    public:
        location();
        location(const location& L);
        void operator=(const location& L);

        ~location();

        inline operator bool() const {
            return linenumber;
        }

        inline const char* getFile() const {
            return filename ? filename->getStr() : 0;
        }
        inline unsigned getLine() const {
            return linenumber;
        }
        inline void newline() {
            ++linenumber;
        }

        void show(OutputStream &s) const;

        void start(const char* fn);

        void clear();
};

inline OutputStream& operator<< (OutputStream &s, const location &L)
{
    L.show(s);
    return s;
}

#endif
