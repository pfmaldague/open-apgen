#ifndef _EVENT_IMPL_H_
#define _EVENT_IMPL_H_

#include "EventBase.H"

extern bool	debug_event_loop;

class init_eventPLD;

class init_event: public mEvent {
public:
	init_event(const CTime_base& T)
		: mEvent( /* eventPLD::INIT, */ T) {}
	~init_event() {}

	const alpha_time&	getKey() const;
	static init_eventPLD*	initialEventFactory(
					CTime_base	init_time);

};

class init_eventPLD: public eventPLD {
public:
	// Constructor used for initializer nodes only.
	init_eventPLD(mEvent* the_event);
	~init_eventPLD();

	virtual void			do_model() override;
	virtual const Cstring&		get_key() const override;
	virtual Cstring			get_info() const override;

	virtual eventlist*		get_queue() override {
		return &mEvent::initializationQueue();
	}
};

class usage_event: public eventPLD {
public:

	//
	// for 'use at ...':
	//
	usage_event(	mEvent*			ref_event,
			execution_context*	context,
			behaving_element	obj_used,
			behaving_base*		storage = NULL);

	//
	// for 'use from ... to ...':
	//
	usage_event(	mEvent*			ref_event,
			execution_context*	context,
			behaving_element	obj_used,
			bool			FromTo, // always true...
			CTime_base		ToTime,
			behaving_base*		storage = NULL);
	~usage_event();

	//
	// for 'use at ...':
	//
	static usage_event*		resEventFactory(
						/* eventPLD::event_type	T, */
						execution_context*	context,
						behaving_element	obj_used,
						behaving_base*		storage);

	//
	// for 'use from ... to ...':
	//
	static usage_event*		resEventFactory(
						/* eventPLD::event_type	T, */
						execution_context*	context,
						behaving_element	obj_used,
						bool			FromTo,
						CTime_base		ToTime,
						behaving_base*		storage);

	static usage_event*		signalEventFactory(
						execution_context*	context);

	virtual const Cstring&		get_key() const override;
	virtual Cstring			get_info() const override;
	virtual void			do_model() override;

	virtual const behaving_element	getEffect() const {
		return Effect;
	}
	virtual pEsys::Usage* get_usage_clause() const {
		return UsageClause;
	}

					// debug function to support step-wise remodeling
	// void				Export(FILE* outfile);

					//
					// Pointer to the clause that caused 'this'
					// node to be created in the first place
					//
	pEsys::Usage*			UsageClause;
	behaving_element		by_value_storage;

	// virtual bool			get_enabled() const {
	// 	return Enabled;
	// }
	// virtual void			set_enabled(bool B) {
	// 	Enabled = B;
	// }
	// bool				Enabled;
	bool				from_to;
	CTime_base			to_time;


					//
					// refers to the object (act., abs. res.)
					// that initiated the usage event:
					//
	behaving_element		Cause;

					//
					// refers to the object (resource) being used:
					//
	behaving_element		Effect;

	virtual eventlist*		get_queue() override {

		//
		// The possibilities are
		//
		// 	- eventQueue() (old event_type = START_USING or ACT_SIGNAL)
		//
		// 	- nothing else! An abstract resource usage instruction results
		// 	  in either a WAIT being put on the queue, or an immediate
		// 	  execution of a resource usage section.
		//
		return &mEvent::eventQueue();
	}
};

class simple_wait_event: public eventPLD {
public:
	//
	// For use by simple wait events (pEsys::WaitFor)
	//

	simple_wait_event(
			mEvent*			ref_event,
			execution_context*	current_context);
	~simple_wait_event();

	static simple_wait_event*	simpleWaitEventFactory(
						execution_context*	EC);
	virtual void			do_model() override;
	virtual const Cstring&		get_key() const override;
	virtual Cstring			get_info() const override;
	virtual execution_context*	get_context() const;
	virtual pEsys::execStack&	getStack() {
		return hiddenStack;
	}
	virtual const pEsys::execStack&	getStack() const {
		return hiddenStack;
	}
	pEsys::execStack		hiddenStack;

	virtual eventlist*		get_queue() override {

		//
		// This will be modified in model_intfc::add_timevent()
		// if the ambient section is scheduling, or if the time
		// stamp of the event is the same as that of CurrentEvent:
		//
		return &mEvent::expansionQueue();
	}
};

class wait_until_event: public eventPLD {
public:
	//
	// For use by simple wait events (pEsys::WaitFor)
	//

	wait_until_event(
			mEvent*			ref_event,
			execution_context*	current_context);
	~wait_until_event();

	static wait_until_event*	waitUntilEventFactory(
						execution_context*	EC);
	virtual void			do_model() override;
	virtual const Cstring&		get_key() const override;
	virtual Cstring			get_info() const override;
	virtual execution_context*	get_context() const;
	virtual pEsys::execStack&	getStack() {
		return hiddenStack;
	}
	virtual const pEsys::execStack&	getStack() const {
		return hiddenStack;
	}
	pEsys::execStack		hiddenStack;

	virtual eventlist*		get_queue() override {

		//
		// This will be modified in model_intfc::add_timevent()
		// if the ambient section is scheduling, or if the time
		// stamp of the event is the same as that of CurrentEvent:
		return &mEvent::expansionQueue();
	}
};

class wait_regexp_event: public eventPLD {
public:
	//
	// For use by simple wait events (pEsys::WaitFor)
	//

	wait_regexp_event(
			mEvent*			ref_event,
			execution_context*	current_context);
	~wait_regexp_event();

	static wait_regexp_event*	waitRegexpEventFactory(
						execution_context*	EC);
	virtual void			do_model() override;
	virtual const Cstring&		get_key() const override;
	virtual Cstring			get_info() const override;
	virtual execution_context*	get_context() const;
	virtual pEsys::execStack&	getStack() {
		return hiddenStack;
	}
	virtual const pEsys::execStack&	getStack() const {
		return hiddenStack;
	}
	pEsys::execStack		hiddenStack;

	virtual eventlist*		get_queue() override {

		//
		// This will be modified in model_intfc::add_timevent()
		// if the ambient section is scheduling, or if the time
		// stamp of the event is the same as that of CurrentEvent:
		return &mEvent::expansionQueue();
	}
};

class resume_event: public eventPLD {
public:
	resume_event(
			mEvent*			ref_event,
			execution_context*	stack);
	~resume_event() {
	}

	static resume_event*		resumeEventFactory(
						execution_context* ec);
	virtual void			do_model() override;
	virtual const Cstring&		get_key() const override;
	virtual Cstring			get_info() const override;
	virtual execution_context*	get_context() const;
	virtual pEsys::execStack&	getStack() {
		return hiddenStack;
	}
	virtual const pEsys::execStack&	getStack() const {
		return hiddenStack;
	}
	pEsys::execStack		hiddenStack;

	virtual eventlist*		get_queue() override {

		//
		// This will be modified in model_intfc::add_timevent()
		// if the ambient section is scheduling, or if the time
		// stamp of the event is the same as that of CurrentEvent:
		return &mEvent::expansionQueue();
	}
};

#endif /* _EVENT_IMPL_H_ */
