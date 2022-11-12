
#include "mod_def.h"
#include "mod_inst.h"
#include "mod_vars.h"
#include "../Utils/strings.h"
#include "../Streams/streams.h"
#include "../Options/options.h"
#include "arrays.h"
#include "exprman.h"

#include <string.h>
#include <stdlib.h>

// #define ARRAY_TRACE

// ******************************************************************
// *                                                                *
// *                       model_def  methods                       *
// *                                                                *
// ******************************************************************

warning_msg model_def::not_our_var;

model_def::model_def(const location &W, const type* t, char* n,
      formal_param **pl, int np)
 : function(W, t, n)
{
  stmt_block = 0;
  mysymbols = 0;
  num_symbols = 0;
  if (np) {
    current_params = new result[np];
    last_params = new result[np];
  } else {
    current_params = 0;
    last_params = 0;
  }
  for (int i=0; i<np; i++) {
    current_params[i].setNull();
    last_params[i].setNull();
  }
  last_build = 0;
  current = 0;
  formals.setAll(np, pl, true);
  formals.setStack(&current_params);
  dotfile = 0;
}

model_def::~model_def()
{
  // is this ever called?
  Delete(stmt_block);
  for (int i=0; i<num_symbols; i++)  Delete(mysymbols[i]);
  delete[] mysymbols;
  for (int i=0; i<formals.getLength(); i++) {
    current_params[i].deletePtr();
    last_params[i].deletePtr();
  }
  delete[] current_params;
  delete[] last_params;

  Delete(last_build);
}

int model_def::FindVisible(const char* name) const
{
  int low = 0;
  int high = num_symbols;
  while (low < high) {
    int mid = (high+low) / 2;
    const char* peek = mysymbols[mid]->Name();
    int cmp = strcmp(name, peek);
    if (0==cmp) return mid;
    if (cmp<0) {
      // name < peek
      high = mid;
    } else {
      // name > peek
      low = mid+1;
    }
  }
  // not found
  return -1;
}

void model_def::SetDotFile(result& x)
{
  Delete(dotfile);
  dotfile = smart_cast <shared_string*> (Share(x.getPtr()));
}

void model_def::BuildModel(traverse_data &x)
{
  if (model_debug.startReport()) {
    model_debug.report() << "Building model " << Name() << "\n";
    model_debug.stopIO();
  }

  // Build new instance
  current = new model_instance(x.parent ? x.parent->Where() : location::NOWHERE(), this);

  InitModel();
  DCASSERT(stmt_block);
  stmt_block->Compute(x);

  // do we want to dump a dot file
  FILE* foo = 0;
  if (dotfile) {
    // check if file exists
    foo = fopen(dotfile->getStr(), "r");
    if (foo) {
      // file exists
      fclose(foo);
      foo = 0;
    } else {
      // create file
      foo = fopen(dotfile->getStr(), "w");
    }
  }
  DisplayStream temp(foo);
  if (0==foo) temp.Deactivate();
  else        temp.Activate();

  FinalizeModel(temp);

  DCASSERT(current->GetState() == model_instance::Error ||
     current->GetState() == model_instance::Ready);

  // clear out any data
  Delete(dotfile);
  dotfile = 0;
  x.which = traverse_data::ModelDone;
  stmt_block->Traverse(x);
  x.which = traverse_data::Compute;

  if (model_debug.startReport()) {
    model_debug.report() << "Finished with instantiation of model ";
    model_debug.report() << Name() << "\n";
    model_debug.stopIO();
  }
}

bool model_def::StartWarning(const warning_msg &who, const expr* cause) const
{
  if (!who.startWarning())  return false;
  if (cause) {
      who.causedBy(cause->Where());
  } else {
      who.causedBy(location::NOWHERE());
  }
  return true;
}

void model_def::DoneWarning() const
{
  DCASSERT(current);
  em->newLine();
  em->warn() << "within model " << Name() << " built " << current->Where();
  em->stopIO();
}

bool model_def::StartError(const expr* cause) const
{
  if (!em->startError())  return false;
  em->causedBy(cause);
  return true;
}

void model_def::DoneError() const
{
  DCASSERT(current);
  em->newLine();
  em->cerr() << "within model " << Name() << " built " << current->Where();
  em->stopIO();
}

bool model_def::isVariableOurs(const model_var* mv,
        const expr* cause, const char* what) const
{
  DCASSERT(mv);
  if (mv->getParent() == current)  return true;

  if (StartWarning(not_our_var, cause)) {
    const type* mvt = mv->Type();
    DCASSERT(mvt);
    em->warn() << mvt->getName() << " " << mv->Name();
    em->warn() << " is from another model";
    if (what) em->warn() << ", " << what;
    DoneWarning();
  }
  return false;
}

