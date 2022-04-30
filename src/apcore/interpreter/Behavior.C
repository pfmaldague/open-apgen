#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <alphastring.H>
#include <BehavingElement.H>
#include <Rsource.H>
#include <accessorkey.H>
#include <ExecutionContext.H>
#include <ParsedExpressionSystem.H>
#include <RES_eval.H>
#include <apcoreWaiter.H>

extern int abdebug;

//
// private map; only Behavior can change it:
//
map<Cstring, int>& Behavior::WritableIndices() {
	static map<Cstring, int>	M;
	return M;
}

//
// const wrapper to ensure that external classes
// don't attempt to modify the map:
//
const map<Cstring, int>& Behavior::ClassIndices() {
	return WritableIndices();
}

//
// private map; only Behavior can change it:
//
map<Cstring, map<Cstring, int> >& Behavior::WritableRealms() {
	static map<Cstring, map<Cstring, int> >	M;
	return M;
}

//
// const wrapper to ensure that external classes
// don't attempt to modify the map:
//
const map<Cstring, map<Cstring, int> >& Behavior::ClassRealms() {
	return WritableRealms();
}

Behavior& Behavior::GlobalType() {
	return *ClassTypes()[0];
}

behaving_base*	behaving_base::temporary_storage = NULL;
bool		behaving_base::var_should_be_stored = false;

vector<Behavior*>& Behavior::ClassTypes() {
	static vector<Behavior*>	Types;
	return Types;
}

//
// invoked by ACT_exec::create_subsystems():
//
void Behavior::initialize() {
	static bool	initialized = false;

	if(!initialized) {
		initialized = true;

		//
		// define the globals Type
		//
		WritableIndices()["globals"] = 0;
		ClassTypes().push_back(new Behavior(
						"globals",
						"global",
						NULL));

		//
		// define the generic activity type - COORDINATE THESE WITH ActivityInstance::varlocation
		//
		WritableIndices()["generic"] = 1;
		Behavior*	genericActivityType = new Behavior(
							"generic",
							"activity",
							NULL);
		genericActivityType->tasks[CONSTRUCTOR]->add_variable("id",		apgen::DATA_TYPE::STRING);
		genericActivityType->tasks[CONSTRUCTOR]->add_variable("type",		apgen::DATA_TYPE::STRING);
		genericActivityType->tasks[CONSTRUCTOR]->add_variable("parent",		apgen::DATA_TYPE::INSTANCE);
		genericActivityType->tasks[CONSTRUCTOR]->add_variable("start",		apgen::DATA_TYPE::TIME);
		genericActivityType->tasks[CONSTRUCTOR]->add_variable("finish",		apgen::DATA_TYPE::TIME);
		genericActivityType->tasks[CONSTRUCTOR]->add_variable("subsystem",	apgen::DATA_TYPE::STRING);

		genericActivityType->tasks[CONSTRUCTOR]->add_variable("this",		apgen::DATA_TYPE::INSTANCE);

		/* I commented out seldom-used attributes.  Re-instate then as custom attributes if needed. */

		genericActivityType->tasks[CONSTRUCTOR]->add_variable("Color",		apgen::DATA_TYPE::STRING);
		genericActivityType->tasks[CONSTRUCTOR]->add_variable("color",		apgen::DATA_TYPE::INTEGER);
		genericActivityType->tasks[CONSTRUCTOR]->add_variable("decomp_suffix",	apgen::DATA_TYPE::STRING);
		genericActivityType->tasks[CONSTRUCTOR]->add_variable("description",	apgen::DATA_TYPE::STRING);
		genericActivityType->tasks[CONSTRUCTOR]->add_variable("label",		apgen::DATA_TYPE::STRING);
		genericActivityType->tasks[CONSTRUCTOR]->add_variable("legend",		apgen::DATA_TYPE::STRING);
		genericActivityType->tasks[CONSTRUCTOR]->add_variable("misc",		apgen::DATA_TYPE::ARRAY);
		genericActivityType->tasks[CONSTRUCTOR]->add_variable("name",		apgen::DATA_TYPE::STRING);
		genericActivityType->tasks[CONSTRUCTOR]->add_variable("pattern",	apgen::DATA_TYPE::INTEGER);
		genericActivityType->tasks[CONSTRUCTOR]->add_variable("plan",		apgen::DATA_TYPE::STRING);
		genericActivityType->tasks[CONSTRUCTOR]->add_variable("sasf",		apgen::DATA_TYPE::ARRAY);
		genericActivityType->tasks[CONSTRUCTOR]->add_variable("span",		apgen::DATA_TYPE::DURATION);
		genericActivityType->tasks[CONSTRUCTOR]->add_variable("status",		apgen::DATA_TYPE::BOOL_TYPE);
		ClassTypes().push_back(genericActivityType);

		// genericActivityType->tasks[CONSTRUCTOR]->add_variable("baggage",	apgen::DATA_TYPE::STRING);
		// genericActivityType->tasks[CONSTRUCTOR]->add_variable("group",	apgen::DATA_TYPE::STRING);
		// genericActivityType->tasks[CONSTRUCTOR]->add_variable("history",	apgen::DATA_TYPE::STRING);
		// genericActivityType->tasks[CONSTRUCTOR]->add_variable("originator",	apgen::DATA_TYPE::STRING);
		// genericActivityType->tasks[CONSTRUCTOR]->add_variable("owner",	apgen::DATA_TYPE::STRING);
		// genericActivityType->tasks[CONSTRUCTOR]->add_variable("sequence",	apgen::DATA_TYPE::STRING);

	}
}

