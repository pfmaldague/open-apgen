#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <apcoreWaiter.H>
#include <apDEBUG.H>
#include <AbstractResource.H>
#include <ActivityInstance.H>

#include <GLOBdata.H>
#include <IO_ApgenDataOptions.H>
#include <APbasic.H>

extern thread_local int thread_index;

static bool time_saver_is_initialized = false;
int time_saver::now_index = -1;

model_control::modelingPass& model_control::current_pass() {
	static modelingPass mp = INACTIVE;
	return mp;
}

void	model_control::set_pass_to(
			model_control::modelingPass thePass) {
	current_pass() = thePass;
	globalData::modeling_pass() = spell_pass(thePass);
}

static stringtlist& the_list_of_epochs() {
	static stringtlist s;
	return s;
}
static stringtlist& the_list_of_timesystems() {
	static stringtlist s;
	return s;
}
static stringtlist& the_list_of_reserved_items() {
	static stringtlist s;
	return s;
}

bool globalData::isAnEpoch(const Cstring& s) {
	if(the_list_of_epochs().find(s)) {
		return true;
	}
	return false;
}

bool globalData::isATimeSystem(const Cstring& s) {
	if(the_list_of_timesystems().find(s)) {
		return true;
	}
	return false;
}

bool globalData::isReserved(const Cstring& s) {
	if(the_list_of_reserved_items().find(s)) {
		return true;
	}
	return false;
}

#ifdef OBSOLETE
stringtlist&	globalData::theNewGlobals() {
	static stringtlist	s;
	return s;
}
#endif /* OBSOLETE */

TypedValue&		globalData::modeling_pass() {
	static bool already_initialized = false;
	static int mod_pass_index = -1;
	static TypedValue V;
	static pseudo_vector& vals = behaving_object::GlobalObject()->values;

	if(!already_initialized) {
		task* T = Behavior::GlobalType().tasks[0];
		map<Cstring, int>::const_iterator iter = T->get_varindex().find("modeling_pass");
		if(iter != T->get_varindex().end()) {
			already_initialized = true;
			mod_pass_index = iter->second;
		} else {
			V = "unknown value";
			return V;
		}
	}
	return vals[mod_pass_index];
}

stringslist& compiler_intfc::CompilationWarnings() {
	static stringslist l;
	return l;
}

TypedValue& globalData::get_symbol(const Cstring& n) {
	task* T = Behavior::GlobalType().tasks[0];
	pseudo_vector& vals = behaving_object::GlobalObject()->values;
	map<Cstring, int>::const_iterator iter = T->get_varindex().find(n);

	if(iter == T->get_varindex().end()) {
		Cstring errs;
		errs << "globalData::get_symbol(): global " << n << " not found";
		throw(eval_error(errs));
	}
	return vals[iter->second];
}

void globalData::WriteGlobalsToStream(
			aoString &fout,
			IO_APFWriteOptions *options,
			long top_level_chunk) {
	task* T = Behavior::GlobalType().tasks[0];
	pseudo_vector& vals = behaving_object::GlobalObject()->values;
	bool	wrote_something = false;

	for(int i = 0; i < T->get_varinfo().size(); i++) {
		if(	!isReserved(T->get_varinfo()[i].first)
			&& !isAnEpoch(T->get_varinfo()[i].first)
			&& !isATimeSystem(T->get_varinfo()[i].first)
			) {
			TypedValue	val(vals[i]);

			if(options->GetGlobalsOption()
				== Action_request::INCLUDE_NOT_AT_ALL) {
				;
			} else {
				Cstring		temp;
				const char*	c;
				bool		add_pound = false;

				if(!wrote_something) {
					wrote_something = true;
					fout << "\n";
				}
				if(options->GetGlobalsOption()
						== Action_request::INCLUDE_AS_COMMENTS) {
					add_pound = true;
					fout << "# ";
				}
				fout << "global ";

				fout << spell(val.declared_type);
				fout << " ";
				fout << T->get_varinfo()[i].first;
				fout << " = ";
				temp << val.to_string();
				if(add_pound) {
					c = *temp;
					while(*c) {
						if(*c == '\n') {
							fout << *c;
							fout << "# "; }
						else {
							fout << *c; }
						c++;
					}
				} else {
					fout << temp;
				}
				fout << ";";
				fout << "\n";
			}
		}
	}
}