void model_def::Compute(traverse_data &x, expr** pass, int np)
{
  formals.compute(x, pass, current_params);
  BuildModel(x);
  // store instantiated model in result
  DCASSERT(x.answer);
  x.answer->setPtr(current);
  current = 0;
}

void model_def::PrintHeader(OutputStream &s, bool hide) const
{
  if (Type())  s << Type()->getName();
  s << " " << Name();
  formals.PrintHeader(s, hide);
}

symbol* model_def::FindFormal(const char* name) const
{
  return formals.find(name);
}

bool model_def::IsHidden(int fpnum) const
{
  return formals.isHidden(fpnum);
}

bool model_def::HasNameConflict(symbol** fp, int np, int* tmp) const
{
  return formals.hasNameConflict(fp, np, tmp);
}

void model_def::Traverse(traverse_data &x)
{
  switch (x.which) {
    case traverse_data::Substitute:
        // Substitute the model we are building for ourself.
        DCASSERT(x.answer);
        x.answer->setPtr(Share(current));
        return;

    case traverse_data::PreCompute:
        return;

    default:
        function::Traverse(x);
  }
}

int model_def::Traverse(traverse_data &x, expr** pass, int np)
{
  switch (x.which) {
    case traverse_data::Typecheck:
        return formals.check(em, pass, np, Type());

    case traverse_data::Promote:
        formals.promote(em, pass, np, Type());
        return Promote_Success;

    case traverse_data::GetType:
        x.the_type = formals.getType(em, pass, np, Type());
        x.the_model_type = this;
        return 0;

    default:
        return function::Traverse(x, pass, np);
  }
}

int model_def::maxNamedParams() const
{
  return formals.getLength();
}

int model_def
::named2Positional(symbol** np, int nnp, expr** buffer, int bufsize) const
{
  return formals.named2Positional(em, np, nnp, buffer, bufsize);
}


model_instance* model_def::Instantiate(traverse_data &x, expr** pass, int np)
{
  formals.compute(x, pass, current_params);
  if (!SameParams()) {
    BuildModel(x);
    last_build = current;
    current = 0;
    SaveParams();
  }
  return last_build;
}

bool model_def::SameParams() const
{
  if (0==last_build) return false;
  // compare current_params with last_params
  for (int i=0; i<formals.getLength(); i++) {
    const type* t = formals.getType(i);
    DCASSERT(t);
    if (! t->equals(last_params[i], current_params[i]))  return false;
  }
  return true;
}

void model_def::SaveParams()
{
  for (int i=0; i<formals.getLength(); i++) {
    last_params[i] = current_params[i];
  }
}


//
// HIDDEN STUFF
//


// ******************************************************************
// *                                                                *
// *                         md_call  class                         *
// *                                                                *
// ******************************************************************

/**  An expression to compute a model call.
     That is, this computes expressions of the type
        model(p1, p2, p3).measure;
*/

class md_call : public expr {
protected:
  model_def *mdl;
  expr** pass;
  int numpass;
  int msr_slot;
public:
  md_call(const location &W, model_def *m, expr **p, int np, int slot);
  virtual ~md_call();

  virtual void Compute(traverse_data &x);
  virtual void Traverse(traverse_data &x);
  virtual bool Print(OutputStream &s, int) const;
};

md_call::md_call(const location &W, model_def *m,
      expr **p, int np, int slot)
  : expr (W, (typelist*) 0)
{
  const symbol* s = m->GetSymbol(slot);
  SetType(em->SafeType(s));
  mdl = m;
  pass = p;
  numpass = np;
  msr_slot = slot;
}

md_call::~md_call()
{
  for (int i=0; i<numpass; i++)  Delete(pass[i]);
  delete[] pass;
}

void md_call::Compute(traverse_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(mdl);
  const expr* oldp = x.parent;
  x.parent = this;

  // instantiate the model
  model_instance* mi = mdl->Instantiate(x, pass, numpass);

  if (mi->NotProperInstance(this, 0)) {
    x.answer->setNull();
  } else {
    // find and compute the measure
    symbol* find = mi->FindExternalSymbol(msr_slot);
    SafeCompute(find, x);
  }

  x.parent = oldp;
}