task&	Behavior::GlobalConstructor() {
	Behavior& the_type = *ClassTypes()[0];

	//
	// assert that Behavior::initialize() has already been invoked:
	//
	assert(the_type.tasks.size() > 0);
	return *the_type.tasks[0];
}

smart_ptr<global_behaving_object>& static_global_obj() {
	static smart_ptr<global_behaving_object> B(new global_behaving_object());
	return B;
}

//
// Returns a thread-specific copy. All copies share the same
// basic symbol table, except for 'now' which is thread-specific.
//
global_behaving_object*& behaving_object::GlobalObject() {
    if(thread_index == 0) {
	static global_behaving_object* O = static_global_obj().object();
	return O;
    } else {

	//
	// index 0 is not used. The appropriate Copy must
	// be initialized by any new thread that wants to
	// use behaving objects.
	//
	static global_behaving_object* Copy[10];
	return Copy[thread_index];
    }
}

// tlist<alpha_string, Cnode0<alpha_string, behaving_object*> >&
// 				behaving_object::abstract_resources() {
//     static tlist<alpha_string, Cnode0<alpha_string, behaving_object*> > a;
//     return a;
// }

void	report_profile_info() {

    tlist<alpha_int, Cnode0<alpha_int, const task*> > ordered(true);

    map<Cstring, map<Cstring, int> >::const_iterator realm_iter
	= Behavior::ClassRealms().find("abstract resource");

    if(realm_iter == Behavior::ClassRealms().end()) {
	cout << "# No abstract resources have been defined\n";
	return;
    }

    const map<Cstring, int>&	the_types = realm_iter->second;
    map<Cstring, int>::const_iterator	type_iter;

    cout << "# Abstract resource profiles\n";
    cout << "Name | calls | total time (s) | cumul. time (s) | time/call (ms)\n";
    cout << "---- | ----- | -------------- | --------------- | --------------\n";

    //
    // Organize output in decreasing order of CPU time used:
    //
    for(    type_iter = the_types.begin();
	    type_iter != the_types.end();
	    type_iter++) {
	Behavior*	abs_res_type = Behavior::ClassTypes()[type_iter->second];

	//
	// 'use' task always has index 1
	//
	const task*	use_task = abs_res_type->tasks[1];
	double cputime = use_task->cpu_time();
	long int microseconds = (long int) (cputime * 1.0E+6);
	ordered << new Cnode0<alpha_int, const task*>(microseconds, use_task);
    }
    Cnode0<alpha_int, const task*>* ptr2;
    double cumul_time = 0.0;
    for(ptr2 = ordered.last_node(); ptr2; ptr2 = ptr2->previous_node()) {
	long int num_called = ptr2->payload->calls();
	double cpu = ((double)ptr2->Key.get_int()) / 1.0E+6;
	cumul_time += cpu;
	double cpu_per_call = 0.0;
	if(num_called) {
	    cpu_per_call = ((double)ptr2->Key.get_int()) / (num_called * 1000.0);
	}
	cout << ptr2->payload->Type.name << " | "
	     << num_called << " | "
	     << cpu << " | "
	     << cumul_time << " | ";
	if(num_called) {
	     cout << cpu_per_call << "\n";
	} else {
	     cout << "-\n";
	}
    }
}

void	Behavior::delete_subsystems() {
	for(int i = 0; i < ClassTypes().size(); i++) {
		delete ClassTypes()[i];
	}
	ClassTypes().clear();
	WritableIndices().clear();
	static_global_obj().dereference();
}

