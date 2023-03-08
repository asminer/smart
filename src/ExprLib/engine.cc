
#include "engine.h"
#include "../Utils/splay.h"
#include "../Utils/ordarray.h"
#include "../Utils/initializer.h"
#include "../Options/options.h"
#include "../Options/optman.h"
#include "../Options/radio_opt.h"
#include "measures.h"
#include "mod_inst.h"

// #define DEBUG_REGISTRY

// ********************************************************
// *               finalizer_visitor class                *
// ********************************************************

class finalizer_visitor : public shared_visitor {
        option_manager &om;
    public:
        finalizer_visitor(option_manager &_om) : om(_om) { };
        virtual void visit(shared_object* obj) {
            engtype* et = smart_cast <engtype*> (obj);
            if (!et) return;
            et->finalizeRegistry(om);
        }
};

// ********************************************************
// *                engine_watcher  class                 *
// ********************************************************

/*
 * This updates the selected engine in the engtype object,
 * when there's an option statement.
 */
class engine_watcher : public option::watcher {
        unsigned selected;
        engtype* ET;
    public:
        engine_watcher(engtype* et);
        inline unsigned& Link() { return selected; }
        virtual void notify(const option* opt);
};

engine_watcher::engine_watcher(engtype* et)
{
    ET = et;
    DCASSERT(ET);
    DCASSERT(ET->EngList);
}

void engine_watcher::notify(const option* opt)
{
    DCASSERT(selected < ET->EngList->numElements());
    ET->selected_engine = smart_cast <engine*> (ET->EngList->get(selected));
}

// ********************************************************
// *                  subengine methods                   *
// ********************************************************

subengine::subengine()
{
}

subengine::~subengine()
{
}

const char* subengine::getNameOfError(error e)
{
    switch (e) {
        case Finalized:
            return "Engine manager should / should not be finalized";

        case No_Engine:
            return "No solution engine available";

        case Bad_Option:
            return "Unknown engine selection";

        case Duplicate:
            return "Duplicate engine name";

        case Call_Mismatch:
            return "Incorrect measure signature";

        case Out_Of_Memory:
            return "Engine ran out of memory";

        case Terminated:
            return "Engine was terminated";

        case Assertion_Failure:
            return "Assertion failure";

        case Bad_Value:
            return "Illegal value for engine parameter";

        case Engine_Failed:
            return "Engine failed";
    }
    return "Unknown error";
}

void subengine::RunEngine(result*, int, traverse_data &)
{
    throw Call_Mismatch;
}

void subengine::RunEngine(hldsm*, result &)
{
    throw Call_Mismatch;
}

void subengine::SolveMeasure(hldsm* , measure* )
{
    throw Call_Mismatch;
}

void subengine::SolveMeasures(hldsm* , set_of_measures* )
{
    throw Call_Mismatch;
}


// ********************************************************
// *                   engine  methods                    *
// ********************************************************

unsigned engine::num_hlm_types = 0;

engine::engine(const char* n, const char* d) : shared_string(n)
{
    etype = nullptr;
    doc = d;
    next = nullptr;
    children = new subengine*[num_hlm_types];
    for (unsigned i=0; i<num_hlm_types; i++) children[i] = 0;
    options = 0;
}

engine::~engine()
{
    // Is this ever called?
    delete[] children;
    Delete(options);
}

void engine::AddSubEngine(subengine* child)
{
    if (0==child) return;
    for (unsigned i=0; i<num_hlm_types; i++) {
        hldsm::model_type mi = (hldsm::model_type) i;
        if (child->AppliesToModelType(mi)) {
            if (children[i]) {
                internal_error E(__FILE__, __LINE__);
                E << "Registering subengine for " << Name();
                E << " with existing model type " << i;
            }
            children[i] = child;
        }
    } // for i
}

void engine::addButtonToOption(option* o, unsigned ndx)
{
    DCASSERT(o);
    DCASSERT(Name());
    DCASSERT(Documentation());

    option_enum* rb = o->addRadioButton(Name(), Documentation(), ndx);

    if (options) {
        options->DoneAddingOptions();
        rb->makeSettings(options);
    }
}

option_manager* engine::internalOpts()
{
    if (!options) {
        options = new option_manager();
    }
    return options;
}

