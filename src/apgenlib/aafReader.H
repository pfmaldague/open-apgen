#ifndef _AAF_INTFC_H_
#define _AAF_INTFC_H_

#include <sstream>

#include "ParsedExpressionSystem.H"
#include "CompilerIntfc.H"

namespace pEsys {
class temporal_spec;
}

namespace aafReader {

std::ostringstream*& IllegalChars();

typedef enum {
	DeclarationPass,
	ImplementationPass
} ConsolidationPass;

typedef enum {
	NotAConditional,
	IfAlone,
	ElseIf,
	ElseAlone
} ConditionalFlag;

ConsolidationPass&	CurrentPass();

string make_space(int n);

//
// These vectors are containers for the top-level parsed objects.
// They are initially put into input_files(), then into
// consolidated_files() after they've been consolidated.
//
vector<parsedExp>&	input_files();
vector<parsedExp>&	consolidated_files();

//
// For AAF typedefs, which turn specific struct-style arrays
// into new data types.
//
stringtlist&		typedefs();

//
// For AAF-defined functions; this list is cumulative. It is
// not cleared when a new file is opened for parsing.
//
// functions() contains names of functions that have been
// declared, defined or both. defined_functions() only
// contains names of functions that have been defined. A function
// can only be defined once.
//
stringtlist&		functions();
stringtlist&		defined_functions();

//
// A trivial class to hold 6 doubles from the Spice toolkit
//
class state6 {
public:
    state6() = default;
    state6(const state6&) = default;
    ~state6() = default;

    double	s[6];
};


//
// A trivial class to hold 2 doubles for precomputed
// Geometry resources
//
class state2 {
public:
    state2() = default;
    state2(const state2&) = default;
    ~state2() = default;

    double	s[2];
};

typedef tlist<alpha_time, Cntnr<alpha_time, state2> > state_series;

//
// The precomputed resource class. It contains everything
// that is necessary to evaluate the interpolation formula.
// The index of an arrayed precomputed resource is the same
// as in the original adaptation. If the precomputed resource
// is not an array, its index is arbitrary; the container
// only contains one resource.
//
class single_precomp_res: public baseC<alpha_string, single_precomp_res> {
public:
    single_precomp_res() : baseC<alpha_string, single_precomp_res>() {}
    single_precomp_res(const Cstring& s)
	: baseC<alpha_string, single_precomp_res>(s) {}
    single_precomp_res(const single_precomp_res& R)
	: baseC<alpha_string, single_precomp_res>(R),
		payload(R.payload) {}
    ~single_precomp_res() = default;

    state_series		payload;

    single_precomp_res*	get_this() {
	return this;
    }
    const single_precomp_res*	get_this() const {
	return this;
    }
    single_precomp_res*	copy() {
	return new single_precomp_res(*this);
    }

    static single_precomp_res* UnderConsolidation;

    //
    // Provides easy-to-call access to the computation
    // of a precomputed resource:
    //
    TypedValue			Evaluate(
			const TypedValue& time_argument,
			double		  result[]);
};

//
// An arrayed precomputed container is a list of precomputed
// resources indexed the same way as arrayed resources in
// the adaptation.
//
typedef tlist<alpha_string, single_precomp_res>	precomp_res_list;
typedef Cntnr<alpha_string, precomp_res_list>	precomp_container;

//
// Precomputed containers are indexed by the name of the
// (possibly arrayed) Geometry resources in the adaptation.
// This list is used only while parsing. During consolidation,
// precomputed resources are built as settable resources with
// a special list (the time series) that provides fast
// evaluation using cubic interpolation.
//
tlist<alpha_string, precomp_container>&		precomp_containers();

//
// For internal functions; it is initialized once and for all.
//
stringtlist&		internal_functions();

//
// For methods defined in AAFs. This list is cumulative, just
// like functions().
//
tlist<alpha_string, Cntnr<alpha_string, apgen::METHOD_TYPE> >&	methods();

pairtlist&		global_items();
slist<alpha_string, bsymbolnode>& directives();

//
// This list stores activity types defined so far. It is the parsing
// counterpart of the Types stored in the behavior subsystem. The
// behavior subsystem contains the functional objects; the ones held
// here are only strings meant to assist the parser before consolidation
// has taken place.
//
stringtlist&		activity_types();

pairtlist&		globals_used_as_indices_in_resources();
pairtlist&		assignments_to_global_arrays();

stringtlist&		resources();
Cstring&		current_resource();

tlist < alpha_string,
	Cntnr<
		alpha_string,
		smart_ptr<pEsys::FunctionDefinition>
	     >
      >& functions_declared_but_not_implemented();

void			readAdaptationFile(
				compiler_intfc::source_type,
				const char*);

Cstring&		current_file();

Cstring&		TaskForStoringArgsPassedByValue();
extern bool		AVariableWasPassedByValue;

//
// various counts:
//
extern long		ActivityTypeCount;
extern long		EpochAndTimeSystemCount;
extern long	 	FunctionCount;
extern long	 	ConcreteResourceCount;
extern long	 	AbstractResourceCount;
extern long	 	ConstraintCount;
extern long		ActivityInstanceCount;

extern bool		file_has_adaptation();

