#include "mod_vars.h"
#include "mod_def.h"
#include "../Streams/streams.h"
#include "exprman.h"
#include "arrays.h"
#include "measures.h"
#include "sets.h"
#include "formalism.h"
#include "unary.h"
#include "binary.h"
#include "trinary.h"
#include "dd_front.h"

#include "../include/heap.h"

#include <stdlib.h>  // for free()

// #define DEBUG_SUBSTATE
// #define DEBUG_BLEVLTB
// #define DEBUG_VUPDATE

// #define DEBUG_DDLIB
// #define DEBUG_BLEVOP
// #define DEBUG_BLEVLTBOP

// ******************************************************************
// *                                                                *
// *                       model_var  methods                       *
// *                                                                *
// ******************************************************************

model_var::model_var(const symbol* wrapper, const model_instance* p) :
		symbol(wrapper) {
	parent = p;
	SetSubstitution(false);
	state_type = Unknown;
}

model_var::model_var(const location &W, const type* t, char* n,
		const model_instance* p) :
		symbol(W, t, n) {
	parent = p;
	SetSubstitution(false);
	state_type = Unknown;
}

model_var::~model_var() {
}

void model_var::Compute(traverse_data &x) {
	DCASSERT(x.answer);DCASSERT(0==x.aggregate);
	x.answer->setPtr(Share(this));
}

void model_var::ComputeInState(traverse_data &x) const {
	DCASSERT(x.answer);
	x.answer->setNull();
}

void model_var::AddToState(traverse_data &x, long delta) const {
	DCASSERT(x.answer);
	x.answer->setNull();
}

void model_var::SetNextState(traverse_data &x, shared_state* ns,
		long rhs) const {
	DCASSERT(x.answer);
	x.answer->setNull();
}

void model_var::SetNextUnknown(traverse_data &x, shared_state* ns) const {
	DCASSERT(x.answer);
	x.answer->setNull();
}

// ******************************************************************
// *                                                                *
// *                     model_statevar methods                     *
// *                                                                *
// ******************************************************************

result model_statevar::tempresult;

model_statevar::model_statevar(const symbol* wrapper, const model_instance* p,
		shared_object* bnds) :
		model_var(wrapper, p) {
	Init(bnds);
}

model_statevar::model_statevar(const location &W, const type* t, char* n,
		const model_instance* p, shared_object* bnds) :
		model_var(W, t, n, p) {
	Init(bnds);
}

void model_statevar::Init(shared_object* bnds) {
	state_index = 0; // -1
	part_index = 0;
	if (bnds) {
		shared_set* ss = smart_cast<shared_set*>(bnds);
		DCASSERT(ss);
		bounds = Share(ss);
	} else {
		bounds = 0;
	}
	setStateType(Integer);
}

model_statevar::~model_statevar() {
	Delete(bounds);
}

bool model_statevar::HasBounds() const {
	return (bounds != 0);
}

long model_statevar::NumPossibleValues() const {
	return (bounds) ? bounds->Size() : -1;
}

void model_statevar::SetToValueNumber(long) {
	DCASSERT(0);
}

void model_statevar::GetValueNumber(long i, result& foo) const {
	if (bounds)
		bounds->GetElement(i, foo);
	else
		foo.setNull();
}

long model_statevar::GetValueIndex(result& foo) const {
	if (bounds)
		return bounds->IndexOf(foo);
	else
		return -1;
}

void model_statevar::ComputeInState(traverse_data &x) const {
	DCASSERT(x.answer);DCASSERT(x.current_state);
	const hldsm* hm = x.current_state->Parent();
	DCASSERT(hm);
	if (hm->GetParent() != getParent()) {
		ownerError(x);
	} else {
		if (x.current_state->unknown(GetIndex())) {
			x.answer->setUnknown();
		} else {
			x.answer->setInt(x.current_state->get(GetIndex()));
		}
	}
}

void model_statevar::AddToState(traverse_data &x, long delta) const {
	DCASSERT(x.answer);DCASSERT(x.current_state);DCASSERT(x.next_state);

	if (x.current_state->unknown(GetIndex())) {
		x.next_state->set_unknown(GetIndex());
		return;
	}

	long newst = x.current_state->get(GetIndex()) + delta;
	if (x.current_state->omega(GetIndex())) {
//		printf("NEWST set to OMEGA in next state %d", GetIndex());
		x.next_state->set_omega(GetIndex());
		newst = OOmega; // -10 for omega reserved.
	}
	if (bounds) {
		tempresult.setInt(newst);
		if (bounds->IndexOf(tempresult) < 0)
			boundsError(x, newst);
	} else {
		if (((newst < 0) && (!x.current_state->omega(GetIndex())))
				|| ((x.current_state->omega(GetIndex())) && (newst != OOmega)))

			//if (newst < 0)
			boundsError(x, newst);
	}

	x.next_state->set(GetIndex(), newst);
}

void model_statevar::SetNextState(traverse_data &x, shared_state* ns,
		long rhs) const {
	DCASSERT(x.answer);DCASSERT(x.current_state);

	if (bounds) {
		tempresult.setInt(rhs);
		if (bounds->IndexOf(tempresult) < 0)
			boundsError(x, rhs);
	} else {
		if (rhs < 0)
			boundsError(x, rhs);
	}

	DCASSERT(ns);
	ns->set(GetIndex(), rhs);
}

void model_statevar::SetNextUnknown(traverse_data &x, shared_state* ns) const {
	DCASSERT(x.answer);DCASSERT(x.current_state);DCASSERT(ns);
	ns->set_unknown(GetIndex());
}

void model_statevar::ownerError(traverse_data &x) const {
	DCASSERT(x.current_state);
	const hldsm* hm = x.current_state->Parent();
	DCASSERT(hm);DCASSERT(hm->GetParent() != getParent());
	if (hm->StartError(x.parent)) {
		em->cerr() << "state variable " << Name()
				<< " belongs to another model";
		hm->DoneError();
	}DCASSERT(x.answer);
	x.answer->setNull();
}

void model_statevar::printBoundsError(const result &x) const {
	em->cerr() << "state variable " << Name() << " assigned value ";
	em->cerr() << x.getInt() << ",";
	em->newLine();
	em->cerr() << "which falls out of bounds ";
	bounds->Print(em->cerr(), 0);
}

// ******************************************************************
// *                                                                *
// *                    model_enum_value methods                    *
// *                                                                *
// ******************************************************************

model_enum_value::model_enum_value(const symbol* wrapper,
		const model_instance* p, int ndx) :
		model_var(wrapper, p) {
	index = ndx;
	setStateType(No_State);
}

model_enum_value::~model_enum_value() {
}

// ******************************************************************
// *                                                                *
// *                       model_enum methods                       *
// *                                                                *
// ******************************************************************

long* model_enum::indexes = 0;

model_enum::model_enum(const symbol* w, const model_instance* p, symbol* list) :
		model_statevar(w, p, 0) {
	// get size of list
	num_values = 0;
	symbol* ptr;
	for (ptr = list; ptr; ptr = ptr->Next()) {
		num_values++;
	}

	// convert the list to the values array.
	if (num_values)
		values = new model_enum_value*[num_values];
	else
		values = 0;
	for (int i = 0; i < num_values; i++)
		values[i] = 0;
	for (ptr = list; ptr; ptr = ptr->Next()) {
		model_enum_value* data = dynamic_cast<model_enum_value*>(ptr);
		DCASSERT(data);
		int i = data->GetIndex();
		CHECK_RANGE(0, i, num_values);DCASSERT(0==values[i]);
		values[i] = data;
	}
}

model_enum::~model_enum() {
	for (int i = 0; i < num_values; i++)
		Delete(values[i]);
	delete[] values;
}

void model_enum::MakeSortedMap(long* I) const {
	indexes = I;
	for (long i = 0; i < num_values; i++)
		I[i] = i;
	HeapSortAbstract(this, num_values);
	indexes = 0;
}

// ******************************************************************
// *                                                                *
// *                      int_statevar methods                      *
// *                                                                *
// ******************************************************************

int_statevar::int_statevar(const symbol* wrapper, const model_instance* p,
		shared_object* b) :
		model_statevar(wrapper, p, b) {
	DCASSERT(wrapper);DCASSERT(wrapper->Type() == em->INT);
}

void int_statevar::Compute(traverse_data &x) {
	DCASSERT(x.answer);DCASSERT(0==x.aggregate);
	x.answer->setInt(value);
}

