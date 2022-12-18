
#ifndef OUTSTREAM_H
#define OUTSTREAM_H

#include "../include/defines.h"

#include <iostream>
#include <fstream>

class outputStream {
    public:
        static const unsigned GENERAL    = 0;
        static const unsigned FIXED      = 1;
        static const unsigned SCIENTIFIC = 2;
    public:
        outputStream(std::ostream &_deflt);

        virtual ~outputStream();

        outputStream(const outputStream &) = delete;
        void operator=(const outputStream &) = delete;

        //
        // Set the real format for this stream.
        inline void set_real_format(unsigned rf) {
            CHECK_RANGE(0, rf, 3);
            realfmt = rf;
            update_real_format();
        }

        //
        // Get the current real format for this stream.
        inline unsigned get_real_format() const { return realfmt; }

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

        inline std::ostream& stream() {
            return (fout.is_open()) ? fout : deflt;
        }

        inline void activate()          { active = true; }
        inline void deactivate()        { active = false; }
        inline bool isActive() const    { return active; }

        inline void flush()             { stream().flush(); }

        inline void incIndent()         { indent_spaces += 4; }
        inline void decIndent()         {
            if (indent_spaces) indent_spaces -= 4;
        }
        inline void clearIndent(unsigned sp=0) {
            indent_spaces = sp*4;
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
        void putWithCommas(long x, int width=0);

        /*
         * Write an unsigned integer with commas.
         */
        void putWithCommas(unsigned long x, int width=0);

        /*
         * Write an integer or real, encoded as a string, with commas.
         * The integer portion may start with -, +, or a digit,
         * and ends with the first non-digit.
         */
        void putWithCommas(const char* x, int width=0);


        /*
         * For lack of a better place: global output stream
         * Basically, cout but with possibility of redirection to a file.
         */
        static inline outputStream& globalOut() { return Out; }

    public:
        const char* comma;
    private:
        static outputStream Out;

        unsigned indent_spaces;

        std::ostream &deflt;
        std::ofstream fout;

        unsigned realfmt;

        bool active;

        void update_real_format();
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

/*
 * Write an array.
 */
template <class DATA>
class element_writer {
        const DATA* array;
        unsigned elements;
    public:
        inline element_writer(const DATA* a, unsigned e) {
            array = a;
            elements = e;
        }
        std::ostream& show(std::ostream &s) const {
            if (elements) s << array[0];
            for (unsigned i=1; i<elements; i++) {
                s << ", " << array[i];
            }
            return s;
        }
};

/*
 * Padding
 */
class padding {
        unsigned count;
        char fill;
    public:
        inline padding(unsigned n, char f=' ') {
            count = n;
            fill = f;
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

template <class DATA>
inline std::ostream& operator<< (std::ostream &s, element_writer <DATA> m)
{
    return m.show(s);
}

inline std::ostream& operator<< (std::ostream &s, padding p)
{
    return p.show(s);
}


#endif

