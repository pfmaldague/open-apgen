#ifndef _TOL_WRITE_H_
#define _TOL_WRITE_H_

#include	<assert.h>
#include	<sstream>
#include	<string.h>
#include	<sys/param.h>
#include	<unistd.h>

#ifdef have_xmltol
#	include <libxml++/libxml++.h>   // for generating XML TOL's
#endif /* have_xmltol */

#include	"action_request.H"
#include	"ActivityInstance.H"
#include	"ACT_exec.H"
#include	"Constraint.H"
#include	"IO_write.H"
#include	"RES_def.H"
#include	"apcoreWaiter.H"

extern coutx excl;
extern streamx& tol_debug_stream();

using namespace std;

// for xslt filter processing:
#include <libxml/xmlmemory.h>
#include <libxml/debugXML.h>
#include <libxml/HTMLtree.h>
#include <libxml/xmlIO.h>
#include <libxml/xinclude.h>
#include <libxml/catalog.h>
#include <libxslt/xslt.h>
#include <libxslt/xsltInternals.h>
#include <libxslt/transform.h>
#include <libxslt/xsltutils.h>

extern int xmlLoadExtDtdDefaultValue;

//
// the following enum and class definitions are
// useful for all TOL output (regular and XML)
//
typedef enum {
	TOL_ACT_START,
	TOL_ACT_END,
	TOL_RES_VAL,
	TOL_VIO,
	TOL_META } publisher_type;

typedef enum {
	TOL_REGULAR = 0,
	TOL_TRANSIENT,
	TOL_FINAL } tol_output_flag;

// for storing TOL values in a tlist:
typedef Cnode0<alpha_void, pair<Rsource*, TypedValue> > res_record;

// for storing TOL values in a vector:
class res_record2 {
public:
	res_record2()
	    : last_index(-1),
		selected(false) {}
	res_record2(const res_record2&) = default;
	~res_record2() = default;

	bool		selected;
	TypedValue	val;
	int		last_index;
};

//
// A class that encapsulates all the data needed to produce
// a TOL, an XMLTOL or both
//
class TOL_kit {
public:
    TOL_kit()			= delete;
    TOL_kit(const TOL_kit&)	= delete;
    TOL_kit(
	const CTime_base&					s,
	const CTime_base&					e,
	const bool&						vis,
	const bool&						incl_all,
    	tlist<alpha_string, Cnode0<alpha_string, Rsource*> >&	incl_res,
	dual_purpose_iterator&					res_iter,
	status_aware_multiterator*				start_act_iter,
	bool							write_both,
	pinfotlist&						param_info)
	:		start(				s),
			end(				e),
			AllActsVisible(			vis),
			include_all_res_and_acts(	incl_all),
			all_included_res(		incl_res),
			last_processed_vio(		NULL),
			next_vio_to_process(		NULL),
			theResIterator(			res_iter),
			priority(			1),
			startIterator(			start_act_iter),
			WriteBothFiles(			write_both),
			parameter_info(			param_info) {
    }
    ~TOL_kit() {}

    bool							WriteBothFiles;
    const CTime_base&						start;
    const CTime_base&						end;
    const bool&							AllActsVisible;
    const bool&							include_all_res_and_acts;
    int								priority;

    tlist<alpha_string, Cnode0<alpha_string, Rsource*> >&	all_included_res;
    con_violation*						last_processed_vio;
    con_violation*						next_vio_to_process;

    //
    // the dual_purpose_iterator wraps either a res_curval_miter if
    // the context is single-threaded, or a trailing_miter if the
    // context is multi-threaded
    //
    dual_purpose_iterator&					theResIterator;
    status_aware_multiterator*					startIterator;

    //
    // parameter_info holds information about any
    // activity parameters of type ARRAY, in case
    // the value of that parameter for a specific
    // activity instance happens to be the empty
    // array. In that case, it is impossible for
    // a program reading the TOL to figure out
    // whether that array should be of the LIST
    // type or of the STRUCT type. The purpose
    // of parameter_info is to hold information
    // that tells the style of all array-type
    // parameters.
    //
    pinfotlist&							parameter_info;
};

class publisher {
public:

	//
	// Common
	//
	publisher_type		type;
	ActivityInstance*	activity;
	Rsource*		Rnode;
	con_violation*		violation;
	bool			unreleased;
	Cstring			resource_id;
	CTime_base      	time_tag;
	TypedValue		Val;
	pinfotlist*		Pinfo;

	const CTime_base&	getetime() const {
		return time_tag;
	}

	publisher();

	// for activity instance records:
	publisher(	ActivityInstance* A,
			CTime_base time,
			publisher_type type,
			pinfotlist& param_info);

