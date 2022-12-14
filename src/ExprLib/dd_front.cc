
#include "dd_front.h"

sv_encoder::sv_encoder()
{
}

sv_encoder::~sv_encoder()
{
}

bool sv_encoder::Print(std::ostream &s, int) const
{
    DCASSERT(0);
    s << "sv_encoder";
    return true;
}

int sv_encoder::Compare(const shared_object *o) const
{
    const shared_object* me = this;
    return me - o;
}

int sv_encoder::Compare(const char*) const
{
    DCASSERT(0);
    return 0;
}


const char* sv_encoder::getNameOfError(error e)
{
    switch (e) {
        case Invalid_Edge:        return "Invalid edge";
#ifdef DEVELOPMENT_CODE
        case Shared_Output_Edge:  return "Output edge is shared";
#endif
        case Out_Of_Memory:       return "Not enough memory";
        case Empty_Set:           return "No minterms available";
        case Failed:              return "Operation failed";
    }
    return "Unknown error";
}