int	Behavior::add_type(Behavior* bt) {
    int n = ClassTypes().size();
    ClassTypes().push_back(bt);
    WritableIndices()[bt->name] = n;
    map<Cstring, map<Cstring, int> >::iterator iter
		= WritableRealms().find(bt->realm);
    if(iter == WritableRealms().end()) {
	map<Cstring, int> a_map;
	a_map[bt->name] = n;
	WritableRealms()[bt->realm] = a_map;
    } else {
	iter->second[bt->name] = n;
    }
    if(abdebug) {
	cerr << bt->full_name() << " -> ClassTypes\n";
    }
    return n;
}

//
// Can be called repeatedly with no adverse effect
//
void	pseudo_vector::cleanup(const vector<int>& overridden) {
    if(valarray) {

	if(overridden.size() == 0) {

	    //
	    // modeling thread
	    //
	    for(int i = 0; i < raw_size; i++) {
		delete valarray[i];
	    }
	} else {

	    //
	    // trailing thread
	    //
	    for(int i = 0; i < overridden.size(); i++) {
		delete valarray[overridden[i]];
	    }
	}

	free((char*)valarray);
	valarray = NULL;
    }
    dimension = 0;
    raw_size = 0;
}

void	pseudo_vector::push_back(const TypedValue& tv) {
	unsigned int u = size();
	resize(u + 1, /* allocate = */ true);
	*valarray[u] = tv;
}

void		pseudo_vector::resize(unsigned int new_size, bool allocate) {
    unsigned int newdim;
    if(new_size > raw_size) {
	if(raw_size) {
	    newdim = 2 * raw_size + new_size;
	    valarray = (StickyValue**) realloc(valarray, newdim * sizeof(StickyValue*));
	} else {
	    newdim = 20 + new_size;
	    valarray = (StickyValue**) malloc(newdim * sizeof(StickyValue*));
	}
	if(allocate) {
	    for(int i = raw_size; i < newdim; i++) {
		valarray[i] = new StickyValue;
	    }
	}
	raw_size = newdim;
	dimension = new_size;
    } else if(new_size > dimension) {
	dimension = new_size;
    }

    //
    // else, nothing to do
    //
}

StickyValue&	pseudo_vector::operator[](unsigned int i) {
	return *valarray[i];
}

const StickyValue& pseudo_vector::operator[](unsigned int i) const {
	return *valarray[i];
}


static bool GlobalObjectWasCreated = false;

//
// only used once per global object:
//
behaving_base::behaving_base()
	: ref_count(0),
		Task(*Behavior::ClassTypes()[0]->tasks[0]),
		Type(*Behavior::ClassTypes()[0]) {
    if(!thread_index) {
	assert(!GlobalObjectWasCreated);
    }
    level[0] = this;
    level[1] = NULL; // should never be invoked
    level[2] = NULL; // ditto
}

behaving_base::behaving_base(
		task& T)
	: ref_count(0),
		Task(T),
		Type(T.Type) {

	assert(GlobalObjectWasCreated);

	//
	// Change with respect to fast APGen: we define the object's level as
	// 1 for a function now.
	//
	// level[1] = level[0];
	// level[2] = this;

	level[0] = behaving_object::GlobalObject();
	level[1] = this;
	level[2] = NULL;

	// debug
	map<Cstring, int>::const_iterator iter = T.get_varindex().find("enabled");
	assert(iter != T.get_varindex().end());
	assert(iter->second == 0);
}

behaving_base::behaving_base(
		task&		T,
		behaving_base*	bo)
	: ref_count(0),
		Task(T),
		Type(T.Type) {

	assert(GlobalObjectWasCreated);
	level[0] = behaving_object::GlobalObject();
	level[1] = bo;
	level[2] = this;

	// debug
	map<Cstring, int>::const_iterator iter = T.get_varindex().find("enabled");
	assert(iter != T.get_varindex().end());
	assert(iter->second == 0);

}

behaving_object::behaving_object(
		task& T)
	: behaving_base(T) {

	assert(GlobalObjectWasCreated);

	vals.resize(T.get_varinfo().size());
	for(int i = 0; i < vals.size(); i++) {
		apgen::DATA_TYPE dt = Task.get_varinfo()[i].second;
		vals[i].declared_type = dt;
		vals[i].generate_default_for_type(dt);
	}
	// set 'enabled'
	vals[0] = true;
}

behaving_object::behaving_object(
		task&			T,
		behaving_base*	bo)
	: behaving_base(T, bo) {

	vals.resize(T.get_varinfo().size());
	for(int i = 0; i < vals.size(); i++) {
		apgen::DATA_TYPE dt = Task.get_varinfo()[i].second;
		vals[i].declared_type = dt;
		vals[i].generate_default_for_type(dt);
	}
	// set 'enabled'
	vals[0] = true;
}

