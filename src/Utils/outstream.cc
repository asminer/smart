
#include "outstream.h"
#include "strings.h"

#include <iomanip>

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

// ======================================================================
//
// These will eventually become global functions
//
// ======================================================================


void Pad(std::ostream &s, char repeat, int count)
{
    for (; count>0; count--) {
        s.put(repeat);
    }
}



void outputStream::PutMemoryCount(size_t bytes, int prec)
{
    const double kilo = bytes / 1024.0;
    const double mega = kilo / 1024.0;
    const double giga = mega / 1024.0;
    const double tera = giga / 1024.0;

    const char* units = " bytes";
    double show = bytes;
    if (tera > 1.0) {
        show = tera;
        units = " Tibytes";
    } else if (giga > 1.0) {
        show = giga;
        units = " Gibytes";
    } else if (mega > 1.0) {
        show = mega;
        units = " Mibytes";
    } else if (kilo > 1.0) {
        show = kilo;
        units = " Kibytes";
    }
    prec++;
    if (show >= 10.0)  prec++;
    if (show >= 100.0)  prec++;

    Put(show, 0, prec);
    out() << units;
}

void outputStream::PutHex(unsigned char data)
{
    out() << std::hex << std::setfill('0') << std::setw(2) << data << std::dec;
}

void outputStream::PutHex(unsigned data)
{
    out() << std::hex << std::setfill('0') << std::setw(8) << data << std::dec;
}

void outputStream::PutHex(unsigned long data)
{
    out() << std::hex << std::setfill('0') << std::setw(16) << data << std::dec;
}

void outputStream::PutInteger(const std::string &data, int width)
{
    static std::stringstream ss;

    unsigned next_comma = data.length() % 3;

    unsigned i;
    if (data[0] == '-') {
        ss << data[0] << data[1];
        i=2;
    } else {
        ss << data[0];
        i=1;
    }
    if (i>next_comma) next_comma += 3;

    while (data[i]) {
        if (i==next_comma) {
            ss << thousands->getStr();
            next_comma += 3;
        }
        ss << data[i];
        i++;
    } // while

    out() << std::setw(width) << ss.str();
}

void outputStream::Put(double data, int width)
{
    out() << std::setw(width) << data;
}

void outputStream::Put(double data, int width, int prec)
{
    int oldprec = out().precision();
    out().precision(prec);
    out() << std::setw(width) << data;
    out().precision(oldprec);
}

void outputStream::setRealFormat(unsigned rf)
{
    switch (rf) {
        case 1:     out() << std::fixed;            return;
        case 2:     out() << std::scientific;       return;
        default:    out() << std::defaultfloat;     return;
    }
}

