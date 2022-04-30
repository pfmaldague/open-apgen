#ifndef _BEHAVING_ELEMENT_H_
#define _BEHAVING_ELEMENT_H_

#include <vector>
#include <map>
#include <set>

#include "APbasic.H"
#include "smartptr.H"

using namespace std;

class StickyValue;
class behaving_base;
class behaving_object;
class global_behaving_object;
class ActivityInstance;
class RES_resource;
class AbstractResource;
class Rsource;
class task;

//
// in line with IMCE Ontology:
//
typedef smart_ptr<behaving_base> behaving_element;

namespace pEsys {
	class Program;
	class Symbol;
	class QualifiedSymbol;
}

typedef smart_ptr<pEsys::Program> parsedProg;

/* The purpose of this class is to wrap the tasks vector
 * into a protected class member that is not accessible
 * to classes other than Behavior.  This means that only
 * the Behavior class can add new tasks to the vector,
 * which allows us to give special treatment to the first
 * task of the vector (private_tasks[0]), which is always
 * the class constructor.  If we allowed classes other
 * than Behavior to access the vector directly, the
 * connection between vector index 0 and the constructor
 * task would be impossible to enforce. */
class Actor {
protected:
	vector<task*>	private_tasks;
public:
	Actor() {}
	virtual ~Actor() {}
};

/* The word "Behavior" is used to denote APGen's version
 * of what would be called a class in a standard programming
 * language. It emphasizes the fact that APGen's purpose
 * is to simulate the behavior of a real system.
 *
 * Unlike classes which can contain both data and functions
 * (called methods), a Behavior only contains "tasks",
 * which are analogous to functions. The most important
 * task of a Behavior is its constructor task, which is
 * always the first element of the tasks vector (see the
 * class member called "tasks" below).
 *
 * We store Behaviors in the static vector ClassTypes(). */
class Behavior: public Actor {
public:
	// not used much:
	typedef enum {
		GLOBAL = 0,
		UNKNOWN_TYPE = 1
	} TypeIndex;

	// not used much:
	typedef enum {
		CONSTRUCTOR = 0,
		UNKNOWN_TASK = 1
	} TaskIndex;

	Behavior();
	Behavior(
			const Cstring&	type_name,
			const Cstring&	realm_name,
			Behavior*	parent_behavior);
	// do not use
	Behavior(const Behavior& bt) = delete;

	virtual ~Behavior();

	static void		initialize();
	static task&		GlobalConstructor();

	Behavior*		parent;
	Cstring			name;
	Cstring			realm;
	void			to_stream(
					aoString*,
					int indent) const;
	Cstring			full_name() const {
		return Cstring(realm) + "::" + name;
	}

	/* Access to all top-level types is provided by global
	 * vectors and maps:
	 * 	- GlobalType().tasks[0] contains all global declarations
	 *	- GlobalType().varinfo contains all global variable names
	 *	- beh_obj::GlobalObject() provisions task GlobalType().tasks[0]
	 *	  and holds all global values */

	static vector<Behavior*>&	ClassTypes();
	static int			add_type(
						Behavior*);
	static Behavior&		GlobalType();

	//
	// NOTE: Behaviors are indexed by name, not by full name
	//
	static const map<Cstring, int>&	ClassIndices();
	static const map<Cstring, map<Cstring, int> >& ClassRealms();
private:
	static map<Cstring, int>&	WritableIndices();
	static map<Cstring, map<Cstring, int> >& WritableRealms();
public:
	static void			delete_subsystems();

	//
	// convenience method - returns sought Behavior if it exists.
	// Reminder: the index of a Behavior is its name, not its full name.
	//
	static Behavior*		find_type(
						const Cstring& the_name) {
		map<Cstring, int>::const_iterator iter = ClassIndices().find(the_name);
		if(iter != ClassIndices().end()) {
			return ClassTypes()[iter->second];
		}
		return NULL;
	}
	static Behavior*		find_type(
						const Cstring& the_realm,
						const Cstring& the_name) {
	    map<Cstring, map<Cstring, int> >::const_iterator iter1
					= ClassRealms().find(the_realm);
	    if(iter1 != ClassRealms().end()) {
		const map<Cstring, int>& the_names = iter1->second;
		map<Cstring, int>::const_iterator iter = the_names.find(the_name);
		if(iter != the_names.end()) {
		    return ClassTypes()[iter->second];
		}
	    }
	    return NULL;
	}