TypedValue&	behaving_object::operator[](unsigned int i) {
	return vals[i];
}

const TypedValue&	behaving_object::operator[](unsigned int i) const {
	return vals[i];
}

const char* behaving_base::serial() const {
	static Cstring		tmp;
	TypedValue		V = to_struct();
	tmp.undefine();
	tmp = V.to_string();
	return *tmp;
}

TypedValue&	behaving_object::lval(
				const Cstring& s) {
	map<Cstring, int>::const_iterator iter = Task.get_varindex().find(s);
	if(iter == Task.get_varindex().end()) {
		Cstring err;
		err << "variable " << s << " not found in task " << Task.name;
	throw(eval_error(err));
	}
	assert(vals.size() > iter->second);
	// if(vals[iter->second].is_read_only()) {
	// 	Cstring err;
	// 	err << "variable " << s << " is read-only in " << Task.name;
	// 	throw(eval_error(err));
	// }
	return vals[iter->second];
}
const TypedValue& behaving_object::lval(
				const Cstring& s) const {
	map<Cstring, int>::const_iterator iter = Task.get_varindex().find(s);
	if(iter == Task.get_varindex().end()) {
		Cstring err;
		err << "variable " << s << " not found in task " << Task.name;
	throw(eval_error(err));
	}
	assert(vals.size() > iter->second);
	return vals[iter->second];
}

TypedValue&	behaving_object::operator[](
					const Cstring& s) {
	map<Cstring, int>::const_iterator iter = Task.get_varindex().find(s);
	if(iter == Task.get_varindex().end()) {
		Cstring err;
		err << "variable " << s << " not found in task " << Task.name;
		throw(eval_error(err));
	}
	assert(vals.size() > iter->second);
	// if(vals[iter->second].is_read_only()) {
	// 	Cstring err;
	// 	err << "variable " << s << " is read-only in " << Task.name;
	// 	throw(eval_error(err));
	// }
	return vals[iter->second];
}

const TypedValue&	behaving_object::operator[](
					const Cstring& s) const {
	map<Cstring, int>::const_iterator iter = Task.get_varindex().find(s);
	if(iter == Task.get_varindex().end()) {
		Cstring err;
		err << "variable " << s << " not found in task " << Task.name;
		throw(eval_error(err));
	}
	assert(vals.size() > iter->second);
	return vals[iter->second];
}

unsigned int behaving_object::size() const {
	return vals.size();
}

//
// Does not delete the values in vals:
//
void behaving_object::clear() {
	for(int i = 0; i < vals.size(); i++) {
		vals[i].undefine();
	}
}

void	behaving_object::assign_w_side_effects(
		int			key,
		const TypedValue&	val,
		bool			B) {
	vals[key] = val;
}

TypedValue behaving_object::to_struct() const {
	ListOVal*	lov = new ListOVal;
	ArrayElement*	ae;
	TypedValue	V;

	for(int i = 0; i < Task.get_varinfo().size(); i++) {
		Cstring the_key = Task.get_varinfo()[i].first;
		const TypedValue& W(operator[](i));
		if(the_key == "parent") {
			V = W.get_object()->to_struct();
		}
		ae = new ArrayElement(the_key, V);
		lov->add(ae);
	}
	V = (*lov);
	return V;
}

global_behaving_object::global_behaving_object()
		: behaving_base() {
	assert(!GlobalObjectWasCreated);
	GlobalObjectWasCreated = true;
	Behavior::initialize();
	values.resize(Task.get_varinfo().size(), /* allocate = */ true);
	for(int i = 0; i < values.size(); i++) {
		apgen::DATA_TYPE dt = Task.get_varinfo()[i].second;
		values[i].declared_type = dt;
		values[i].generate_default_for_type(dt);
	}
	// set 'enabled'
	values[0] = true;
}

void	global_behaving_object::assign_w_side_effects(
		int			key,
		const TypedValue&	val,
		bool			B) {
	values[key] = val;
}

void global_behaving_object::print_constant_status(int option) {
#ifdef DEBUG_ONLY
	if(option == -1) {
	    for(int i = 0; i < values.size(); i++) {
		pair<Cstring, apgen::DATA_TYPE>& one =
			Behavior::GlobalConstructor().get_varinfo()[i];
		cerr << "\tglob[" << i << "] = " << one.first << ": " << spell(one.second)
			<< " constant(" << values[i].is_constant
			<< ")\n";
	    }
	} else if(option < values.size()) {
	    pair<Cstring, apgen::DATA_TYPE>& one =
			Behavior::GlobalConstructor().get_varinfo()[option];
	    cerr << "\tglob[" << option << "] = " << one.first << ": " << spell(one.second)
			<< " constant(" << values[option].is_constant
			<< ")\n";
	}
#endif /* DEBUG_ONLY */
}

