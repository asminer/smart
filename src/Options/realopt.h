
#ifndef REALOPT_H
#define REALOPT_H

#include "options.h"

// **************************************************************************
// *                                                                        *
// *                              real options                              *
// *                                                                        *
// **************************************************************************

class real_opt : public option {
        bool has_min;
        bool includes_min;
        double min;
        bool has_max;
        bool includes_max;
        double max;
        double &value;
    public:
        real_opt(const char* n, const char* d, double &v,
            bool hl, bool il, double l,
            bool hu, bool iu, double u);

        virtual ~real_opt();
        virtual error SetValue(double b);
        virtual void ShowHeader(OutputStream &s) const;
        virtual void ShowRange(doc_formatter* df) const;
};

#endif
