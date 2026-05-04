
#include "expl_ssets.h"

#include "../_IntSets/intset.h"

#include "../Utils/initializer.h"

#include "../ExprLib/casting.h"
#include "../ExprLib/sets.h"
#include "../ExprLib/mod_vars.h"

#include "../Formlsms/rss_enum.h"

// ******************************************************************
// *                                                                *
// *                      sos_promotions class                      *
// *                                                                *
// ******************************************************************

/**
    Promote a set of explicit states (e.g., a Markov chain state)
    to a stateset.
*/
class sos_promotions : public general_conv {
    public:
        sos_promotions();
        virtual int getDistance(const type* src, const type* dest) const;
        virtual bool requiresConversion(const type* src, const type* dest) const        {
            return true;
        }
        virtual expr* convert(const location &W, expr* e, const type* t) const {
            return new converter(W, t, e);
        }

    protected:
        class converter : public typecast {
            public:
                converter(const location &W, const type* nt, expr* x);
                virtual void Compute(traverse_data &x);
            protected:
                virtual expr* buildAnother(expr* x) const {
                    return new converter(Where(), Type(), x);
                }
        };

};

// ******************************************************************

sos_promotions::converter::converter(const location &W, const type* nt,
        expr* x) : typecast(W, nt, x)
{
}

void sos_promotions::converter::Compute(traverse_data &x)
{
    DCASSERT(x.answer);
    DCASSERT(0==x.aggregate);
    DCASSERT(opnd);
    opnd->Compute(x);
    DCASSERT(x.answer->isNormal());
    shared_set* ss = smart_cast<shared_set*> (x.answer->getPtr());
    DCASSERT(ss);

    //
    // Grab a valid element to determine the model
    //
    result elem;
    elem.setNull();
    for (long i=0; i<ss->Size(); i++) {
        ss->GetElement(i, elem);
        if (elem.isNormal()) break;
    }

    if (!elem.isNormal()) {
        // empty set, or all elements invalid
        x.answer->setNull();
        return;
    }

    DCASSERT(elem.getPtr());
    model_enum_value* frst = smart_cast<model_enum_value*> (elem.getPtr());
    DCASSERT(frst);

    const model_instance* par = frst->getParent();

    //
    // Check if all elements are from the same model
    //
    for (long i=0; i<ss->Size(); i++)
    {
        ss->GetElement(i, elem);
        if (!elem.isNormal()) continue;
        DCASSERT(elem.getPtr());
        model_enum_value* svi = smart_cast<model_enum_value*> (elem.getPtr());
        DCASSERT(svi);
        if (svi->getParent() != par) {
            // TBD: error message here
            x.answer->setNull();
            return;
        }
    }

    //
    // Get the number of states
    //
    hldsm* foo = par->GetCompiledModel();
    DCASSERT(foo);
    const state_lldsm* bar = dynamic_cast <const state_lldsm*> (foo->GetProcess());
    DCASSERT(bar);
    const enum_reachset* rss = dynamic_cast <const enum_reachset*> (bar->getRSS());
    DCASSERT(rss);
    long numstates;
    rss->getNumStates(numstates);

    //
    // Build a bitvector
    //
    intset* explset = new intset(numstates);
    explset->removeAll();

    //
    // Populate bitvector
    //
    for (long i=0; i<ss->Size(); i++)
    {
        ss->GetElement(i, elem);
        if (!elem.isNormal()) continue;
        DCASSERT(elem.getPtr());
        model_enum_value* svi = smart_cast<model_enum_value*> (elem.getPtr());
        DCASSERT(svi);

        int ndx = svi->GetIndex();
        DCASSERT(rss->getEnumeratedState(ndx) == svi);

        explset->addElement(ndx);
    }

    //
    // return an explicit stateset
    //
    x.answer->setPtr(new expl_stateset(bar, explset));
}

// ******************************************************************

sos_promotions::sos_promotions() : general_conv()
{
}

int sos_promotions::getDistance(const type* src, const type* dest) const
{
    if (!src->isASet())                             return -1;
    if (!src->getSetElemType()->matches("state"))   return -1;
    if (!dest->matches("stateset"))                 return -1;

    return 1;
}

// ******************************************************************
// *                                                                *
// *                                                                *
// *                         Initialization                         *
// *                                                                *
// *                                                                *
// ******************************************************************

class init_sos2sset : public initializer {
    public:
        init_sos2sset();
    protected:
        virtual void execute();
};
static init_sos2sset the_sos2sset_initializer;

init_sos2sset::init_sos2sset() : initializer(__FILE__, 3)
{
    builds_resource("sos2sset");
    needs_resource("model_state");
    needs_resource("stateset");
}

void init_sos2sset::execute()
{
    //
    // Register type conversion
    //
    new sos_promotions;
}