void int_statevar::SetToValueNumber(long i) {
	if (!bounds)
		return;
	result foo;
	bounds->GetElement(i, foo);
	value = foo.getInt();
}

// ******************************************************************
// *                                                                *
// *                     bool_statevar  methods                     *
// *                                                                *
// ******************************************************************

bool_statevar::bool_statevar(const symbol* wrapper, const model_instance* p) :
		model_statevar(wrapper, p, 0) {
	DCASSERT(wrapper);DCASSERT(wrapper->Type() == em->BOOL);
}

void bool_statevar::Compute(traverse_data &x) {
	DCASSERT(x.answer);DCASSERT(0==x.aggregate);
	x.answer->setBool(value);
}

bool bool_statevar::HasBounds() const {
	return true;
}

long bool_statevar::NumPossibleValues() const {
	return 2;
}

void bool_statevar::SetToValueNumber(long i) {
	value = (i > 0);
}

void bool_statevar::GetValueNumber(long i, result& foo) const {
	foo.setBool(i > 0);
}

long bool_statevar::GetValueIndex(result& foo) const {
	if (foo.isNormal())
		return (foo.getBool()) ? 1 : 0;
	else
		return -1;
}

// ******************************************************************
// *                                                                *
// *                      shared_state methods                      *
// *                                                                *
// ******************************************************************

shared_state::shared_state(const hldsm* p) {
	DCASSERT(p);
	// DCASSERT(hldsm::Fixed_Ints == p->StateType());
	parent = p;
	num_buckets = p->NumStateVars();
	is_unknown = 0;

	if (p->hasPartInfo()) {
		const hldsm::partinfo &part = p->getPartInfo();
		num_substates = part.num_levels;
		substate_offset = new int[1 + num_substates];
		for (int i = 0; i <= num_substates; i++)
			substate_offset[i] = 0;
	} else {
		substate_offset = 0;
		num_substates = 0;
	}

	// Check: are we fixed size?
	if (p->containsListVar()) {
		is_list = (bool*) malloc(num_buckets * sizeof(bool));
		p->determineListVars(is_list);

		// NOT IMPLEMENTED YET
		DCASSERT(0);

	} else {
		// Fixed Size: everything is easier
		is_list = 0;
		bucket_ptr = 0;
		next = 0;
		data_alloc = num_buckets;
		data_size = num_buckets;
		if (data_alloc) {
			data = (int*) malloc(data_alloc * sizeof(int));
			DCASSERT(data);
		} else
			data = 0;
		// set substate pointers (they're fixed!)
		if (substate_offset) {
			const hldsm::partinfo &part = p->getPartInfo();
			for (int i = 1; i <= num_substates; i++) {
				int p = part.pointer[i];
				int d = part.variable[p]->GetIndex();
				substate_offset[i] = d;
			}
			substate_offset[0] = data_size;
#ifdef DEBUG_SUBSTATE
			printf("substate offsets: [%d", substate_offset[0]);
			for (int i=1; i<=num_substates; i++) printf(", %d", substate_offset[i]);
			printf("]\n");
#endif
		}
	}

	// printf("Created shared_state\n");
}

shared_state::~shared_state() {
	free(is_list);
	free(bucket_ptr);
	free(data);
	free(next);
	delete[] substate_offset;
	delete[] is_unknown;
	// printf("Destroyed shared_state\n");
}

void shared_state::fillFrom(const shared_state &s) {
	DCASSERT(parent == s.parent);DCASSERT(isFixedSize() == s.isFixedSize());DCASSERT(getNumStateVars() == s.getNumStateVars());

	// Does s have any unknown values?
	bool has_unknown = false;
	if (s.is_unknown) {
		for (int b = num_buckets; b;) {
			if (s.is_unknown[--b]) {
				has_unknown = true;
				break;
			}
		}
	}

	// Set our unknown values appropriately.
	if (has_unknown) {
		if (!is_unknown)
			is_unknown = new bool[num_buckets];
		memcpy(is_unknown, s.is_unknown, num_buckets * sizeof(bool));
	} else {
		if (is_unknown)
			clear_unknown();

		// Copy the data.
		if (isFixedSize()) {
			memcpy(data, s.data, data_size * sizeof(int));
		} else {
			DCASSERT(0);
		}
		if (s.is_omega) {
			this->Unset_omega();
			for (int b = 0; b < num_buckets; b++) {
				if (s.is_omega[b]) {
//					printf("\nXXX SET OMEGA IN%d\n", b);
					this->set_omega(b);
				}else{
//					printf("\nXXX SET %d TO%d\n",b,s.data[b]);
					this->data[b]=s.data[b];
				}
			}
		}

	}
}

bool shared_state::Print(OutputStream &s, int) const {
	if (0 == parent)
		return false;
	parent->showState(s, this);
	return true;
}

bool shared_state::Equals(const shared_object* o) const {
	DCASSERT(0);
	return false;
}

//
// HIDDEN STUFF
//

// ******************************************************************
// *                                                                *
// *                       model_symbol class                       *
// *                                                                *
// ******************************************************************

/**
 Variables within a model definition.
 These are simply placeholders until the corresponding
 model_var or measure has been created in the model instance.
 */
class model_symbol: public symbol {
	symbol* link;
public:
	model_symbol(const location &W, const type* t, char* n);
	virtual ~model_symbol();

	inline void SetLink(symbol *s) {
		DCASSERT(0==link);
		link = s;
	}

	inline void ClearLink() {
		link = 0;
	}

	virtual void Compute(traverse_data &x);
	virtual void Traverse(traverse_data &x);
};

model_symbol::model_symbol(const location &W, const type* t, char *n) :
		symbol(W, t, n) {
	link = 0;
	SetSubstitution(true);
}

model_symbol::~model_symbol() {
}

void model_symbol::Compute(traverse_data &x) {
	SafeCompute(link, x);
}

void model_symbol::Traverse(traverse_data &x) {
	if (link)
		link->Traverse(x);
}

// ******************************************************************
// *                                                                *
// *                      model_var_stmt class                      *
// *                                                                *
// ******************************************************************

class model_var_stmt: public expr {
protected:
	model_def* parent;
	expr* bounds;
	model_symbol** names;
	int numvars;
public:
	model_var_stmt(const location &W, model_def *p, const type* t,
			expr* bnds, model_symbol** n, int nv);

	virtual ~model_var_stmt();

	virtual bool Print(OutputStream &s, int) const;
	virtual void Compute(traverse_data &x);
	virtual void Traverse(traverse_data &x);
};

model_var_stmt::model_var_stmt(const location &W, model_def *p,
		const type* t, expr* bnds, model_symbol** n, int nv) :
		expr(W, em->VOID) {
	parent = p;
	numvars = nv;
	names = n;
	for (int i = 0; i < numvars; i++) {
		names[i]->SetType(t);
	}
	bounds = bnds;
}

model_var_stmt::~model_var_stmt() {
	for (int i = 0; i < numvars; i++) {
		Delete(names[i]);
	}
	delete[] names;
	Delete(bounds);
}

bool model_var_stmt::Print(OutputStream &s, int w) const {
	s.Pad(' ', w);
	DCASSERT(names[0]);
	const type* t = names[0]->Type();
	DCASSERT(t);
	s << t->getName() << " ";
	for (int i = 0; i < numvars; i++) {
		if (i)
			s << ", ";
		s << names[i]->Name();
	}
	if (bounds) {
		s << " in ";
		bounds->Print(s, 0);
	}
	s << ";\n";
	return true;
}

void model_var_stmt::Compute(traverse_data &x) {
	DCASSERT(x.which == traverse_data::Compute);DCASSERT(x.answer);
	if (x.stopExecution())
		return;
	result bset;
	result* old = x.answer;
	x.answer = &bset;
	SafeCompute(bounds, x);
	x.answer = old;
	for (int i = 0; i < numvars; i++) {
		model_var* z = parent->MakeModelVar(names[i], bset.getPtr());
		names[i]->SetLink(z);
		if (model_debug.startReport()) {
			model_debug.report() << "model " << parent->Name();
			model_debug.report() << " built symbol " << z->Name() << "\n";
			model_debug.stopIO();
		}
	}
}

void model_var_stmt::Traverse(traverse_data &x) {
	if (traverse_data::ModelDone == x.which) {
		for (int i = 0; i < numvars; i++)
			names[i]->ClearLink();
	} else {
		expr::Traverse(x);
	}
}