// ******************************************************************
// *             traversal to build  groups of measures             *
// ******************************************************************

class build_groups_traversal : public shared_visitor {
        set_of_measures** groups;
        unsigned numgroups;
    public:
        build_groups_traversal(set_of_measures** g, unsigned ng);
        virtual void visit(shared_object* item);
};

build_groups_traversal::build_groups_traversal(set_of_measures** g,
        unsigned ng)
{
    groups = g;
    numgroups = ng;
}

void build_groups_traversal::visit(shared_object* item)
{
    const engtype* et = dynamic_cast <const engtype*> (item);
    DCASSERT(et);
    const unsigned i = et->getIndex();
    CHECK_RANGE(__FILE__, __LINE__, 0, i, numgroups);
    DCASSERT(nullptr == groups[i]);
    groups[i] = et->makeMeasureSet();
}

// ******************************************************************
// *                        engtype  methods                        *
// ******************************************************************

splayOfShared* engtype::registry = nullptr;
unsigned engtype::registry_size = 0;

engtype::engtype(const char* n, const char* d, calling_form f)
    : shared_string(n)
{
    doc = d;
    form = f;

    finalized = false;
    is_blocked_engine = false;

    EngTree = nullptr;
    EngList = nullptr;

    selected_engine = nullptr;
}

engtype::~engtype()
{
    killEngTree();
    delete EngList;
}

void engtype::registerEngine(engine* e)
{
    if (!e)  throw subengine::No_Engine;
#ifdef DEBUG_REGISTRY
    std::cerr << "Engine type " << *this << ", registering engine " << *e << "\n";
#endif
    e->etype = this;
    if (finalized) throw subengine::Finalized;
    if (Nothing == form) {
        if (!selected_engine) selected_engine = e;
        return;
    }
    if (!EngTree)  EngTree = new splayOfShared(16, 0);
    engine* f = smart_cast <engine*> (EngTree->insert(e));
    if (f==e)  {
        if (!selected_engine) selected_engine = e;
        return;
    }
    // there is an engine with the same name.
    Delete(e);
    throw subengine::Duplicate;
}

void engtype::registerSubengine(const char* name, subengine* se)
{
    if (!se)            return;
    if (!name)          throw  subengine::No_Engine;
    if (finalized)      throw  subengine::Finalized;
    const_string S(name);
    engine* f = smart_cast <engine*> (EngTree->find(&S));
    if (!f)             throw  subengine::No_Engine;
    f->AddSubEngine(se);
}

void engtype::finalizeRegistry(option_manager &om)
{
    if (finalized)      return;
    if (!EngTree)       return;
#ifdef DEBUG_REGISTRY
    std::cerr << "finalizing registry for engine type " << *this << "\n";
#endif

    //
    // Convert engines into an ordered array
    //
    EngList = new orderedShared(*EngTree);
    killEngTree();
    finalized = true;

    if (EngList->numElements() < 2) {
        engine* item0 = dynamic_cast <engine*> (EngList->get(0));
        DCASSERT(item0);
        if (!item0->hasOptions()) return;
    }

    //
    // Build an option, automagically
    //

    engine_watcher* EW = new engine_watcher(this);
    option* ro = om.addRadioOption(Name(), Documentation(),
        EngList->numElements(), EW->Link());
    ro->registerWatcher(EW);
    for (unsigned i=0; i<EngList->numElements(); i++) {
        engine* item = dynamic_cast <engine*> (EngList->get(i));
        DCASSERT(item);
        if (item == selected_engine) {
            EW->Link() = i;
        }
        item->addButtonToOption(ro, i);
    }
}

void engtype::runEngine(result* pass, int np, traverse_data &x)
{
    if (!selected_engine)  throw subengine::No_Engine;
    selected_engine->RunEngine(pass, np, x);
}

void engtype::runEngine(hldsm* m, result &p)
{
    if (!selected_engine)  throw  subengine::No_Engine;
    selected_engine->RunEngine(m, p);
}

void engtype::solveMeasure(hldsm* m, measure* what)
{
    if (!selected_engine)  throw  subengine::No_Engine;
    selected_engine->SolveMeasure(m, what);
}

