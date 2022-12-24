
#ifndef ENV_H
#define ENV_H

#include "../include/shared.h"

/**
    Mechanism for passing around
    environment variables and such.
*/
class environ : public shared_object {
    public:
        const char** env;
        const char* version;

        environ(const char* v, const char** e);

        virtual bool Print(std::ostream &s, int width) const;
        virtual int Compare(const shared_object* o) const;
};

#endif