	//
	// A similar structure describes all types within this type,
	// analogous to nested class definitions:
	//
	vector<Behavior*>		SubclassTypes;

	// the i-th element provides the <dimension> indices of the i-th SubclassType:
	vector<vector<Cstring> >	SubclassIndices;
	// use this to compute the index of a subtype, given <dimension> strings:
	vector<map<Cstring, int> >	SubclassMaps;
	// The flat map takes a string index of the form ["a"]["b"]...["z"]:
	map<Cstring, int>		SubclassFlatMap;

	// most types need only 1 index but arrayed resources may need more:
	void				add_resource_subtypes(
						const Cstring&	base_name,
						const Cstring&	new_realm,
						unsigned int	dimension,
						int*		index_sizes,
						vector<vector<Cstring> >& index_vectors,
						vector<map<Cstring, int> >& index_maps);

	/* For each Behavior, tasks are arranged as
	 * a vector/map combination for fast access. */

				// tasks[0] is always the constructor:
	const vector<task*>&	tasks;
	map<Cstring, int>	taskindex;

	// easiest way to add a task to a type:
	virtual int		add_task(
					const Cstring&	task_name,
					bool		global = false);
	void			rename_task(
					task*		to_rename,
					const Cstring&	new_name);
	void			delete_task(
					const Cstring&	task_name);

};

//
// Notes:
//
// 	(1) tasks come in 2 flavors:
//
// 		- programs	(example: modeling section)
// 		- interfaces	(example: resource usage)
//
// 	(2) interface tasks are used for both invocation
// 	    and conclusion of a run-style task
//
// 	    NOTE: conclusion!? That may be an obsolete comment
//
// 	(3) the only intrinsic storage required for an interface
// 	    task is that required for variables passed by value
// 	    in a resource usage statement
//
class task {
public:
	task(const Cstring& n, Behavior& t, int	level);
	task(const task& m) = delete;
	virtual ~task();

	task&			operator=(const task& m);

	//
	// type this task belongs to:
	//
	Behavior&		Type;

	//
	// task name:
	//
	Cstring			name;

	//
	// tells whether this task is an script containing X commands:
	//
	bool			script_flag;

	//
	// useful to store the type of data returned by the program if the
	// latter has not yet been defined
	//
	apgen::DATA_TYPE	return_type;

	//
	// true if this is the constructor of a class
	//
	bool			constructor_flag;

	//
	// Behaving_object and task are organized in a consistent 3-level structure:
	//
	// 	level 0 - global scope
	// 	level 1 - class or function scope
	// 	level 2 - local scope
	//
	int			scope;

protected:
	//
	// given variable index, provides name and data type:
	//
	vector<pair<Cstring, apgen::DATA_TYPE> > varinfo;

	//
	// given variable name, provides address:
	//
	map<Cstring, int>	varindex;
public:

	//
	// declare accessors const to make sure clients add
	// variables via the add_variable() method
	//
	const vector<pair<Cstring, apgen::DATA_TYPE> >& get_varinfo() const {
		return varinfo;
	}
	const map<Cstring, int>& get_varindex() const {
		return varindex;
	}
	void			clear_varinfo() {
		varinfo.clear();
	}
	void			clear_varindex() {
		varindex.clear();
	}
	void	set_varinfo_to(
			const vector<pair<Cstring, apgen::DATA_TYPE> >& V) {
		varinfo = V;
	}
	void	set_varindex_to(
			const map<Cstring, int>& V) {
		varindex = V;
	}

	//
	// given array variable name, provide element type (if homogeneous -
	// otherwise, return apgen::UNINITIALIZED) and array dimension
	//
	map<Cstring, pair<apgen::DATA_TYPE, int> > homogeneoustype;

	//
	// tells the address of the i-th param.:
	//
	vector<int>		paramindex;

	//
	// run this to execute the task:
	//
	parsedProg		prog;

	//
	// to store parameter declarations, which are never executed
	// but are needed when serializing the task:
	//
	parsedProg		parameters;


	//
	// for information only - prepends realm and type:
	//
	Cstring			full_name() const {
		return Cstring(Type.full_name()) + "::" + name;
	}