// ******************************************************************
// *                                                                *
// *                       model_array  class                       *
// *                                                                *
// ******************************************************************

/**
 Array variables within a model definition.
 These are simply placeholders until the corresponding
 array has been created in the model instance.
 */
class model_array: public array {
	array* link;
public:
	model_array(const location &W, const type* t, char* n, iterator** il,
			int dim);
	virtual ~model_array();

	inline array* MakeLink(model_def* parent, int slot) {
		if (0 == link) {
			link = instantiateMe();
			parent->AcceptSymbolOwnership(link);
			if (slot >= 0)
				parent->AcceptExternalSymbol(slot, link);
		}
		return link;
	}

	inline void ClearLink() {
		link = NULL;
	}

	virtual array_item* GetItem(expr** indices, result &x);
	virtual void Traverse(traverse_data &x);
};

model_array::model_array(const location &W, const type* t, char *n,
		iterator** il, int dim) :
		array(W, t, n, il, dim) {
	link = 0;
}

model_array::~model_array() {
}

array_item* model_array::GetItem(expr** indices, result &x) {
	if (link)
		return link->GetItem(indices, x);
	x.setNull();
	return 0;
}

void model_array::Traverse(traverse_data &x) {
	if (link)
		link->Traverse(x);
}

// ******************************************************************
// *                                                                *
// *                    model_varray_stmt  class                    *
// *                                                                *
// ******************************************************************

class model_varray_stmt: public expr {
protected:
	model_def* parent;
	model_array** vars;
	int numvars;
public:
	model_varray_stmt(const location &W, model_def *p, const type* t,
			model_array** a, int nv);

	virtual ~model_varray_stmt();

	virtual bool Print(OutputStream &s, int) const;
	virtual void Compute(traverse_data &x);
	virtual void Traverse(traverse_data &x);
};

model_varray_stmt::model_varray_stmt(const location &W, model_def *p,
		const type* t, model_array** a, int nv) :
		expr(W, em->VOID) {
	parent = p;
	numvars = nv;
	DCASSERT(numvars);
	vars = a;
	for (int i = 0; i < numvars; i++) {
		vars[i]->SetType(t);
	}
}

model_varray_stmt::~model_varray_stmt() {
	for (int i = 0; i < numvars; i++) {
		Delete(vars[i]);
	}
	delete[] vars;
}

bool model_varray_stmt::Print(OutputStream &s, int w) const {
	s.Pad(' ', w);
	DCASSERT(vars[0]);
	const type* t = vars[0]->Type();
	DCASSERT(t);
	s << t->getName() << " ";
	for (int i = 0; i < numvars; i++) {
		if (i)
			s << ", ";
		vars[i]->PrintHeader(s);
	}
	s << ";\n";
	return true;
}

void model_varray_stmt::Compute(traverse_data &x) {
	DCASSERT(x.which == traverse_data::Compute);DCASSERT(x.answer);
	if (x.stopExecution())
		return;
	for (int i = 0; i < numvars; i++) {
		array* inst = vars[i]->MakeLink(parent, -1);
		model_var* z = parent->MakeModelVar(inst, 0);
		inst->SetCurrentReturn(z, true);
		if (model_debug.startReport()) {
			model_debug.report() << "model " << parent->Name();
			model_debug.report() << " built symbol " << z->Name() << "\n";
			model_debug.stopIO();
		}
	} // for i
}

void model_varray_stmt::Traverse(traverse_data &x) {
	if (traverse_data::ModelDone == x.which) {
		for (int i = 0; i < numvars; i++)
			vars[i]->ClearLink();
	} else {
		expr::Traverse(x);
	}
}

// ******************************************************************
// *                                                                *
// *                      measure_assign class                      *
// *                                                                *
// ******************************************************************

/**  A statement used for measure assignments.
 Necessary to instantiate the measure within each instantiation
 of the model.
 */
class measure_assign: public expr {
	model_def *parent;
	model_symbol* wrapper;
	int msr_slot;
	expr* retval;
public:
	measure_assign(const location &W, model_def *p, model_symbol* w,
			expr* rhs);
	virtual ~measure_assign();

	virtual bool Print(OutputStream &s, int) const;
	virtual void Compute(traverse_data &x);
	virtual void Traverse(traverse_data &x);
};

measure_assign::measure_assign(const location &W, model_def *p,
		model_symbol* w, expr* rhs) :
		expr(W, em->VOID) {
	parent = p;
	wrapper = w;
	msr_slot = -1; // can't get the slot yet, model still being constructed
	retval = rhs;
}

measure_assign::~measure_assign() {
	Delete(retval);
}

bool measure_assign::Print(OutputStream &s, int w) const {
	s.Pad(' ', w);
	wrapper->PrintType(s);
	s.Put(' ');
	s << wrapper->Name() << " := ";
	if (retval)
		retval->Print(s, 0);
	else
		s << "null";
	s << ";\n";
	return true;
}

void measure_assign::Compute(traverse_data &x) {
	DCASSERT(x.which == traverse_data::Compute);DCASSERT(x.answer);DCASSERT(parent);
	if (x.stopExecution())
		return;
	if (msr_slot < 0) {
		msr_slot = parent->FindVisible(wrapper->Name());
		if (msr_slot < 0) {
			if (em->startInternal(__FILE__, __LINE__)) {
				em->causedBy(this);
				em->internal() << "Couldn't find slot for measure "
						<< wrapper->Name();
				if (parent->Name())
					em->internal() << " in model " << parent->Name();
				em->stopIO();
			}
			return;
		}
	}
	expr* rv = 0;
	List<symbol> *dl = 0;
	if (retval) {
		rv = retval->Measurify(parent);
		dl = new List<symbol>;
		rv->BuildSymbolList(traverse_data::GetMeasures, 0, dl);
		if (0 == dl->Length()) {
			delete dl;
			dl = 0;
		}
	}
	symbol* m = em->makeConstant(wrapper, rv, dl);

	parent->AcceptExternalSymbol(msr_slot, m);
	wrapper->SetLink(m);
	if (model_debug.startReport()) {
		model_debug.report() << "Model " << parent->Name();
		model_debug.report() << " built measure " << m->Name() << "\n";
		model_debug.stopIO();
	}

}

void measure_assign::Traverse(traverse_data &x) {
	if (traverse_data::ModelDone == x.which) {
		wrapper->ClearLink();
	} else {
		expr::Traverse(x);
	}
}

// ******************************************************************
// *                                                                *
// *                   measure_array_assign class                   *
// *                                                                *
// ******************************************************************

/**  A statement used for measure array assignments.
 */
class measure_array_assign: public expr {
	model_def *parent;
	model_array* wrapper;
	int msr_slot;
	expr *retval;
public:
	measure_array_assign(const location &W, model_def* p, model_array* w,
			expr *e);
	virtual ~measure_array_assign();

	virtual bool Print(OutputStream &s, int) const;
	virtual void Compute(traverse_data &x);
	virtual void Traverse(traverse_data &x);
};

measure_array_assign::measure_array_assign(const location &W, model_def *p,
		model_array* w, expr *e) :
		expr(W, em->VOID) {
	parent = p;
	wrapper = w;
	msr_slot = -1;
	retval = e;
}

measure_array_assign::~measure_array_assign() {
	Delete(retval);
}

bool measure_array_assign::Print(OutputStream &s, int w) const {
	s.Pad(' ', w);
	wrapper->PrintType(s);
	s.Put(' ');
	wrapper->PrintHeader(s);
	s << " := ";
	if (retval)
		retval->Print(s, 0);
	else
		s << "null";
	s << ";\n";
	return true;
}

void measure_array_assign::Compute(traverse_data &x) {
	DCASSERT(x.which == traverse_data::Compute);DCASSERT(x.answer);
	if (x.stopExecution())
		return;
	if (msr_slot < 0) {
		msr_slot = parent->FindVisible(wrapper->Name());
		if (msr_slot < 0) {
			if (em->startInternal(__FILE__, __LINE__)) {
				em->causedBy(this);
				em->internal() << "Couldn't find slot for measure "
						<< wrapper->Name();
				em->internal() << " in model " << parent->Name();
				em->stopIO();
			}
			return;
		}
	}

	array* inst = wrapper->MakeLink(parent, msr_slot);
	symbol* m = 0;
	if (retval) {
		List<symbol> *dl = 0;
		expr* rv = retval->Measurify(parent);
		dl = new List<symbol>;
		rv->BuildSymbolList(traverse_data::GetMeasures, 0, dl);
		if (0 == dl->Length()) {
			delete dl;
			dl = 0;
		}
		m = em->makeConstant(wrapper, rv, dl);
	}
	inst->SetCurrentReturn(m, true);
	if (model_debug.startReport()) {
		model_debug.report() << "Model " << parent->Name();
		model_debug.report() << " built measure " << m->Name() << "\n";
		model_debug.stopIO();
	}
}

