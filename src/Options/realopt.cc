
#include "../include/defines.h"
#include "../Streams/streams.h"
#include "realopt.h"

real_opt::real_opt(const char* n, const char* d, double &v,
       bool hl, bool il, double l, bool hu, bool iu, double u)
 : option(Real, n, d), value(v)
{
    has_min = hl;
    includes_min = il;
    min = l;
    has_max = hu;
    includes_max = iu;
    max = u;
}

real_opt::~real_opt()
{
}

option::error real_opt::SetValue(double b)
{
    if (b == value) return Success;
    bool bad = false;
    if (has_min) {
        if (includes_min) {
            bad = b < min;
        } else {
            bad = b <= min;
        }
        if (bad) return RangeError;
    }

    if (has_max) {
        if (includes_max) {
            bad = b > max;
        } else {
            bad = b >= max;
        }
        if (bad) return RangeError;
    }

    value = b;
    return notifyWatchers();
}

void real_opt::ShowHeader(OutputStream &s) const
{
    show(s);
    s << " " << value;
}

void real_opt::ShowRange(doc_formatter* df) const
{
    DCASSERT(df);
    df->Out() << "Legal values: reals in ";
    if (has_min) {
        if (includes_min)  df->Out().Put('[');
        else     df->Out().Put('(');
        df->Out() << min;
    } else {
        df->Out() << "(-oo";
    }
    df->Out().Put(',');
    df->Out().Put(' ');
    if (has_max) {
        df->Out() << max;
        if (includes_max) df->Out().Put(']');
        else df->Out().Put(')');
    } else {
        df->Out() << "oo)";
    }
}