	//
	// easiest way to add a variable to this task:
	//
	virtual int		add_variable(
					const Cstring& var_name,
					apgen::DATA_TYPE var_type);
	void			to_stream(
					aoString*,
					int indent) const;

    //
    // Profiling-related members and methods. Remember that
    // in the following, level[1] is a pointer to the class-level
    // object, i. e., the object that contains the 'static'
    // variables of the abstract resource class.
    //
    // level[2] refers to the object containing variables for
    // a single instance (usage) of the abstract resource, and
    // is not suitable for holding profiling information.
    //
    mutable chrono::high_resolution_clock::time_point current_wall_time;
    mutable double				time_consumed;
    mutable long int				how_many_times_called;
    void					called_once() {
	how_many_times_called++;
    }
    long int					calls() const {
	return how_many_times_called;
    }
    void					start_timer() const {
	current_wall_time = chrono::high_resolution_clock::now();
    }
    void					stop_timer() const {
	chrono::high_resolution_clock::time_point	t_now
		= chrono::high_resolution_clock::now();
	chrono::duration<double> delta_t
		= chrono::duration_cast<chrono::duration<double> >(
			t_now - current_wall_time);
	time_consumed += delta_t.count();
    }
    double					cpu_time() const {
	return time_consumed;
    }

	//
	// not very useful except to create anonymous objects:
	//
	static task&		EmptyTask();
};

//
// This class automatically updates the global object.
// NOTE: it's a de-facto singleton
//
class global_task: public task {
public:
	global_task(
		const Cstring& n,
		Behavior& t)
		: task(n, t, 0) {}
	global_task() = delete;
	global_task(const global_task& m) = delete;

	virtual ~global_task() {}

	// automatically updates the global object:
	virtual int		add_variable(
					const Cstring& var_name,
					apgen::DATA_TYPE var_type);
};

//
// This class provides a drop-in replacement for vector<StickyValue> in
// the behaving_object class. It uses a C-like array of StickValue
// pointers instead of an std::vector container.
//
// The advantage of this class is that the C-like pointers can be easily
// modified for specific symbols, such as 'now', which need to have
// thread-specific values that can be safely modified by their respective
// thread owners.
//
// Symbols that do not need to be simultaneously read and written by multiple
// threads do need to be re-allocated; global objects used by different
// threads can simply reference the same StickyValue.
//
class pseudo_vector {
public:
	pseudo_vector()
	    : valarray(NULL),
		raw_size(0),
		dimension(0) {}
	pseudo_vector(const pseudo_vector&) = delete;

	//
	// NOTE: all the destructor does is delete (free, actually)
	// valarray; it does not deallocate storage that was allocated
	// for individual array elements. That must be done by calling
	// clean_up(), which is the responsibility of the owners of
	// instances of this class.
	//
	~pseudo_vector() {
		if(valarray) {
			free((char*)valarray);
		}
	}

	void		push_back(const TypedValue&);

	unsigned int	size() const {
		return dimension;
	}

	//
	// Guarantees that the size will be at least new_size:
	//
	void		resize(unsigned int new_size, bool allocate);

	//
	// If overridden_vars is empty, this is assumed to
	// be the modeling thread's version of the globals,
	// and every variable is deleted.
	//
	// Otherwise, only those variables with an index
	// listed in the vector are deleted.
	//
	void		cleanup(const vector<int>& overridden_vars);

	//
	// The main methods of this class:
	//
	StickyValue&	operator[](unsigned int i);
	const StickyValue& operator[](unsigned int i) const;

	//
	// The data. The pseudo_vector class does not manage
	// this data itself; it relies on the object owning
	// an instance of it to do the managing.
	//
	// There are basically two types of owner-managers:
	//
	// 	- the true global object, which allocates
	// 	  memory for valarray and then allocates
	// 	  memory for each element of valarray
	//
	// 	- the global trailing object, which allocates
	// 	  memory for valarray but references the
	// 	  individual elements of the global true object,
	// 	  except for a chosen few (like 'now') for which
	// 	  it allocates memory because it wants to have
	// 	  its own location for that array element
	//
	StickyValue**	valarray;
	unsigned int	dimension;
	unsigned int	raw_size;
};