	// for resource values:
	publisher(
		const Cstring& name,
		const TypedValue& V,
		CTime_base time_of_event,
		Rsource* R);

	// for constraint violations:
	publisher(
		con_violation*	V,

		//
		// if unrelease_status is true, that means
		// that the violation was never released
		//
		bool		unrelease_status = false);

	// for reporting resource metadata:
	publisher(
		Rsource* R);
	publisher(const publisher& T);
	virtual ~publisher() {}

	//
	// for TOL output
	//
	static aoString*	theStream;
	static FILE*		theTOLFile;
	static FILE*		theAuxiliaryFile;
	static bool		dual_flag;
	static void		set_stream_to(
					aoString& aos,
					FILE*	tolf,
					FILE*	auxf = NULL) {
		theStream = &aos;
		theTOLFile = tolf;
		theAuxiliaryFile = auxf;
	}
	static void		set_dual_flag_to(bool b) {
		dual_flag = b;
	}

	//
	// for XMLTOL output
	//
	static bool		first_time_writing;
	static const char*	timesystem;
	static CTime_base	offset;
	static double		multiplier;
	static FILE*		theOutputFile;
	static bool		all_acts_visible;
#ifdef have_xmltol
	static xmlpp::Document*	theDocument;

	static void		set_document_to(xmlpp::Document* doc) {
					theDocument = doc;
	}

	static xmlpp::Element*  theElement;
	static void		set_element_to(xmlpp::Element* el) {
		theElement = el;
	}

	static xmlpp::Element*  theMetadata;
	static void		set_metadata_to(xmlpp::Element* el) {
		theMetadata = el;
	}
#endif /* have_xmltol */
};

class tol_publisher: public publisher {
public:
	// 0: act_start,
	// 1: act_end,
	// 2: resource,
	// 3: violation
	// 4: resource metadata

	void			write_to_stream(
					tol_output_flag is_this_a_transient,
					CTime_base*	bounds_if_any);

	static void		cleanup() {
		if(theStream) {
			delete theStream;
		}
	}
	static void		handle_timesystem(const Cstring&) {}
	static Cstring		convert_to_timesystem(CTime_base& given_time) {
		return Cstring();
	}

	//
	// Used to avoid storing the whole output file in memory
	//
	static void		write_everything_you_got_up_to_this_point() {

		//
		// Note: aoString::write_to() clears the stream
		//
		theStream->write_to(theTOLFile);
	}
	static void		handle_globals(TypedValue*) {}
	static void		write_end_of_metadata();

	// for std::vector:
	tol_publisher();
	// for activity instance records:
	tol_publisher(
		ActivityInstance* A,
		CTime_base time,
		publisher_type type,
		pinfotlist& param_info);

	// for resource values:
	tol_publisher(
		const Cstring& name,
		const TypedValue& V,
		CTime_base time_of_event,
		Rsource* R);

	// for constraint violations:
	tol_publisher(
		con_violation*	V,
		bool		unrelease_status = false);

	// for reporting resource metadata:
	tol_publisher(
		Rsource* R);

	tol_publisher(const tol_publisher& T);
	~tol_publisher() {}

	static bool		is_xml_publisher() {
		return false;
	}
};

#ifdef have_xmltol
class xml_publisher: public publisher {
public:

	vector<Cstring>		resource_indices;

	void			write_to_stream(
				    tol_output_flag is_this_a_transient,
				    CTime_base*	time_bounds_if_any);


	static void		apply_xslt_filter_to_doc_and_output(
					xmlDocPtr	doc,
					const char*	xslt_filter_file,
					FILE*		fout);

	static bool		test_for_xml_tol_element(const char* s);

	static void		cleanup() {
		if(theDocument) {
			delete theDocument;
		}
	}
	static void		handle_timesystem(const Cstring& ts) {
		if(!ts.length()) return;
		TypedValue&     tdv = globalData::get_symbol(timesystem);
		ListOVal*       tv = &tdv.get_array();

		offset = tv->find("origin")->Val().get_time_or_duration();
		multiplier = tv->find("scale")->Val().get_double();
	}

	static Cstring	  convert_to_timesystem(
					CTime_base&	given_time) {
		CTime_base      given_dur(given_time - offset);
		CTime_base      delta(given_dur / multiplier);
		if(delta >= CTime_base(0, 0, true)) {
			return Cstring(delta.to_string());
		} else {
			CTime_base      D(-delta) ;
			return Cstring("-") + D.to_string();
		}
	}
	static void		handle_globals(TypedValue*);

