#ifndef _MODEL_EVENT_H_
#define _MODEL_EVENT_H_

#include <APbasic.H>
#include <APdata.H>
#include <alphatime.H>
#include <multitemplate.H>
#include <multilist.H>
#include <smartptr.H>

class eval_intfc;
class mEvent;
class usage_event;

typedef multilist<alpha_time, mEvent, eval_intfc*> eventlist;

//
// Pure virtual class for the payload of mEvent nodes (defined below).
//
// Complete catalog of mEvents and their factories:
//
//	Factory			Event class	PLD class in EventImpl.H
//	=======			===========	========================
//
//	initialEventFactory	init_event	init_eventPLD
//
//	resEventFactory		mEvent		usage_event ("at" style)
//
//	resEventFactory		mEvent		usage_event ("from to" style)
//
//	signalEventFactory	mEvent		usage_event ("signal" style)
//
//	resumeEventFactory	mEvent		resume_event
//
//	waitEventFactory	mEvent		wait_event
//
//	simpleWaitEventFactory	mEvent		simple_wait_event
//
//	waitUntilEventFactory	mEvent		wait_until_event
//
//	waitRegexpEventFactory	mEvent		wait_regexp_event
//
class eventPLD {
public:
	eventPLD() {
	}
	virtual ~eventPLD() {
	}

	// for everybody:
	virtual void			do_model() = 0;
	virtual Cstring			get_descr() const;	// puts get_key() on one line
	virtual const Cstring&		get_key() const = 0;	// provides continuity with the past
	virtual Cstring			get_info() const = 0;	// provides the part of the descr
								// that's not time nor seq number
	virtual eventlist*		get_queue() = 0;

	mEvent*				source;
	static long			event_count;

	//
	// UUUUU This should be in the resource subsystem:
	//
	static tlist<alpha_void, Cnode0<alpha_void, RCsource*> >& resources_with_no_dependencies();
	static tlist<alpha_void, Cnode0<alpha_void, RCsource*> >& resources_nothing_depends_on();
};

//
// Does not work with Cnode0, which should be a final class. The only derived class is initialEvent.
//
// NOTE: payload is pure virtual. Do not use constructors; factories are provided for that purpose.
//	 The factories create payloads that are instances of the (concrete) interp_event class.
//
class mEvent: public Dnode0<alpha_time, mEvent, eval_intfc*> {
private:
	eventPLD*			Payload;
public:
	//
	// do not use outside of factory (null payload), except for comparison
	//
	mEvent( /* eventPLD::event_type et, */ const CTime_base& T);

	//
	// Do not use. Event lists are not to be copied.
	// Note copy() implementation below; it will crash APGenX.
	//
	mEvent(const mEvent& me) = delete;

	~mEvent() {
		if(list) {
			list->remove_node(this);
		}
		if(Payload) delete Payload;
	}

	eventPLD*			get_payload() {
		if(!Payload) {
			char buf[20];
			sprintf(buf, "%ld", eventID);
			Cstring t("Event ");
			t << buf << " has no payload.";
			throw(eval_error(t));
		}
		return Payload;
	}
	void				set_payload(eventPLD* p) {
		Payload = p;
	}
	eventPLD* const&		get_const_payload() const {
		if(!Payload) {
			char buf[20];
			sprintf(buf, "%ld", eventID);
			Cstring t("Event ");
			t << buf << " has no payload.";
			throw(eval_error(t));
		}
		return Payload;
	}
	void				unload() {
		if(Payload) {
			delete Payload;
			Payload = NULL;
		}
	}

	mEvent*				copy() {
		assert(false);
	}
	mEvent*				get_this() {
		return this;
	}
	const mEvent*			get_this() const {
		return this;
	}
	virtual void			do_model() {
		get_payload()->do_model();
	}
					/* provides a time that is consistent with the
					 * ordering supplied by eval_intfc::compare_events: */
	const CTime_base&		getetime() const;
	Cstring				get_descr() const {
		return get_const_payload()->get_descr();
	}
	long				get_event_id() {
		return eventID;
	}

	// eventPLD::event_type		get_event_type() const {
	// 	return EventType;
	// }

	virtual const Cstring&		get_key() const {
		return get_const_payload()->get_key();
	}

	long				eventID;

	//
	// There are 5 lists that make up "the" list of events. All contain
	// mEvents. The reason why there are 5 queues and not just one
	// is that this allows us to process events with different priorities:
	//
	//	- the InitializationQueue is always first; it initializes all
	//	  concrete resources
	//
	//	- the eventQueue is next. This is the queue that contains
	//	  actual usage events
	//
	//	- the currentQueue is next. This is to enable rather complex
	//	  behavior in the APGEN interpreter, which gave the modeling
	//	  section that is currently executing higher priority than
	//	  other expansion/scheduling events just after one or more
	//	  usage events have been processed. The "use immediately"
	//	  feature of the adaptation is a better, less obscure way
	//	  of taking advantage of this feature
	//
	//	- the expansionQueue is next. This is the queue that contains
	//	  wait-type events that originate from modeling sections
	//
	//	- the schedulingQueue is last. This is the queue that contains
	//	  wait-type events that originate from scheduling sections
	//
	static eventlist&		eventQueue();
	static eventlist&		expansionQueue();
	static eventlist&		schedulingQueue();
	static eventlist&		initializationQueue();
	static eventlist&		currentQueue();

	virtual eventlist*		get_queue() {
		return get_payload()->get_queue();
	}

	static int			currentEventID;
	static int			lastEventIDbeforeScheduling;
	static int			firstEventIDbeforeScheduling;
};

#endif /* _MODEL_EVENT_H_ */
