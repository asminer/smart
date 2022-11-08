
#ifndef OUTSTREAM_H
#define OUTSTREAM_H

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

class shared_string;

class outputStream {
    public:
        /*
         * How to format real values
         */
        static const unsigned RF_GENERAL = 0;
        static const unsigned RF_FIXED = 1;
        static const unsigned RF_SCIENTIFIC = 2;
    public:
        outputStream(std::ostream &_deflt);
        virtual ~outputStream();

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

        inline std::ostream& out() {
            return (fout.is_open()) ? fout : deflt;
        }

        inline void flush() {
            out().flush();
        }

        /*
         * A bit ugly, but it allows us to use an outputStream
         * in place of an ostream before <<
         */
        template <class X>
        inline std::ostream& operator<< (X x) {
            return out() << x;
        }

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

    private:
        std::ostream &deflt;
        std::ofstream fout;
        shared_string* thousands;
};


/*
 * Repeat the given character.
 */
void Pad(std::ostream &s, char repeat, int count);


#endif