TypedValue	global_behaving_object::to_struct() const {
	ListOVal*	lov = new ListOVal;
	ArrayElement*	ae;
	TypedValue	V;

	for(int i = 0; i < Task.get_varinfo().size(); i++) {
		Cstring the_key = Task.get_varinfo()[i].first;
		const TypedValue& W(operator[](i));
		if(the_key == "parent") {
			V = W.get_object()->to_struct();
		}
		ae = new ArrayElement(the_key, V);
		lov->add(ae);
	}
	V = (*lov);
	return V;
}

TypedValue&	global_behaving_object::operator[](
			const Cstring& s) {
	map<Cstring, int>::const_iterator iter = Task.get_varindex().find(s);
	if(iter == Task.get_varindex().end()) {
		Cstring err;
		err << "variable " << s << " not found in task " << Task.name;
		throw(eval_error(err));
	}
	assert(values.size() > iter->second);
	// if(values[iter->second].is_read_only()) {
	// 	Cstring err;
	// 	err << "variable " << s << " is read-only in " << Task.name;
	// 	throw(eval_error(err));
	// }
	return values[iter->second];
}

const TypedValue& global_behaving_object::operator[](
			const Cstring& s) const {
	map<Cstring, int>::const_iterator iter = Task.get_varindex().find(s);
	if(iter == Task.get_varindex().end()) {
		Cstring err;
		err << "variable " << s << " not found in task " << Task.name;
		throw(eval_error(err));
	}
	assert(values.size() > iter->second);
	return values[iter->second];
}

TypedValue&	global_behaving_object::operator[](unsigned int i) {
	return values[i];
}

const TypedValue& global_behaving_object::operator[](
		unsigned int i) const {
	return values[i];
}

TypedValue&	global_behaving_object::lval(
		const Cstring& s) {
	map<Cstring, int>::const_iterator iter = Task.get_varindex().find(s);
	if(iter == Task.get_varindex().end()) {
		Cstring err;
		err << "variable " << s << " not found in task " << Task.name;
		throw(eval_error(err));
	}
	assert(values.size() > iter->second);
	// if(values[iter->second].is_read_only()) {
	// 	Cstring err;
	// 	err << "variable " << s << " is read-only in " << Task.name;
	// 	throw(eval_error(err));
	// }
	return values[iter->second];
}
const TypedValue& global_behaving_object::lval(
		const Cstring& s) const {
	map<Cstring, int>::const_iterator iter = Task.get_varindex().find(s);
	if(iter == Task.get_varindex().end()) {
		Cstring err;
		err << "variable " << s << " not found in task " << Task.name;
		throw(eval_error(err));
	}
	assert(values.size() > iter->second);
	return values[iter->second];
}


void		global_behaving_object::clear() {
	for(int i = 0; i < size(); i++) {
		values[i].undefine();
	}
}

unsigned int	global_behaving_object::size() const {
	return values.size();
}

global_behaving_object::global_behaving_object(
			const global_behaving_object& G)
	: behaving_base() {

    //
    // Task was initialized to the global constructor
    // by behaving_base::behaving_base()
    //
    values.resize(Task.get_varinfo().size(), /* allocate = */ false);
    for(int i = 0; i < values.size(); i++) {

	//
	// valarray elements point to the values
	// held by the global object:
	//
	values.valarray[i] = G.values.valarray[i];
    }

    //
    // Presently, the only trailing data name is 'now'
    //
    for(int i = 0; i < trailing_data_names().size(); i++) {

	Cstring	s = trailing_data_names()[i];

	//
	// First, find where the symbol is defined in the global
	// object
	//
	map<Cstring, int>::const_iterator iter = Task.get_varindex().find(s);
	if(iter == Task.get_varindex().end()) {
		Cstring errs;
		errs << "trailing thread initialization: cannot find symbol \""
			<< s << "\" in globals";
		throw(eval_error(errs));
	}
	int data_index = iter->second;
	overridden_variables.push_back(data_index);

	//
	// Second, get the data type from the global object
	//
	apgen::DATA_TYPE dt = Task.get_varinfo()[data_index].second;

	//
	// Third, allocate a new value with the right type
	//
	StickyValue* tempval = new StickyValue(dt);

	//
	// Fourth, make the value content the same value as 
	// the corresponding value in the global object
	// (which the value pointer currently points to; that's
	// how it was initialized by the global_behaving_object
	// copy constructor)
	//
	*tempval = G[data_index];

	//
	// Fifth and last, stick the new value in the right place
	// in the values array
	//
	values.valarray[data_index] = tempval;

	//
	// Don't forget to destroy it when we go away!
	//
    }
}

