
// $Id$

/** 
  \file engine.h
  The engine interface is specified here.
  Note that you should only need to include this file
  if you are building a solution engine, or if
  you are implementing an engine manager.
*/

#ifndef ENGINE_H
#define ENGINE_H

#include "exprman.h"
#include "mod_inst.h"
#include "functions.h"

class engtype;
class measure;
class set_of_measures;
class engine_list;
class engine_tree;
class radio_button;
class option_manager;

// ******************************************************************
// *                                                                *
// *                        sub_engine class                        *
// *                                                                *
// ******************************************************************

/** Sub-Engine class.
    All solution engines should be derived from this class,
    and included as part of an "engine" (see below).

    Essentially, a sub-engine is an algorithm 
    for performing a certain task (e.g., state space generation),
    on a certain class of models (e.g., asynchronous event models).

    Similar algorithms can be collected together, for different
    classes of models, under a similar name; this is the "parent" engine.
*/
class subengine {
public:
  /// Error codes, thrown as exceptions.
  enum error {
    /// Operation required finalization but we aren't, or vice-versa.
    Finalized,
    /// No solution engine available!
    No_Engine,
    /// Bad option selection for an engine type.
    Bad_Option,
    /// Duplicate solution engine name.
    Duplicate,
    /// Mismatch between engine type and analysis requested.
    Call_Mismatch,
    /// The engine ran out of memory before solution was completed.
    Out_Of_Memory,
    /// A sigterm signal was caught.
    Terminated,
    /// A model assertion failed, and the engine terminated.
    Assertion_Failure,
    /// Some other fatal run-time error with the engine.
    Engine_Failed
  };
protected:
// do we need a pointer to the parent?
  static exprman* em;
public:
  subengine();
  virtual ~subengine();
public:
  /** Can this engine be applied to the specified model type.
      Must be provided in derived classes.
      A model type of 0 is used for "function calls".
        @param  mt  Model type.
  */
  virtual bool AppliesToModelType(hldsm::model_type mt) const = 0;

  virtual void RunEngine(result* pass, int np, traverse_data &x);
  virtual void RunEngine(hldsm* m, result &parm);
  virtual void SolveMeasure(hldsm* m, measure* what);
  virtual void SolveMeasures(hldsm* m, set_of_measures* list);

  /** Produce a "human-readable" name for an error.
        @param  e  The engine error.
        @return  A string constant.
  */
  static const char* getNameOfError(error e);

  friend void InitEngines(exprman* em);
};

// ******************************************************************
// *                                                                *
// *                          engine class                          *
// *                                                                *
// ******************************************************************

/** Engine class.
    This is a collection of similar subengines that can be 
    applied to different model types.

    For example, there could be a single engine for
    "Explicit state space generation", consisting of
    several subengines, each subengine applicable to
    different model types.

    Note: there should be no more need of virtual functions!
*/
class engine {
  engtype* etype;
  const char* name;
  const char* doc;
  static exprman* em;
  engine* next;
  static int num_hlm_types;
  subengine** children; // dimension is num_hlm_types;
  option_manager* options; // options local to this engine
public:
  engine(const char* n, const char* d);
  ~engine();

  void AddSubEngine(subengine* child);

  void AddOption(option* o);
  
  inline const engtype* getType() const { return etype; }
  inline const char* Name() const { return name; }
  inline const char* Documentation() const { return doc; }

  // Name comparison, so we can put it in a tree
  int Compare(const char* name2) const;  
  inline int Compare(const engine* x) const {
    const char* n2 = x ? x->name : 0;
    return Compare(n2);
  }

  inline void RunEngine(result* pass, int np, traverse_data &x) {
    DCASSERT(children);
    if (children[0]) {
      DCASSERT(children[0]->AppliesToModelType(hldsm::Nothing));
      children[0]->RunEngine(pass, np, x);
    } else {
      throw subengine::No_Engine;
    }
  }

  inline void RunEngine(hldsm* m, result &parm) {
    if (0==m) return;
    DCASSERT(children);
    int i = int(m->Type());
    if (i<0) return;
    CHECK_RANGE(0, i, num_hlm_types);
    if (children[i]) {
      DCASSERT(children[i]->AppliesToModelType(m->Type()));
      children[i]->RunEngine(m, parm);
    } else {
      throw subengine::No_Engine;
    }
  }

  inline void SolveMeasure(hldsm* m, measure* what) {
    if (0==m) return;
    DCASSERT(children);
    int i = int(m->Type());
    if (i<0) return;
    CHECK_RANGE(0, i, num_hlm_types);
    if (children[i]) {
      DCASSERT(children[i]->AppliesToModelType(m->Type()));
      children[i]->SolveMeasure(m, what);
    } else {
      throw subengine::No_Engine;
    }
  }

