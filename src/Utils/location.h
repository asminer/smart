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
            return filename;
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

#ifdef OLD_STREAMS
        void show(OutputStream &s) const;
#else
        void show(std::ostream &s) const;
#endif

        void start(const char* fn);

        void clear();

        static const location& NOWHERE();
};


#ifdef OLD_STREAMS
inline OutputStream& operator<< (OutputStream &s, const location &L)
{
    L.show(s);
    return s;
}
#else
inline std::ostream& operator<< (std::ostream &s, const location &L)
{
    L.show(s);
    return s;
}
#endif

#endif