			/* the nickname is a pair (varname, flag)
			 * where flag = 0 for activity attributes,
			 * 1 for resource attributes */
extern map<Cstring, pair<Cstring, int> >& string_to_nickname();
extern map<Cstring, Cstring>& nickname_to_activity_string();
extern map<Cstring, Cstring>& nickname_to_resource_string();
extern map<Cstring, apgen::DATA_TYPE>& nickname_to_activity_data_type();
extern map<Cstring, apgen::DATA_TYPE (*)(apgen::DATA_TYPE)>& nickname_to_resource_data_type();

extern void initialize_counts_and_behaviors();
extern void delete_global_objects();

			//
			// data and functions adapted from the old Syntax class
			//

extern task**		CurrentTasks;
extern int		LevelOfCurrentTask;
extern int		CurrentTaskSize;
void			push_current_task(task* T);
task*			get_current_task();
task*			get_task(int i);
task*			pop_current_task();
Behavior&		CurrentType();

			//
			// helper methods
			//


bool			array_is_homogeneous(
					parsedExp& Expression,
					apgen::DATA_TYPE& element_type,
					int& array_size,
					int& dimensions,
					int dent);

void			add_a_custom_type(
					const Cstring& name,
					apgen::DATA_TYPE dt);

bool			find_symbol_in_task( 
					const Cstring&		symbol_name,
					int&			level_to_use,
					int&			index_to_use,
					apgen::DATA_TYPE&	dt);
					

			//
			// consolidation methods
			//

extern void		consolidate(bool skip_remodeling = false);

extern void		consolidate_assignment(
				smart_ptr<pEsys::executableExp>& at,
				int dent = 0,
				bool inside_directive = false);

extern void		consolidate(
				pEsys::DataTypeDefaultValue& ca,
				int dent = 0);
extern void		consolidate_kw_pairs(
				pEsys::Assignment& a,
				vector<parsedExp>& keys,
				vector<parsedExp>& vals,
				int dent = 0);
extern void		consolidate_function_call_exp(
				parsedExp& E,
				int dent = 0);
extern void		consolidate_function_call_exe(
				smart_ptr<pEsys::executableExp>& E,
				int dent = 0);
extern void		consolidate_if(
				smart_ptr<pEsys::executableExp>& E,
				apgen::DATA_TYPE& return_type,
				apgen::METHOD_TYPE method_type,
				int dent = 0);

extern void		consolidate(
				pEsys::InputFile& iF,
				int dent = 0);
extern void		consolidate_multidim_indices(
				smart_ptr<pEsys::MultidimIndices>& mi_ptr,
				int dent = 0);
extern void		consolidate(
				pEsys::Parentheses& pc,
				int dent = 0);
extern void		consolidate_program(
				pEsys::Program& pc,

				//
				// get this from pc.orig_section:
				//
				// apgen::METHOD_TYPE whichSection,
				int dent = 0);
extern void		consolidate_qualified_symbol(
				parsedExp& pe,
				int dent = 0,
				bool is_assigned_to = false);
extern void		consolidate(
				pEsys::Resource& r,
				int dent = 0);
extern void		consolidate_return(
				pEsys::Return& r,
				apgen::DATA_TYPE& dt,
				int dent = 0);

			//
			// The purpose of cond_flag is to allow the
			// program consolidator to track the sequences
			// of if / else if / else statements and make
			// sure they are occurring in the proper sequence.
			//
extern void		consolidate_statement(
				smart_ptr<pEsys::executableExp>& stmt,
				apgen::DATA_TYPE&	dt,
				apgen::METHOD_TYPE	method_type,
				ConditionalFlag&	cond_flag,
				bool&			is_return,
				int dent = 0);
extern void		consolidate_symbol(
				parsedExp& pe,
				int dent = 0,
				bool is_assigned_to = false);
extern void		consolidate_temporal_spec(
				smart_ptr<pEsys::temporal_spec>& ts,
				parsedExp& timeExpression,
				int dent = 0);
extern void		consolidate(
				pEsys::Typedef& t,
				int dent = 0);
extern void		consolidate_continue(
				pEsys::Continue&,
				int dent = 0);
extern void		consolidate_while(
				smart_ptr<pEsys::executableExp>& E,
				apgen::DATA_TYPE& dt,
				apgen::METHOD_TYPE method_type,
				int dent = 0);

			// a non-default value for the last argument
			// will be used to set the type of the expression 
extern void		consolidate_expression(
				parsedExp& E,
				int dent = 0,
				apgen::DATA_TYPE = apgen::DATA_TYPE::UNINITIALIZED);

extern void consolidate_resource_dependencies();
extern void consolidate_constraints_round_two();

} // end namespace aafReader

#endif /* _AAF_INTFC_H_ */