//
// List of symbols that need to be implemented separately in
// separate threads.  Right now there is only one: 'now'. In
// the future there might be more.
//
vector<Cstring>&	global_behaving_object::trailing_data_names() {
	static vector<Cstring>	V;
	static bool		initialized = false;

	if(!initialized) {
		initialized = true;
		V.push_back("now");
	}
	return V;
}

//
// Set of names of constant globals:
//
set<Cstring>&		global_behaving_object::constants() {
	static set<Cstring> S;
	return S;
}

//
// NOTE: this says that the object defines the property in question.
// It says nothing about the value of that property.
//
bool behaving_base::defines_property(
		const Cstring& s) const {
	map<Cstring, int>::const_iterator iter = Task.get_varindex().find(s);
	if(iter == Task.get_varindex().end()) {
		return false;
	}
	if(operator[](iter->second).get_type() == apgen::DATA_TYPE::UNINITIALIZED) {
		return false;
	}
	return true;
}

void add_one_layer(
		Behavior*		container_behavior,
		const Cstring&		current_layer_name,
		const Cstring&		resource_realm,
		unsigned int		current_layer_dimension,
		int*			current_layer_index_sizes,
		vector<vector<Cstring> >& current_layer_index_vectors,
		vector<Cstring>&	current_sequence_of_res_indices) {
    for(int i = 0; i < current_layer_index_sizes[0]; i++) {
	Cstring new_layer_name = current_layer_name + "[" + addQuotes(current_layer_index_vectors[0][i]) + "]";
	vector<Cstring>	 new_sequence_of_res_indices = current_sequence_of_res_indices;
	new_sequence_of_res_indices.push_back(current_layer_index_vectors[0][i]);
	if(current_layer_dimension == 1) {

	    if(abdebug) {
		cerr << "\tAdding resource["
		     << container_behavior->SubclassTypes.size() << "] = "
		     << new_layer_name << "\n";
	    }

	    container_behavior->SubclassFlatMap[new_layer_name] = container_behavior->SubclassTypes.size();
	    Behavior* sub_beh;
	    container_behavior->SubclassTypes.push_back(
			sub_beh = new Behavior(new_layer_name, resource_realm, container_behavior));
	    container_behavior->SubclassIndices.push_back(new_sequence_of_res_indices);
	    sub_beh->tasks[0]->add_variable("this", apgen::DATA_TYPE::INSTANCE);
	    sub_beh->tasks[0]->add_variable("id", apgen::DATA_TYPE::STRING);
	    sub_beh->tasks[0]->add_variable("indices", apgen::DATA_TYPE::ARRAY);
	} else {
	    vector<vector<Cstring> > new_layer_index_vectors;
	    for(int j = 1; j < current_layer_dimension; j++) {
		new_layer_index_vectors.push_back(current_layer_index_vectors[j]);
	    }
	    unsigned int new_layer_dimension = current_layer_dimension - 1;
	    int* new_layer_index_sizes = current_layer_index_sizes + 1;
	    add_one_layer(
		container_behavior,
		new_layer_name,
		resource_realm,
		new_layer_dimension,
		new_layer_index_sizes, 
		new_layer_index_vectors,
		new_sequence_of_res_indices);
	}
    }
}

void	Behavior::add_resource_subtypes(
			const Cstring&		base_name,
			const Cstring&		sub_realm,
			unsigned int		dimension,
			int*			index_sizes,
			vector<vector<Cstring> >& index_vectors,
			vector<map<Cstring, int> >& index_maps) {
    if(!dimension) {
	if(abdebug) {
	    cerr << full_name() << "->add sub-type(" << base_name << ")\n";
	}
	Behavior* sub_beh = new Behavior(base_name, sub_realm, this);
	sub_beh->tasks[0]->add_variable("this", apgen::DATA_TYPE::INSTANCE);
	sub_beh->tasks[0]->add_variable("id", apgen::DATA_TYPE::STRING);
	sub_beh->tasks[0]->add_variable("indices", apgen::DATA_TYPE::ARRAY);
	SubclassTypes.push_back(sub_beh);
	map<Cstring, int> M;
	M[base_name] = 0;
	SubclassMaps.push_back(M);
	SubclassFlatMap[base_name] = 0;
    } else {
	int N = 1;
	for(int i = 0; i < dimension; i++) {
	    N *= index_sizes[i];
	    SubclassMaps.push_back(index_maps[i]);
	}
	if(abdebug) {
	    cerr << full_name() << "->add_resource_subtypes(how many = " << N << ")\n";
	}
	vector<Cstring> this_set_of_indices;
	add_one_layer(
	    this,
	    base_name,
	    sub_realm,
	    dimension,
	    index_sizes,
	    index_vectors,
	    this_set_of_indices);

    }
}