void engtype::solveMeasures(hldsm* m, set_of_measures* list)
{
    if (!selected_engine)  throw  subengine::No_Engine;
    selected_engine->SolveMeasures(m, list);
}

set_of_measures* engtype::makeMeasureSet() const
{
    DCASSERT(Grouped != form);
    return nullptr;
}

engtype* engtype::registerEngineType(engtype* et)
{
    if (!et) return nullptr;
#ifdef DEBUG_REGISTRY
    std::cout << "Registering engine type " << *et << "\n";
#endif
    if (!registry) {
        registry = new splayOfShared(16, 0);
        registry_size = 0;
    }
    et->index = registry_size;
    engtype* ret = smart_cast <engtype*> (registry->insert(et));
    if (ret != et) {
        Delete(et);
    } else {
        ++ registry_size;
    }
    return ret;
}

set_of_measures** engtype::buildMeasureGroups()
{
    set_of_measures** sets = registry_size
        ? new set_of_measures*[registry_size]
        : nullptr;

    for (unsigned i=0; i<registry_size; i++) {
        sets[i] = nullptr;
    }

    build_groups_traversal T(sets, registry_size);
    if (registry) registry->traverse(T);

    return sets;
}

engtype* engtype::findEngineType(const char* name)
{
    if (!registry) return nullptr;
    const_string S(name);
    return smart_cast <engtype*> (registry->find(&S));
}

void engtype::finalizeAll(option_manager &om)
{
    finalizer_visitor v(om);
    if (registry) registry->traverse(v);
}

void engtype::killEngTree()
{
    delete EngTree;
    EngTree = nullptr;
}

// ******************************************************************
// *                   unordered_engtype  methods                   *
// ******************************************************************

unordered_engtype::unordered_engtype(const char* n, const char* d)
 : engtype(n, d, Grouped)
{
}

set_of_measures* unordered_engtype::makeMeasureSet() const
{
    return MakeUnsortedMeasures();
}

// ******************************************************************
// *                      time_engtype methods                      *
// ******************************************************************

time_engtype::time_engtype(const char* n, const char* d)
 : engtype(n, d, Grouped)
{
}

set_of_measures* time_engtype::makeMeasureSet() const
{
    return MakeTimeSortedMeasures();
}


// ******************************************************************
// *                      func_engine  methods                      *
// ******************************************************************

func_engine::func_engine(const type* rt, const char* name, int np, engtype* w)
 : simple_internal(rt, name, np)
{
    whicheng = w;
    engpass = new result[np];
    for (int i=0; i<np; i++) engpass[i].setNull();
}

func_engine::~func_engine()
{
    delete[] engpass;
}

void func_engine::Compute(traverse_data &x, expr** pass, int np)
{
    try {
        if (whicheng) {
            BuildParams(x, pass, np);
            whicheng->runEngine(engpass, np, x);
            for (int i=0; i<np; i++) engpass[i].setNull();
        } else {
            throw subengine::No_Engine;
        }
    } // try
    catch (subengine::error e) {
        switch (e) {
            case subengine::No_Engine: {
                expr_error E(x.parent, x.answer);
                E << "No solution engine available for " << Name();
                formals.PrintHeader(E.stream(), false);
                return;
            }

            default: {
                internal_error E(__FILE__, __LINE__,
                    x.parent ? x.parent->Where() : location::NOWHERE());
                E << "unanticipated error: " << subengine::getNameOfError(e);
                E.newLine();
                E << "for " << Name();
                formals.PrintHeader(E.stream(), false);
                E << " engine";
            }
        }
        x.answer->setNull();
    } // catch
}


// ******************************************************************
// *                                                                *
// *                     redirect_engine  class                     *
// *                                                                *
// ******************************************************************

class redirect_engine : public subengine {
    engtype* link;
public:
    redirect_engine(engtype* l);

    virtual bool AppliesToModelType(hldsm::model_type mt) const;
    virtual void RunEngine(result* pass, int np, traverse_data &x);
    virtual void RunEngine(hldsm* m, result &p);
    virtual void SolveMeasure(hldsm* m, measure* what);
    virtual void SolveMeasures(hldsm* m, set_of_measures* list);
};