	//
	// No-op in the XMLTOL case:
	//
	static void		write_end_of_metadata() {}

	//
	// Used to avoid storing the whole output file in memory
	//
	static void		write_everything_you_got_up_to_this_point();

	// for std::vector:
	xml_publisher();

	//
	// To allow a tol_publisher to create an xml_publisher,
	// which will come in handy in passing data to an additional
	// thread:
	//
	xml_publisher(const tol_publisher& T);

	// for activity instance records:
	xml_publisher(	ActivityInstance* A,
			CTime_base time,
			publisher_type type,
			pinfotlist& param_info);

	// for resource values:
	xml_publisher(	const Cstring& name, 
			const TypedValue& V,
			CTime_base time_of_event,
			Rsource* R);

	// for constraint violations:
	xml_publisher(	con_violation*	V,
			bool		unrelease_status = false);
 
	// for reporting resource metadata:
	xml_publisher(Rsource* R);

	xml_publisher(const xml_publisher& T);
	xml_publisher& operator=(const xml_publisher& T) = default;

	~xml_publisher() = default;

	static bool		is_xml_publisher() {
		return true;
	}
};

#endif /* have_xmltol */


//
// an interface for the payload of tol_record:
//
class tolEventPLD {
public:
	typedef enum {
		ACT_START,
		ACT_END,
		RES_VAL,
		VALUE_NODE,		// to support tol_record in TOL output processing
		ASSOC_NODE,		// ditto
		CONS_VIO } eventType;

	eventType		eType;

	tolEventPLD(eventType et) : eType(et) {}
	virtual ~tolEventPLD() {}

	// virtual Cstring		get_descr() const = 0;
	/* the next 4 methods return NULL if inappropriate.
	 * The first is for activities, the second for violations,
	 * and the last 2 for resources. */
	virtual ActivityInstance* get_instance() = 0;
	virtual con_violation*	get_violation() = 0;
	virtual Rsource*	get_resource() = 0;
	virtual TypedValue*	get_value() = 0;
	virtual value_node*	get_value_node() = 0;
};

class act_start_event: public tolEventPLD {
public:
	ActivityInstance*	Act;
	act_start_event(ActivityInstance* act) : tolEventPLD(ACT_START), Act(act) {}
	~act_start_event() {}

	// virtual Cstring		get_descr() const;
	virtual ActivityInstance*	get_instance() {
		return Act;
	}
	virtual con_violation*	get_violation() {
		return NULL;
	}
	virtual Rsource*	get_resource() {
		return NULL;
	}
	virtual TypedValue*	get_value() {
		return NULL;
	}
	virtual value_node*	get_value_node() {
		return NULL;
	}
};

class act_end_event: public tolEventPLD {
public:
	ActivityInstance*	Act;
	act_end_event(ActivityInstance* act) : tolEventPLD(ACT_END /* , p */ ), Act(act) {}
	~act_end_event() {}

	// virtual Cstring		get_descr() const;
	virtual ActivityInstance* get_instance() {
		return Act;
	}
	virtual con_violation*	get_violation() {
		return NULL;
	}
	virtual Rsource*	get_resource() {
		return NULL;
	}
	virtual TypedValue*	get_value() {
		return NULL;
	}
	virtual value_node*	get_value_node() {
		return NULL;
	}
};

class res_val_event: public tolEventPLD {
public:
	Rsource*	Res;
	TypedValue	val;

	res_val_event(
		Rsource* res,
		const TypedValue& V)
			: tolEventPLD(RES_VAL),
				Res(res),
				val(V) {}
	~res_val_event() {}

	// virtual Cstring		get_descr() const {
	// 	Cstring s = ((res_val_event*) this)->get_value()->to_string();
	// 	return Cstring("RES_VAL - ") + Res->name + ": val = " + s;
	// }
	virtual ActivityInstance* get_instance() {
		return NULL;
	}
	virtual con_violation*	get_violation() {
		return NULL;
	}
	virtual Rsource*	get_resource() {
		return Res;
	}
	virtual TypedValue*	get_value() {
		return &val;
	}
	virtual value_node*	get_value_node() {
		return NULL;
	}
};

class vio_event: public tolEventPLD {
public:
	con_violation*	vio;
	vio_event(con_violation* V) : tolEventPLD(CONS_VIO), vio(V) {}
	~vio_event() {}

	// virtual Cstring		get_descr() const;
	virtual ActivityInstance* get_instance() {
		return NULL;
	}
	virtual con_violation*	get_violation() {
		return vio;
	}
	virtual Rsource*	get_resource() {
		return NULL;
	}
	virtual TypedValue*	get_value() {
		return NULL;
	}
	virtual value_node*	get_value_node() {
		return NULL;
	}
};