  inline void SolveMeasures(hldsm* m, set_of_measures* list) {
    if (0==m) return;
    DCASSERT(children);
    int i = int(m->Type());
    if (i<0) return;
    CHECK_RANGE(0, i, num_hlm_types);
    if (children[i]) {
      DCASSERT(children[i]->AppliesToModelType(m->Type()));
      children[i]->SolveMeasures(m, list);
    } else {
      throw subengine::No_Engine;
    }
  }

  friend class engtype;
  friend void InitEngines(exprman* em);

private:
  /** 
      Build a radio button for this engine
      Called by engtype methods, probably should not be called otherwise.
  */
  radio_button* BuildOptionConst(int index);

  /// Are there internal options for this engine
  inline bool hasOptions() const { return options; }
};


// ******************************************************************
// *                                                                *
// *                         engtype  class                         *
// *                                                                *
// ******************************************************************

/** Engine type class.
    Basically, this is a combination of
      - information about different types of solution engines
      - a mini-registry for different solution methods
    for a single "problem" (e.g., state space generation).

    Also, it allows different applications to have different
    solution engine types, which is nice.

    More than one engine may be registered for a given engine type.
    If this is the case, when the engine type is finalize()d, we will
    build a radio button style option for the engine type, using
    the first registered engine as the default.  The engine "names"
    will correspond to the possible choices for the option.
    The option name is taken from the engine type name.

    After the engine type is "finalize()d", new engines may not be 
    registered, but engines may be launched.  If there are multiple 
    solution engines registered for a given type, we will use the 
    current option setting to decide which solution engine to launch.
*/
class engtype {
public:
  /// Different forms for invoking an engine.
  enum calling_form {
    /// None, used for "no engine".
    Nothing,
    /// The engine is a function call.
    FunctionCall,
    /// The engine is ron on a model.
    Model,
    /// The engine is run on a single measure (with a model).
    Single,
    /// The engine is run on a group of measures (with a model).
    Grouped
  };

private:
  const char* name;
  const char* doc;
  calling_form form;
  int index;
  // pre-finalization
  engine_tree* EngTree;
  engine* default_engine;
  // post-finalize
  engine* selected_engine;
  int selected_engine_index;
  friend class engine_selection;
public:
  engtype(const char* n, const char* d, calling_form f);
  virtual ~engtype(); 

  inline const char* Name() const { return name; }
  inline const char* Documentation() const { return doc; }
  inline calling_form getForm() const { return form; }

  inline int Compare(const char* x) const {
    DCASSERT(x);
    return strcmp(name, x);
  }
  inline int Compare(const engtype* x) const {
    DCASSERT(x);
    DCASSERT(x->name);
    return strcmp(name, x->name);
  }

  inline void setIndex(int ndx) {
    DCASSERT(0==index);
    index = ndx;
  }

  inline int getIndex() const { return index; }

  /// Register a solution engine. 
  void registerEngine(engine* e);

  /** Reset the default engine.
      If this is never called, then the first registered engine 
      becomes the default.

        @param  d   Engine to use as the default.
                    If 0, then the next registered engine becomes the default.
                    If 0 and no more engines are registered, 
                    then some engine will be arbitrarily chosen as default.

        @throws An appropriate error code.
  */
  void setDefaultEngine(engine* d);

  /** Register a solution sub-engine.
      Registry must not be finalized.
        @param  engname   Name of the engine that \a se should belong to.
                          An error occurs if no such engine can be found.
        @param  se        Subengine.
        @throws An appropriate error code.
  */
  void registerSubengine(const char* engname, subengine* se);

  /// Call when we are done registering engines.
  void finalizeRegistry(option_manager* om);

  /** Run an engine on parameters.
      Call this for engines of form "FunctionCall".
      Finds the currently-selected solution engine, and launches it.
        @param  pass  The parameters to pass to the engine.
        @param  np    Number of passed parameters.
        @param  x     Where to place the solution and such.
        @throws An appopriate error code
  */
  void runEngine(result* pass, int np, traverse_data &x);

  /** Run an engine on a model.
      Call this for engines of form "Model".
      Finds the currently-selected solution engine, and launches it.
        @param  m   The model to analyze.
                    (The solution, if any, is stored in the
                    model itself.)
        @throws An appopriate error code
  */
  void runEngine(hldsm* m, result &p);