// ******************************************************************
// *                    redirect_engine  methods                    *
// ******************************************************************

redirect_engine::redirect_engine(engtype* l)
{
    link = l;
}

bool redirect_engine::AppliesToModelType(hldsm::model_type mt) const
{
    return true;
}

void redirect_engine::RunEngine(result* p, int np, traverse_data &x)
{
    DCASSERT(link);
    link->runEngine(p, np, x);
}

void redirect_engine::RunEngine(hldsm* m, result &p)
{
    DCASSERT(link);
    link->runEngine(m, p);
}

void redirect_engine::SolveMeasure(hldsm* m, measure* what)
{
    DCASSERT(link);
    link->solveMeasure(m, what);
}

void redirect_engine::SolveMeasures(hldsm* m, set_of_measures* list)
{
    DCASSERT(link);
    link->solveMeasures(m, list);
}


// ******************************************************************
// *                                                                *
// *                       noop_engine  class                       *
// *                                                                *
// ******************************************************************

class noop_engine : public subengine {
public:
    virtual ~noop_engine();
    virtual void SolveMeasure(hldsm* m, measure* what);
    virtual bool AppliesToModelType(hldsm::model_type mt) const;
};

// ******************************************************************
// *                      noop_engine  methods                      *
// ******************************************************************

noop_engine::~noop_engine()
{
}

void noop_engine::SolveMeasure(hldsm*, measure* what)
{
    if (!what)  return;
    traverse_data x(traverse_data::Compute);
    result foo;
    x.answer = &foo;
    what->ComputeRHS(x);
    what->SetValue(foo);
}

bool noop_engine::AppliesToModelType(hldsm::model_type mt) const
{
    return (mt != 0);
}

static noop_engine the_noop_engine;


// ******************************************************************
// *                                                                *
// *                       bogus_engine class                       *
// *                                                                *
// ******************************************************************

/** Used as a placeholder until the real engine is discovered.
*/
class bogus_engine : public subengine {
public:
    virtual ~bogus_engine();
    virtual void SolveMeasure(hldsm* m, measure* what);
    virtual bool AppliesToModelType(hldsm::model_type mt) const;
};

// ******************************************************************
// *                      bogus_engine methods                      *
// ******************************************************************

bogus_engine::~bogus_engine()
{
}

void bogus_engine::SolveMeasure(hldsm*, measure* what)
{
    internal_error E(__FILE__, __LINE__,
          what ? what->Where() : location::NOWHERE()
    );
    E << "Calling a placeholder engine.";
    throw No_Engine;  // Probably best, if we manage to get here
}

bool bogus_engine::AppliesToModelType(hldsm::model_type mt) const
{
    return (mt != 0);
}

static bogus_engine the_bogus_engine;

// ******************************************************************
// *                                                                *
// *                         Initialization                         *
// *                                                                *
// ******************************************************************

class engine_init : public initializer {
    public:
        engine_init();
    protected:
        virtual void execute();
};

static engine_init the_engine_init;

engine_init::engine_init() : initializer(__FILE__, 1)
{
    builds_resource("engines");
}

void engine_init::execute()
{
    engine::num_hlm_types = 1+hldsm::Last_Model_Type;

    //
    // Set up special "no engine".
    //

    engtype* noengine = engtype::registerEngineType(
        new engtype("No Engine", "No solution engine", engtype::Single)
    );
    RegisterEngine(noengine, "no-op", "Do nothing", &the_noop_engine);
    noengine->finalizeRegistry(option_manager::global());

    //
    // Set up special "blocked" engine type.
    //

    engtype* blocked = engtype::registerEngineType(
        new engtype("Blocked Engine", "Blocked measures", engtype::Single)
    );
    blocked->is_blocked_engine = true;
    RegisterEngine(blocked, "fail", "Fail and bail out", &the_bogus_engine);
    blocked->finalizeRegistry(option_manager::global());
}

// ******************************************************************
// *                                                                *
// *                           Front  end                           *
// *                                                                *
// ******************************************************************

engine* MakeRedirectionEngine(const char* n, const char* d, engtype* et)
{
    engine* e = new engine(n, d);
    e->AddSubEngine(new redirect_engine(et));
    return e;
}


