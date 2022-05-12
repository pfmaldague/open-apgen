#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <sys/stat.h>

#include "lex_intfc.H"
#include "aafReader.H"
#include "BehavingElement.H"
#include "apcoreWaiter.H"
#include "fileReader.H"
#include "ActivityInstance.H"

extern int abdebug;
extern int yylineno;

namespace aafReader {

ostringstream*&		IllegalChars() {
	static ostringstream* ill = NULL;
	return ill;
}

ConsolidationPass&	CurrentPass() {
	static ConsolidationPass p = DeclarationPass;
	return p;
}

vector<parsedExp>&	input_files() {
	static vector<parsedExp> V;
	return V;
}

vector<parsedExp>&	consolidated_files() {
	static vector<parsedExp> V;
	return V;
}

stringtlist&		typedefs() {
	static stringtlist	T;
	return T;
}

pairtlist&		globals_used_as_indices_in_resources() {
	static pairtlist	T;
	return T;
}

pairtlist&		assignments_to_global_arrays() {
	static pairtlist	A;
	return A;
}

stringtlist&		functions() {
	static stringtlist	T;
	return T;
}

stringtlist&		defined_functions() {
	static stringtlist	T;
	return T;
}

tlist<alpha_string, precomp_container>& precomp_containers() {
    static tlist<alpha_string, precomp_container> T;
    return T;
}

single_precomp_res*	single_precomp_res::UnderConsolidation = NULL;

tlist<alpha_string, Cntnr<alpha_string, apgen::METHOD_TYPE> >&		methods() {
	static tlist<alpha_string, Cntnr<alpha_string, apgen::METHOD_TYPE> >	T;
	return T;
}

stringtlist&		internal_functions() {
	static stringtlist	T;
	return T;
}

pairtlist&		global_items() {
	static pairtlist	P;
	return P;
}

slist<alpha_string, bsymbolnode>& directives() {
	static slist<alpha_string, bsymbolnode> S;
	return S;
}

stringtlist& activity_types() {
	static stringtlist at;
	return at;
}

stringtlist& resources() {
	static stringtlist 	s;
	return s;
}

Cstring&	current_resource() {
	static Cstring s;
	return s;
}

tlist<
	alpha_string,
	Cntnr<
		alpha_string,
		smart_ptr<pEsys::FunctionDefinition>
	      >
     >& functions_declared_but_not_implemented() {
	static tlist<
			alpha_string,
			Cntnr<
				alpha_string,
				smart_ptr<pEsys::FunctionDefinition>
			      >
		    > ST;
	return ST;
}

Cstring&		current_file() {
	static Cstring c;
	return c;
}

Cstring&		TaskForStoringArgsPassedByValue() {
	static Cstring t;
	return t;
}

long		ActivityTypeCount = 0;
long	 	FunctionCount = 0;
long		EpochAndTimesystemCount = 0;
long	 	ConcreteResourceCount = 0;
long	 	ConstraintCount = 0;
long	 	AbstractResourceCount = 0;
long		ActivityInstanceCount = 0;

bool		AVariableWasPassedByValue = false;

bool		file_has_adaptation() {
	 return ActivityTypeCount != 0
		 || FunctionCount != 0
		 || ConcreteResourceCount != 0
		 || ConstraintCount != 0
		 || AbstractResourceCount != 0
		 || ConstraintCount != 0;
}

task**			CurrentTasks = NULL;
int			LevelOfCurrentTask = -1;
int			CurrentTaskSize = 0;

void	push_current_task(
		task* T) {
	if(!CurrentTasks) {
		CurrentTasks = (task**) malloc(sizeof(task*) * 20);
		CurrentTaskSize = 20;
		LevelOfCurrentTask = -1;
	}
	assert(LevelOfCurrentTask < 2);
	if(++LevelOfCurrentTask + 1 >= CurrentTaskSize) {
		CurrentTaskSize += 20;
		CurrentTasks = (task**) realloc(CurrentTasks, sizeof(task*) * CurrentTaskSize);
	}
	CurrentTasks[LevelOfCurrentTask] = T;
	if(APcloptions::theCmdLineOptions().debug_grammar) {
		cerr << "pushing task->" << LevelOfCurrentTask << " " << T->full_name() << "\n";
	}
}

task*	get_current_task() {
	if(!CurrentTasks) {
		push_current_task(&Behavior::GlobalConstructor());
	}
	assert(LevelOfCurrentTask >= 0 && LevelOfCurrentTask <= 2);
	return CurrentTasks[LevelOfCurrentTask];
}

task*	get_task(int i) {
	if(i <= LevelOfCurrentTask) {
		return CurrentTasks[i];
	}
	return NULL;
}

task*	pop_current_task() {
	if(LevelOfCurrentTask >= 0) {
		if(APcloptions::theCmdLineOptions().debug_grammar) {
			cerr << "popping task->" << (LevelOfCurrentTask - 1) << " "
				<< CurrentTasks[LevelOfCurrentTask]->full_name() << "\n";
		}
		return CurrentTasks[LevelOfCurrentTask--];
	}
	return NULL;
}

Behavior& CurrentType() {
	assert(get_current_task());
	return get_current_task()->Type;
}

//
// TO ADD A NEW RESOURCE ATTRIBUTE TO THE LIST
// ===========================================
//
// 1. how the attribute is specified in the AAF
// --------------------------------------------
//
// The way attributes are defined in the AAF are through a statement in the
// attributes section of the resource definition.  This statement has the form
//
// 	"Nice Name" = <attribute value>;
//
// where "Nice Name" a any constant string that expresses the attribute name
// in human-readable form; it can have embedded special characters such as
// spaces, ampersands, etc. (as long as they are properly escaped if
// necessary).
//
// An attribute must have a well-defined type, which is either a fixed type
// or the same type as the resource.
//
// When an attribute is referenced in the adaptation code, it is often not
// possible to use the "nice name" e. g. because it contains a space or an
// ampersand and is therefore not suitable as a C or C++ variable name. So, a
// "nickname" is defined for each attribute. The rules for choosing nicknames
// are that
//
// 	1. they should be unique - you can't pick one that already exists
//
// 	2. they should be suitable as variable names in C, e. g. start with a
// 	   letter and contain only letters, numbers and underscores ('_')
//
// 2. how the attribute is programmed into APGenX
// ----------------------------------------------
//
// 1. decide on a "nice name" which will appear in the attributes section of
//    the resource.
//
// 2. decide on a "nickname" for your attribute.
//
// 2. decide whether the type of the attribute will be constant (e. g.
//    duration) or the same as the resource (as is the case e. g. for
//    "Minimum".)
//
// 3. add a line in the string_to_nickname() map. The left-hand side should
//    insert the string that defines the "nice name" of your attribute into the
//    map called N.  The right-hand side should be a pair whose first element is
//    your chosen nickname for the attribute, i. e., a variable name suitable
//    for use in C or C++. The second element of the pair should be 0 if the
//    attribute is intended for an activity, 1 if it is intended for a resource
//    and 2 if it is intended for both. Here was assume that the attribute is
//    for use with resources.
//
// 4. add a line in the nickname_to_resource_string() map. The left-hand side
//    should insert the nickname of your attribute into the map called A, and
//    the right-hand side should be the (doubly quoted!) string that represents
//    the "nice name" of your attribute.
//
// 5. add a line in the nickname_to_resource_data_type map. The left-hand side
//    should insert the nickname of your attribute into the map called R. The
//    right-hand side should be a pointer to a function that is either
//    type_is_same_as_resource or the template type_is_constant instantiated
//    with a data type which is the chosen constant data type for your
//    attribute, for example apgen::STRING if you want the attribute to be a
//    string.
//
// This will handle the logistics of attaching your attribute to resources
// whose AAF definition features it in their attributes section. Now, you have
// to implement the semantics of your attribute. Many attributes govern the
// way attributes are used by "use", "set" or "reset" statements in the AAF
// code. If your attribute is of this type, you will have to modify the files
// ResourceValue.C and/or ResourceUsage.C in the apcore/resources directory,
// and possibly one of the resource header files:
//
// 	Rsource.H
// 	RES_def.H
//

map<Cstring, pair<Cstring, int> >& string_to_nickname() {
	static bool initialized = false;
	static map<Cstring, pair<Cstring, int> > N;

	if(!initialized) {
		initialized = true;

		//
		// Activities only
		//
		N["\"Allowed Creators\""] = pair<Cstring, int>("allowed_creators",	0);
		N["\"Color\""] = pair<Cstring, int>("Color",				0);
		N["\"Decomposition Suffix\""] = pair<Cstring, int>("decomp_suffix",	0);
		N["\"Duration\""] = pair<Cstring, int>("span",				0);
		N["\"Label\""] = pair<Cstring, int>("label",				0);
		N["\"Legend\""] = pair<Cstring, int>("legend",				0);
		N["\"Misc.\""] = pair<Cstring, int>("misc",				0);
		N["\"Pattern\""] = pair<Cstring, int>("pattern",			0);
		N["\"Plan\""] = pair<Cstring, int>("plan",				0);
		N["\"SASF\""] = pair<Cstring, int>("sasf",				0);
		N["\"Start\""] = pair<Cstring, int>("start",				0);
		N["\"Status\""] = pair<Cstring, int>("status",				0);
		N["\"Undocumented\""] = pair<Cstring, int>("undocumented",		0);

		//
		// Both
		//
		N["\"Description\""] = pair<Cstring, int>("description",		2);
		N["\"Subsystem\""] = pair<Cstring, int>("subsystem",			2);

		//
		// Resources only
		//
		N["\"Allowed Users\""] = pair<Cstring, int>("allowed_users",		1);
		N["\"Auxiliary\""] = pair<Cstring, int>("auxiliary",			1);
		N["\"Error High\""] = pair<Cstring, int>("errorhigh",			1);
		N["\"Error Low\""] = pair<Cstring, int>("errorlow",			1);
		N["\"Hidden\""] = pair<Cstring, int>("hidden",				1);
		N["\"Interpolation\""] = pair<Cstring, int>("interpolation",		1);
		N["\"Maximum\""] = pair<Cstring, int>("maximum",			1);
		N["\"Minimum\""] = pair<Cstring, int>("minimum",			1);
		N["\"Min Abs Delta\""] = pair<Cstring, int>("min_abs_delta",		1);
		N["\"Min Rel Delta\""] = pair<Cstring, int>("min_rel_delta",		1);
		N["\"ModelingOnly\""] = pair<Cstring, int>("modeling_only",		1);
		N["\"Multiplier\""] = pair<Cstring, int>("multiplier",			1);
		N["\"Resolution\""] = pair<Cstring, int>("resolution",			1);
		N["\"Units\""] = pair<Cstring, int>("units",				1);
		N["\"Subsystem\""] = pair<Cstring, int>("subsystem",			1);
		N["\"Warning High\""] = pair<Cstring, int>("warninghigh",		1);
		N["\"Warning Low\""] = pair<Cstring, int>("warninglow",			1);
		N["\"No Filtering\""] = pair<Cstring, int>("nofiltering",		1);
	}
	return N;
}

map<Cstring, Cstring>& nickname_to_activity_string() {
	static bool initialized = false;
	static map<Cstring, Cstring> A;
	if(!initialized) {
		initialized = true;
		A["allowed_creators"] = "\"Allowed Creators\"";
		A["Color"] = "\"Color\"";
		A["decomp_suffix"] = "\"Decomposition Suffix\"";
		A["description"] = "\"Description\"";
		A["span"] = "\"Duration\"";
		A["label"] = "\"Label\"";
		A["legend"] = "\"Legend\"";
		A["misc"] = "\"Misc.\"";
		A["pattern"] = "\"Pattern\"";
		A["plan"] = "\"Plan\"";
		A["sasf"] = "\"SASF\"";
		A["start"] = "\"Start\"";
		A["status"] = "\"Status\"";
		A["subsystem"] = "\"Subsystem\"";
		A["undocumented"] = "\"Undocumented\"";
	}
	return A;
}

map<Cstring, apgen::DATA_TYPE>& nickname_to_activity_data_type() {
	static bool initialized = false;
	static map<Cstring, apgen::DATA_TYPE> A;
	if(!initialized) {
		initialized = true;
		A["allowed_creators"]	= apgen::DATA_TYPE::ARRAY;
		A["Color"]		= apgen::DATA_TYPE::STRING;
		A["decomp_suffix"]	= apgen::DATA_TYPE::STRING;
		A["description"]	= apgen::DATA_TYPE::STRING;
		A["span"]		= apgen::DATA_TYPE::DURATION;
		A["label"]		= apgen::DATA_TYPE::STRING;
		A["legend"]		= apgen::DATA_TYPE::STRING;
		A["misc"]		= apgen::DATA_TYPE::ARRAY;
		A["pattern"]		= apgen::DATA_TYPE::INTEGER;
		A["plan"]		= apgen::DATA_TYPE::STRING;
		A["sasf"]		= apgen::DATA_TYPE::ARRAY;
		A["start"]		= apgen::DATA_TYPE::TIME;
		A["status"]		= apgen::DATA_TYPE::BOOL_TYPE;
		A["subsystem"]		= apgen::DATA_TYPE::STRING;
		A["undocumented"]	= apgen::DATA_TYPE::STRING;
	}
	return A;
}

map<Cstring, Cstring>& nickname_to_resource_string() {
	static bool initialized = false;
	static map<Cstring, Cstring> R;
	if(!initialized) {
		R["allowed_users"] = "\"Allowed Users\"";
		R["auxiliary"] = "\"Auxiliary\"";
		R["description"] = "\"Description\"";
		R["errorhigh"] = "\"Error High\"";
		R["errorlow"] = "\"Error Low\"";
		R["hidden"] = "\"Hidden\"";
		R["interpolation"] = "\"Interpolation\"";
		R["maximum"] = "\"Maximum\"";
		R["minimum"] = "\"Minimum\"";
		R["min_abs_delta"] = "\"Min Abs Delta\"";
		R["min_rel_delta"] = "\"Min Rel Delta\"";
		R["modeling_only"] = "\"ModelingOnly\"";
		R["multiplier"] = "\"Multiplier\"";
		R["nofiltering"] = "\"No Filtering\"";
		R["resolution"] = "\"Resolution\"";
		R["units"] = "\"Units\"";
		R["subsystem"] = "\"Subsystem\"";
		R["warninghigh"] = "\"Warning High\"";
		R["warninglow"] = "\"Warning Low\"";
	}
	return R;
}

template<apgen::DATA_TYPE dt>
apgen::DATA_TYPE type_is_constant(
			apgen::DATA_TYPE res_type) {
	return dt;
}

apgen::DATA_TYPE type_is_same_as_resource(
			apgen::DATA_TYPE res_type) {
	return res_type;
}

apgen::DATA_TYPE type_is_same_as_resource_except_for_time(
			apgen::DATA_TYPE res_type) {
	if(res_type == apgen::DATA_TYPE::TIME) {
		return apgen::DATA_TYPE::DURATION;
	}
	return res_type;
}

map<Cstring, apgen::DATA_TYPE (*)(apgen::DATA_TYPE)>& nickname_to_resource_data_type() {
	static bool initialized = false;
	static map<Cstring, apgen::DATA_TYPE (*)(apgen::DATA_TYPE)> R;
	if(!initialized) {
		R["allowed_users"]	= &type_is_constant<apgen::DATA_TYPE::ARRAY>;
		R["auxiliary"]		= &type_is_constant<apgen::DATA_TYPE::ARRAY>;
		R["description"]	= &type_is_constant<apgen::DATA_TYPE::STRING>;
		R["errorhigh"]		= &type_is_same_as_resource_except_for_time;
		R["errorlow"]		= &type_is_same_as_resource_except_for_time;
		R["hidden"]		= &type_is_constant<apgen::DATA_TYPE::BOOL_TYPE>;
		R["interpolation"]	= &type_is_constant<apgen::DATA_TYPE::BOOL_TYPE>;
		R["maximum"]		= &type_is_same_as_resource;
		R["minimum"]		= &type_is_same_as_resource;
		R["min_abs_delta"]	= &type_is_same_as_resource_except_for_time;
		R["min_rel_delta"]	= &type_is_constant<apgen::DATA_TYPE::FLOATING>;
		R["modeling_only"]	= &type_is_constant<apgen::DATA_TYPE::BOOL_TYPE>;
		R["multiplier"]		= &type_is_constant<apgen::DATA_TYPE::FLOATING>;
		R["nofiltering"]	= &type_is_constant<apgen::DATA_TYPE::BOOL_TYPE>;
		R["resolution"]		= &type_is_constant<apgen::DATA_TYPE::DURATION>;
		R["units"]		= &type_is_constant<apgen::DATA_TYPE::STRING>;
		R["subsystem"]		= &type_is_constant<apgen::DATA_TYPE::STRING>;
		R["warninghigh"]	= &type_is_same_as_resource_except_for_time;
		R["warninglow"]		= &type_is_same_as_resource_except_for_time;
	}
	return R;
}

void add_a_custom_type(const Cstring& name, apgen::DATA_TYPE dt) {
	map<Cstring, Cstring>::iterator iter1 = nickname_to_activity_string().find(name);
	if(iter1 != nickname_to_activity_string().end()) {
		// already handled (not an error)
		return;
	}
	Cstring quoted = addQuotes(name);
	pair<Cstring, int> the_pair(name, 2);
	string_to_nickname()[quoted] = the_pair;
	nickname_to_activity_string()[name] = quoted;
	nickname_to_activity_data_type()[name] = dt;
	nickname_to_resource_string()[name] = quoted;
	switch(dt) {
		case apgen::DATA_TYPE::ARRAY:
			nickname_to_resource_data_type()[name]
				= &type_is_constant<apgen::DATA_TYPE::ARRAY>;
			break;
		case apgen::DATA_TYPE::BOOL_TYPE:
			nickname_to_resource_data_type()[name]
				= &type_is_constant<apgen::DATA_TYPE::BOOL_TYPE>;
			break;
		case apgen::DATA_TYPE::DURATION:
			nickname_to_resource_data_type()[name]
				= &type_is_constant<apgen::DATA_TYPE::DURATION>;
			break;
		case apgen::DATA_TYPE::FLOATING:
			nickname_to_resource_data_type()[name]
				= &type_is_constant<apgen::DATA_TYPE::FLOATING>;
			break;
		case apgen::DATA_TYPE::INSTANCE:
			nickname_to_resource_data_type()[name]
				= &type_is_constant<apgen::DATA_TYPE::INSTANCE>;
			break;
		case apgen::DATA_TYPE::INTEGER:
			nickname_to_resource_data_type()[name]
				= &type_is_constant<apgen::DATA_TYPE::INTEGER>;
			break;
		case apgen::DATA_TYPE::STRING:
			nickname_to_resource_data_type()[name]
				= &type_is_constant<apgen::DATA_TYPE::STRING>;
			break;
		case apgen::DATA_TYPE::TIME:
			nickname_to_resource_data_type()[name]
				= &type_is_constant<apgen::DATA_TYPE::TIME>;
			break;
		case apgen::DATA_TYPE::UNINITIALIZED:
			assert(false);
	}
	map<Cstring, int>::const_iterator iter = Behavior::ClassIndices().find("generic");
	Behavior* generic_act = Behavior::ClassTypes()[iter->second];
	task*	constructor = generic_act->tasks[0];
	constructor->add_variable(name, dt);
}

void	initialize_counts_and_behaviors() {

	//
	// The first step of Behavior initialization was already
	// performed by Behavior::initialize(), called by
	// ACT_exec::create_subsystems().  In that step,
	// the first item in the BehaviorClassTypes() vector
	// was set to the (newly created) Global Behavior.
	//
	// It is convenient to have a readily available
	// handler on the constructor for this top-level
	// object; that handle is Behavior::GlobalConstructor().
	// We push it onto the stack of tasks.
	//
	LevelOfCurrentTask = -1;
	push_current_task(&Behavior::GlobalConstructor());

	//
	// set counts:
	//
	ActivityInstanceCount = 0;
	ActivityTypeCount = 0;
	ConcreteResourceCount = 0;
	ConstraintCount = 0;
	AbstractResourceCount = 0;
	ConstraintCount = 0;
	FunctionCount = 0;
	EpochAndTimesystemCount = 0;

	map<Cstring, int>::const_iterator	wait_iter
			= Behavior::ClassIndices().find("get windows");
	if(wait_iter == Behavior::ClassIndices().end()) {

		//
		// need to initialize the "complex wait" types
		//
		Behavior::add_type(new Behavior(
					"wait for <time or duration>",
					"modeling",
					NULL));
		Behavior::add_type(new Behavior(
					"wait until <condition or signal>",
					"modeling",
					NULL));
		Behavior::add_type(new Behavior(
					"wait until <regular expression>",
					"modeling",
					NULL));
		Behavior::add_type(new Behavior(
					"get windows",
					"modeling",
					NULL));
		Behavior::add_type(new Behavior(
					"get interpolated windows",
					"modeling",
					NULL));
	}
	if(APcloptions::theCmdLineOptions().debug_grammar) {
		abdebug = 1;
		pEsys::Symbol::debug_symbol = true;
  		cerr << "initialize_counts_and_behaviors() recap: Class Types\n";
		for(int i = 0; i < Behavior::ClassTypes().size(); i++) {
			aoString aos1;
			Behavior::ClassTypes()[i]->to_stream(&aos1, 4);
			cerr << "  type[" << i << "] =\n" << aos1.str() << "\n";
  		}
	}

	directives().clear();
}

//
// Invoked by delete_subsystems() in ACT_exec.C:
//
void delete_global_objects() {
	input_files().clear();
	consolidated_files().clear();
	typedefs().clear();
	internal_functions().clear();
	global_items().clear();
	directives().clear();
	activity_types().clear();
	globals_used_as_indices_in_resources().clear();
	resources().clear();
	functions_declared_but_not_implemented().clear();
	string_to_nickname().clear();
	nickname_to_activity_string().clear();
	nickname_to_resource_string().clear();
	nickname_to_activity_data_type().clear();
	nickname_to_resource_data_type().clear();
}


void readAdaptationFile(
		compiler_intfc::source_type	InputType,
		const char*			InputString) {
	char*	theString = NULL;
	const char* theConstString;
	if(InputType == compiler_intfc::FILE_NAME) {
	    FILE*		f = fopen(InputString, "r");
	    struct stat	FileStats;

	    current_file() = InputString;
	    int ret_from_stat = stat(InputString, &FileStats);
	    if(ret_from_stat) {
			Cstring errs;
			errs << "Cannot open file " << InputString;
			throw(eval_error(errs));
	    }
	    long		contentLength = FileStats.st_size;
	    theString = new char[contentLength + 1];
	    theString[contentLength] = '\0';
	    fread(theString, 1, contentLength, f);
	    fclose(f);
	    theConstString = theString;

	    //
	    // Detect "non-standard characters"
	    //
	    const char* c = theConstString;
	    const char* begin_line = c;
	    unsigned int lines = 1;
	    bool	line_has_been_flagged = false;

	    if(!IllegalChars()) {
		IllegalChars() = new ostringstream;
	    }
	    ostringstream& st = *IllegalChars();
	    while(*c) {
		unsigned char d = *c;
		if(d == '\n') {
		    lines++;
		    begin_line = c + 1;
		    line_has_been_flagged = false;
		} else {
		    bool first = true;
		    vector<unsigned char> illegals;

		    //
		    // (*) see later comment
		    //
		    while(
			(d > 127
			|| (d < 32 && d != '\t' && d != '\n' && d != '\r'))
			&& !line_has_been_flagged) {

			//
			// c is outside of the "restricted ASCII set" (see JIRA AP-883)
			//
			if(first) {
			    first = false;
			    line_has_been_flagged = true;
			    st << "File " << InputString << ", line " << lines << ":\n\t";
			    for(const char* e = begin_line; e < c; e++) {
			    	st << *e;
			    }
			    // st << " -->'" << d << "' (ASCII " << ((unsigned int)d) << ")<--\n";
			}
			illegals.push_back(d);
			c++;
			d = *c;
		    }
		    if(line_has_been_flagged) {

			//
			// We have caught illegals.size() bad characters
			//
			st << " -->'";
			char* nonprintable = (char*) malloc(illegals.size() + 1);
			for(int illeg = 0; illeg < illegals.size(); illeg++) {
			    nonprintable[illeg] = illegals[illeg];
			}
			nonprintable[illegals.size()] = '\0';
			st << nonprintable;
			free(nonprintable);
			st << "' (ASCII ";
			for(int illeg = 0; illeg < illegals.size(); illeg++) {
			    st << "<" << ((unsigned int)illegals[illeg]) << ">";
			}
			st << ")<--\n";

			//
			// The first check takes care of files that do not end
			// with a newline character
			//
			while(*(c + 1) && *(c + 1) != '\n') {
			    c++;
		    	}

			//
			// c now points to the character just before the newline
			//
		    } else {

			//
			// Nothing happened; c is still what it used to be before (*)
			//
		    }
		}

		//
		// If *c was a regular ASCII character, nothing happened to c
		//
		// If *c was the beginning of a non-ASCII sequence, a complete
		// error message has been appended to IllegalChars(), including
		// the offending sequence; c points to the character just before
		// the newline that terminates the current line
		//
		c++;
	    }
	} else {
	    theConstString = InputString;
	}


	yy_lex_buffers_initialize_to(theConstString);
	if(theString) {
		delete [] theString;
	}

	fileReader::theErrorStream().str("");
	ab_initialize_tokens();
	yylineno = 1;

	try {
		abparse();
	} catch(eval_error Err) {
		yy_cleanup_tokens_buffer();
		throw(Err);
	}
	string errs = fileReader::theErrorStream().str();
	yy_cleanup_tokens_buffer();
	if(errs.size()) {
		throw(eval_error(errs.c_str()));
	}
}

TypedValue single_precomp_res::Evaluate(
		const TypedValue&	time_argument,
		double			result[]) {
    TypedValue	val;
    CTime_base	time_arg = time_argument.get_time_or_duration();

    //
    // We have to extract the right time interval and compute
    // the interpolation formula.
    //
    aafReader::state_series&	   sampling_list = payload;

    //
    // Declare error if the time arg is before the start of the list
    // or if the time arg is after the end of the list.
    //
    // Well... no. It turns out that APGenX will request early or
    // late values of resources that may be outside any reasonable
    // range, simply because there is an activity there. So, we
    // just provide the earliest/latest value available.
    //
    double	*state_1, *state_2;
    Cntnr<alpha_time, aafReader::state2>* first_endpoint = NULL;

    //
    // Handle out-of-range cases first
    //
    if(time_arg < sampling_list.first_node()->getKey().getetime()) {
	first_endpoint = sampling_list.first_node();
	state_1 = first_endpoint->payload.s;
	result[0] = state_1[0];
	result[1] = state_1[1];
	val = result[0];
	return val;
    } else if(time_arg > sampling_list.last_node()->getKey().getetime()) {
	first_endpoint = sampling_list.last_node();
	state_1 = first_endpoint->payload.s;
	result[0] = state_1[0];
	result[1] = state_1[1];
	val = result[0];
	return val;
    } else if(!sampling_list.get_length()) {
	Cstring err;
	err << ": time " << time_arg.to_string()
	    << " is outside the range of precomputed resource "
	    << get_key() << "\n";
	throw(eval_error(err));
    }

    first_endpoint = sampling_list.find_at_or_before(time_arg);

    CTime_base	itime = first_endpoint->getKey().getetime();
    CTime_base	ftime = first_endpoint->next_node()->getKey().getetime();

    state_1 = first_endpoint->payload.s;
    state_2 = first_endpoint->next_node()->payload.s;

    double t_m_t1 = (time_arg - itime).convert_to_double_use_with_caution();
    double delta_t = (ftime - itime).convert_to_double_use_with_caution();

    int k = 0;

    //
    // velocity index
    //
    int vk = 1;

    double v_av = (state_2[k] - state_1[k]) / delta_t;
    double x = 6.0 * (v_av - 0.5 * (state_2[vk] + state_1[vk])) / (delta_t * delta_t);
    double y = (state_2[vk] - state_1[vk]) / delta_t + x * delta_t;
    result[vk] = state_1[vk] + y * t_m_t1 - x * t_m_t1 * t_m_t1;
    result[k] = state_1[k] + state_1[vk] * t_m_t1
    		+ y * t_m_t1 * t_m_t1 * 0.5
    		- x * t_m_t1 * t_m_t1 * t_m_t1 / 3.0;

    //
    // Store the result.
    //
    val = result[0];
    return val;
}

} // interface aafReader