void measure_array_assign::Traverse(traverse_data &x) {
	if (traverse_data::ModelDone == x.which) {
		wrapper->ClearLink();
	} else {
		expr::Traverse(x);
	}
}

// ******************************************************************
// *                        clev_op  methods                        *
// ******************************************************************

clev_op::clev_op(const location &W, expr* b, model_var* v) :
		unary(W, em->BOOL->addProc(), v) {
	DCASSERT(b);DCASSERT(0==b->BuildExprList(traverse_data::GetSymbols, 0, 0));
	traverse_data x(traverse_data::Compute);
	result foo;
	x.answer = &foo;
	b->PreCompute();
	b->Compute(x);
	DCASSERT(foo.isNormal());
	lower = foo.getInt();
	Delete(b);
}

clev_op::clev_op(const location &W, long b, model_var* v) :
		unary(W, em->BOOL->addProc(), v) {
	lower = b;
}

long clev_op::getLower() const {
	return lower;
}

void clev_op::Compute(traverse_data &x) {
	DCASSERT(x.answer);DCASSERT(x.current_state);DCASSERT(0==x.aggregate);

	model_var* sv = smart_cast <model_var*>(opnd);
	DCASSERT(sv);
	sv->ComputeInState(x);

	if (x.answer->isOmega()) {
		x.answer->setBool(true);
		return;
	}
	if (x.answer->isUnknown())
		return;

	DCASSERT(x.answer->isNormal());
	x.answer->setBool(
			(lower <= x.answer->getInt()) || (x.answer->getInt() == OOmega));
}

void clev_op::Traverse(traverse_data &x) {
	DCASSERT(0==x.aggregate);
	if (x.which != traverse_data::BuildDD) {
		unary::Traverse(x);
		return;
	}DCASSERT(x.answer);DCASSERT(x.ddlib);
	symbol* sv = smart_cast <model_var*>(opnd);
	DCASSERT(sv);
#ifdef DEBUG_DDLIB
	fprintf(stderr, "building dd for (%ld <= %s)\n", lower, sv->Name());
#endif

	shared_object* ans = x.ddlib->makeEdge(0);
	DCASSERT(ans);
	x.ddlib->buildSymbolicSV(sv, false, this, ans);
	x.answer->setPtr(ans);
}

bool clev_op::Print(OutputStream &s, int w) const {
	DCASSERT(opnd);
	s << lower;
#ifdef DEBUG_BLEVLTB
	s.Put('L');
#endif
	s << "<=";
	opnd->Print(s, 0);
	return true;
}

expr* clev_op::buildAnother(expr *r) const {
	model_var* pl = smart_cast <model_var*>(r);
	DCASSERT(pl);
	return new clev_op(Where(), lower, pl);
}

// **************************************************************************
// *                                                                        *
// *                             blev_op  class                             *
// *                                                                        *
// **************************************************************************

/** Checks b <= v.
 b is an expression.
 v is a model state variable.
 */
class blev_op: public binary {
public:
	blev_op(const location &W, expr* b, model_var* v);
	virtual void Compute(traverse_data &x);
	virtual void Traverse(traverse_data &x);
protected:
	virtual expr* buildAnother(expr *l, expr *r) const;
};

// ******************************************************************
// *                        blev_op  methods                        *
// ******************************************************************

blev_op::blev_op(const location &W, expr* b, model_var* v) :
		binary(W, exprman::bop_le, em->BOOL->addProc(), b, v) {
}

void blev_op::Compute(traverse_data &x) {
//	printf("\nInside blev_op \n");
	DCASSERT(x.answer);DCASSERT(x.current_state);DCASSERT(0==x.aggregate);

	result* answer = x.answer;
	result r;
	x.answer = &r;
	model_var* sv = smart_cast <model_var*>(right);
	DCASSERT(sv);
	sv->ComputeInState(x);
	if (r.isOmega()) {
		x.answer->setBool(true);
		return;
	}
	if (r.isUnknown()) {
		x.answer->setUnknown();
		return;
	}DCASSERT(r.isNormal());

	x.answer = answer;
	DCASSERT(left);
	left->Compute(x);

	if (x.answer->isNormal()) {
		x.answer->setBool(x.answer->getInt() <= r.getInt());
		return;
	}
	if (x.answer->isInfinity()) {
		int sign = x.answer->signInfinity();
		x.answer->setBool(sign < 0);
		return;
	}
	// any other strangeness can just propogate through.
}

void blev_op::Traverse(traverse_data &x) {
	DCASSERT(0==x.aggregate);
	if (x.which != traverse_data::BuildDD) {
		binary::Traverse(x);
		return;
	}DCASSERT(x.answer);DCASSERT(x.ddlib);

	left->Traverse(x);
	if (!x.answer->isNormal())
		return;
	shared_object* ldd = Share(x.answer->getPtr());
	DCASSERT(ldd);

	symbol* sv = smart_cast <model_var*>(right);
	DCASSERT(sv);
	shared_object* ans = x.ddlib->makeEdge(0);
	DCASSERT(ans);
	x.ddlib->buildSymbolicSV(sv, false, 0, ans);
	// ans := ldd <= ans
#ifdef DEBUG_BLEVOP
	em->cout() << "Built DD for " << sv->Name() << ": ";
	x.ddlib->dumpNode(em->cout(), ans);
	em->cout() << "\n";
	left->Print(em->cout(), 0);
	em->cout() << ": ";
	x.ddlib->dumpNode(em->cout(), ldd);
	em->cout() << "\n";
	x.ddlib->dumpForest(em->cout());
#endif
	x.ddlib->buildBinary(ldd, exprman::bop_le, ans, ans);
	Delete(ldd);
	x.answer->setPtr(ans);
}

expr* blev_op::buildAnother(expr *l, expr *r) const {
	model_var* pl = smart_cast <model_var*>(r);
	DCASSERT(pl);
	return new blev_op(Where(), l, pl);
}

// **************************************************************************
// *                                                                        *
// *                             vltc_op  class                             *
// *                                                                        *
// **************************************************************************

/** Checks v < c.
 c is an integer constant.
 v is a model state variable.
 */
class vltc_op: public unary {
	long upper;
public:
	vltc_op(const location &W, model_var* v, expr* b);
	vltc_op(const location &W, model_var* v, long b);
	virtual void Compute(traverse_data &x);
	virtual void Traverse(traverse_data &x);
	virtual bool Print(OutputStream &s, int w) const;
	virtual long getUpper() const;
protected:
	virtual expr* buildAnother(expr *r) const;
};

// ******************************************************************
// *                        vltc_op  methods                        *
// ******************************************************************

vltc_op::vltc_op(const location &W, model_var* v, expr* b) :
		unary(W, em->BOOL->addProc(), v) {
	DCASSERT(b);DCASSERT(0==b->BuildExprList(traverse_data::GetSymbols, 0, 0));
	traverse_data x(traverse_data::Compute);
	result foo;
	x.answer = &foo;
	b->PreCompute();
	b->Compute(x);
	DCASSERT(foo.isNormal());
	upper = foo.getInt();
	Delete(b);
}

vltc_op::vltc_op(const location &W, model_var* v, long b) :
		unary(W, em->BOOL->addProc(), v) {
	upper = b;
}

void vltc_op::Compute(traverse_data &x) {
	//printf("\nInside vltc_op \n");
	DCASSERT(x.answer);DCASSERT(x.current_state);DCASSERT(0==x.aggregate);

	model_var* sv = smart_cast <model_var*>(opnd);
	DCASSERT(sv);
	sv->ComputeInState(x);
	if (x.answer->isOmega()) {
		x.answer->setBool(true);
		return;
	}
	if (x.answer->isUnknown())
		return;

	DCASSERT(x.answer->isNormal());

	x.answer->setBool(x.answer->getInt() < upper);
}