function_object::function_object(
		TypedValue&	V,
		task&		T)
	: theReturnedValue(V),
		behaving_object(T) {
}

function_object::function_object(
		TypedValue&	V,
		task&		T,
		behaving_base*  parent_o)
	: theReturnedValue(V),
		behaving_object(T, parent_o) {
}

//
// This constructor and the one above are only OK
// for methods that are defined inside an activity
// or a(n) (abstract) resource:
//
method_object::method_object(
		task&		T,
		behaving_base*	parent_o
		)
	: behaving_object(T) {

	operator[](PARENT) = behaving_element(parent_o);
}

Behavior::Behavior()
	: name("unknown type"),
		tasks(private_tasks),
		parent(NULL),
		realm("unknown realm") {
	add_task("constructor");
}

Behavior::Behavior(
		const Cstring& s,
		const Cstring& meta,
		Behavior* p)
	: name(s),
		tasks(private_tasks),
		parent(p),
		realm(meta) {
	if(s == "globals" && meta == "global") {
		static bool the_only_global_type_already_exists = false;
		assert(!the_only_global_type_already_exists);
		the_only_global_type_already_exists = true;
		add_task("constructor", true);
	} else {
		add_task("constructor");
	}
}

int Behavior::add_task(
			const Cstring&	task_name,
			bool		global /* = false */) {
	if(taskindex.find(task_name) != taskindex.end()) {
		Cstring err;
		err << "Task " << task_name << " already exists";
		throw(eval_error(err));
	}
	int n = tasks.size();
	taskindex[task_name] = n;
	int level = 65535;

	//
	// Do the first check because the very first call to add_task
	// is made from within the very first invocation of the
	// Behavior constructor, which creates GlobalType()
	//
	if(global) {
		private_tasks.push_back(new global_task("constructor", *this));
	} else {
		if(this == &GlobalType()) {

			//
			// only one global constructor is allowed and it has
			// already been taken care of:
			//
			assert(n > 0);
			level = 1;		// function scope
		} else {
			if(!n) {

				//
				// This is the constructor
				//
				level = 1;	// class scope
			} else {

				//
				// This is a method
				//
				level = 2;	// method scope
			}
		}
		private_tasks.push_back(new task(task_name, *this, level));
	}

	if(abdebug) {
		cerr << full_name() << "->add_task[" << n << "] = " << task_name << "\n";
	}
	return n;
}

void Behavior::delete_task(
		const Cstring& task_name) {
	map<Cstring, int>::iterator iter = taskindex.find(task_name);
	if(iter == taskindex.end()) {
		Cstring err;
		err << "task " << task_name << " does not exist in " << full_name();
		throw(eval_error(err));
	}
	delete tasks[iter->second];
	private_tasks.erase(private_tasks.begin() + iter->second);
	taskindex.erase(iter);
}

Behavior::~Behavior() {
	for(int i = 0; i < tasks.size(); i++) {
		delete tasks[i];
	}
	private_tasks.clear();
	taskindex.clear();
	for(int i = 0; i < SubclassTypes.size(); i++) {
		delete SubclassTypes[i];
	}
	SubclassTypes.clear();
}

static void dump_indent(aoString* s, int k) {
	for(int h = 0; h < k; h++) {
		(*s) << " ";
	}
}

void task::to_stream(
		aoString* s,
		int indent) const {
	dump_indent(s, indent);
	(*s) << full_name() << "\n";
	if(varinfo.size()) {
		dump_indent(s, indent + 2);
		(*s) << "variables:\n";
		for(int l = 0; l < varinfo.size(); l++) {
			dump_indent(s, indent + 4);
			(*s) << l << ": "
				<< apgen::spell(varinfo[l].second)
				<< " " << varinfo[l].first << "\n";
		}
	}
	if(parameters) {
		dump_indent(s, indent + 2);
		(*s) << "parameters:\n";
		parameters->to_stream(s, indent + 4);
	}
	if(prog) {
		dump_indent(s, indent + 2);
		(*s) << "instructions:\n";
		prog->to_stream(s, indent + 4);
	}
}

