
// $Id$

#include "models.h"
#include "measures.h"
#include "../Base/memtrack.h"

// For solutions

#include "../Engines/api.h"

//@Include: models.h

/** @name models.cc
    @type File
    @args \ 

   Implementation of (language support for) models.

 */

//@{

//#define DEBUG_MODEL

// ******************************************************************
// *                                                                *
// *                       model_var  methods                       *
// *                                                                *
// ******************************************************************

model_var::model_var(const char* fn, int line, type t, char* n)
 : symbol(fn, line, t, n)
{
  ALLOC("model_var", sizeof(model_var));
  state_index = -1;
  part_index = -1;
  SetSubstitution(false);
  has_lower_bound = has_upper_bound = false;
}

model_var::~model_var()
{
  FREE("model_var", sizeof(model_var));
}

void model_var::Compute(int i, result &x)
{
  DCASSERT(0==i);
  x.Clear();
  x.other = this;
/*
  if (state_index < 0) {
    // the index was never set
    // not sure what to do here, try this...
    x.setError();  
    Error.Start(Filename(), Linenumber());
    Error << "No index set for state " << Name();
    Error.Stop();
  } else {
    x.ivalue = state_index;
  }
*/
}

void model_var::ClearCache()
{
  // Do nothing?
}

// ******************************************************************
// *                                                                *
// *                         model  methods                         *
// *                                                                *
// ******************************************************************

bool model::SameParams()
{
  if (never_built) return false;  // force construction the first time

  // compare current_params with last_params
  int i;
  for (i=0; i<num_params; i++) {
    if (!Equals(parameters[i]->Type(0), current_params[i], last_params[i]))
      return false;
  }
  return true;
}

model::model(const char* fn, int l, type t, char* n, formal_param **pl, int np)
 : function(fn, l, t, n, pl, np)
{
  ALLOC("model", sizeof(model));
  // Important!
  SetSubstitution(false); 
  // we can probably allow forward defs, but for now let's not.
  never_built = true;
  isForward = false;
  stmt_block = NULL;
  num_stmts = 0;
  mtable = NULL;
  size_mtable = 0;
  dsm = NULL;
  // measure structures
  mlist = new List <measure>(16);
  mtrans = new List <measure>(16);
  msteady = new List <measure>(16);
  macc_trans = new List <measure>(16);
  macc_steady = new List <measure>(16);
  // Allocate space to save parameters
  if (np) {
    current_params = new result[np];
    last_params = new result[np];
  } else {
    current_params = last_params = NULL;
  }
  // link the parameters
  int i;
  for (i=0; i<np; i++) {
    pl[i]->LinkUserFunc(&current_params, i);
    last_params[i].setNull();
  }
  last_build.Clear();
  last_build.setNull();
}

model::~model()
{
  FREE("model", sizeof(model));
  // is this ever called?
  Delete(dsm);
  int i;
  for (i=0; i<num_stmts; i++) delete stmt_block[i];
  delete[] stmt_block;
  FreeLast();
  delete[] current_params;
  delete[] last_params;
  // delete measure structs
  delete mlist;
  // delete symbols...
}

void model::Instantiate(expr **p, int np, result &x, const char* f, int ln)
{
  if (NULL==stmt_block) {
    x.setNull();
    return;
  }
  DCASSERT(np <= num_params);
  // compute parameters
  int i;
  for (i=0; i<np; i++) SafeCompute(p[i], 0, current_params[i]);
  for (i=np; i<num_params; i++) current_params[i].setNull();

  // same as last time?
  if (SameParams()) {
    FreeCurrent();
    x = last_build;
    return;
  }

  // nope... rebuild
#ifdef DEBUG_MODEL
  Output << "Building model " << Name() << "\n";
  Output.flush();
#endif

  Clear();
  InitModel();

  for (i=0; i<num_stmts; i++) {
    stmt_block[i]->Execute();
  }
  GroupMeasures();

  FinalizeModel(last_build);
  x = last_build;
  if (x.other == this) dsm = BuildStateModel(f, ln); 
  else dsm = NULL;

  // save parameters
  FreeLast();
  SWAP(current_params, last_params);  // pointer swap!
  never_built = false;
}