void vltc_op::Traverse(traverse_data &x) {
	DCASSERT(0==x.aggregate);
	if (x.which != traverse_data::BuildDD) {
		unary::Traverse(x);
		return;
	}DCASSERT(x.answer);DCASSERT(x.ddlib);
	symbol* sv = smart_cast <model_var*>(opnd);
	DCASSERT(sv);

	shared_object* ans = x.ddlib->makeEdge(0);
	DCASSERT(ans);
	x.ddlib->buildSymbolicSV(sv, false, this, ans);
	x.answer->setPtr(ans);
}

bool vltc_op::Print(OutputStream &s, int w) const {
	DCASSERT(opnd);
	opnd->Print(s, 0);
	s << "<";
	s << upper;
#ifdef DEBUG_BLEVLTB
	s.Put('L');
#endif
	return true;
}

long vltc_op::getUpper() const {
  return upper;
}

expr* vltc_op::buildAnother(expr *r) const {
	model_var* pl = smart_cast <model_var*>(r);
	DCASSERT(pl);
	return new vltc_op(Where(), pl, upper);
}

// **************************************************************************
// *                                                                        *
// *                             vltb_op  class                             *
// *                                                                        *
// **************************************************************************

/** Checks v < b.
 b is an expression.
 v is a model state variable.
 */
class vltb_op: public binary {
public:
	vltb_op(const location &W, model_var* v, expr* b);
	virtual void Compute(traverse_data &x);
	virtual void Traverse(traverse_data &x);
protected:
	virtual expr* buildAnother(expr *l, expr *r) const;
};

// ******************************************************************
// *                        vltb_op  methods                        *
// ******************************************************************

vltb_op::vltb_op(const location &W, model_var* v, expr* b) :
		binary(W, exprman::bop_lt, em->BOOL->addProc(), v, b) {
}

void vltb_op::Compute(traverse_data &x) {
//	printf("\nInside vltb_op \n");
	DCASSERT(x.answer);DCASSERT(x.current_state);DCASSERT(0==x.aggregate);

	result* answer = x.answer;
	result l;
	x.answer = &l;
	model_var* sv = smart_cast <model_var*>(left);
	DCASSERT(sv);
	sv->ComputeInState(x);
	if (l.isOmega()) {
		x.answer->setBool(true);
		return;
	}
	if (l.isUnknown()) {
		x.answer->setUnknown();
		return;
	}DCASSERT(l.isNormal());

	x.answer = answer;
	DCASSERT(right);
	right->Compute(x);

	if (x.answer->isNormal()) {
		x.answer->setBool(l.getInt() < x.answer->getInt());
		return;
	}
	if (x.answer->isInfinity()) {
		int sign = x.answer->signInfinity();
		x.answer->setBool(sign > 0);
		return;
	}
	// any other strangeness can just propogate through.
}

void vltb_op::Traverse(traverse_data &x) {
	DCASSERT(0==x.aggregate);
	if (x.which != traverse_data::BuildDD) {
		binary::Traverse(x);
		return;
	}DCASSERT(x.answer);DCASSERT(x.ddlib);

	right->Traverse(x);
	if (!x.answer->isNormal())
		return;
	shared_object* rdd = Share(x.answer->getPtr());
	DCASSERT(rdd);

	symbol* sv = smart_cast <model_var*>(left);
	DCASSERT(sv);
	shared_object* ans = x.ddlib->makeEdge(0);
	DCASSERT(ans);
	x.ddlib->buildSymbolicSV(sv, false, 0, ans);
	// ans := ans < rdd
	x.ddlib->buildBinary(ans, exprman::bop_lt, rdd, ans);
	Delete(rdd);
	x.answer->setPtr(ans);
}

expr* vltb_op::buildAnother(expr *l, expr *r) const {
	model_var* pl = smart_cast <model_var*>(l);
	DCASSERT(pl);
	return new vltb_op(Where(), pl, r);
}

// **************************************************************************
// *                                                                        *
// *                            clevltc_op class                            *
// *                                                                        *
// **************************************************************************

class clevltc_op: public unary {
	long lower;
	long upper;
public:
	clevltc_op(const location &W, expr* lb, model_var* v, expr* ub);
	clevltc_op(const location &W, long lb, model_var* v, long ub);
	virtual void Compute(traverse_data &x);
	virtual void Traverse(traverse_data &x);
	virtual bool Print(OutputStream &s, int w) const;
	virtual long getLower() const;
	virtual long getUpper() const;
protected:
	virtual expr* buildAnother(expr *r) const;
};

// ******************************************************************
// *                       clevltc_op methods                       *
// ******************************************************************

clevltc_op::clevltc_op(const location &W, expr* lb, model_var* v,
		expr* ub) :
		unary(W, em->BOOL->addProc(), v) {
	DCASSERT(lb);DCASSERT(ub);DCASSERT(0==lb->BuildExprList(traverse_data::GetSymbols, 0, 0));DCASSERT(0==ub->BuildExprList(traverse_data::GetSymbols, 0, 0));
	traverse_data x(traverse_data::Compute);
	result foo;
	x.answer = &foo;
	lb->PreCompute();
	lb->Compute(x);
	DCASSERT(foo.isNormal());
	lower = foo.getInt();
	Delete(lb);
	ub->PreCompute();
	ub->Compute(x);
	DCASSERT(foo.isNormal());
	upper = foo.getInt();
	Delete(ub);
}

clevltc_op::clevltc_op(const location &W, long lb, model_var* v, long ub) :
		unary(W, em->BOOL->addProc(), v) {
	lower = lb;
	upper = ub;
}

void clevltc_op::Compute(traverse_data &x) {
//	printf("\nInside clevltc_op\n");
	DCASSERT(x.answer);DCASSERT(x.current_state);DCASSERT(0==x.aggregate);

	model_var* sv = smart_cast <model_var*>(opnd);
	DCASSERT(sv);
	sv->ComputeInState(x);

	if (x.answer->isOmega()) {
		x.answer->setBool(true);
		return;
	}
	if (x.answer->isUnknown())
		return;

	DCASSERT(x.answer->isNormal());

	x.answer->setBool(
			(lower <= x.answer->getInt()) && (x.answer->getInt() < upper));
}

void clevltc_op::Traverse(traverse_data &x) {
	DCASSERT(0==x.aggregate);
	if (x.which != traverse_data::BuildDD) {
		unary::Traverse(x);
		return;
	}DCASSERT(x.answer);DCASSERT(x.ddlib);
	symbol* sv = smart_cast <model_var*>(opnd);
	DCASSERT(sv);

	shared_object* ans = x.ddlib->makeEdge(0);
	DCASSERT(ans);
	x.ddlib->buildSymbolicSV(sv, false, this, ans);
	x.answer->setPtr(ans);
}

bool clevltc_op::Print(OutputStream &s, int w) const {
	DCASSERT(opnd);
	s << lower;
#ifdef DEBUG_BLEVLTB
	s.Put('L');
#endif
	s << "<=";
	opnd->Print(s, 0);
	s << "<";
	s << upper;
#ifdef DEBUG_BLEVLTB
	s.Put('L');
#endif
	return true;
}

long clevltc_op::getLower() const {
  return lower;
}

long clevltc_op::getUpper() const {
  return upper;
}

expr* clevltc_op::buildAnother(expr *r) const {
	model_var* pl = smart_cast <model_var*>(r);
	DCASSERT(pl);
	return new clevltc_op(Where(), lower, pl, upper);
}

// **************************************************************************
// *                                                                        *
// *                            blevltb_op class                            *
// *                                                                        *
// **************************************************************************

/** Checks lb <= v < ub.
 lb is an expression.
 ub is an expression.
 v is a model state variable.
 */
class blevltb_op: public trinary {
public:
	blevltb_op(const location &W, expr* lb, model_var* v, expr* ub);
	virtual void Compute(traverse_data &x);
	virtual void Traverse(traverse_data &x);
	virtual bool Print(OutputStream &s, int) const;
protected:
	virtual expr* buildAnother(expr *l, expr* m, expr *r) const;
};

// ******************************************************************
// *                       blevltb_op methods                       *
// ******************************************************************

blevltb_op::blevltb_op(const location &W, expr* lb, model_var* v,
		expr* ub) :
		trinary(W, em->BOOL->addProc(), lb, v, ub) {
}

