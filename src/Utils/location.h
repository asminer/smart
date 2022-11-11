#ifndef LOCATION_H
#define LOCATION_H

#include "strings.h"

class location {
        /*
         * Allow special locations:
         *      'f' : normal file or stdin (linenumber > 0)
         *      'c' : command line
         *      '$' : end of input
         *      'i' : internal
         *      ' ' : nowhere
         */
        char ltype;

        unsigned linenumber;

        /// If null then standard input
        shared_string* filename;

    public:
        /// Defaults to nowhere
        location();
        location(shared_string* fn, unsigned ln);
        location(const location& L);

        void reset(shared_string* fn, unsigned ln);

        inline void operator=(const location& L) {
            reset(L.filename, L.linenumber);
        }

        ~location();

        inline operator bool() const {
            return filename;
        }

        // TBD: need to remove these:
        //
        inline shared_string* shareFile() const {
            return Share(filename);
        }
        inline const char* getFile() const {
            return filename ? filename->getStr() : 0;
        }
        inline unsigned getLine() const {
            return linenumber;
        }
        //
        //
        //

        inline void newline() {
            ++linenumber;
        }
        inline void setline(unsigned ln) {
            linenumber = ln;
        }

#ifdef OLD_STREAMS
        void show(OutputStream &s) const;
#else
        void show(std::ostream &s) const;
#endif

        void start(const char* fn);

        void clear();

        inline bool is_stdin() const {
            return (0==filename) && ('f' == ltype);
        }

        // the special locations

        static const location& CMDLINE();
        static const location& EOINPUT();
        static const location& NOWHERE();
        static const location& INTERNALLY();
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