void	Behavior::rename_task(
			task*		to_rename,
			const Cstring&	new_name) {
	map<Cstring, int>::iterator iter = taskindex.find(to_rename->name);
	assert(iter != taskindex.end());
	int k = iter->second;
	taskindex.erase(iter);
	taskindex[new_name] = k;
	to_rename->name = new_name;
}

void Behavior::to_stream(
		aoString* s,
		int indent) const {
	dump_indent(s, indent);
	(*s) << full_name() << "\n";
	for(int k = 0; k < tasks.size(); k++) {
		dump_indent(s, indent);
		(*s) << k << ":\n";
		task* T = tasks[k];
		T->to_stream(s, indent + 2);
	}
	if(SubclassTypes.size()) {
		dump_indent(s, indent + 2);
		(*s) << "subtypes:\n";
		for(int j = 0; j < SubclassTypes.size(); j++) {
			SubclassTypes[j]->to_stream(s, indent + 4);
		}
	}
}

void behaving_base::to_stream(
		aoString* s,
		int indent) const {
	dump_indent(s, indent);
	(*s) << "{\n";
	for(int i = 0; i < size(); i++) {
		if(operator[](i).get_type() != apgen::DATA_TYPE::UNINITIALIZED) {
			dump_indent(s, indent + 2);
			(*s) << i << " - " << Task.get_varinfo()[i].first << ": ";
			(*s) << operator[](i).to_string() << "\n";
		}
	}
	dump_indent(s, indent);
	(*s) << "}\n";
}

task&	task::EmptyTask() {
	static task T(
			"Empty Task",
			*Behavior::ClassTypes()[0],
			65535);
	return T;
}

task::task(
		const Cstring& n,
		Behavior& type,
		int level)
	: name(n),
		return_type(apgen::DATA_TYPE::UNINITIALIZED),
		Type(type),
		scope(level),
		script_flag(false),
		time_consumed(0.0),
		how_many_times_called(0L) {
	add_variable("enabled", apgen::DATA_TYPE::BOOL_TYPE);
}

task::~task() {
}

int task::add_variable(
		const Cstring& vname,
		apgen::DATA_TYPE the_type) {
	if(varindex.find(vname) != varindex.end()) {
		Cstring err;
		err << "Task " << name << ": variable called \""
			<< vname << "\" already exists.";
		throw(eval_error(err));
	}
	if(the_type == apgen::DATA_TYPE::UNINITIALIZED) {
		Cstring err;
		err << "Task " << name << "->add_variable("
			<< vname << "): trying to add UNINITIALIZED variable.";
		throw(eval_error(err));
	}

	int n = varinfo.size();
	varindex[vname] = n;
	varinfo.push_back(pair<Cstring, apgen::DATA_TYPE>(vname, the_type));

	if(abdebug) {
		cerr << full_name() << "->add_variable(" << vname << ", "
			<< apgen::spell(the_type) << ")\n";
	}
	return n;
}

int global_task::add_variable(
		const Cstring& vname,
		apgen::DATA_TYPE the_type) {
	task::add_variable(vname, the_type);
	int n = behaving_object::GlobalObject()->values.size();
	behaving_object::GlobalObject()->values.push_back(StickyValue(the_type));
	return n;
}

/* Unlike act_object, we should only define the attributes that exist.
 * Activity instances can always add attributes as they please, but
 * resources can't. So we don't really need a prototype with all
 * possible attributes instantiated.
 *
 * Besides, this would be wrong - APGen relies on resource not
 * having certain properties such as interpolation etc. */
concrete_res_object::concrete_res_object(
		Rsource*	r)
		: res(r),
			behaving_object(*r->Type.tasks[0]) {
	behaving_element be;
	operator[](THIS) = be;
	operator[](THIS).get_object().reference(this);
	operator[](ID) = r->name;
	TypedValue& A = operator[](INDICES);

	for(int z = 0; z < r->indices.size(); z++) {
		A.get_array().add(z, r->indices[z]);
	}
}

const Cstring&	concrete_res_object::get_key() const {
	return res->name;
}

task&	signal_object::SignalTask() {
	static bool	initialized = false;
	static task*	T = NULL;
	if(!initialized) {
		initialized = true;
		int n = Behavior::GlobalType().add_task("signal task");
		T = Behavior::GlobalType().tasks[n];
		T->add_variable("signal", apgen::DATA_TYPE::STRING);
	}
	return *T;
}

signal_object::signal_object(const Cstring& s)
		: behaving_object(signal_object::SignalTask()) {

	//
	// should set name here...
	//
	operator[](1) = s; // variable 'signal'
}