// use this code after adding blocks to handle arrays and structs:

json_object*	TypedValue::to_json() const {
	json_object*		typed_value = json_object_new_object();
	apgen::DATA_TYPE	type_to_use = type;

	if(type_to_use == apgen::DATA_TYPE::UNINITIALIZED) {
		type_to_use = declared_type;
	}
	if(type == apgen::DATA_TYPE::ARRAY) {
		if(get_array().get_array_type() == TypedValue::arrayType::STRUCT_STYLE) {
			json_object_object_add(
				typed_value,
				"type",
				json_object_new_string("struct"));
		} else if(get_array().get_array_type() == TypedValue::arrayType::LIST_STYLE) {
			json_object_object_add(
				typed_value,
				"type",
				json_object_new_string("list"));
		}
	} else {
		json_object_object_add(
			typed_value,
			 "type",
			 json_object_new_string(*spell(type_to_use)));
	}
	switch(type) {
		case apgen::DATA_TYPE::FLOATING:
			json_object_object_add(
				typed_value,
				"value",
				json_object_new_double(get_double()));
			break;
		case apgen::DATA_TYPE::TIME:
		case apgen::DATA_TYPE::DURATION:
			{
			CTime_base	T(get_time_or_duration());
			long		timeval = T.get_seconds() * 1000
						+ T.get_milliseconds();
			json_object*	jsonval = json_object_new_object();
			json_object_object_add(jsonval, "as_string", json_object_new_string(*T.to_string()));
			json_object_object_add(jsonval, "as_milliseconds", json_object_new_int(timeval));
			json_object_object_add(
				typed_value,
				"value",
				jsonval);
			}
			break;
		case apgen::DATA_TYPE::INTEGER:
		case apgen::DATA_TYPE::BOOL_TYPE:
			json_object_object_add(
				typed_value,
				"value",
				json_object_new_int(get_int()));
			break;
		case apgen::DATA_TYPE::STRING:
			{
			Cstring	unquoted = get_string();
			json_object_object_add(
				typed_value,
				"value",
				json_object_new_string(*get_string()));
			}
			break;
		case apgen::DATA_TYPE::ARRAY:
			{
			ListOVal&	lov(get_array());
			json_object*	the_array = NULL;
			ArrayElement*	ae;
			if(lov.get_array_type() == TypedValue::arrayType::STRUCT_STYLE) {
				the_array = json_object_new_object();
				for(int k = 0; k < lov.get_length(); k++) {
					ae = lov[k];
					json_object_object_add(the_array, *ae->get_key(), ae->payload.to_json());
				}
			} else if(lov.get_array_type() == TypedValue::arrayType::LIST_STYLE) {
				the_array = json_object_new_array();
				for(int k = 0; k < lov.get_length(); k++) {
					ae = lov[k];
					json_object_array_add(the_array, ae->payload.to_json());
				}
			} else {
				the_array = json_object_new_string("<NULL>");
			}
			json_object_object_add(
				typed_value,
				"value",
				the_array);
			}
			break;
		case apgen::DATA_TYPE::INSTANCE:
			{
			if(*value.INST) {
			    if((*value.INST)->get_req()) {
				json_object_object_add(
					typed_value,
					"value",
					json_object_new_string(
					    *(*value.INST)->get_req()->identify()));
			    } else if((*value.INST)) {
				json_object_object_add(
					typed_value,
					"value",
					json_object_new_string(
					    *(*value.INST)->Task.Type.name));
			    }
			} else {
				json_object_object_add(
					typed_value,
					"value",
					json_object_new_string("null"));
			}
			}
			break;
		default:
			json_object_object_add(
				typed_value,
				"value",
				json_object_new_string("UNDEFINED"));
			break;
	}
	return typed_value;
}

