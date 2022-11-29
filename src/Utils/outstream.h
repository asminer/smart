
#ifndef OUTSTREAM_H
#define OUTSTREAM_H

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

        // TBD: option for thousands separator

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

        inline void indentMore() {
            indent_spaces += 4;
        }
        inline void indentLess() {
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
         * TBD - remove comma parameter and pull it from option
         */
        void putWithCommas(long x, const char* comma);

        /*
         * Write an unsigned integer with commas.
         */
        void putWithCommas(unsigned long x, const char* comma);

        /*
         * Write an integer or real, encoded as a string, with commas.
         * The integer portion may start with -, +, or a digit,
         * and ends with the first non-digit.
         */
        void putWithCommas(const char* x, const char* comma);


        /*
        //
        // Custom put thingies here for convenience
        //
        void PutHex(unsigned char data);
        void PutHex(unsigned int  data);
        void PutHex(unsigned long data);
        void PutMemoryCount(size_t bytes, int prec);

        /// Makes comma-separated integers.
        template <class T>
        inline void PutInt(T data, int width=0)
        {
            static std::stringstream ss;
            ss << data;
            PutInteger(ss.str(), width);
        }
        void PutInteger(const std::string &integer, int width);

        // For reals
        void Put(double data, int width);
        void Put(double data, int width, int prec);

        //
        // For options later
        //
        inline shared_string* & linkThousands() {
            return thousands;
        }
        void setRealFormat(unsigned rf);
        */

    private:
        unsigned indent_spaces;

        std::ostream &deflt;
        std::ofstream fout;

        unsigned realfmt;

        void update_real_format();
        friend class rfwatch;
};

template <class TYPE>
inline outputStream& operator<< (outputStream &s, const TYPE &t)
{
    s.stream() << t;
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

inline std::ostream& operator<< (std::ostream &s, memoryCount m)
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

