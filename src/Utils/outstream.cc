
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
    clearIndent();
}

outputStream::~outputStream()
{
}

bool outputStream::switchOutput(const char* outfile)
{
    if (fout.is_open()) fout.close();
    fout.open(outfile, std::fstream::out | std::fstream::app);
    // update_real_format();
    return fout.good();
}

void outputStream::defaultOutput()
{
    if (fout.is_open()) {
        fout.close();
    }
}


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