void	TypedValue::from_json(json_object* Obj) {
	assert(json_object_get_type(Obj) == json_type_object);
	json_object*	type_as_object;
	json_object*	O;
	json_object_object_get_ex(Obj, "type", &type_as_object);
	json_object_object_get_ex(Obj, "value", &O);
	const char*	type_as_string = json_object_get_string(type_as_object);

	switch(json_object_get_type(O)) {
		case json_type_null:
			return;
		case json_type_boolean:
			type = apgen::DATA_TYPE::BOOL_TYPE;
			operator=((bool) json_object_get_int64(O));
			break;
		case json_type_double:
			type = apgen::DATA_TYPE::FLOATING;
			operator=((double) json_object_get_double(O));
			break;
		case json_type_int:
			type = apgen::DATA_TYPE::INTEGER;
			operator=((long int) json_object_get_int64(O));
			break;
		case json_type_object:
			if(!strcmp(type_as_string, "struct")) {
				ListOVal* lov = new ListOVal;

				type = apgen::DATA_TYPE::ARRAY;
				operator=(*lov);
				json_object_iterator	theEnd = json_object_iter_end(O);
				json_object_iterator	iter = json_object_iter_begin(O);

				while(!json_object_iter_equal(&iter, &theEnd)) {
					TypedValue	v;
					const char*	name = json_object_iter_peek_name(&iter);
					json_object*	value = json_object_iter_peek_value(&iter);
					v.from_json(value);
					lov->add(name, v);
					json_object_iter_next(&iter);
				}
			}
			else if((!strcmp(type_as_string, "time")) || (!strcmp(type_as_string, "duration"))) {
				json_object*	time_as_string;
				json_object_object_get_ex(O, "as_string", &time_as_string);
				const char*	the_string = json_object_get_string(time_as_string);
				CTime_base	the_time(the_string);
				operator=(the_time);
			}
			break;
		case json_type_array:
			type = apgen::DATA_TYPE::ARRAY;
			{
			ListOVal*	lov = new ListOVal;
			operator=(*lov);
			int		L = json_object_array_length(O);

			for(int i = 0; i < L; i++) {
				TypedValue	v;
				json_object*	value = json_object_array_get_idx(O, i);
				v.from_json(value);
				lov->add((long)i, v);
			}
			}
			break;
		case json_type_string:
			if(!strcmp(type_as_string, "string")) {
				type = apgen::DATA_TYPE::STRING;
				const char* s = (const char*) json_object_get_string(O);
				operator=(s);
			}
			else if(!strcmp(type_as_string, "array")) {
				// empty array
				type = apgen::DATA_TYPE::ARRAY;
				ListOVal*	lov = new ListOVal;
				operator=(*lov);
			}
			else if(!strcmp(type_as_string, "instance")) {
				operator=("generic");
				cast(apgen::DATA_TYPE::INSTANCE);
			}
			break;
		default:
			break;
			;
	}
}

json_object* globalData::GetJsonGlobals() {
	task* constr = Behavior::GlobalType().tasks[0];
	pseudo_vector& vals = behaving_object::GlobalObject()->values;
	json_object*	obj = json_object_new_object();

	for(int i = 0; i < constr->get_varinfo().size(); i++) {
		const TypedValue&	val(vals[i]);
		json_object*		jo = val.to_json();
		json_object_object_add(
				obj,
				*constr->get_varinfo()[i].first,
				jo);
	}
	return obj;
}

void globalData::WriteGlobalsToJson(
		map<string, string>& result) {
	task* T = Behavior::GlobalType().tasks[0];
	pseudo_vector& vals = behaving_object::GlobalObject()->values;

	for(int i = 0; i < T->get_varinfo().size(); i++) {
		const TypedValue&	val(vals[i]);
		json_object*		jo = val.to_json();

		result[*T->get_varinfo()[i].first]
			= json_object_to_json_string_ext(
					val.to_json(),
					JSON_C_TO_STRING_PRETTY);
		json_object_put(jo);
	}
}

void globalData::WriteTimeSystemsToStream(
		aoString &fout,
		IO_APFWriteOptions *options,
		long top_level_chunk) {
	task* T = Behavior::GlobalType().tasks[0];
	pseudo_vector& vals = behaving_object::GlobalObject()->values;
	bool			wrote_something = false;

	for(int i = 0; i < T->get_varinfo().size(); i++) {
		if(isATimeSystem(T->get_varinfo()[i].first)) {
			if(options->GetTimeSystemsOption() == Action_request::INCLUDE_NOT_AT_ALL) {
				;
			} else {
				if(!wrote_something) {
					wrote_something = true;
					fout << "\n";
				}
				if(options->GetTimeSystemsOption() == Action_request::INCLUDE_AS_COMMENTS) {
					fout << "# ";
				}
				fout << "time_system " << T->get_varinfo()[i].first << " = ";
				fout << vals[i].to_string() << ";\n";
			}
		}
	}
}