void blevltb_op::Compute(traverse_data &x) {
	DCASSERT(x.answer);DCASSERT(x.current_state);DCASSERT(0==x.aggregate);DCASSERT(left);DCASSERT(right);
	result* answer = x.answer;
	result l, m, r;
	x.answer = &l;
	left->Compute(x);
	x.answer = &r;
	right->Compute(x);
	model_var* sv = smart_cast <model_var*>(middle);
	DCASSERT(sv);
	x.answer = &m;
	sv->ComputeInState(x);
	if (l.isOmega()) {
		x.answer->setBool(true);
		return;
	}
	if (m.isUnknown()) {
		x.answer->setUnknown();
		return;
	}DCASSERT(m.isNormal());

	x.answer = answer;

	if (l.isNormal()) {
		if (l.getInt() > m.getInt()) {
			answer->setBool(false);
			return;
		}
		if (r.isNormal()) {
			answer->setBool(m.getInt() < r.getInt());
			return;
		}
		if (r.isInfinity()) {
			answer->setBool(r.signInfinity() > 0);
			return;
		}
		// r is strange, push the error through
		*answer = r;
	}

	if (r.isNormal()) {
		if (m.getInt() >= r.getInt()) {
			answer->setBool(false);
			return;
		}DCASSERT(!l.isNormal()); // already handled
		if (l.isInfinity()) {
			answer->setBool(l.signInfinity() < 0);
			return;
		}
		// l is strange, push the error through
		*answer = r;
	}

	// neither one is normal
	if (l.isNull() || r.isNull()) {
		answer->setNull();
	}

	// What cases are left?
	DCASSERT(l.isUnknown());DCASSERT(r.isUnknown());
	answer->setUnknown();
}

void blevltb_op::Traverse(traverse_data &x) {
	DCASSERT(0==x.aggregate);
	if (x.which != traverse_data::BuildDD) {
		trinary::Traverse(x);
		return;
	}DCASSERT(x.answer);DCASSERT(x.ddlib);

	left->Traverse(x);
	if (!x.answer->isNormal())
		return;
	shared_object* ldd = Share(x.answer->getPtr());

	right->Traverse(x);
	if (!x.answer->isNormal()) {
		Delete(ldd);
		return;
	}
	shared_object* udd = Share(x.answer->getPtr());
	x.answer->setNull();

	symbol* sv = smart_cast <model_var*>(middle);
	DCASSERT(sv);
	shared_object* vdd = x.ddlib->makeEdge(0);
	x.ddlib->buildSymbolicSV(sv, false, 0, vdd);

#ifdef DEBUG_BLEVLTBOP
	em->cout() << "Built DD for " << sv->Name() << ": ";
	x.ddlib->dumpNode(em->cout(), vdd);
	em->cout() << "\n";
	left->Print(em->cout(), 0);
	em->cout() << ": ";
	x.ddlib->dumpNode(em->cout(), ldd);
	em->cout() << "\n";
	right->Print(em->cout(), 0);
	em->cout() << ": ";
	x.ddlib->dumpNode(em->cout(), udd);
	em->cout() << "\n";
	x.ddlib->dumpForest(em->cout());
#endif

	// ldd := ldd <= vdd
	x.ddlib->buildBinary(ldd, exprman::bop_le, vdd, ldd);
	// udd := vdd < udd
	x.ddlib->buildBinary(vdd, exprman::bop_lt, udd, udd);
	// vdd := ldd & udd
	x.ddlib->buildAssoc(ldd, false, exprman::aop_and, udd, vdd);
	Delete(ldd);
	Delete(udd);
	x.answer->setPtr(vdd);
}

bool blevltb_op::Print(OutputStream &s, int) const {
	s << "(";
	left->Print(s, 0);
	s << " <= ";
	model_var* pl = smart_cast <model_var*>(middle);
	DCASSERT(pl);
	s << pl->Name();
	s << " < ";
	right->Print(s, 0);
	s << ")";
	return true;
}

expr* blevltb_op::buildAnother(expr *l, expr* m, expr *r) const {
	model_var* pl = smart_cast <model_var*>(m);
	DCASSERT(pl);
	return new blevltb_op(Where(), l, pl, r);
}

// ******************************************************************
// *                       cupdate_op methods                       *
// ******************************************************************

cupdate_op::cupdate_op(const location &W, model_var* v, long d) :
		expr(W, em->NEXT_STATE) {
	var = v;
	DCASSERT(var);
	delta = d;
}

cupdate_op::~cupdate_op() {
	Delete(var);
}

long cupdate_op::getDelta() const {
	return delta;
}

void cupdate_op::Traverse(traverse_data &x) {
	switch (x.which) {
	case traverse_data::PreCompute:
	case traverse_data::GetVarDeps:
	case traverse_data::GetSymbols:
		var->Traverse(x);
		return;

	case traverse_data::BuildDD: {
#ifdef DEBUG_DDLIB
		fprintf(stderr, "building dd for %s += %ld\n", var->Name(), delta);
#endif
		DCASSERT(x.answer);DCASSERT(x.ddlib);

		// v := var + delta
		shared_object* v = x.ddlib->makeEdge(0);
		x.ddlib->buildSymbolicSV(var, false, this, v);

		shared_object* vv = x.ddlib->makeEdge(0);
		x.ddlib->buildSymbolicSV(var, true, 0, vv);

		// vv := vv == v
		x.ddlib->buildBinary(vv, exprman::bop_equals, v, vv);
		Delete(v);
		x.answer->setPtr(vv);
		return;
	}

	default:
		expr::Traverse(x);
	}
}

void cupdate_op::Compute(traverse_data &x) {
	//printf("\nInside cupdate_op\n");
	DCASSERT(x.answer);DCASSERT(x.current_state);DCASSERT(0==x.aggregate);
	if (x.next_state) {
		// ordinary behavior
		var->AddToState(x, delta);
	} else {
		// hack... for buildDD
		var->ComputeInState(x);
		DCASSERT(x.answer->isNormal());
		x.answer->setInt(x.answer->getInt() + delta);
	}
}

bool cupdate_op::Print(OutputStream &s, int) const {
	s << var->Name() << "' := " << var->Name();
	if (delta > 0) {
		s << " + " << delta;
#ifdef DEBUG_VUPDATE
		s.Put('L');
#endif
	}
	if (delta < 0) {
		s << " - " << -delta;
#ifdef DEBUG_VUPDATE
		s.Put('L');
#endif
	}
	return true;
}

// **************************************************************************
// *                                                                        *
// *                            vupdate_op class                            *
// *                                                                        *
// **************************************************************************

/** Performs v := v + inc - dec;
 v is a state variable.
 inc is an expression.
 dec is an expression.
 */
class vupdate_op: public expr {
	model_var* var;
	expr* dec_amount;
	expr* inc_amount;
public:
	vupdate_op(const location &W, model_var* v, expr* dec, expr* inc);
	virtual ~vupdate_op();
	virtual void Traverse(traverse_data &x);
	virtual void Compute(traverse_data &x);
	virtual bool Print(OutputStream &s, int) const;
};

// ******************************************************************
// *                       vupdate_op methods                       *
// ******************************************************************

vupdate_op::vupdate_op(const location &W, model_var* v, expr* dec,
		expr* inc) :
		expr(W, em->NEXT_STATE) {
	var = v;
	dec_amount = dec;
	inc_amount = inc;
	DCASSERT(var);
}

vupdate_op::~vupdate_op() {
	Delete(var);
	Delete(dec_amount);
	Delete(inc_amount);
}

void vupdate_op::Traverse(traverse_data &x) {
	switch (x.which) {
	case traverse_data::PreCompute:
	case traverse_data::GetVarDeps:
	case traverse_data::GetSymbols:
		var->Traverse(x);
		if (dec_amount)
			dec_amount->Traverse(x);
		if (inc_amount)
			inc_amount->Traverse(x);
		return;

	case traverse_data::BuildDD: {
		DCASSERT(x.answer);DCASSERT(x.ddlib);
		// Build DD for inc
		shared_object* inc;
		if (inc_amount) {
			inc_amount->Traverse(x);
			if (!x.answer->isNormal())
				return;
			inc = Share(x.answer->getPtr());
			x.answer->setNull();
		} else {
			inc = x.ddlib->makeEdge(0);
			x.ddlib->buildSymbolicConst(0L, inc);
		}
		// Build DD for dec
		shared_object* dec;
		if (dec_amount) {
			dec_amount->Traverse(x);
			if (!x.answer->isNormal()) {
				Delete(inc);
				return;
			}
			dec = Share(x.answer->getPtr());
			x.answer->setNull();
		} else {
			dec = x.ddlib->makeEdge(0);
			x.ddlib->buildSymbolicConst(0L, dec);
		}
		// inc -= dec
		x.ddlib->buildAssoc(inc, true, exprman::aop_plus, dec, inc);
		Delete(dec);
		// Build var
		shared_object* v = x.ddlib->makeEdge(0);
		x.ddlib->buildSymbolicSV(var, false, 0, v);
		// var += inc
		x.ddlib->buildAssoc(v, false, exprman::aop_plus, inc, v);
		Delete(inc);
		// Build var'
		shared_object* vv = x.ddlib->makeEdge(0);
		x.ddlib->buildSymbolicSV(var, true, 0, vv);
		// Build var' := var' == var
		x.ddlib->buildBinary(vv, exprman::bop_equals, v, vv);
		Delete(v);
		x.answer->setPtr(vv);
		return;
	}

	default:
		expr::Traverse(x);
	}
}

