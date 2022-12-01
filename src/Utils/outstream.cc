
#include "outstream.h"
#include "strings.h"
#include "location.h"
// #include "sigman.h"
#include "../Options/optman.h"
#include "../Options/options.h"

#include <iomanip>

// real format codes
static const unsigned RF_GENERAL = 0;
static const unsigned RF_FIXED = 1;
static const unsigned RF_SCIENTIFIC = 2;

outputStream outputStream::Out(std::cout);

// ======================================================================

/// Update the stream when the option changes
class rfwatch : public option::watcher {
        outputStream& stream;
    public:
        rfwatch(outputStream& s);
        virtual void notify(const option* o);
};

rfwatch::rfwatch(outputStream &s) : stream(s)
{
}

void rfwatch::notify(const option*)
{
    stream.update_real_format();
}

// ======================================================================

outputStream::outputStream(std::ostream &_deflt) : deflt(_deflt)
{
    realfmt = RF_FIXED;

    update_real_format();

    clearIndent();

    comma = 0;
}

outputStream::~outputStream()
{
    Delete(comma);
}

void outputStream::buildRealOption(option_manager* om,
        const char* name, const char* doc)
{
    if (0==om) return;
    option* rbo = om->addRadioOption(name, doc, 3, realfmt);
    rbo->registerWatcher(new rfwatch(*this));

    rbo->addRadioButton("FIXED", "Same as printf(%f)", RF_FIXED);
    rbo->addRadioButton("GENERAL", "Same as printf(%g)", RF_GENERAL);
    rbo->addRadioButton("SCIENTIFIC", "Same as printf(%e)", RF_SCIENTIFIC);
}

void outputStream::buildThousandsOption(option_manager* om,
        const char* name, const char* doc)
{
    if (0==om) return;
    om->addStringOption(name, doc, comma);
}

bool outputStream::switchOutput(const char* outfile)
{
    if (fout.is_open()) fout.close();
    fout.open(outfile, std::fstream::out);
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

void outputStream::putWithCommas(long x)
{
    if ((x>-1000)&&(x<1000)) {
        stream() << x;
        return;
    }
    putWithCommas(x/1000);
    if (comma) {
        stream() << comma->getStr();
    }
    stream().fill('0');
    stream() << std::setw(3) << ABS(x%1000);
    stream().fill(' ');
}

void outputStream::putWithCommas(unsigned long x)
{
    if (x<1000) {
        stream() << x;
        return;
    }
    putWithCommas(x/1000);
    if (comma) {
        stream() << comma->getStr();
    }
    stream().fill('0');
    stream() << std::setw(3) << x%1000;
    stream().fill(' ');
}

void outputStream::putWithCommas(const char* x)
{
    if (nullptr == x) return;
    if (('-' == x[0]) || ('+' == x[0])) {
        stream() << x[0];
        ++x;
    }
    unsigned digits=0;
    for (; x[digits]; ++digits) {
        if (x[digits] < '0') break;
        if (x[digits] > '9') break;
    }
    if (digits < 4) {
        stream() << x;
        return;
    }
    switch (digits%3) {
        case 1:
            stream() << x[0];
            ++x;
            break;
        case 2:
            stream() << x[0] << x[1];
            x += 2;
            break;

        default:
            stream() << x[0] << x[1] << x[2];
            x += 3;
            break;
    }
    for (; x[0]; x += 3) {
        if (x[0] < '0') break;
        if (x[0] > '9') break;
        if (comma) {
            stream() << comma->getStr();
        }
        stream() << x[0] << x[1] << x[2];
    }
    stream() << x;
}


void outputStream::update_real_format()
{
    switch (realfmt) {
        case RF_FIXED:          stream() << std::fixed;            return;
        case RF_SCIENTIFIC:     stream() << std::scientific;       return;
        default:                stream() << std::defaultfloat;     return;
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

    unsigned p = prec+1;
    if (show >= 10.0)  p++;
    if (show >= 100.0)  p++;

    auto oldprec = s.precision();
    s.precision(p);
    s << show;
    s.precision(oldprec);
    return s << units;
}

// ======================================================================

std::ostream& formatted_int::show(std::ostream &s) const
{
    if (width < 0) {
        return s << std::setw(-width) << std::left << val;
    } else {
        return s << std::setw(width) << std::right << val;
    }
}

// ======================================================================

std::ostream& formatted_real::show(std::ostream &s) const
{
    int oldprec = s.precision();
    if (prec>=0) {
        s.precision(prec);
    }
    if (width < 0) {
        s << std::setw(-width) << std::left << val;
    } else {
        s << std::setw(width) << std::right << val;
    }
    s.precision(oldprec);
    return s;
}

// ======================================================================

std::ostream& formatted_string::show(std::ostream &s) const
{
    if (width < 0) {
        return s << std::setw(-width) << std::left << val;
    } else {
        return s << std::setw(width) << std::right << val;
    }
}