void globalData::WriteEpochsToStream(
		aoString &fout,
	       	IO_APFWriteOptions *options,
		long top_level_chunk) {
	task* T = Behavior::GlobalType().tasks[0];
	pseudo_vector& vals = behaving_object::GlobalObject()->values;
	bool			wrote_something = false;

	for(int i = 0; i < T->get_varinfo().size(); i++) {
		if(isAnEpoch(T->get_varinfo()[i].first)) {
			if(options->GetEpochsOption() == Action_request::INCLUDE_NOT_AT_ALL) {
				;
			} else {
				if(!wrote_something) {
					wrote_something = true;
					fout << "\n";
				}
				if(options->GetEpochsOption() == Action_request::INCLUDE_AS_COMMENTS) {
					fout << "# ";
				}
				fout << "epoch " << T->get_varinfo()[i].first << " = ";
				fout << vals[i].to_string() << ";\n";
			}
		}
	}
}

void globalData::qualifySymbol(
		const Cstring&	name,
		apgen::DATA_TYPE type,
		bool		reserved,
		bool		is_epoch,
		bool		is_time_system) {

	if(is_epoch) {
		the_list_of_epochs() << new emptySymbol(name);
		if(APcloptions::theCmdLineOptions().debug_execute) {
			cerr << "adding epoch ";
		}
	} else if(is_time_system) {
		the_list_of_timesystems() << new emptySymbol(name);
		if(APcloptions::theCmdLineOptions().debug_execute) {
			cerr << "adding time system ";
		}
	} else if(reserved) {
		the_list_of_reserved_items() << new emptySymbol(name);
		if(APcloptions::theCmdLineOptions().debug_execute) {
			cerr << "adding reserved item ";
		}
	} else {
		if(APcloptions::theCmdLineOptions().debug_execute) {
			cerr << "adding global ";
		}
	}
	if(APcloptions::theCmdLineOptions().debug_execute) {
		cerr << name << " of type " << apgen::spell(type) << "\n";
	}

	// int n = T->add_variable(name, type);
	// return n;
}

// replaces model_intfc::reinitialize_globals()
void globalData::CreateReservedSymbols() {
	TypedValue		v;
	int			begint;
	int			endt;
	int			mod_pass;

	task* T = Behavior::GlobalType().tasks[0];
	pseudo_vector& vals = behaving_object::GlobalObject()->values;
	map<Cstring, int>::const_iterator iter;

	// initialize the 'hidden, special' symbols
	if((iter = T->get_varindex().find("SASF_BEGIN_TIME")) == T->get_varindex().end()) {
		begint = T->add_variable("SASF_BEGIN_TIME", apgen::DATA_TYPE::TIME);
		qualifySymbol("SASF_BEGIN_TIME", apgen::DATA_TYPE::TIME, true);
	} else {
		begint = iter->second;
	}
	if((iter = T->get_varindex().find("SASF_CUTOFF_TIME")) == T->get_varindex().end()) {
		endt = T->add_variable("SASF_CUTOFF_TIME", apgen::DATA_TYPE::TIME);
		qualifySymbol("SASF_CUTOFF_TIME", apgen::DATA_TYPE::TIME, true);
	} else {
		endt = iter->second;
	}
	if((iter = T->get_varindex().find("modeling_pass")) == T->get_varindex().end()) {
		mod_pass = T->add_variable("modeling_pass", apgen::DATA_TYPE::STRING);
		qualifySymbol("modeling_pass", apgen::DATA_TYPE::STRING, true);
	} else {
		mod_pass = iter->second;
	}
	vals[begint] = CTime_base("2000-001T00:00:00.000");
	vals[endt] = CTime_base("2000-001T00:00:00.000");
	vals[mod_pass] = "INACTIVE";
}

