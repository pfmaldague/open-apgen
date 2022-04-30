#include "RES_exec.H"

class window_finder {
public:

	//
	// res_curval_miter support:
	//
	static value_node* initializer(
			const slist<prio_time, value_node, Rsource*>&,
			const CTime_base&
			);

	//
	// smart ptr support:
	//
	int		ref_count;
	int		ref() {
		ref_count++;
		return ref_count;
	}
	int		deref() {
		ref_count--;
		return ref_count;
	}

	//
	// This constructor copies the specified Miterator into theTriggers
	// and then uses the initializer function to refine the
	// initialization. This is appropriate when the event loop is ongoing,
	// i. e., when the call to get_windows occurs within a scheduling loop.
	//
	window_finder(
		const model_res_miter&	events,
		parsedExp&		Criterion,
		const CTime_base&	requestedDuration,
		const CTime_base&	maximumDuration,
		bool			maxDurIsSpecified,
		bool			criterionIsSatisfiedAtStart,
		bool			weMustWaitOnTheFirstWindow,
		TypedValue*		wins,
		TypedValue*		safe_wins,
		TypedValue*		actual_dur,
		behaving_element&	the_symbols
		) : criterion_is_satisfied_at_start(criterionIsSatisfiedAtStart),
			we_need_to_wait(weMustWaitOnTheFirstWindow),
			theDurationToRememberForTheActualDuration(0, 0, true),
			criterion_exp(Criterion),
			requested_duration(requestedDuration),
			max_duration(maximumDuration),
			max_duration_is_specified(maxDurIsSpecified),
			theTriggers(
				"window_finder",
				events,
				initializer,
				time_saver::get_now()),
			we_need_to_check_windows(wins != NULL || safe_wins != NULL),
			twiceTheNumberOfWindows(0L),
			theWindows(wins),
			theSafeWindows(safe_wins),
			theActualDuration(actual_dur),
			Symbols(the_symbols),
			ref_count(0) {
	}

	//
	// This constructor initializes theTriggers to the start of the
	// resource histories. This is appropriate when get_windows is invoked
	// by a decomposition request outside of the modeling loop.
	//
	window_finder(
		parsedExp&		Criterion,
		const CTime_base&	requestedDuration,
		const CTime_base&	maximumDuration,
		bool			maxDurIsSpecified,
		bool			criterionIsSatisfiedAtStart,
		bool			weMustWaitOnTheFirstWindow,
		TypedValue*		wins,
		TypedValue*		safe_wins,
		TypedValue*		actual_dur,
		behaving_element&	the_symbols
		) : criterion_is_satisfied_at_start(criterionIsSatisfiedAtStart),
			we_need_to_wait(weMustWaitOnTheFirstWindow),
			theDurationToRememberForTheActualDuration(0, 0, true),
			criterion_exp(Criterion),
			requested_duration(requestedDuration),
			max_duration(maximumDuration),
			max_duration_is_specified(maxDurIsSpecified),
			theTriggers("window_finder"),
			we_need_to_check_windows(wins != NULL || safe_wins != NULL),
			twiceTheNumberOfWindows(0L),
			theWindows(wins),
			theSafeWindows(safe_wins),
			theActualDuration(actual_dur),
			Symbols(the_symbols),
			ref_count(0) {
		initialize_triggers_at_start();
	}

	//
	// for interpolated windows (tentative - placeholder)
	//
	window_finder(
		parsedExp&		Criterion,
		const CTime_base&	requestedDuration,
		const CTime_base&	maximumDuration,
		bool			maxDurIsSpecified,
		bool			criterionIsSatisfiedAtStart,
		TypedValue*		wins,
		behaving_element&	the_symbols
		) : max_duration(maximumDuration),
			max_duration_is_specified(maxDurIsSpecified),
			requested_duration(requestedDuration),
			criterion_is_satisfied_at_start(criterionIsSatisfiedAtStart),
			we_need_to_wait(false),
			theDurationToRememberForTheActualDuration(0, 0, true),
			criterion_exp(Criterion),
			theTriggers(
				"window_finder",
				EventLoop::theEventLoop().potential_triggers,
				initializer,
				time_saver::get_now()),
			we_need_to_check_windows(true),
			twiceTheNumberOfWindows(0L),
			theWindows(wins),
			theSafeWindows(NULL),
			theActualDuration(NULL),
			Symbols(the_symbols),
			ref_count(0) {
	}

	~window_finder() {
	}

	bool compute_windows();
	bool evaluate_criterion(bool debug = false);

	void			initialize_triggers_at_start();


	parsedExp&		criterion_exp;
	bool			criterion_is_satisfied_at_start;
	bool			we_need_to_wait;
	TypedValue*		theWindows;
	TypedValue*		theSafeWindows;
	TypedValue*		theActualDuration;
	behaving_element	Symbols;
	CTime_base		theDurationToRememberForTheActualDuration;
	bool			we_need_to_check_windows;

	//
	// NOTE: EventLoop::potential_triggers.next() returns the node that
	// triggered try_scheduling(); its time tag = now. Calling next()
	// on the copy should return the next value-node in the queue.
	//
	res_curval_miter	theTriggers;
	CTime_base		requested_duration;
	CTime_base		max_duration;
	CTime_base		start_time_of_search_window;
	CTime_base		min_time_limit;
	CTime_base		max_time_limit;
	bool			max_duration_is_specified;
	long			twiceTheNumberOfWindows;

	static bool		is_disabled;
	static void		get_search_parameters(
					TypedValue*&		windows,
					TypedValue*&		safe_windows,
					TypedValue*&		actual_duration,
					const CTime_base&	max_dur,
					bool&			max_dur_is_specified,
					behaving_element&	act_obj,
					Cstring&		name_of_actual_dur_symbol
					);
};
