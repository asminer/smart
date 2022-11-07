
#ifndef OUTSTREAM_H
#define OUTSTREAM_H

#include <iostream>
#include <fstream>

class outputStream {
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

        /*
         * A bit ugly, but it allows us to use an outputStream
         * in place of an ostream before <<
         */
        template <class X>
        inline std::ostream& operator<< (X x) {
            return out() << x;
        }

        //
        // Custom put thingies here
        //

    private:
        std::ostream &deflt;
        std::ofstream fout;
};

#endif

