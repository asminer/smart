
#ifndef STROPT_H
#define STROPT_H

#include "options.h"

class string_opt : public option {
    shared_string* value;
    const char* &link;
public:
    string_opt(const char* n, const char* d, const char* &L);
    virtual ~string_opt();
    virtual error SetValue(shared_string* v);
    virtual void ShowHeader(std::ostream &s) const;
    virtual void ShowRange(doc_formatter &df) const;
};

#endif

