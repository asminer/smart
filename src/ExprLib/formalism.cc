
#include "formalism.h"
#include "measures.h"

// **********************************************************************

// Traversal to copy from the global, "all models" table into a formalism
class copy_into_formalism : public shared_visitor {
        formalism &F;
    public:
        copy_into_formalism(formalism &f);
        virtual void visit(shared_object* item);
};

copy_into_formalism::copy_into_formalism(formalism &f) : F(f)
{
    DCASSERT(F.symb_tree);
}

void copy_into_formalism::visit(shared_object* item)
{
    msr_func* mf = dynamic_cast <msr_func*> (item);
    if (!mf) return;
    switch (mf->getEngClass()) {
        case msr_func::CTL:
            if (!F.include_ctl) return;
            break;

        case msr_func::Stochastic:
            if (!F.include_stoch) return;
            break;

        case msr_func::CSL:
            if (!F.include_ctl) return;
            if (!F.include_stoch) return;
            break;

        case msr_func::DCP:
            if (!F.include_dcp) return;
            break;

        default:
            break;
    }; // end switch
    F.symb_tree->addSymbol(mf);
}

// **********************************************************************

formalism::formalism(const char* n, const char* sd, const char* ld)
 : simple_type(n, sd, ld)
{
    setFormalism();
    include_ctl = false;
    include_stoch = false;
    include_dcp = false;

    symb_tree = new symbol_table;
    symb_list = nullptr;
}

formalism::~formalism()
{
    delete symb_tree;
    delete symb_list;

    // Should we delete the individual symbols?
}

void formalism::finish()
{
    copy_into_formalism T(*this);
    symbol_table::allModelsTable().traverse(T);

    symb_list = new orderedArray <symbol> (symb_tree->getTable());
    delete symb_tree;
    symb_tree = nullptr;
}

bool formalism::isLegalMeasureType(const type* mtype) const
{
    if (0==mtype)                   return 0;
    if (mtype->matches("void"))     return 1;
    if (mtype->matches("bool"))     return 1;
    if (mtype->matches("int"))      return 1;
    if (mtype->matches("real"))     return 1;
    if (mtype->matches("bigint"))   return 1;
    if (include_ctl) {
        if (mtype->matches("stateset")) return 1;
        if (mtype->matches("trace")) return 1;
    }
    if (include_stoch) {
        if (mtype->matches("ph int"))     return 1;
        if (mtype->matches("ph real"))    return 1;
        if (mtype->matches("statedist"))  return 1;
        if (mtype->matches("stateprobs")) return 1;
    }
    return 0;
}