void vupdate_op::Compute(traverse_data &x) {
//	printf("\nInside vupdate_op\n");
	DCASSERT(x.answer);DCASSERT(x.current_state);DCASSERT(x.next_state);DCASSERT(0==x.aggregate);
	long delta = 0;
	if (dec_amount) {
		dec_amount->Compute(x);
		if (!x.answer->isNormal())
			return;
		delta = -x.answer->getInt();
	}
	if (inc_amount) {
		inc_amount->Compute(x);
		if (!x.answer->isNormal())
			return;
		delta += x.answer->getInt();
	}

	var->AddToState(x, delta);
}

bool vupdate_op::Print(OutputStream &s, int) const {
	s << var->Name() << "' := " << var->Name();
	if (inc_amount) {
		s << " + ";
		inc_amount->Print(s, 0);
	}
	if (dec_amount) {
		s << " - ";
		dec_amount->Print(s, 0);
	}
	return true;
}

// **************************************************************************
// *                                                                        *
// *                            cassign_op class                            *
// *                                                                        *
// **************************************************************************

/** Performs v := c;
 v is a state variable.
 c is a constant.
 */
class cassign_op: public expr {
	model_var* var;
	long rhs;
public:
	cassign_op(const location &W, model_var* v, long c);
	virtual ~cassign_op();
	virtual void Traverse(traverse_data &x);
	virtual void Compute(traverse_data &x);
	virtual bool Print(OutputStream &s, int) const;
};

// ******************************************************************
// *                       cassign_op methods                       *
// ******************************************************************

cassign_op::cassign_op(const location &W, model_var* v, long d) :
		expr(W, em->NEXT_STATE) {
	var = v;
	DCASSERT(var);
	rhs = d;
}

cassign_op::~cassign_op() {
	Delete(var);
}

void cassign_op::Traverse(traverse_data &x) {
	switch (x.which) {
	case traverse_data::PreCompute:
	case traverse_data::GetVarDeps:
	case traverse_data::GetSymbols:
		var->Traverse(x);
		return;

	case traverse_data::BuildDD: {
		DCASSERT(x.answer);DCASSERT(x.ddlib);
		shared_object* d = x.ddlib->makeEdge(0);
		x.ddlib->buildSymbolicConst(rhs, d);
		shared_object* vv = x.ddlib->makeEdge(0);
		x.ddlib->buildSymbolicSV(var, true, 0, vv);
		// vv := (vv == d)
		x.ddlib->buildBinary(vv, exprman::bop_equals, d, vv);
		Delete(d);
		x.answer->setPtr(vv);
		return;
	}

	default:
		expr::Traverse(x);
	}
}

void cassign_op::Compute(traverse_data &x) {
//	printf("\nInside cassign_op\n");
	DCASSERT(x.answer);DCASSERT(x.current_state);DCASSERT(0==x.aggregate);
	var->SetNextState(x, x.next_state, rhs);
}

bool cassign_op::Print(OutputStream &s, int) const {
	s << var->Name() << "' := " << rhs;
	return true;
}

// **************************************************************************
// *                                                                        *
// *                            vassign_op class                            *
// *                                                                        *
// **************************************************************************

/** Performs v := rhs.
 v is a state variable.
 rhs is an expression.
 */
class vassign_op: public expr {
	model_var* var;
	expr* rhs;
public:
	vassign_op(const location &W, model_var* v, expr* c);
	virtual ~vassign_op();
	virtual void Traverse(traverse_data &x);
	virtual void Compute(traverse_data &x);
	virtual bool Print(OutputStream &s, int) const;
};

// ******************************************************************
// *                       vassign_op methods                       *
// ******************************************************************

vassign_op::vassign_op(const location &W, model_var* v, expr* d) :
		expr(W, em->NEXT_STATE) {
	var = v;
	DCASSERT(var);
	rhs = d;
}

vassign_op::~vassign_op() {
	Delete(var);
	Delete(rhs);
}

void vassign_op::Traverse(traverse_data &x) {
	switch (x.which) {
	case traverse_data::PreCompute:
	case traverse_data::GetVarDeps:
	case traverse_data::GetSymbols:
		var->Traverse(x);
		rhs->Traverse(x);
		return;

	case traverse_data::BuildDD: {
		DCASSERT(x.answer);DCASSERT(x.ddlib);DCASSERT(rhs);
		rhs->Traverse(x);
		shared_object* d = x.answer->getPtr();
		if (0 == d) {
			em->startError();
			em->causedBy(rhs);
			em->cerr() << "Couldn't build DD for ";
			rhs->Print(em->cerr(), 0);
			em->stopIO();
			x.answer->setNull();
			return;
		}
		shared_object* vv = x.ddlib->makeEdge(0);
		x.ddlib->buildSymbolicSV(var, true, 0, vv);
		// vv := (vv == d)
		x.ddlib->buildBinary(vv, exprman::bop_equals, d, vv);
		x.answer->setPtr(vv);
		return;
	}

	default:
		expr::Traverse(x);
	}
}

void vassign_op::Compute(traverse_data &x) {
//	printf("\nInside vassign_op\n");
	DCASSERT(x.answer);DCASSERT(x.current_state);DCASSERT(0==x.aggregate);DCASSERT(rhs);
	rhs->Compute(x);
	if (x.answer->isNormal()) {
		var->SetNextState(x, x.next_state, x.answer->getInt());
		return;
	}
	if (x.answer->isUnknown()) {
		var->SetNextUnknown(x, x.next_state);
		return;
	}
	// error ... propogate it
}

bool vassign_op::Print(OutputStream &s, int) const {
	s << var->Name() << "' := ";
	if (rhs)
		rhs->Print(s, 0);
	else
		s << "null";
	return true;
}

// ******************************************************************
// *                                                                *
// *                        exprman  methods                        *
// *                                                                *
// ******************************************************************

symbol* exprman::makeModelSymbol(const location &W, const type* t,
		char* name) const {
	return new model_symbol(W, t, name);
}

symbol* exprman::makeModelArray(const location &W, const type* t, char* n,
		symbol** indexes, int dim) const {
	if (0 == indexes) {
		free(n);
		return 0;
	}
	// Check indexes
	for (int i = 0; i < dim; i++) {
		iterator* it = dynamic_cast<iterator*>(indexes[i]);
		if (it)
			continue;
		// bad iterator, bail out
		for (int j = 0; j < dim; j++)
			Delete(indexes[j]);
		delete[] indexes;
		free(n);
		return 0;
	}

	return new model_array(W, t, n, (iterator**) indexes, dim);
}

expr* exprman::makeModelVarDecs(const location &W, model_def* p,
		const type* t, expr* bnds, symbol** names, int N) const {
	bool bailout = (0 == names || 0 == t || 0 == p);
	if (!bailout) {
		// make sure we can declare vars of this type!
		const formalism* ft = smart_cast <const formalism*>(p->Type());
		DCASSERT(ft);
		if (!ft->canDeclareType(t)) {
			if (startError()) {
				causedBy(W);
				cerr() << "Cannot declare a variable of type ";
				cerr() << t->getName();
				cerr() << " in formalism ";
				cerr() << ft->getName();
				stopIO();
			}
			bailout = 1;
		}
	}
	// typecheck the bounds
	if (!bailout)
		if (bnds) {
			if (!isPromotable(bnds->Type(), t->getSetOfThis())) {
				bailout = 1;
			} else {
				bnds = makeTypecast(W, t->getSetOfThis(), bnds);
				DCASSERT(isOrdinary(bnds));
			}
		}
	// check the names
	if (!bailout)
		for (int i = 0; i < N; i++) {
			model_symbol* ms = dynamic_cast<model_symbol*>(names[i]);
			if (0 == ms) {
				bailout = 1;
				break;
			}
		} // for i
	if (bailout) {
		// don't delete individual symbols, they're in a symbol table
		delete[] names;
		Delete(bnds);
		return 0;
	}

	return new model_var_stmt(W, p, t, bnds, (model_symbol**) names, N);
}