Engine_type model::GetEngine(engineinfo *e)
{
  if (e) e->setNone();
  return ENG_None;
}

void model::AcceptMeasure(measure *m)
{
  mlist->Append(m); 
  m->ComputeDependencies(mlist);
}

symbol* model::FindVisible(char* name) const
{
  int low = 0;
  int high = size_mtable;
  while (low < high) {
    int mid = (high+low) / 2;
    const char* peek = mtable[mid]->Name(); 
    int cmp = strcmp(name, peek);
    if (0==cmp) return mtable[mid];
    if (cmp<0) {
      // name < peek
      high = mid;
    } else {
      // name > peek
      low = mid+1;
    }
  }
  // not found
  return NULL;
}

void model::SolveMeasure(measure *m)
{
  if (CS_Computed == m->state) return;  // already solved

  // First: solve any measures that this one depends on.
  int i;
  for (i=m->NumDependencies()-1; i>=0; i--) {
    SolveMeasure(m->GetDependency(i));
  }

#ifdef DEBUG_MODEL
  Output << "Model " << Name() << " solving measure " << m << "\n";
  Output.flush();
#endif

  // Based on the engine type of m, 
  // solve a huge batch of measures with the same engine
  // or just solve this one
  result foo;
  switch (m->GetEngine(NULL)) {

    	case ENG_Error:
    		// not sure about this
		foo.Clear();
		foo.setError();
		m->SetValue(foo);
		break;

	case ENG_SS_Inst:
		SolveSteadyInst(this, msteady);
		break;

	case ENG_SS_Acc:
		SolveSteadyAcc(this, macc_steady);
		break;

	case ENG_T_Inst:
		SolveTransientInst(this, mtrans);
		break;

	case ENG_T_Acc:
		SolveTransientAcc(this, mtrans);
		break;

	default:
		// no engine
		m->Compute(0, foo);
		m->SetValue(foo);
  }
}

void model::GroupMeasures()
{
  // for each measure in the list 
  int i;
  for (i=0; i<mlist->Length(); i++) {
    measure *m = mlist->Item(i);

    // If the measure is a certain type, 
    // group with others to be solved in batch
    switch (m->GetEngine(NULL)) {

	case ENG_T_Inst:
   				mtrans->Append(m);
				break;

	case ENG_SS_Inst:
				msteady->Append(m);
				break;

	case ENG_T_Acc:
   				macc_trans->Append(m);
				break;

	case ENG_SS_Acc:
				macc_steady->Append(m);
				break;

	default:
				break;
				// Do nothing 
				// (keeps compiler happy)
    } // switch
  }
#ifdef DEBUG_MODEL
  Output << "Classified measures for model " << Name() << "\n";
  Output.flush();
  if (mtrans->Length()) {
    Output << "\tTransient: ";
    for (i=0; i<mtrans->Length(); i++) {
      if (i) Output << ", ";
      measure *foo = mtrans->Item(i);
      Output << foo->Name();
    }
    Output << "\n";
    Output.flush();
  }
  if (msteady->Length()) {
    Output << "\tSteady-state: ";
    for (i=0; i<msteady->Length(); i++) {
      if (i) Output << ", ";
      measure *foo = msteady->Item(i);
      Output << foo->Name();
    }
    Output << "\n";
    Output.flush();
  }
  if (macc_trans->Length()) {
    Output << "\tAccumulated (transient): ";
    for (i=0; i<macc_trans->Length(); i++) {
      if (i) Output << ", ";
      measure *foo = macc_trans->Item(i);
      Output << foo->Name();
    }
    Output << "\n";
    Output.flush();
  }
  if (macc_steady->Length()) {
    Output << "\tAccumulated (steady-state): ";
    for (i=0; i<macc_steady->Length(); i++) {
      if (i) Output << ", ";
      measure *foo = macc_steady->Item(i);
      Output << foo->Name();
    }
    Output << "\n";
    Output.flush();
  }
  Output << "Remaining are custom engines\n";
  Output.flush();
#endif
}

