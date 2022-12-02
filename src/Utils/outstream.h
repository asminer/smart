
#ifndef OUTSTREAM_H
#define OUTSTREAM_H

#include "../include/defines.h"

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

class shared_string;
class location;
class option_manager;

class outputStream {
    public:
        outputStream(std::ostream &_deflt);
        virtual ~outputStream();

        /**
         * Build an option to set the real format for this stream.
         *  @param  om      Option manager to get the option
         *  @param  name    Name of the option
         *  @param  doc     Documentation for the option
         */
        void buildRealOption(option_manager* om, const char* name,
                const char* doc);

        /**
         * Build an option to set the thousands separator for this stream.
         *  @param  om      Option manager to get the option
         *  @param  name    Name of the option
         *  @param  doc     Documentation for the option
         */
        void buildThousandsOption(option_manager* om, const char* name,
                const char* doc);


        /** Switch to a file with given name.
         *  The current file, if any, is closed.
         *
         *      @param  outfile     Name of new output file to write to.
         *                          If it cannot be opened, use the default.
         *
         *      @return true        Iff we were able to open the file.
         */
        bool switchOutput(const char* outfile);

        /** Switch to the default output.
         *  The current file, if any, is closed.
         */
        void defaultOutput();

        inline void activate()          { active = true; }
        inline void deactivate()        { active = false; }
        inline bool isActive() const    { return active; }

        inline std::ostream& stream() {
            DCASSERT(active);
            return (fout.is_open()) ? fout : deflt;
        }

        inline void incIndent() {
            indent_spaces += 4;
        }
        inline void decIndent() {
            if (indent_spaces) indent_spaces -= 4;
        }
        inline void clearIndent() {
            indent_spaces = 0;
        }
        inline void newLine(const char* prefix=nullptr) {
            if (prefix) {
                stream()    << std::endl << prefix
                            << std::setw(indent_spaces) << "";
            } else {
                stream()    << std::endl << std::setw(indent_spaces) << "";
            }
        }

        /*
         * Write a signed integer with commas.
         */
        void putWithCommas(long x);

        /*
         * Write an unsigned integer with commas.
         */
        void putWithCommas(unsigned long x);

        /*
         * Write an integer or real, encoded as a string, with commas.
         * The integer portion may start with -, +, or a digit,
         * and ends with the first non-digit.
         */
        void putWithCommas(const char* x);


        /*
         * For lack of a better place: global output stream
         * Basically, cout but with possibility of redirection to a file.
         */
        static inline outputStream& globalOut() { return Out; }

    private:
        static outputStream Out;

        unsigned indent_spaces;

        std::ostream &deflt;
        std::ofstream fout;

        unsigned realfmt;
        shared_string* comma;

        bool active;

        void update_real_format();
        friend class rfwatch;
};

template <class TYPE>
inline outputStream& operator<< (outputStream &s, const TYPE &t)
{
    if (s.isActive()) {
        s.stream() << t;
    }
    return s;
}


/*
 * Formatted memory usage.
 */
class memoryCount {
        size_t bytes;
        unsigned prec;
    public:
        /*
         * Constructor.
         *  @param  b       Number of bytes
         *  @param  p       Desired precision
         */
        inline memoryCount(size_t b, unsigned p) : bytes(b), prec(p) {
        }
        std::ostream& show(std::ostream &s) const;
};

/*
 * Formatted integers.
 */
class formatted_int {
        long val;
        int width;
    public:
        inline formatted_int(long v, int w) : val(v), width(w) {
        }
        std::ostream& show(std::ostream &s) const;
};

/*
 * Formatted reals.
 */
class formatted_real {
        double val;
        int width;
        int prec;
    public:
        inline formatted_real(double v, int w, int p=-1)
            : val(v), width(w), prec(p) {
            }
        std::ostream& show(std::ostream &s) const;
};

/*
 * Formatted strings.
 */
class formatted_string {
        const char* val;
        int width;
    public:
        inline formatted_string(const char* v, int w) : val(v), width(w) {
        }
        std::ostream& show(std::ostream &s) const;
};



inline std::ostream& operator<< (std::ostream &s, memoryCount m)
{
    return m.show(s);
}

inline std::ostream& operator<< (std::ostream &s, formatted_int m)
{
    return m.show(s);
}

inline std::ostream& operator<< (std::ostream &s, formatted_real m)
{
    return m.show(s);
}

inline std::ostream& operator<< (std::ostream &s, formatted_string m)
{
    return m.show(s);
}

//
// TBD: how to tie thousands separator to a std::ostream&
//      current thoughts:
//
//      * DON'T.  For output thousands separator, put
//        an option in type base class or something so
//        that integer and bigint types have access,
//        and add the commas when printing integers
//        in those types.
//
//        For report thousands separator, put it
//        in the report message class, and add a
//        method to comma separate strings, longs, unsigned longs.
//
//
//  TBD: how to correctly handle width and precision stuff
//
//      precision: must be passed to type/print stuff.
//
//      widths: in the function library for printing,
//              for any item that is being printed,
//              dump it to a string stream first,
//              then print the string with the appropriate width.
//
//  TBD: how to print out byte sizes
//
//      make a simple class here, memcount or something,
//      and overload << for it, so we can use
//      something like
//
//          cout << memcount(1231312223) << "\n"
//
//      or maybe
//
//          cout << binprefix(1231241341, "bytes") << "\n";
//



#endif