void md_call::Traverse(traverse_data &x)
{
  DCASSERT(mdl);
  switch (x.which) {
    case traverse_data::Substitute: {
        DCASSERT(x.answer);
        expr** newpass = new expr* [numpass];
        bool notequal = false;
        for (int i=0; i<numpass; i++) {
          if (pass[i]) {
            pass[i]->Traverse(x);
            newpass[i] = smart_cast <expr*> (Share(x.answer->getPtr()));
          } else {
            newpass[i] = 0;
          }
          if (newpass[i] != pass[i])  notequal = true;
         } // for i
        if (notequal) {
          x.answer->setPtr(new md_call(Where(), mdl,
                newpass, numpass, msr_slot));
        } else {
          for (int i=0; i<numpass; i++)  Delete(newpass[i]);
          delete[] newpass;
          x.answer->setPtr(Share(this));
        }
        return;
    } // traverse_data::Substitute

    case traverse_data::PreCompute:
    case traverse_data::ClearCache:
        for (int i=0; i<numpass; i++) if (pass[i]) {
          pass[i]->Traverse(x);
        } // for i
        return;

    default:
        mdl->Traverse(x, pass, numpass);
  }
}

bool md_call::Print(OutputStream &s, int) const
{
  if (mdl->Name()==NULL) return false; // can this happen?
  s << mdl->Name();
  if (numpass) {
    s << "(";
    int i;
    for (i=0; i<numpass; i++) {
      if (i) s << ", ";
      if (pass[i])  pass[i]->Print(s, 0);
      else          s << "null";
    }
    s << ")";
  }
  s << ".";
  const symbol* msr = mdl->GetSymbol(msr_slot);
  s << msr->Name();
  return true;
}


// ******************************************************************
// *                                                                *
// *                         md_acall class                         *
// *                                                                *
// ******************************************************************

/**  An expression to compute a model call for an array measure.
     That is, this computes expressions of the type
        model(p1, p2, p3).measure[i][j];
*/

class md_acall : public expr {
protected:
  // model call part
  model_def* mdl;
  expr** pass;
  int numpass;
  // measure call part
  int msr_slot;
  expr** indx;
  int numindx;
public:
  md_acall(const location &W, model_def* m, expr** p, int np,
    int slot, expr** i, int ni);
  virtual ~md_acall();

  virtual void Compute(traverse_data &x);
  virtual void Traverse(traverse_data &x);
  virtual bool Print(OutputStream &s, int) const;
};

md_acall::md_acall(const location &W, model_def* m, expr** p, int np,
      int slot, expr** i, int ni)
 : expr (W, (typelist*) 0)
{
  const symbol* s = m->GetSymbol(slot);
  SetType(em->SafeType(s));
  mdl = m;
  pass = p;
  numpass = np;
  msr_slot = slot;
  indx = i;
  numindx = ni;
}

md_acall::~md_acall()
{
  for (int i=0; i<numpass; i++)  Delete(pass[i]);
  delete[] pass;
  for (int i=0; i<numindx; i++) Delete(indx[i]);
  delete[] indx;
}

void md_acall::Compute(traverse_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(mdl);
  const expr* oldp = x.parent;
  x.parent = this;

  // instantiate the model
  model_instance* mi = mdl->Instantiate(x, pass, numpass);

  if (mi->NotProperInstance(this, 0)) {
    x.answer->setNull();
  } else {
    // find the measure array
    array_item* elem;
    symbol* find = mi->FindExternalSymbol(msr_slot);
    if (find) {
      array* rewards = smart_cast <array*> (find);
      DCASSERT(rewards);
      // compute the array element.
      elem = rewards->GetItem(indx, *x.answer);
    } else {
      elem = 0;
    }
    if (elem)   elem->Compute(x, false);
    else        x.answer->setNull();
  }

  x.parent = oldp;
}

void md_acall::Traverse(traverse_data &x)
{
  DCASSERT(mdl);
  switch (x.which) {
    case traverse_data::Substitute: {
        DCASSERT(x.answer);
         // build new list of passed parameters
        expr** newpass = new expr* [numpass];
        bool notequal = false;
        for (int i=0; i<numpass; i++) {
          if (pass[i]) {
            pass[i]->Traverse(x);
            newpass[i] = smart_cast <expr*> (Share(x.answer->getPtr()));
          } else {
            newpass[i] = 0;
          }
          if (newpass[i] != pass[i])  notequal = true;
         } // for i
        // build new list of array indexes
        expr** newindx = new expr*[numindx];
        for (int i=0; i<numindx; i++) {
          if (indx[i]) {
            indx[i]->Traverse(x);
            newindx[i] = smart_cast <expr*> (Share(x.answer->getPtr()));
          } else {
            newindx[i] = 0;
          }
          if (newindx[i] != indx[i])  notequal = true;
         } // for i

        // build a new call
        if (notequal) {
          x.answer->setPtr(new md_acall(Where(), mdl,
                newpass, numpass, msr_slot, newindx, numindx));
        } else {
          for (int i=0; i<numpass; i++)  Delete(newpass[i]);
          delete[] newpass;
          for (int i=0; i<numindx; i++)  Delete(newindx[i]);
          delete[] newindx;
          x.answer->setPtr(Share(this));
        }
        return;
    } // Substitute

    case traverse_data::PreCompute:
    case traverse_data::ClearCache:
        for (int i=0; i<numpass; i++) if (pass[i]) {
          pass[i]->Traverse(x);
        } // for i
        return;

    default:
        mdl->Traverse(x, pass, numpass);
  }
}