void model::Clear()
{
  Delete(dsm);
  dsm = NULL;
  mlist->Clear();
  if (mtrans) mtrans->Clear();
  msteady->Clear();
  macc_trans->Clear();
  macc_steady->Clear();
  for (int i=0; i<num_stmts; i++) {
    stmt_block[i]->Clear();
  }

#ifdef MEM_TRACE_ON
  Memory_Log.Stop(Output);
  Output.flush();
  Memory_Log.Start();
#endif
}

// ******************************************************************
// *                                                                *
// *                          mcall  class                          *
// *                                                                *
// ******************************************************************

/**  An expression to compute a model call.
     That is, this computes expressions of the type
        model(p1, p2, p3).measure;
*/

class mcall : public expr {
protected:
  model *mdl;
  expr **pass;
  int numpass;
  measure *msr;
public:
  mcall(const char *fn, int line, model *m, expr **p, int np, measure *s);
  virtual ~mcall();
  virtual type Type(int i) const;
  virtual void ClearCache();
  virtual void Compute(int i, result &x);
  virtual expr* Substitute(int i);
  virtual int GetSymbols(int i, List <symbol> *syms=NULL);
  virtual void show(OutputStream &s) const;
  virtual Engine_type GetEngine(engineinfo *e);
  virtual expr* SplitEngines(List <measure> *mlist);
};

// ******************************************************************
// *                         mcall  methods                         *
// ******************************************************************

mcall::mcall(const char *fn, int l, model *m, expr **p, int np,
measure *s) : expr (fn, l)
{
  ALLOC("mcall", sizeof(mcall));
  mdl = m;
  pass = p;
  numpass = np;
  msr = s;
}

mcall::~mcall()
{
  FREE("mcall", sizeof(mcall));
  // don't delete the model
  int i;
  for (i=0; i < numpass; i++) Delete(pass[i]);
  delete[] pass;
  // don't delete the measure
}

type mcall::Type(int i) const
{
  DCASSERT(0==i);
  return msr->Type(i);
}

void mcall::ClearCache()
{
  int i;
  for (i=0; i < numpass; i++) if (pass[i]) pass[i]->ClearCache();
}

void mcall::Compute(int i, result &x)
{
  DCASSERT(0==i);
  DCASSERT(mdl);
  result m;

  mdl->Instantiate(pass, numpass, m, Filename(), Linenumber());
  /*
  if (m.isError()) {
    Error.Continue(Filename(), Linenumber());
    Error << "model instantiation " << mdl;
    if (numpass>1) Error << "(" << pass[1];
    for (int i=2; i<numpass; i++) {
      Error << ", " << pass[i];
    }
    if (numpass>1) Error << ")";
    Error.Stop();
  }
  */
  if (m.isError() || m.isNull()) {
      x = m;
      return;
  }
  DCASSERT(m.other == mdl);

  mdl->SolveMeasure(msr);
  msr->Compute(0, x);
}

expr* mcall::Substitute(int i)
{
  DCASSERT(0==i);
  if (0==numpass) return Copy(this);

  // substitute each passed parameter
  expr ** pass2 = new expr*[numpass];
  int n;
  for (n=0; n<numpass; n++) pass2[n] = pass[n]->Substitute(0);

  return new mcall(Filename(), Linenumber(), mdl, pass2, numpass, msr);
}

int mcall::GetSymbols(int i, List <symbol> *syms)
{
  DCASSERT(0==i);
  int n;
  int answer = 0;
  for (n=0; n<numpass; n++) {
    answer += pass[n]->GetSymbols(0, syms);
  }
  return answer;
}

