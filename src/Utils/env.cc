
#include "env.h"

environ::environ(const char* v, const char** e)
{
    version = v;
    env = e;
}

bool environ::Print(std::ostream &s, int width) const
{
    return false;
}

int environ::Compare(const shared_object* o) const
{
    const shared_object* t = this;
    return t - o;
}