  /** Solve a single measure.
      Call this for engines of form "Single".
      Finds the currently-selected solution engine, and launches it.
        @param  m     The model to analyze.
        @param  what  The measure to compute.
        @throws An appopriate error code
  */
  void solveMeasure(hldsm* m, measure* what);

  /** Solve a group of measures with the same engine type.
      Call this for engines of form "Grouped".
      Finds the currently-selected solution engine, and launches it.
        @param  m     The model to analyze.
        @param  list  Set of measures to compute.
        @throws An appopriate error code
  */
  void solveMeasures(hldsm* m, set_of_measures* list);

  /** Build a measure set for this engine type.
      Default behavior is to return 0.
  */
  virtual set_of_measures* makeMeasureSet() const;

private:
  void killEngTree();
  engine* getSelectedForModelType(int mt) const;
};

// ******************************************************************
// *                                                                *
// *                    unordered_engtype  class                    *
// *                                                                *
// ******************************************************************

class unordered_engtype : public engtype {
public:
  unordered_engtype(const char* n, const char* d);
  virtual set_of_measures* makeMeasureSet() const;
};

// ******************************************************************
// *                                                                *
// *                       time_engtype class                       *
// *                                                                *
// ******************************************************************

class time_engtype : public engtype {
public:
  time_engtype(const char* n, const char* d);
  virtual set_of_measures* makeMeasureSet() const;
};

// ******************************************************************
// *                                                                *
// *                       func_engine  class                       *
// *                                                                *
// ******************************************************************

class func_engine : public simple_internal {
  engtype* whicheng;
  result* engpass;
public:
  func_engine(const type* rettype, const char* name, int np, engtype* w);
  virtual ~func_engine();
  virtual void Compute(traverse_data &x, expr** pass, int np);
protected:
  virtual void BuildParams(traverse_data &x, expr** pass, int np) = 0;

  inline result& setParam(int i) { 
    CHECK_RANGE(0, i, formals.getLength());
    return engpass[i];
  }
};

// ******************************************************************
// *                                                                *
// *                           Front  end                           *
// *                                                                *
// ******************************************************************

/** Build an engine that simply calls another engine (type).
    Used for building hierarchies of engines.
    Example:
      StateGeneration (type) can have engines
          EXPLICIT: calls ExplicitStateGeneration (type)
          IMPLICIT: calls ImplicitStateGeneration (type)

      ImplicitStateGeneration (type) can have engines
          TRADITIONAL: normal iterations
          SATURATION:  saturation algorithm

      @param  n   Name of engine
      @param  d   Documentation of engine
      @param  e   Redirection location, i.e., the engine (type) to call.

      @return   A new engine, or 0 on error.
*/
engine* MakeRedirectionEngine(const char* n, const char* d, engtype* e);



/** Safe and proper registration of engine types.
*/
inline engtype* MakeEngineType(exprman* em, const char* n, 
                            const char* d, engtype::calling_form f)
{
  DCASSERT(em);
  engtype* et = em->findEngineType(n);
  if (et) {
    DCASSERT(et->getForm() == f);
    return et;
  }
  et = new engtype(n, d, f);
  CHECK_RETURN(em->registerEngineType(et), true);
  return et;
}


/** Safe and proper registration of engines.
*/
inline void RegisterEngine(engtype* et, engine* e)
{
  if (0==e) return;
  if (0==et) {
    delete e;
    return;
  }
  et->registerEngine(e);
}

inline void RegisterEngine(exprman* em, const char* etname, engine* e)
{
  RegisterEngine(em ? em->findEngineType(etname) : 0, e);
}

/** Handy: registration of engines that consist of a single subengine.
    This is the common case.
*/
inline engine*
RegisterEngine(engtype* et, const char* name, const char* doc, subengine* se)
{
  if (0==se || 0==et) return 0;
  engine* e = new engine(name, doc);
  et->registerEngine(e);
  e->AddSubEngine(se);
  return e;
}

inline engine*
RegisterEngine(exprman* em, const char* etname, 
                const char* name, const char* doc, subengine* se)
{
  return RegisterEngine(em ? em->findEngineType(etname) : 0, name, doc, se);
}

/** Also handy: registration of subengines with an existing engine.
    Fairly rare.
    This case, plus the previous, should handle 99.9% of engines.
*/
inline void
RegisterSubengine(engtype* et, const char* engname, subengine* se)
{
  if (0==se || 0==et) return;
  et->registerSubengine(engname, se);
}

inline void
RegisterSubengine(exprman* em, const char* etname, 
                  const char* engname, subengine* se)
{
  RegisterSubengine(em ? em->findEngineType(etname) : 0, engname, se);
}

/** Initialize fundamental engines.
*/
void InitEngines(exprman* em);

#endif