void globalData::destroy() {
	vector<int> empty;
	behaving_object::GlobalObject()->values.cleanup(empty);
	Behavior::GlobalType().tasks[0]->clear_varindex();
	Behavior::GlobalType().tasks[0]->clear_varinfo();
	the_list_of_epochs().clear();
	the_list_of_timesystems().clear();
	the_list_of_reserved_items().clear();
	time_saver_is_initialized = false;
}

void globalData::copy_symbols(ListOVal& in_here) {
	task* T = Behavior::GlobalType().tasks[0];
	ArrayElement* ae;
	pseudo_vector& vals = behaving_object::GlobalObject()->values;
	for(int i = 0; i < T->get_varinfo().size(); i++) {
		in_here.add(ae = new ArrayElement(T->get_varinfo()[i].first));
		ae->SetVal(vals[i]);
	}
}

void globalData::dump() {
}

IO_APFWriteOptions::IO_APFWriteOptions(
		std::ostream &outstream,
		Action_request::save_option globalsOption,
		Action_request::save_option epochsOption,
		Action_request::save_option timeSystemsOption,
		Action_request::save_option legendsOption,
		Action_request::save_option timeParamsOption,
		Action_request::save_option windowSizeOption,
		int registeredFunctionOption,
		int formatOption)
  : OutStream(outstream),
    GlobalsOption(globalsOption),
    EpochsOption(epochsOption),
    TimeSystemsOption(timeSystemsOption),
    LegendsOption(legendsOption),
    TimeframeOption(timeParamsOption),
    WindowSizeOption(windowSizeOption),
    RegisteredFunctionOption(registeredFunctionOption),
    FormatOption(formatOption),
	StreamPtr(NULL) {}


IO_APFWriteOptions::IO_APFWriteOptions(
	aoString &o,
	Action_request::save_option globalsOption,
	Action_request::save_option epochsOption,
	Action_request::save_option timeSystemsOption,
	Action_request::save_option legendsOption,
	Action_request::save_option timeParamsOption,
	Action_request::save_option windowSizeOption,
	int registeredFunctionOption,
	int formatOption)
  : OutStream(std::cout),
    GlobalsOption(globalsOption),
    EpochsOption(epochsOption),
    TimeSystemsOption(timeSystemsOption),
    LegendsOption(legendsOption),
    TimeframeOption(timeParamsOption),
    WindowSizeOption(windowSizeOption),
    RegisteredFunctionOption(registeredFunctionOption),
    FormatOption(formatOption),
	StreamPtr(&o) {}

Action_request::save_option IO_APFWriteOptions::GetGlobalsOption() const {
  return GlobalsOption; }

Action_request::save_option IO_APFWriteOptions::GetEpochsOption() const {
  return EpochsOption; }

Action_request::save_option IO_APFWriteOptions::GetTimeSystemsOption() const {
  return TimeSystemsOption; }

Action_request::save_option IO_APFWriteOptions::GetLegendsOption() const {
  return LegendsOption; }

Action_request::save_option IO_APFWriteOptions::GetTimeframeOption() const {
  return TimeframeOption; }

Action_request::save_option IO_APFWriteOptions::GetWindowSizeOption() const {
  return WindowSizeOption; }

int IO_APFWriteOptions::GetRegisteredFunctionOption() const {
	return RegisteredFunctionOption; }

int IO_APFWriteOptions::GetFormatOption() const {
	return FormatOption; }

APcloptions &APcloptions::theCmdLineOptions() {
	static APcloptions a;
	return a; }

void	APcloptions::getFlags(stringtlist& L) {
		if(theCmdLineOptions().refresh_info_requested) {
			L << new emptySymbol("refreshinfo"); } }

stringslist&	APcloptions::FilesToRead() {
		static stringslist L;
		return L;
}

pairslist&	APcloptions::FilesToWrite() {
		static pairslist L;
		return L;
}

apcoreWaiter &apcoreWaiter::theWaiter() {
	static apcoreWaiter aw;
	return aw; }

apcoreWaiter::apcoreWaiter()
		: // immediate(false),
		ar(NULL) {
	// pthread_cond_init(&signalRequestReady, NULL);
	// pthread_mutex_init(&usingAPcore, NULL);
	// pthread_cond_init(&signalApcoreReady, NULL);
}

Action_request*& apcoreWaiter::getTheCommand() {
	return ar; }
