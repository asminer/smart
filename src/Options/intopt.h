
#ifndef INTOPT_H
#define INTOPT_H

#include "options.h"

// **************************************************************************
// *                                                                        *
// *                            integer  options                            *
// *                                                                        *
// **************************************************************************

class int_opt : public option {
    long max;
    long min;
    long &value;
public:
    int_opt(const char* n, const char* d, long &v, long mn, long mx);
    virtual ~int_opt();
    virtual error SetValue(long v);
    virtual void ShowHeader(OutputStream &s) const;
    virtual void ShowRange(doc_formatter* df) const;
};

#endif