void mcall::show(OutputStream &s) const
{
  if (mdl->Name()==NULL) return; // can this happen?
  s << mdl->Name();
  if (numpass) {
    s << "(";
    int i;
    for (i=0; i<numpass; i++) {
      if (i) s << ", ";
      s << pass[i];
    }
    s << ")";
  }
  s << "." << msr;
}

Engine_type mcall::GetEngine(engineinfo *e)
{
  // Since we are outside the model, treat this as NONE
  if (e) e->engine = ENG_None;
  return ENG_None;
}

expr* mcall::SplitEngines(List <measure> *mlist)
{
  return Copy(this);
}

// ******************************************************************
// *                                                                *
// *                         ma_call  class                         *
// *                                                                *
// ******************************************************************

/**  An expression to compute a model call for an array measure.
     That is, this computes expressions of the type
        model(p1, p2, p3).measure[i][j];
*/

class ma_call : public expr {
protected:
  // model call part
  model *mdl;
  expr **pass;
  int numpass;
  // measure call part
  array *msr;
  expr **indx;
  int numindx;
public:
  ma_call(const char *fn, int line, model *m, expr **p, int np, 
	  array *s, expr **i, int ni);
  virtual ~ma_call();
  virtual type Type(int i) const;
  virtual void ClearCache();
  virtual void Compute(int i, result &x);
  virtual expr* Substitute(int i);
  virtual int GetSymbols(int i, List <symbol> *syms=NULL);
  virtual void show(OutputStream &s) const;
  virtual Engine_type GetEngine(engineinfo *e);
  virtual expr* SplitEngines(List <measure> *mlist);
};

// ******************************************************************
// *                        ma_call  methods                        *
// ******************************************************************

ma_call::ma_call(const char *fn, int l, model *m, expr **p, int np,
array *s, expr **i, int ni) : expr (fn, l)
{
  ALLOC("ma_call", sizeof(ma_call));
  mdl = m;
  pass = p;
  numpass = np;
  msr = s;
  indx = i;
  numindx = ni;
}

ma_call::~ma_call()
{
  FREE("ma_call", sizeof(ma_call));
  // don't delete the model
  int i;
  for (i=0; i < numpass; i++) Delete(pass[i]);
  delete[] pass;
  // TODO: delete the measures?
}

type ma_call::Type(int i) const
{
  DCASSERT(0==i);
  return msr->Type(i);
}

void ma_call::ClearCache()
{
  int i;
  for (i=0; i < numpass; i++) if (pass[i]) pass[i]->ClearCache();
}

void ma_call::Compute(int i, result &x)
{
  DCASSERT(0==i);
  DCASSERT(mdl);

  // builds the model (if necessary)
  mdl->Instantiate(pass, numpass, x, Filename(), Linenumber());
  if (x.isError() || x.isNull()) return;
  DCASSERT(x.other == mdl);

  // grab the measure from the array
  msr->Compute(indx, x);
  if (x.isError() || x.isNull()) return;
  measure *m = (measure*) x.other;
 
  mdl->SolveMeasure(m);
  m->Compute(0, x);
}

expr* ma_call::Substitute(int i)
{
  DCASSERT(0==i);
  if (0==numpass) return Copy(this);

  // substitute each passed parameter
  expr ** pass2 = new expr*[numpass];
  int n;
  for (n=0; n<numpass; n++) pass2[n] = pass[n]->Substitute(0);

  // substitute each index
  expr** indx2 = new expr*[numindx];
  for (n=0; n<numindx; n++) indx2[n] = indx[n]->Substitute(0);

  return new ma_call(Filename(), Linenumber(), mdl, 
			pass2, numpass, msr, indx2, numindx);
}

int ma_call::GetSymbols(int i, List <symbol> *syms)
{
  DCASSERT(0==i);
  int n;
  int answer = 0;
  for (n=0; n<numpass; n++) {
    answer += pass[n]->GetSymbols(0, syms);
  }
  for (n=0; n<numindx; n++) {
    answer += indx[n]->GetSymbols(0, syms);
  }
  return answer;
}

