
#include "outstream.h"
#include "strings.h"

#include <iomanip>
#include <sstream>

outputStream outputStream::Out(std::cout);

// ******************************************************************
// *                                                                *
// *                      outputStream methods                      *
// *                                                                *
// ******************************************************************

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

/*

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
*/

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

// ******************************************************************
// *                                                                *
// *                   Helpers for adding  commas                   *
// *                                                                *
// ******************************************************************

inline void align_string(std::ostream &s, const char* x, int width)
{
    if (width < 0) {
        s << std::setw(-width) << std::left << x;
    } else {
        s << std::setw(width) << std::right << x;
    }
}

static void show_with_commas(std::ostream &s, const char* x,
        int width, const char* comma)
{
    DCASSERT(x);
    if (!comma) {
        align_string(s, x, width);
        return;
    }

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
    if (digits > 3) {
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
    }
    ss << x;
    align_string(s, ss.str().c_str(), width);
}

// ******************************************************************
// *                                                                *
// *                      memoryCount  methods                      *
// *                                                                *
// ******************************************************************

memoryCount::memoryCount(size_t b, unsigned p)
{
    bytes = b;
    prec = p;
}

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

// ******************************************************************
// *                                                                *
// *                     formatted_int  methods                     *
// *                                                                *
// ******************************************************************

formatted_int::formatted_int(long v, int w, const char* c)
{
    val = v;
    width = w;
    comma = c;
}

std::ostream& formatted_int::show(std::ostream &s) const
{
    std::stringstream ss;
    ss << val;
    show_with_commas(s, ss.str().c_str(), width, comma);
    return s;
}

// ******************************************************************
// *                                                                *
// *                    formatted_number methods                    *
// *                                                                *
// ******************************************************************

formatted_number::formatted_number(const char* v, int w, const char* c)
{
    val = v;
    width = w;
    comma = c;
}

std::ostream& formatted_number::show(std::ostream &s) const
{
    show_with_commas(s, val, width, comma);
    return s;
}

// ******************************************************************
// *                                                                *
// *                    scientific_real  methods                    *
// *                                                                *
// ******************************************************************

scientific_real::scientific_real(double v, int w, int p, const char* c)
{
    val = v;
    width = w;
    prec = p;
    comma = c;
}

std::ostream& scientific_real::show(std::ostream &s) const
{
    std::stringstream ss;
    if (prec>=0) ss.precision(prec);
    ss << std::scientific << val;
    show_with_commas(s, ss.str().c_str(), width, comma);
    return s;
}

// ******************************************************************
// *                                                                *
// *                       fixed_real methods                       *
// *                                                                *
// ******************************************************************

fixed_real::fixed_real(double v, int w, int p, const char* c)
{
    val = v;
    width = w;
    prec = p;
    comma = c;
}

std::ostream& fixed_real::show(std::ostream &s) const
{
    std::stringstream ss;
    if (prec>=0) ss.precision(prec);
    ss << std::fixed << val;
    show_with_commas(s, ss.str().c_str(), width, comma);
    return s;
}

// ******************************************************************
// *                                                                *
// *                      general_real methods                      *
// *                                                                *
// ******************************************************************

general_real::general_real(double v, int w, int p, const char* c)
{
    val = v;
    width = w;
    prec = p;
    comma = c;
}

std::ostream& general_real::show(std::ostream &s) const
{
    std::stringstream ss;
    if (prec>=0) ss.precision(prec);
    ss << val;
    show_with_commas(s, ss.str().c_str(), width, comma);
    return s;
}

// ******************************************************************
// *                                                                *
// *                    formatted_string methods                    *
// *                                                                *
// ******************************************************************

formatted_string::formatted_string(const char* v, int w)
{
    val = v;
    width = w;
}

std::ostream& formatted_string::show(std::ostream &s) const
{
    align_string(s, val, width);
    return s;
}

// ******************************************************************
// *                                                                *
// *                        padding  methods                        *
// *                                                                *
// ******************************************************************

padding::padding(unsigned n, char f)
{
    count = n;
    fill = f;
}

std::ostream& padding::show(std::ostream &s) const
{
    for (unsigned i=0; i<count; i++) {
        s << fill;
    }
    return s;
}