bool md_acall::Print(OutputStream &s, int) const
{
  if (mdl->Name()==NULL) return false; // can this happen?
  s << mdl->Name();
  if (numpass) {
    s << "(";
    int i;
    for (i=0; i<numpass; i++) {
      if (i) s << ", ";
      if (pass[i])  pass[i]->Print(s, 0);
      else          s << "null";
    }
    s << ")";
  }
  const symbol* msr = mdl->GetSymbol(msr_slot);
  s << "." << msr->Name();
  for (int n=0; n<numindx; n++) {
    s << "[";
    if (indx[n])  indx[n]->Print(s, 0);
    else          s << "null";
    s << indx[n] << "]";
  }
  return true;
}


// ******************************************************************
// *                                                                *
// *                        exprman  methods                        *
// *                                                                *
// ******************************************************************

void exprman::finishModelDef(model_def* p, expr* stmts,
      symbol** st, int ns) const
{
  if (0==p) {
    Delete(stmts);
    for (int i=0; i<ns; i++)  Delete(st[i]);
    delete[] st;
  } else {
    p->SetStatementBlock(stmts);
    p->SetSymbolTable(st, ns);
  }
}


inline void TrashPass(expr** p, int np)
{
  for (int i=0; i<np; i++)  Delete(p[i]);
  delete[] p;
}

expr* exprman::makeMeasureCall(const location &W, model_def* m,
      expr** p, int np, const char* msr_name) const
{
  if (0==m || 0==msr_name) {
    TrashPass(p, np);
    return 0;
  }

  int slot = m->FindVisible(msr_name);
  if (slot < 0) {
    if (startError()) {
      causedBy(W);
      cerr() << "Measure " << msr_name;
      cerr() << " does not exist in model ";
      if (m->Name()) cerr() << m->Name();
      stopIO();
    }
    TrashPass(p, np);
    return makeError();
  }

  const symbol* msr = m->GetSymbol(slot);
  DCASSERT(msr);
  const array* foo = dynamic_cast <const array*> (msr);
  if (foo) {
    if (startError()) {
      causedBy(W);
      cerr() << "Measure " << msr->Name();
      cerr() << " within model " << m->Name();
      cerr() << " is an array";
      stopIO();
    }
    TrashPass(p, np);
    return makeError();
  }

  m->PromoteParams(p, np);
  return new md_call(W, m, p, np, slot);
}


expr* exprman::makeMeasureCall(const location &W, model_def* m,
      expr** p, int np, const char* msr_name,
      expr** indexes, int ni) const
{
  if (0==m || 0==msr_name) {
    TrashPass(p, np);
    TrashPass(indexes, ni);
    return 0;
  }

  int slot = m->FindVisible(msr_name);
  if (slot < 0) {
    if (startError()) {
      causedBy(W);
      cerr() << "Measure " << msr_name;
      cerr() << " does not exist in model " << m->Name();
      stopIO();
    }
    TrashPass(p, np);
    TrashPass(indexes, ni);
    return makeError();
  }

  const symbol* foo = m->GetSymbol(slot);
  DCASSERT(foo);
  const array* msr = dynamic_cast <const array*> (foo);
  if (!msr) {
    if (startError()) {
      causedBy(W);
      cerr() << "Measure " << foo->Name();
      cerr() << " within model " << m->Name();
      cerr() << " is not an array";
      stopIO();
    }
    TrashPass(p, np);
    TrashPass(indexes, ni);
    return makeError();
  }

  if (!msr->checkArrayCall(W, indexes, ni)) {
    TrashPass(p, np);
    TrashPass(indexes, ni);
    return makeError();
  }

  m->PromoteParams(p, np);
  return new md_acall(W, m, p, np, slot, indexes, ni);
}

// ******************************************************************
// *                                                                *
// *                           Front  end                           *
// *                                                                *
// ******************************************************************

void InitModelDefs(exprman* em)
{
    if (0==em) return;
    model_def::not_our_var.initialize(em->OptMan(), "model_var_owner",
        "For mismatches in model variable ownership"
    );
}