void ma_call::show(OutputStream &s) const
{
  if (mdl->Name()==NULL) return; // can this happen?
  s << mdl->Name();
  if (numpass) {
    s << "(";
    int i;
    for (i=0; i<numpass; i++) {
      if (i) s << ", ";
      s << pass[i];
    }
    s << ")";
  }
  s << "." << msr->Name();
  for (int n=0; n<numindx; n++) { 
    s << "[" << indx[n] << "]";
  }
}

Engine_type ma_call::GetEngine(engineinfo *e)
{
  // Since we are outside the model, treat this as NONE
  if (e) e->engine = ENG_None;
  return ENG_None;
}

expr* ma_call::SplitEngines(List <measure> *mlist)
{
  return Copy(this);
}

// ******************************************************************
// *                                                                *
// *                         wrapper  class                         *
// *                                                                *
// ******************************************************************

/** Class used as a wrapper around a (not yet existing) model variable.
    The variable can be plugged in after it exists.

    Everything is public because the substitutions are handled
    directly by a statement.

    Since any expression with a wrapper should be "Substituted" before
    execution (for speed), we shouldn't need to provide "compute" or 
    "sample" methods.
*/
class wrapper : public expr {
public:
  /// The (eventual) symbol to be plugged here
  const char* who;
  /// The (eventual) type of that symbol
  type mytype;
  /// The variable.
  model_var* var;
public:
  wrapper(const char* fn, int line) : expr(fn, line) {
    ALLOC("wrapper", sizeof(wrapper));
    who = NULL;
    mytype = VOID; // fill in later
    var = NULL;
  }
  virtual ~wrapper() {
    // don't delete "who", we don't own it
    // not sure about var yet
    FREE("wrapper", sizeof(wrapper));
  }

  virtual type Type(int i) const {
    DCASSERT(i==0);
    return mytype;
  }

  virtual void ClearCache() { } // No cache

  virtual expr* Substitute(int i) {
    DCASSERT(i==0);
    return Copy(var);
  }

  virtual void show(OutputStream &s) const {
    s << " (";
    if (var) s << var; else s << who;
    s << ") ";
  }

  virtual void Compute(int i, result &x) {
    x.Clear();
    if (NULL==var) x.setNull();
    else x.other = var;
  }

};

// ******************************************************************
// *                                                                *
// *                      model_var_stmt class                      *
// *                                                                *
// ******************************************************************

class model_var_stmt : public statement {
protected:
  model* parent;
  type vartype;
  char** names;
  expr** wraps;
  int numvars;
public:
  model_var_stmt(const char *fn, int line, model *p, type t, 
  		char** n, expr** w, int nv);

  virtual ~model_var_stmt();
  virtual void Execute();
  virtual void Clear();
  virtual void showfancy(int dpth, OutputStream &s) const;
  virtual void show(OutputStream &s) const { showfancy(0, s); }
};

model_var_stmt::model_var_stmt(const char *fn, int line, model *p, type t, 
  		char** n, expr** w, int nv) : statement(fn, line) 
{
  ALLOC("model_var_stmt", sizeof(model_var_stmt));
  parent = p;
  vartype = t;
  names = n;
  wraps = w;
  numvars = nv;
  int i;
  for (i=0; i<numvars; i++) {
    wrapper* z = dynamic_cast<wrapper*>(wraps[i]);
    if (NULL==z) {
      Internal.Start(__FILE__, __LINE__, fn, line);
      Internal << "Bad wrapper for model variable";
      Internal.Stop();
    }
    z->mytype = vartype;
    z->who = names[i];
  }
}

model_var_stmt::~model_var_stmt()
{
  FREE("model_var_stmt", sizeof(model_var_stmt));
  Clear();
  int i;
  for (i=0; i<numvars; i++) {
    delete[] names[i];
  }
  delete[] names;
  delete[] wraps;
}

