#ifndef _MULTITERATOR_H
#define _MULTITERATOR_H

#include "multitemplate.H"
#include "C_list.H"

class Multiterator;
class value_node;
class ActivityInstance;

class status_aware_iterator {
public:
	status_aware_iterator(bool scheduled_incl, bool unscheduled_incl, const List& li)
		: includeScheduledActs(scheduled_incl),
		includeUnscheduledActs(unscheduled_incl),
		theIterator(li) {}
	status_aware_iterator(const status_aware_iterator& sai)
		: includeScheduledActs(sai.includeScheduledActs),
		includeUnscheduledActs(sai.includeUnscheduledActs),
		theIterator(sai.theIterator) {}
	~status_aware_iterator() {}

	List_iterator		theIterator;
	ActivityInstance*	next();
	ActivityInstance*	operator()() { return next(); }
	bool			includeScheduledActs;
	bool			includeUnscheduledActs;
};

class act_lists {
private:
				/* blist of ActivityInstances (i. e., Time_nodes) that are current
				 * and visible (automatically sorted by Start time): */
	OwnedCList		someActiveInstances;
				/* activities are put here by their constructor; if anything
				 * is left here at the end of reading a file, it's an error: */
	OwnedCList		someBrandNewActivities;
				/* List Of ActivityInstances (i. e., Time_nodes) that have been decomposed,
				 * and don't have display objects (i.e. are invisible) */
	OwnedCList		someDecomposedActivities;
				/* List Of ActivityInstances (i. e., Time_nodes) that have been abstracted,
				 * and don't have display objects (i.e. are invisible) */
	OwnedCList		someAbstractedActivities;
				/* First List of ActivityInstance nodes without display objects,
				 * used for cut and paste (i.e. CUT/COPY buffer) */
	OwnedCList		somePendingActivities;

	status_aware_iterator	clipboardIterator;
	status_aware_iterator	activeIteratorScheduledOnly;
	status_aware_iterator	activeIteratorScheduledOrNot;
	status_aware_iterator	decomposedIterator;
	status_aware_iterator	abstractedIterator;
	status_aware_iterator	brandNewIterator;
public:
	act_lists();
	~act_lists() {}

	Node			*find_earliest_active_after(Node *N) { return someActiveInstances.find_earliest_after(N); }
	Node			*find_latest_active_before(Node *N) { return someActiveInstances.find_latest_before(N); }
				// for giant window
	Node			*find_earliest_after(Node *N);
				// for giant window
	Node			*find_latest_before(Node *N);

	int			get_active_length() { return someActiveInstances.get_length(); }
	int			get_abstracted_length() { return someAbstractedActivities.get_length(); }
	int			get_brand_new_length() { return someBrandNewActivities.get_length(); }
	int			get_clipboard_length() { return somePendingActivities.get_length(); }
	int			get_decomposed_length() { return someDecomposedActivities.get_length(); }

				// No messages required: acts have already been 'disconnected'
	void			clear_clipboard() { somePendingActivities.clear(); }
	void			clear_brand_new_list() { someBrandNewActivities.clear(); }
	status_aware_iterator&	get_clipboard_iterator() { return clipboardIterator; }
	status_aware_iterator&	get_scheduled_active_iterator() { return activeIteratorScheduledOnly; }
	status_aware_iterator&	get_all_active_iterator() { return activeIteratorScheduledOrNot; }
	status_aware_iterator&	get_abstracted_iterator() { return abstractedIterator; }
	status_aware_iterator&	get_decomposed_iterator() { return decomposedIterator; }
	status_aware_iterator&	get_brand_new_iterator() { return brandNewIterator; }

	void			order_clipboard() {somePendingActivities.order(); }
	void			order_active_list() {someActiveInstances.order(); }
	void			order_decomposed_list() {someDecomposedActivities.order(); }
	void			order_abstracted_list() {someAbstractedActivities.order(); }

	bool			theFirstRequestTime(CTime_base &, int the_print_flag = 0);

				// This function is careful to destroy requests first, then activity instances:
	void			delete_all_instances();
				// includes disconnecting activities:
	void			clean_up_clipboard();
	void			synchronize_all_lists();

	// auxiliary classes that helps ActivityInstance:
	friend class PR_manager;
	friend class instance_state_changer;
	// to help Multiterator get at the blists:
	friend class Multiterator;
	};

class Rsource;


extern int Multiterator_compare(const Node *, const Node * );
class act_lists;

	/*
	 * This class describes the 'current node' of a time-ordered list that was
	 * added to a Multiterator. It is implemented in IO_write.C.
	 */
class tol_thread: public /* concrete_Time_node */ Time_node {
public:
	CTime_base		event_time;
	const char*		kind_of_thread;
	int			size, Index;
	Rsource*		R;
	Time_node*		curNode;
	int			priority;
	const blist*		parent_list;