class val_node_event: public tolEventPLD {
public:
	value_node*	vn;
	val_node_event(value_node* V) : tolEventPLD(VALUE_NODE), vn(V) {}
	~val_node_event() {}

	// virtual Cstring		get_descr() const;
	virtual ActivityInstance* get_instance() {
		return NULL;
	}
	virtual con_violation*	get_violation() {
		return NULL;
	}
	virtual Rsource*	get_resource() {
		return NULL;
	}
	virtual TypedValue*	get_value() {
		return NULL;
	}
	virtual value_node*	get_value_node() {
		return vn;
	}
};

class tol_record: public Dnode0<prio_time, tol_record> {
public:
	tolEventPLD*   payload;
private:
	tol_record(const CTime_base& t, long prio, tolEventPLD* pep)
		: Dnode0<prio_time, tol_record>(prio_time(t, prio)),
			payload(pep) {
	}
	tol_record(const tol_record& tr) = delete;
public:
	~tol_record() {
		if(list) {
			list->remove_node(this);
		}
		delete payload;
		payload = NULL;
	}

	static tol_record*	actStartFactory(ActivityInstance* act, long prio);
	static tol_record*	actEndFactory(ActivityInstance* act, long prio);
	static tol_record*	resValueFactory(value_node* vn, long prio);
	static tol_record*	violationFactory(con_violation* v, long prio);

	tol_record*		copy() {
		return NULL;
	}
	tol_record*		get_this() {
		return this;
	}
	const tol_record*	get_this() const {
		return this;
	}
};

extern void handle_a_resource_event(
	value_node&			vn,
	const CTime_base&		event_time,
	Rsource*			resource,
	tlist<alpha_void, res_record>&	resources_taken_care_of,
	vector<bool>&			res_selection);

template <class eventPublisher>
void handle_last_resource_event(
		const CTime_base&	event_time,
		Rsource*		resource,
		tlist<alpha_void, res_record>& resources_taken_care_of,
		vector<bool>&		res_selection,
		bool			write_both_files) {
    bool		interpolated = false;
    TypedValue	after;
    Cnode0<alpha_void, RCsource*>* ptr_to_dep;
    bool		has_interpolation
		= resource->properties[(int)Rsource::Property::has_interpolation];

    //
    // Avoid repeats within the last recap set of records
    //
    if(resources_taken_care_of.find((void *) resource)) {
	    return;
    }

    resource->evaluate_present_value(
			after,
			event_time);
    after.cast(resource->get_datatype());

    eventPublisher  ep(resource->name, after, event_time, resource);
    ep.write_to_stream(TOL_FINAL, NULL);
    if(write_both_files) {
	xml_publisher xp(ep);
	xp.write_to_stream(TOL_FINAL, NULL);
    }

    slist<alpha_void, Cnode0<alpha_void, RCsource*> >::iterator
		dep_iter(resource->parent_container.payload->containers_whose_profile_invokes_this);

    assert(after.get_type() != apgen::DATA_TYPE::UNINITIALIZED);
    resources_taken_care_of << new res_record(
					resource,
					pair<Rsource*, TypedValue>(resource, after));
    while((ptr_to_dep = dep_iter())) {
	RCsource*	dependent_cont = ptr_to_dep->payload;
	simple_res_iter	iter(dependent_cont);
    	Rsource*	dependent;

	while((dependent = iter.next())) {
	    RES_settable* settable_dependent = dynamic_cast<RES_settable*>(dependent);
	    if(settable_dependent) {
			continue;
	    }
	    if(!resources_taken_care_of.find((void *) dependent)
	       && res_selection[dependent->output_index[thread_index]]) {

		handle_last_resource_event<eventPublisher>(
					event_time,
					dependent,
					resources_taken_care_of,
					res_selection,
					write_both_files);
	    }
	}
    }
}

template <class eventPublisher>
void
writeTOLbody(
	CTime_base		start,
	CTime_base		end,
	const vector<string>&	resources,
	bool			AllActsVisible,
	const Cstring&		timesystem,
	bool			write_both_tol_and_xmltol,
	pinfotlist&		paraminfo,
	thread_intfc*		T);


Cstring get_tol_header_info(
		    const Cstring&	file_name,
		    bool		in_xml_format = false);

//
// Note: the following function template can get at the
// context thread_intfc object via
// TOL_kit.theResIterator.Thread
//
template <class eventPublisher>
void
iterateOverTOLevents(TOL_kit&);

#endif /* _TOL_WRITE_H_ */