void model_var_stmt::Execute()
{
  int i;
  for (i=0; i<numvars; i++) {
    wrapper* z = dynamic_cast<wrapper*>(wraps[i]);
    z->var = parent->MakeModelVar(z->Filename(), z->Linenumber(), vartype, names[i]);
  }
}

void model_var_stmt::Clear()
{
  int i;
  for (i=0; i<numvars; i++) {
    wrapper* z = dynamic_cast<wrapper*>(wraps[i]);
    delete z->var;
    z->var = NULL;
  }
}

void model_var_stmt::showfancy(int dpth, OutputStream &s) const
{
  s.Pad(' ', dpth);
  s << GetType(vartype) << " ";
  int i;
  for (i=0; i<numvars; i++) {
    if (i) s << ", ";
    s << names[i];
  }
  s << ";";
}

// ******************************************************************
// *                                                                *
// *                    model_varray_stmt  class                    *
// *                                                                *
// ******************************************************************

class model_varray_stmt : public statement {
protected:
  model* parent;
  array** vars;
  int numvars;
public:
  model_varray_stmt(const char *fn, int line, model *p, array** a, int nv);

  virtual ~model_varray_stmt();
  virtual void Execute();
  virtual void Clear();
  virtual void showfancy(int dpth, OutputStream &s) const;
  virtual void show(OutputStream &s) const { showfancy(0, s); }
};

model_varray_stmt::model_varray_stmt(const char *fn, int line, model *p, 
  		array** a, int nv) : statement(fn, line) 
{
  ALLOC("model_varray_stmt", sizeof(model_varray_stmt));
  parent = p;
  vars = a;
  numvars = nv;
  DCASSERT(numvars);
}

model_varray_stmt::~model_varray_stmt()
{
  FREE("model_varray_stmt", sizeof(model_varray_stmt));
  Clear();
  int i;
  for (i=0; i<numvars; i++) {
    delete[] vars[i];
  }
  delete[] vars;
}

void model_varray_stmt::Execute()
{
  static StringStream mvs_name;
  int i;
  for (i=0; i<numvars; i++) {
    mvs_name.flush();
    vars[i]->GetName(mvs_name);
    char* name = mvs_name.GetString();
    symbol* var = parent->MakeModelVar( 
    			vars[i]->Filename(), 
			vars[i]->Linenumber(), 
			vars[i]->Type(0),
			name
		);
    vars[i]->SetCurrentReturn(var);
  }
}

void model_varray_stmt::Clear()
{
  int i;
  for (i=0; i<numvars; i++) {
    vars[i]->Clear();
  }
}

void model_varray_stmt::showfancy(int dpth, OutputStream &s) const
{
  s.Pad(' ', dpth);
  s << GetType(vars[0]->Type(0)) << " ";
  int i;
  for (i=0; i<numvars; i++) {
    if (i) s << ", ";
    s << vars[i];
  }
  s << ";";
}

// ******************************************************************
// *                                                                *
// *                      measure_assign class                      *
// *                                                                *
// ******************************************************************

/**  A statement used for measure assignments.
     Only necessary to initialize the measure within the model
     for each model instantiation.
 */

class measure_assign : public statement {
  measure *msr;
  model *parent;
public:
  measure_assign(const char *fn, int l, model *p, measure *msr);
  virtual ~measure_assign(); 

  virtual void Execute();
  virtual void Clear();
  virtual void show(OutputStream &s) const;
  virtual void showfancy(int depth, OutputStream &s) const;
};

measure_assign::measure_assign(const char *fn, int l, 
	model *p, measure *m) : statement(fn, l)
{
  ALLOC("measure_assign", sizeof(measure_assign));
  parent = p;
  msr = m;
}

measure_assign::~measure_assign()
{
  FREE("measure_assign", sizeof(measure_assign));
  Delete(msr);
}