	tol_thread(CTime_base time, int P, const blist* pl)
		: /* concrete_Time_node */ event_time(time),
		R(NULL),
		size(0),
		Index(0),
		curNode(NULL),
		priority(P),
		kind_of_thread(""),
		parent_list(pl)
		{}
	tol_thread(const tol_thread& tt)
		: /* concrete_Time_node */ Time_node(tt), 
		R(tt.R),
		size(tt.size),
		Index(tt.Index),
		curNode(tt.curNode),
		priority(tt.priority),
		kind_of_thread(tt.kind_of_thread),
		parent_list(tt.parent_list),
		event_time(tt.event_time)
		{}

	~tol_thread() {}


	virtual void		setetime(
					const CTime_base& new_time) {
		event_time = new_time;
	}
	virtual const CTime_base& getetime() const {
		return event_time;
	}

	Node*			copy() {
		return new tol_thread(*this);
	}
	Node_type		node_type() const {
		return TOL_THREAD;
	}
	const Cstring&		get_key() const {
		return Cstring::null();
	}
};

	/*
	 * A general list traversal class with some special TOL-related features.
	 * The general idea is to 'add' a set of time-ordered lists to the Multiterator
	 * object. When the next() function is called, the net result will be the same
	 * as if we had merged the individual lists first, then done an iteration on the
	 * merged list. The advantage of the multiterator is that merging is done 'on the
	 * fly'; there is no duplication of the original lists, even through pointers.
	 * This saves enormous amounts of time when handling Gigabyte-size problems.
	 *
	 * This class is implemented in IO_write.C.
	 *
	 */
class Multiterator {
public:
	// set all priorities to 0:
	Multiterator(act_lists *);
	Multiterator(act_lists *, int &priority);
	Multiterator(internal_compare_function F);
	Multiterator(const Multiterator &mit);
	// for general-purpose use, in particular the modeling loop:
	Multiterator();
	~Multiterator();

			/*
			 * This method adds a new thread to the multiterator,
			 * causing the elements of the supplied blist l to be
			 * included in the iteration process.
			 *
			 * The thread is given a name, taken to be the 2nd
			 * argument of the call. The name does not have to be
			 * unique; it is stored in the tol_thread node that
			 * 'represents' the blist l vis-a-vis the multiterator.
			 *
			 * The Multiterator will make sure that next() returns
			 * elements as if the blists in the various threads had
			 * been merged first.
			 *
			 * Concerning priority: lower priority means higher priority.
			 * The priority algorithm is coded in the Multiterator_compare
			 * function, which is used for the Multiterator's thread lists.
			 */
	void		add(	const blist &l,		// must be a blist of Time_nodes
				const char *theTag,	// a convenient name for each thread
				int which_priority	// determines which nodes get returned
							//	first if there is a tie
			   );

			// Special function for generating TOL output. (PLACEHOLDER)
	void		dump_to_TOL(aoString &, CTime_base end_time);

			/* returns true if! n will be returned by next() after
			 * enough calls */
	bool		isAheadOf(Time_node* n);

			// Similar to List_iterator::first()
	Time_node	*first();

	bool		is_empty() { return L0.get_length() == 0; }

			// Similar to List_iterator::first() but starts at the indicated node
	void		adopt_node(Time_node *);

			// Similar to List_iterator::next()
	Time_node*	next();

			// Similar to next() but does not update things
	Time_node*	peek();

			/* advances time pointers until none of them point to
			 * something in the list (assumed to contain
			 * bpointernodes). Returns the current node. */
	Time_node*	remove_nodes(tlist<alpha_void, Cntnr<alpha_void, value_node*> >&);
	void		checkAll();

			// Does what it says
	void		clear() {
				active_threads.clear();
				dead_threads.clear();
				pointers_to_threads.clear();
				L0.clear(); }

			/* The main purpose of this list of pointers is
			 * to provide an efficient method to get the
			 * thread corresponding to a given time-ordered
			 * list: */
	blist		pointers_to_threads;

private:

			/* Internal list of pointers to tol_threads. The 
			 * getetime() method of a tol_thread returns the
			 * time of the current node in the thread. When
			 * a thread is finished, we remove it; therefore,
			 * we need to keep a list with the original 'threads'
			 * in a separate list. */
	clist		active_threads;
	List		dead_threads;

			// simple List of tol_threads.
	List		L0;
};

class status_aware_multiterator {
public:
	status_aware_multiterator(
			act_lists* al,
			int& priority,
			bool incl_scheduled,
			bool incl_unscheduled)
		: includeUnscheduledActs(incl_unscheduled),
			includeScheduledActs(incl_scheduled),
			theMulti(al, priority) {}
	~status_aware_multiterator() {}

	ActivityInstance*	next();
	bool			includeScheduledActs;
	bool			includeUnscheduledActs;
	Multiterator		theMulti;
};

#endif