//
// Base class to encapsulate objects that support execution.
// The name is an oblique reference to the IMCE Ontology.
// 
// CHANGES REQUIRED BY MULTI_THREADING
// -----------------------------------
//
// The challenge is that we want to continue to provide efficient access
// to all adaptation data, but we must do it in such a way that individual
// threads quietly and transparently access their own definition of 'now'.
// We do this by splitting the original behaving_object class into a base
// class and two separate derived classes:
//
// 	- the base class is identical to the old behaving_object except that
// 	  the values vector has been replaced by a virtual values[] operator
// 	  which is pure virtual
//
// 	- a new behaving_object class is identical to its base class with the
// 	  one difference that it implements the values[] interface using a
// 	  std::vector<StickyValue>, exactly like the old behaving_object
// 	  class
//
//	- a new global_object class uses an instance of a new class called
//	  pseudo_vector to implement the pure virtual values[] operator.
//	  pseudo_vector stores a std::vector<StickyValue> structure
//	  internally, like the old behaving_element class, but instead of
//	  using it directly (like the concrete_object class) it stores
//	  pointers to the elements of the std::vector in a C-like array
//	  of pointers. Each thread has its own implementation of this C-like
//	  array. All pointers in it, except the one for 'now' point to the same
//	  elements of the std::vector; the value of now is specific to the
//	  thread under consideration.
//
class behaving_base {
public:

	//
	// for [res|abs]_usage_object, this
	// points to the location where args
	// passed by value are stored.
	//
	// TODO: this should be made obsolete.
	//
	static behaving_base*		temporary_storage;
	static bool			var_should_be_stored;

	typedef enum {

		//
		// These are common with ActivityInstance:
		//
		ENABLED  = 0,
		ID	 = 1,
		TYPE	 = 2,
		PARENT	 = 3,
		START	 = 4,
		FINISH	 = 5,
		SUBSYSTEM= 6
	} varlocation;


private:
	//
	// only used once per global object:
	//
	behaving_base();
public:

	//
	// Makes this a level 1, with backup level 0 = global
	//
	// (Appropriate for global functions)
	//
	behaving_base(task& T);

	//
	// Makes this a level 2, backup level 1 = P, backup level 0 = global
	//
	// (Appropriate for activity and resource methods)
	//
	behaving_base(task& T, behaving_base* P);

	behaving_base(const behaving_base&) = delete;

	virtual ~behaving_base() {
	}

	int			ref_count;
	virtual int		ref() {
		ref_count++;
		return ref_count;
	}
	virtual int		deref() {
		ref_count--;
		return ref_count;
	}

	const task&		Task;
	const Behavior&		Type;

	virtual void		undefine() {
		clear();
	}

	virtual unsigned int	size() const = 0;

	int			type_has_task_named(const Cstring& s) const {
		map<Cstring, int>::const_iterator iter = Type.taskindex.find(s);
		if(iter != Type.taskindex.end()) {
			return iter->second;
		}
		return -1;
	}

	virtual void		assign_w_side_effects(
					int			var_index,
					const TypedValue&	new_value,
					bool			notify = true) = 0;

	/* returns true if it does something worthwhile, false otherwise
	 * somewhat redundant... oh well */
	virtual bool		print(
					aoString&,
					const char* prefix) const {
		return false;
	}
	/* the following is consistent with the APIs of
	 *	Behavior
	 *	task
	 *	pEsys::pE */
	void			to_stream(
					aoString*,
					int indent) const;


	//
	// Behaving_object and task are organized in a consistent 3-level structure:
	//
	// 	level 2 - local
	// 	level 1 - class or function
	// 	level 0 - global
	//
	// The local level provides variable space for class or global functions.
	// A class function object will point its level 1 object at the class; a
	// global function object will point its level 1 object at the global object
	// (which is admittedly a little redundant since level 0 already does that.)
	//
	behaving_base*	level[3];

	virtual int		level_where_start_is_defined() const {
		return 0;
	}

	virtual Cstring		get_id() const {
		return Type.name;
	}

	//
	// In the old API this would throw if the symbol is not settable:
	//
	virtual TypedValue&	lval(
					const Cstring&) = 0;
	virtual const TypedValue& lval(
					const Cstring&) const = 0;


	virtual TypedValue&	operator[](
					const Cstring&) = 0;
	virtual const TypedValue& operator[](
					const Cstring&) const = 0;
	virtual TypedValue&	operator[](
					unsigned int) = 0;
	virtual const TypedValue& operator[](
					unsigned int) const = 0;

	virtual void		clear() = 0;