expr* exprman::makeModelArrayDecs(const location &W, model_def* p,
		const type* t, symbol** arrays, int N) const {
	if (0 == arrays)
		return 0;
	bool bailout = (0 == p);
	if (!bailout) {
		// make sure we can declare vars of this type!
		const formalism* ft = smart_cast <const formalism*>(p->Type());
		DCASSERT(ft);
		if (!ft->canDeclareType(t)) {
			if (startError()) {
				causedBy(W);
				cerr() << "Cannot declare a variable of type ";
				cerr() << t->getName();
				cerr() << " in formalism ";
				cerr() << ft->getName();
				stopIO();
			}
			bailout = 1;
		}
	}
	for (int i = 0; i < N; i++) {
		if (0 == arrays[i]) {
			bailout = 1;
			break;
		}
		model_array* ma = dynamic_cast<model_array*>(arrays[i]);
		if (0 == ma) {
			bailout = 1;
			break;
		}
	} // for i
	if (bailout) {
		for (int i = 0; i < N; i++)
			Delete(arrays[i]);
		delete[] arrays;
		return 0;
	}

	return new model_varray_stmt(W, p, t, (model_array**) arrays, N);
}

expr* exprman::makeModelMeasureAssign(const location &W, model_def* p,
		symbol* m, expr* rhs) const {
	model_symbol* w = dynamic_cast<model_symbol*>(m);
	if (0 == w || 0 == p || isError(rhs)) {
		Delete(rhs);
		return 0;
	}

	const type* rhstype = SafeType(rhs);
	const type* t = w->Type();
	const formalism* ft = smart_cast <const formalism*>(p->Type());
	DCASSERT(ft);

	if (!ft->isLegalMeasureType(t)) {
		if (startError()) {
			causedBy(W);
			cerr() << "Cannot declare a measure of type ";
			cerr() << t->getName();
			cerr() << " in formalism ";
			cerr() << ft->getName();
			stopIO();
		}
		return 0;
	}

	if (!isPromotable(rhstype, t)) {
		if (startError()) {
			causedBy(W);
			cerr() << "Return type for measure ";
			cerr() << w->Name();
			cerr() << " should be ";
			cerr() << t->getName();
			stopIO();
		}
		return 0;
	}
	rhs = promote(rhs, t);
	DCASSERT(! isError(rhs) );
	return new measure_assign(W, p, w, rhs);
}

expr* exprman::makeModelMeasureArray(const location &W, model_def* p,
		symbol* am, expr* rhs) const {
	model_array* w = dynamic_cast<model_array*>(am);
	if (0 == w || 0 == p || isError(rhs)) {
		Delete(rhs);
		return 0;
	}

	const type* rhstype = SafeType(rhs);
	const type* t = w->Type();
	const formalism* ft = smart_cast <const formalism*>(p->Type());
	DCASSERT(ft);

	if (!ft->isLegalMeasureType(t)) {
		if (startError()) {
			causedBy(W);
			cerr() << "Cannot declare a measure of type ";
			cerr() << t->getName();
			cerr() << " in formalism ";
			cerr() << ft->getName();
			stopIO();
		}
		return 0;
	}

	if (!isPromotable(rhstype, t)) {
		if (startError()) {
			causedBy(W);
			cerr() << "Return type for measure ";
			cerr() << w->Name();
			cerr() << " should be ";
			cerr() << t->getName();
			stopIO();
		}
		return 0;
	}
	rhs = promote(rhs, t);
	DCASSERT(! isError(rhs) );
	return new measure_array_assign(W, p, w, rhs);
}

// ******************************************************************
// *                                                                *
// *                                                                *
// *                           Front  end                           *
// *                                                                *
// *                                                                *
// ******************************************************************

expr* MakeBleVltB(const exprman* em, expr* lb, model_var* sv, expr* ub) {
	if ((0 == lb && 0 == ub) || (0 == sv)) {
		Delete(lb);
		Delete(sv);
		Delete(ub);
		return 0;
	}

	bool lb_isconst =
			lb ? (0 == lb->BuildExprList(traverse_data::GetSymbols, 0, 0)) : 0;
	bool ub_isconst =
			ub ? (0 == ub->BuildExprList(traverse_data::GetSymbols, 0, 0)) : 0;

	expr* answer = 0;
	if (0 == lb) {
		if (ub_isconst) {
			answer = new vltc_op(location::NOWHERE(), sv, ub);
		} else {
			answer = new vltb_op(location::NOWHERE(), sv, ub);
		}
	} else if (0 == ub) {
		if (lb_isconst)
			answer = new clev_op(location::NOWHERE(), lb, sv);
		else
			answer = new blev_op(location::NOWHERE(), lb, sv);
	} else {
		if (lb_isconst && ub_isconst)
			answer = new clevltc_op(location::NOWHERE(), lb, sv, ub);
		else
			answer = new blevltb_op(location::NOWHERE(), lb, sv, ub);
	}

#ifdef DEBUG_BLEVLTB
	em->cout() << "Made BleVltB expr: ";
	answer->Print(em->cout(), 0);
	em->cout() << "\n";
#endif

	return answer;
}

expr* MakeVarUpdate(const exprman* em, model_var* sv, expr* dec, expr* inc) {
	if ((0 == dec && 0 == inc) || (0 == sv)) {
		Delete(sv);
		Delete(dec);
		Delete(inc);
		return 0;
	}
	bool dec_isconst =
			dec ? (dec->BuildExprList(traverse_data::GetSymbols, 0, 0) == 0) : 1;
	bool inc_isconst =
			inc ? (inc->BuildExprList(traverse_data::GetSymbols, 0, 0) == 0) : 1;
	expr* answer = 0;
	if (dec_isconst && inc_isconst) {
		traverse_data x(traverse_data::Compute);
		result foo;
		x.answer = &foo;
		long dec_val = 0;
		if (dec) {
			dec->PreCompute();
			dec->Compute(x);
			DCASSERT(foo.isNormal());
			dec_val = foo.getInt();
			Delete(dec);
		}
		long inc_val = 0;
		if (inc) {
			inc->PreCompute();
			inc->Compute(x);
			DCASSERT(foo.isNormal());
			inc_val = foo.getInt();
			Delete(inc);
		}
		if (dec_val == inc_val) {
			Delete(sv);
			return 0; // no-op!
		}
		answer = new cupdate_op(location::NOWHERE(), sv, inc_val - dec_val);
	} else {
		answer = new vupdate_op(location::NOWHERE(), sv, dec, inc);
	}

#ifdef DEBUG_VUPDATE
	em->cout() << "Made vupdate expr: ";
	answer->Print(em->cout(), 0);
	em->cout() << "\n";
#endif

	return answer;
}

expr* MakeVarAssign(const exprman* em, model_var* sv, expr* rhs) {
	if (0 == sv || 0 == rhs) {
		Delete(sv);
		Delete(rhs);
		return 0;
	}
	expr* answer = 0;
	if (rhs->BuildExprList(traverse_data::GetSymbols, 0, 0) == 0) {
		// rhs is a constant, optimize
		traverse_data x(traverse_data::Compute);
		result foo;
		x.answer = &foo;
		rhs->PreCompute();
		rhs->Compute(x);
		DCASSERT(foo.isNormal());
		answer = new cassign_op(location::NOWHERE(), sv, foo.getInt());
	} else {
		answer = new vassign_op(location::NOWHERE(), sv, rhs);
	}

#ifdef DEBUG_VASSIGN
	em->cout() << "Made vassign expr: ";
	answer->Print(em->cout(), 0);
	em->cout() << "\n";
#endif

	return answer;
}

expr* MakeVarAssign(const exprman* em, model_var* sv, long rhs) {
	if (0 == sv)
		return 0;
	expr* answer = new cassign_op(location::NOWHERE(), sv, rhs);
#ifdef DEBUG_VASSIGN
	em->cout() << "Made vassign expr: ";
	answer->Print(em->cout(), 0);
	em->cout() << "\n";
#endif
	return answer;
}
