
#ifndef BOOLOPT_H
#define BOOLOPT_H

#include "options.h"

// **************************************************************************
// *                            boolean  options                            *
// **************************************************************************

class bool_opt : public option {
    bool& value;
public:
    bool_opt(const char* n, const char* d, bool &v);
    virtual ~bool_opt();
    virtual error SetValue(bool b);
    virtual error GetValue(bool &b) const;
    virtual void ShowHeader(OutputStream &s) const;
    virtual void ShowRange(doc_formatter* df) const;
};

#endif

