
#include "swstream.h"

outputStream::outputStream(std::ostream &_deflt) : deflt(_deflt)
{
}

outputStream::~outputStream()
{
}

bool outputStream::switchOutput(const char* outfile)
{
    if (fout.is_open()) fout.close();
    fout.open(outfile, std::fstream::out);
    return fout.good();
}

void outputStream::defaultOutput()
{
    if (fout.is_open()) {
        fout.close();
    }
}

