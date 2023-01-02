
#include "formalism.h"
#include "measures.h"

// **********************************************************************

// Traversal to copy from the global, "all models" table into a formalism
class copy_into_formalism : public splayOfShared::tree_traversal {
        formalism &F;
    public:
        copy_into_formalism(formalism &f);
        virtual void visit(shared_object* item);
};

copy_into_formalism::copy_into_formalism(formalism &f) : F(f)
{
}

void copy_into_formalism::visit(shared_object* item)
{
    msr_func* mf = dynamic_cast <msr_func*> (item);
    if (!mf) return;
    switch (mf->getEngClass()) {
        case msr_func::CTL:
            if (!F.includeCTL()) return;
            break;

        case msr_func::Stochastic:
            if (!F.includeStochastic()) return;
            break;

        case msr_func::CSL:
            if (!F.includeCTL()) return;
            if (!F.includeStochastic()) return;
            break;

        case msr_func::DCP:
            if (!F.includeDCP()) return;
            break;

        default:
            break;
    }; // end switch
    DCASSERT(F.funcs);
    F.funcs->addSymbol(mf);
}

// **********************************************************************

formalism::formalism(const char* n, const char* sd, const char* ld)
 : simple_type(n, sd, ld)
{
    funcs = nullptr;
    idents = nullptr;
    setFormalism();
}

formalism::~formalism()
{
    delete funcs;
    delete idents;
}

void formalism::addCommonFuncs()
{
    copy_into_formalism T(*this);
    symbol_table::allModelsTable().traverse(T);
}

bool formalism::isLegalMeasureType(const type* mtype) const
{
    if (0==mtype)                   return 0;
    if (mtype->matches("void"))     return 1;
    if (mtype->matches("bool"))     return 1;
    if (mtype->matches("int"))      return 1;
    if (mtype->matches("real"))     return 1;
    if (mtype->matches("bigint"))   return 1;
    if (includeCTL()) {
        if (mtype->matches("stateset")) return 1;
        if (mtype->matches("trace")) return 1;
    }
    if (includeStochastic()) {
        if (mtype->matches("ph int"))     return 1;
        if (mtype->matches("ph real"))    return 1;
        if (mtype->matches("statedist"))  return 1;
        if (mtype->matches("stateprobs")) return 1;
    }
    return 0;
}

bool formalism::includeCTL() const
{
    return false;
}

bool formalism::includeStochastic() const
{
    return false;
}

bool formalism::includeDCP() const
{
    return false;
}