	//
	// expresses this object as an array that can be easily
	// manipulated by humans or programs.  Should be
	// complemented by a "from_struct()" method...
	//
	virtual TypedValue	to_struct() const = 0;

	//
	// serializes this object to a string.  We should
	// change this so it uses the JSON format:
	//
	const char*		serial() const;

	//
	// sometimes useful because of overloaded operators
	// in smart pointers:
	//
	behaving_base*		addressof() {
		return this;
	}

	//
	// for compatibility with the older API:
	//
	bool			defines_property(
					const Cstring& s) const;

	//
	// the 2 methods below should eventually disappear (make
	// ActivityInstance and Rsource derived classes of behaving_object):
	//
	virtual ActivityInstance* get_req() const {
		return NULL;
	}

	virtual Rsource*	get_concrete_res() const {
		return NULL;
	}

	friend class global_behaving_object;

};

class behaving_object: public behaving_base {
public:
    //
    // contains all global vars. and functions.
    // Internally, GlobalObject() consults an
    // array of global_behaving_objects and
    // returns the one with index thread_index.
    //
    static global_behaving_object*&	GlobalObject();

    //
    // Migrated here from the old AbstractResource class:
    //
    // static tlist<alpha_string, Cnode0<alpha_string, behaving_object*> >&
    // 							abstract_resources();

    //
    // To support creation of adaptation-specific objects.
    // Implemented in class_generation.C.
    //
    static behaving_object*		AbstractResFactory(task&);

    behaving_object();

    //
    // For objects that support constructors
    //
    behaving_object(task& T = task::EmptyTask());

    //
    // For objects that support methods; P is the higher-level
    // object that contains class variables:
    //
    behaving_object(task& T, behaving_base* P);
    behaving_object(const behaving_object&) = delete;

    virtual ~behaving_object() {}

    //
    // Implement pure virtual functions of behaving_base:
    //
    virtual TypedValue&		operator[](
					const Cstring&);
    virtual const TypedValue&	operator[](
					const Cstring&) const;

    virtual TypedValue&		operator[](
					unsigned int i);
    virtual const TypedValue&	operator[](
					unsigned int i) const;
    virtual TypedValue&		lval(
					const Cstring&);
    virtual const TypedValue&	lval(
					const Cstring&) const;

    virtual void		assign_w_side_effects(
					int		    var_index,
					const TypedValue&   new_value,
					bool		    notify = true);
    virtual TypedValue		to_struct() const;
    virtual unsigned int	size() const;

    //
    // undefines the values but does not delete them:
    //
    void			clear();

private:
    vector<StickyValue>	vals;
};

class global_behaving_object: public behaving_base {
public:

	//
	// only used once, for the global object of the modeling thread:
	//
	global_behaving_object();

	//
	// Used by non-modeling threads to create their
	// own version of GlobalObject:
	//
	global_behaving_object(const global_behaving_object&);

	virtual ~global_behaving_object() {
		values.cleanup(overridden_variables);
	}

	//
	// debug
	//
	void print_constant_status(int);

	//
	// Implement pure virtual functions:
	//
	virtual TypedValue&		operator[](
						const Cstring&);
	virtual const TypedValue&	operator[](
						const Cstring&) const;
	virtual TypedValue&		operator[](
						unsigned int i);
	virtual const TypedValue&	operator[](
						unsigned int i) const;
	virtual TypedValue&		lval(
						const Cstring&);
	virtual const TypedValue&	lval(
						const Cstring&) const;

	//
	// overridden for activity instances:
	//
	virtual void			assign_w_side_effects(
						int			var_index,
						const TypedValue&	new_value,
						bool			notify = true);
	virtual TypedValue		to_struct() const;
	virtual unsigned int		size() const;

	//
	// undefines the values but does not delete them:
	//
	void				clear();

	//
	// The following two statics are for holding the names and
	// content of values that are specific for a specific thread:
	//
	static vector<Cstring>&		trailing_data_names();

	//
	// Holds the names of constant globals:
	//
	static set<Cstring>&		constants();

	//
	// stores the indices of values that were actually
	// overridden in a trailing thread's globals:
	//
	vector<int>			overridden_variables;

protected:
	pseudo_vector			values;

	friend class globalData;
	friend class time_saver;
	friend class global_task;
};

#endif /* _BEHAVING_ELEMENT_H_ */
