
#ifndef STROPT_H
#define STROPT_H

#include "options.h"

class string_opt : public option {
    shared_string* &value;
public:
    string_opt(const char* n, const char* d, shared_string* &v);
    virtual ~string_opt();
    virtual error SetValue(shared_string* v);
    virtual void ShowHeader(OutputStream &s) const;
    virtual void ShowRange(doc_formatter* df) const;
};

#endif

