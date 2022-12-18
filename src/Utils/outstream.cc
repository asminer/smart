
#include "outstream.h"
#include "strings.h"

#include <iomanip>
#include <sstream>

outputStream outputStream::Out(std::cout);

// ======================================================================

outputStream::outputStream(std::ostream &_deflt) : deflt(_deflt)
{
    activate();
    realfmt = GENERAL;

    update_real_format();

    clearIndent();

    comma = nullptr;
}

outputStream::~outputStream()
{
}

bool outputStream::switchOutput(const char* outfile)
{
    if (fout.is_open()) fout.close();
    fout.open(outfile, std::fstream::out | std::fstream::app);
    update_real_format();
    return fout.good();
}

void outputStream::defaultOutput()
{
    if (fout.is_open()) {
        fout.close();
        update_real_format();
    }
}

template <class item>
inline void align_item(std::ostream &s, item x, int width)
{
    if (width < 0) {
        s << std::setw(-width) << std::left << x;
    } else {
        s << std::setw(width) << std::right << x;
    }

}

inline void show_string(std::ostream &s, const std::stringstream &ss, int width)
{
    align_item(s, ss.str(), width);
}

void outputStream::putWithCommas(long x, int width)
{
    std::stringstream ss;
    ss.fill('0');

    long base=1;
    while (x/base <= -1000) base *= 1000;
    while (x/base >=  1000) base *= 1000;

    ss << x/base;
    while (base>1) {
        x = ABS(x % base);
        base /= 1000;
        if (comma) ss << comma;
        ss << std::setw(3) << x/base;
    }

    show_string(stream(), ss, width);
}

void outputStream::putWithCommas(unsigned long x, int width)
{
    std::stringstream ss;
    ss.fill('0');

    unsigned long base=1;
    while (x/base >=  1000) base *= 1000;

    ss << x/base;
    while (base>1) {
        x = x % base;
        base /= 1000;
        if (comma) ss << comma;
        ss << std::setw(3) << x/base;
    }

    show_string(stream(), ss, width);
}

void outputStream::putWithCommas(const char* x, int width)
{
    if (nullptr == x) return;
    std::stringstream ss;
    if (('-' == x[0]) || ('+' == x[0])) {
        ss << x[0];
        ++x;
    }
    unsigned digits=0;
    for (; x[digits]; ++digits) {
        if (x[digits] < '0') break;
        if (x[digits] > '9') break;
    }
    if (digits < 4) {
        ss << x;
        show_string(stream(), ss, width);
        return;
    }
    switch (digits%3) {
        case 1:
            ss << x[0];
            ++x;
            break;
        case 2:
            ss << x[0] << x[1];
            x += 2;
            break;

        default:
            ss << x[0] << x[1] << x[2];
            x += 3;
            break;
    }
    for (; x[0]; x += 3) {
        if (x[0] < '0') break;
        if (x[0] > '9') break;
        if (comma) {
            ss << comma;
        }
        ss << x[0] << x[1] << x[2];
    }
    ss << x;
    show_string(stream(), ss, width);
}


void outputStream::update_real_format()
{
    switch (realfmt) {
        case FIXED:         stream() << std::fixed;         return;
        case SCIENTIFIC:    stream() << std::scientific;    return;
        default:            stream() << std::defaultfloat;  return;
    }
}


/*


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
*/

/*
outputStream& outputStream::Output()
{
    static outputStream out(std::cout);
    return out;
}

outputStream& outputStream::Error()
{
    static outputStream err(std::cerr);
    return err;
}

std::ostream& outputStream::startError(const location &L, const char* text)
{
    std::ostream& cerr = Error().out();
    cerr << "ERROR";
    if (L) {
        cerr << ' ' << L;
        if (text) {
            cerr << " at text '" << text << "'";
        }
    }
    return cerr << ":\n    ";
}

std::ostream& outputStream::startWarning(const location &L)
{
    std::ostream& cerr = Error().out();
    cerr << "Warning";
    if (L) {
        cerr << ' ' << L;
    }
    return cerr << ":\n    ";
}
*/

/*
std::ostream& outputStream::startInternal(const char* sfile, unsigned sline)
{
    std::ostream& cerr = Error().out();
    return cerr << "INTERNAL in file " << sfile << " line " << sline << "\n    ";
}

void outputStream::stopInternal()
{
    Error().out() << std::endl;
    signal_manager::clean_exit(1);
}
*/

// ======================================================================
//
// Globals
//
// ======================================================================


std::ostream& memoryCount::show(std::ostream &s) const
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

    /*
    unsigned p = prec;
    if (show >= 10.0)  p++;
    if (show >= 100.0)  p++;
    */

    int  oldprec = s.precision();
    s.precision(prec);
    s << show;
    s.precision(oldprec);
    return s << units;
}

// ======================================================================

std::ostream& formatted_int::show(std::ostream &s) const
{
    align_item(s, val, width);
    return s;
}

// ======================================================================

std::ostream& formatted_real::show(std::ostream &s) const
{
    int oldprec = s.precision();
    if (prec>=0) {
        s.precision(prec);
    }
    align_item(s, val, width);
    s.precision(oldprec);
    return s;
}

// ======================================================================

std::ostream& formatted_string::show(std::ostream &s) const
{
    align_item(s, val, width);
    return s;
}

// ======================================================================

std::ostream& padding::show(std::ostream &s) const
{
    for (unsigned i=0; i<count; i++) {
        s << fill;
    }
    return s;
}