void measure_assign::Execute()
{
  parent->AcceptMeasure(msr);
}

void measure_assign::Clear()
{
  msr->Clear();
}

void measure_assign::show(OutputStream &s) const
{
  s << msr;
}

void measure_assign::showfancy(int depth, OutputStream &s) const
{
  s.Pad(' ', depth);
  show(s);
  s << ";\n";
}



// ******************************************************************
// *                                                                *
// *                   measure_array_assign class                   *
// *                                                                *
// ******************************************************************

/**  A statement used for measure array assignments.
 */

class measure_array_assign : public statement {
  array *f;
  model *parent;
  expr *retval;
public:
  measure_array_assign(const char *fn, int l, model *p, array *a, expr *e);
  virtual ~measure_array_assign(); 

  virtual void Execute();
  virtual void Clear();
  virtual void show(OutputStream &s) const;
  virtual void showfancy(int depth, OutputStream &s) const;
};

measure_array_assign::measure_array_assign(const char *fn, int l, 
	model *p, array *a, expr *e) : statement(fn, l)
{
  ALLOC("measure_array_assign", sizeof(measure_array_assign));
  parent = p;
  f = a;
  retval = e;
}

measure_array_assign::~measure_array_assign()
{
  FREE("measure_array_assign", sizeof(measure_array_assign));
  Delete(retval);
}

void measure_array_assign::Execute()
{
  // De-iterate the return value
  expr* rv = (retval) ? (retval->Substitute(0)) : NULL;

  if (NULL==rv) {
    f->SetCurrentReturn(NULL);
#ifdef ARRAY_TRACE
    cout << "Array assign: null\n";
#endif
    return;
  }
  
#ifdef DEBUG_MODEL
  // Build a long name.  Useful for debugging, otherwise 
  // I think it is unnecessary work.
  StringStream dealy;
  f->GetName(dealy);
  char* name = dealy.GetString();
#else
  char* name = NULL;
#endif

  measure* frv = new measure(Filename(), Linenumber(), rv->Type(0), name);
  frv->SetReturn(rv);

  parent->AcceptMeasure(frv);

  f->SetCurrentReturn(frv);

#ifdef ARRAY_TRACE
  cout << "Array assign: " << frv << "\n";
#endif
}

void measure_array_assign::Clear()
{
  f->Clear();
}

void measure_array_assign::show(OutputStream &s) const
{
  s << f << " := " << retval;
}

void measure_array_assign::showfancy(int depth, OutputStream &s) const
{
  s.Pad(' ', depth);
  show(s);
  s << ";\n";
}



// ******************************************************************
// *                                                                *
// *                                                                *
// *                          Global stuff                          *
// *                                                                *
// *                                                                *
// ******************************************************************

expr* MakeMeasureCall(model *m, expr **p, int np, measure *s, const char *fn, int line)
{
  return new mcall(fn, line, m, p, np, s);
}

expr* MakeMeasureArrayCall(model *m, expr **p, int np, array *s, expr **i, int ni, const char *fn, int line)
{
  return new ma_call(fn, line, m, p, np, s, i, ni);
}

expr* MakeEmptyWrapper(const char *fn, int line)
{
  return new wrapper(fn, line);  
}

statement* MakeModelVarStmt(model* p, type t, char** names, expr** wraps, int N,
			const char* fn, int l)
{
  return new model_var_stmt(fn, l, p, t, names, wraps, N);
}

statement* MakeModelArrayStmt(model* p, array** alist, int N,
			const char* fn, int l)
{
  return new model_varray_stmt(fn, l, p, alist, N);
}

statement* MakeMeasureAssign(model *p, measure *m, const char *fn, int line)
{
  return new measure_assign(fn, line, p, m);
}

statement* MakeMeasureArrayAssign(model *p, array *f, expr* retval, 
                           const char *fn, int line)
{
  f->state = CS_Defined;
  return new measure_array_assign(fn, line, p, f, retval);
}



//@}

