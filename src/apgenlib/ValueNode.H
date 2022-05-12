#ifndef _RSOURCE_BASE_H_
#define _RSOURCE_BASE_H_

#include "APbasic.H"
#include "APdata.H"
#include "alphatime.H"
#include "multitemplate.H"
#include "multilist.H"
#include "smartptr.H"
#include "linkers.H"

class RCsource;
class Rsource;
class con_violation;
class Constraint;

	/* Class for queries through get_values() and from RES_sys.
	 * 
	 * USERS:
	 *
	 *    RES_history::get_values()
	 *    RES_sys::addResource()
	 *    RES_sys::configure_graphics()
	 */
class unattached_value_node: public baseC<alpha_time, unattached_value_node, Rsource*> {
public:
	unattached_value_node(
			const TypedValue&	cumul_val,
			apgen::DATA_TYPE	t,
			CTime_base		time)
		: baseC<alpha_time, unattached_value_node, Rsource*>(time),
			val(cumul_val, t) {}
protected:
	unattached_value_node(const unattached_value_node& ua)
		: baseC<alpha_time, unattached_value_node, Rsource*>(ua),
			val(ua.val) {}
public:
	virtual ~unattached_value_node() {
		if(list) {
			list->remove_node(this);
		}
	}

	unattached_value_node*				copy() {
		return new unattached_value_node(*this);
	}
	unattached_value_node*				get_this() {
		return this;
	}
	const unattached_value_node*			get_this() const {
		return this;
	}
	TypedValue					val;
	TypedValue&					get_value() {
		return val;
	}
	Rsource*					get_res() {
		return list->Owner;
	}
	virtual double				get_error_high() {
		return 0.0;
	}
	virtual double				get_warn_high() {
		return 0.0;
	}
	virtual double				get_warn_low() {
		return 0.0;
	}
	virtual double				get_error_low() {
		return 0.0;
	}
};

class LimitContainer {
public:
	LimitContainer()
		: error_high_val(0.0),
			warn_high_val(0.0),
			warn_low_val(0.0),
			error_low_val(0.0) {
	}
	LimitContainer(
			double	eh,
			double	wh,
			double	wl,
			double	el)
			: error_high_val(eh),
				warn_high_val(wh),
				warn_low_val(wl),
				error_low_val(el) {
	}
	LimitContainer(const LimitContainer& lc)
			: error_high_val(lc.error_high_val),
				warn_high_val(lc.warn_high_val),
				warn_low_val(lc.warn_low_val),
				error_low_val(lc.error_low_val) {
	}
	virtual ~LimitContainer() {
	}

	double		error_high_val;
	double		warn_high_val;
	double		warn_low_val;
	double		error_low_val;
};

class unattached_node_w_limits: public unattached_value_node {
public:
	unattached_node_w_limits(
			const TypedValue&	v,
			apgen::DATA_TYPE	t,
			CTime_base		time,
			double	eh,
			double	wh,
			double	wl,
			double	el
			) : unattached_value_node(v, t, time),
				lc(eh, wh, wl, el) {
	}
protected:
	unattached_node_w_limits(const unattached_node_w_limits& unwl)
			: unattached_value_node(unwl) {
	}
public:
	~unattached_node_w_limits() {
		if(list) {
			list->remove_node(this);
		}
	}
	LimitContainer		lc;
	virtual double	get_error_high() {
		return lc.error_high_val;
	}
	virtual double	get_warn_high() {
		return lc.warn_high_val;
	}
	virtual double	get_warn_low() {
		return lc.warn_low_val;
	}
	virtual double	get_error_low() {
		return lc.error_low_val;
	}
};

typedef Cntnr<alpha_void, unattached_value_node*> unattached_ptr;

class value_node;
class value_node_w_linker;

//
// Abstract class for all value node payloads
//
class res_value_base {
public:
	res_value_base(apgen::RES_VAL_TYPE rest)
		: type(rest)
			{}

	res_value_base(const res_value_base& a) = delete;

	virtual ~res_value_base() {}

	apgen::RES_VAL_TYPE		type;
	virtual apgen::RES_VAL_TYPE	get_res_val_type() const {
		return type;
	}

	//
	// returns the delta (c_value_node, num_value_node); throws for states
	//
	virtual void			get_delta(TypedValue&) const = 0;

	//
	// returns current resource value (all)
	//
	virtual const TypedValue	get_value(apgen::DATA_TYPE) const = 0;

	//
	// sets current resource value (states only, throws otherwise)
	//
	virtual void			set_value(const TypedValue&) = 0;


	//
	// for non-consumable resources only. In a pair of nodes that span a resource
	// usage interval, the Other pointer of the start/end node points to the
	// end/start node.
	//
	virtual const value_node*	get_other_const() const  = 0;

	virtual value_node*		get_other()  = 0;

	//
	// for states, which can stack multiple 'use' intervals.
	//
	virtual int&			get_usage_depth() = 0;

	virtual void			set_other_to(value_node*) = 0;
};

//
// Abstract Resource Value: the abstract class
// that is a base class for all value node payloads
//
class res_val_w_linker: public res_value_base {
public:
	res_val_w_linker(apgen::RES_VAL_TYPE rest, baseLinker<value_node>* bl)
		: res_value_base(rest),
			myLinker(bl) {}

	res_val_w_linker(const res_val_w_linker& a) = delete;

	virtual ~res_val_w_linker() {
		delete(myLinker);
	}

	baseLinker<value_node>*	myLinker;
};

//
// First derived class: for states
//
class st_value_node: public res_val_w_linker {
public:

	//
	// First constructor: for use by state resources
	// (that's why 'depth' is listed).
	// 
	// Used by
	//   RES_state::append_current_value_to_history()
	//   	with T = apgen::RES_VAL_RESET
	//   RES_state::append_to_history()
	//   	with T = apgen::RES_VAL_START/END_USING
	//   		if usage type == 'use'
	//   RES_state::append_to_history()
	//   	with T = apgen::RES_VAL_SET
	//   		if usage type == 'set'
	//   RES_state::append_to_history()
	//   	with T = apgen::RES_VAL_RESET
	//   		if usage type == 'reset'
	//
	// NOTE: node gets its time tag from the mEvent through a pointer
	//
	st_value_node(	value_node*		self,
			const TypedValue&	orig_val,
			int			depth,
			apgen::RES_VAL_TYPE	T,
			Rsource&		r,
			baseLinker<value_node>* L);

	//
	// Second constructor: for use with the second node of a
	// state resource usage clause.
	//
	// Used by
	//    RES_state::append_to_history()
	//    	with apgen::RES_VAL_END_USING
	//    		if usage type == 'use'
	//
	// NOTE: this node gets its time tag from the mEvent through a pointer.
	//
	st_value_node(	value_node*		self,
			value_node*		VN,
			int			depth,
			apgen::RES_VAL_TYPE	t,
			Rsource&		r,
			baseLinker<value_node>*	L);

	//
	// last constructor: the mandatory copy constructor; it may be
	// that this is not used.
	//
	st_value_node(const st_value_node&	orig_val) = delete;

	~st_value_node();

	TypedValue			val;
	virtual const TypedValue	get_value(apgen::DATA_TYPE) const {
		return val;
	}
	virtual void			get_delta(TypedValue& v) const {
		throw(eval_error("st_value_node::get_delta(): not supported"));
	}
	virtual void			set_value(const TypedValue& x) {
		val = x;
	}

	//
	// for states, which can stack multiple 'use' intervals.
	//
	int				usage_depth;
	virtual int&			get_usage_depth() {
		return usage_depth;
	}

	//
	// for non-consumable resources only. In a pair of nodes that span a resource
	// usage interval, the Other pointer of the start/end node points to the
	// end/start node.
	//
private:
	value_node*			an_Other;

public:
	virtual const value_node*	get_other_const() const {
		return an_Other;
	}
	virtual value_node*		get_other() {
		return an_Other;
	}
	virtual void			set_other_to(value_node* vn) {
		an_Other = vn;
	}
};

//
// Second derived class: for non-consumable resources that are floats
//
class num_value_node_f: public res_val_w_linker {
public:
	num_value_node_f(	value_node*		self,
				double			new_consumption,
				Rsource&		r,
				apgen::RES_VAL_TYPE	rvt,
				baseLinker<value_node>*	L);
	num_value_node_f(	value_node*		self,
				double			new_consumption,
				value_node*		ncv,
				Rsource&		r,
				apgen::RES_VAL_TYPE	rvt,
				baseLinker<value_node>*	L);
	num_value_node_f(const num_value_node_f&	orig_val) = delete;

	// The mandatory destructor.
	~num_value_node_f();

	virtual const TypedValue get_value(apgen::DATA_TYPE) const;
	virtual void		get_delta(TypedValue& v) const;
	virtual void		set_value(const TypedValue&);

private:
	value_node*		an_Other;
public:
				// implement something trivial here:
	virtual int&		get_usage_depth();
	virtual const value_node* get_other_const() const {
		return an_Other;
	}
	virtual value_node*	get_other() {
		return an_Other;
	}
	virtual void		set_other_to(value_node* vn) {
		an_Other = vn;
	}
};

//
// Third derived class: for non-consumable resources that are ints
//
class num_value_node_i: public res_val_w_linker {
private:
public:
	num_value_node_i(	value_node*		self,
				true_long		new_consumption,
				Rsource&		r,
				apgen::RES_VAL_TYPE	rvt,
				baseLinker<value_node>*	L);
	num_value_node_i(	value_node*		self,
				true_long		new_consumption,
				value_node*		ncv,
				Rsource&		r,
				apgen::RES_VAL_TYPE	rvt,
				baseLinker<value_node>*	L);
	num_value_node_i(const num_value_node_i&	orig_val) = delete;

	// The mandatory destructor.
	~num_value_node_i();

	virtual const TypedValue get_value(apgen::DATA_TYPE) const;
	virtual void		get_delta(TypedValue& v) const;
	virtual void		set_value(const TypedValue&);

private:
	value_node*		an_Other;
public:
				// implement something trivial here:
	virtual int&		get_usage_depth();
	virtual const value_node* get_other_const() const {
					return an_Other;
	}
	virtual value_node*	get_other() {
					return an_Other;
	}
	virtual void		set_other_to(value_node* vn) {
		an_Other = vn;
	}
};

//
// Fourth derived class: for settable resources that are strings
//
class value_node_s: public res_value_base {
private:
public:
	value_node_s(		value_node*		self,
				const Cstring&		val_set_to,
				Rsource&		r);

	value_node_s(const value_node_s&	orig_val) = delete;

	~value_node_s() {}

	Cstring			str;

	virtual const TypedValue get_value(apgen::DATA_TYPE) const override;

				//
				// Do not use (throws exception):
				//
	virtual void		get_delta(TypedValue& v) const override;
	virtual void		set_value(const TypedValue&) override;

				//
				// implement something trivial here:
				//
	virtual int&		get_usage_depth() override;

				//
				// ditto for the next 3:
				//
	virtual const value_node* get_other_const() const override {
		return NULL;
	}
	virtual value_node*	get_other() override {
		return NULL;
	}
	virtual void		set_other_to(value_node* vn) override {
	}
};

//
// Fifth derived class: for settable resources that are floats
//
class value_node_f: public res_value_base {
private:
public:
	value_node_f(		value_node*	self,
				double		val_set_to,
				Rsource&	r);

	value_node_f(const value_node_f&	orig_val) = delete;

	~value_node_f() {}

	double			D;

	virtual const TypedValue get_value(apgen::DATA_TYPE) const override;

				//
				// Do not use (throws exception):
				//
	virtual void		get_delta(TypedValue& v) const override;
	virtual void		set_value(const TypedValue&) override;

				//
				// implement something trivial here:
				//
	virtual int&		get_usage_depth() override;

				//
				// ditto for the next 3:
				//
	virtual const value_node* get_other_const() const override {
		return NULL;
	}
	virtual value_node*	get_other() override {
		return NULL;
	}
	virtual void		set_other_to(value_node* vn) override {
	}
};

//
// Sixth derived class: for settable resources that are integers
//
class value_node_i: public res_value_base {
private:
public:
	value_node_i(		value_node*	self,
				long int	val_set_to,
				Rsource&	r);

	value_node_i(const value_node_i&	orig_val) = delete;

	~value_node_i() {}

	long int		I;

	virtual const TypedValue get_value(apgen::DATA_TYPE) const override;

				//
				// Do not use (throws exception):
				//
	virtual void		get_delta(TypedValue& v) const override;
	virtual void		set_value(const TypedValue&) override;

				//
				// implement something trivial here:
				//
	virtual int&		get_usage_depth() override;

				//
				// ditto for the next 3:
				//
	virtual const value_node* get_other_const() const override {
		return NULL;
	}
	virtual value_node*	get_other() override {
		return NULL;
	}
	virtual void		set_other_to(value_node* vn) override {
	}
};

//
// Seventh derived class: for settable resources that are times or durations
//
class value_node_t: public res_value_base {
private:
public:
	value_node_t(		value_node*	self,
				CTime_base	val_set_to,
				Rsource&	r);

	value_node_t(const value_node_t&	orig_val) = delete;

	~value_node_t() {}

	CTime_base		T;

	virtual const TypedValue get_value(apgen::DATA_TYPE) const override;

				//
				// Do not use (throws exception):
				//
	virtual void		get_delta(TypedValue& v) const override;
	virtual void		set_value(const TypedValue&) override;

				//
				// implement something trivial here:
				//
	virtual int&		get_usage_depth() override;

				//
				// ditto for the next 3:
				//
	virtual const value_node* get_other_const() const override {
		return NULL;
	}
	virtual value_node*	get_other() override {
		return NULL;
	}
	virtual void		set_other_to(value_node* vn) override {
	}
};

//
// To support versioning of RES_history (doesn't work very well in the GUI)
//
class version_changer {
public:
	long	res_version;
	long	last_published_version;
	version_changer() : res_version(0), last_published_version(-1) {}
	~version_changer() {}

	void		increment();
	const long	get_res_version() {
				last_published_version = res_version;
				return res_version; }
};

class Violation_info {
public:
	typedef enum {
		WARNING,
		ERROR,
		RELEASE } Severity;

	Violation_info()
		: severity(RELEASE) {}
	Violation_info(const Violation_info&) = default;
	Violation_info(
		// const Cstring& constraint_name,
		Constraint*	parent,
		const Cstring&	msg,
		Severity	s)
		: message(msg),
			parent_constraint(parent),
			severity(s) {}
	~Violation_info() = default;

	//
	// We replace this by a pointer to the
	// constraint that is being violated:
	//
	// Cstring		name;
	Constraint*	parent_constraint;
	Cstring		message;
	Severity	severity;
};


//
// payload is a res_value_base (no linker)
//
class value_node: public baseC<prio_time, value_node, Rsource*> {
protected:

	//
	// NOTE: the factories will have to set the payload pointer
	//
	value_node(const CTime_base& t, long modelOrder, res_value_base* arv)
		: baseC<prio_time, value_node, Rsource*>(prio_time(t, modelOrder)),
			payload(arv) {}

	//
	// mandated by Node - do not use!
	//
	value_node(const value_node& a)
		: baseC<prio_time, value_node, Rsource*>(a.Key),
			payload(a.payload) {}
public:

	typedef baseC<prio_time, value_node, Rsource*> Dbase;

	virtual ~value_node() {

		//
		// no longer necessary because we inherit from backptr:
		//
		// well... we no longer do
		//
		// Pointers()->delete_pointers_to_this();
		//

		if(list) {
			list->remove_node(this);
		}

		delete payload;
		payload = NULL;
	}

	//
	// In a settable resource, apgen::RES_VAL_TYPE is always RES_VAL_SET,
	// so it is not needed as a parameter:
	//
	static value_node*	settableFloatFactory(
					const CTime_base&	t,
					double			value_set_to,
					long			modelOrder,
					Rsource&		r);

	static value_node*	settableIntFactory(
					const CTime_base&	t,
					long			value_set_to,
					long			modelOrder,
					Rsource&		r);

	static value_node*	settableTimeFactory(
					const CTime_base&	t,
					CTime_base		value_set_to,
					long			modelOrder,
					Rsource&		r);

	static value_node*	settableStringFactory(
					const CTime_base&	t,
					const Cstring&		value_set_to,
					long			modelOrder,
					Rsource&		r);

	static value_node_w_linker*	stateFactory(
					const CTime_base&	t,
					const TypedValue&	orig_val,
					long			modelOrder,
					int			depth,
					apgen::RES_VAL_TYPE	T,
					Rsource&		r);

	static value_node_w_linker*	stateFactory(
					const CTime_base&	t,
					value_node* VN,
					long			modelOrder,
					int			depth,
					apgen::RES_VAL_TYPE	T,
					Rsource&		r);

	static value_node_w_linker*	numericFloatFactory(
					const CTime_base&	t,
					double			new_consumption,
					long			modelOrder,
			               	Rsource&		r,
					apgen::RES_VAL_TYPE	rvt);

	static value_node_w_linker*	numericFloatFactory(
					const CTime_base&	t,
					double			new_consumption,
					value_node* ncv,
					long			modelOrder,
		       	         	Rsource&		r,
					apgen::RES_VAL_TYPE	rvt);

	static value_node_w_linker*	numericIntFactory(
					const CTime_base&	t,
					true_long		new_consumption,
					long			modelOrder,
			               	Rsource&		r,
					apgen::RES_VAL_TYPE	rvt);

	static value_node_w_linker*	numericIntFactory(
					const CTime_base&	t,
					true_long		new_consumption,
					value_node* ncv,
					long			modelOrder,
                			Rsource&		r,
					apgen::RES_VAL_TYPE	rvt);

	value_node*			copy() override {
		return new value_node(*this);
	}
	value_node*			get_this() override {
		return this;
	}
	const value_node*		get_this() const override {
		return this;
	}
	const Cstring&			get_key() const override;

	res_value_base*			payload;

	apgen::RES_VAL_TYPE		get_res_val_type() const {
		return payload->type;
	}

	// returns the delta (c_value_node, num_value_node); throws for states
	void				get_delta(TypedValue& v) const {
		payload->get_delta(v);
	}
	// returns current resource value (all)
	virtual const TypedValue	get_value() const;

	// sets current resource value (states only, throws otherwise)
	virtual void			set_value(const TypedValue& v) {
		payload->set_value(v);
	}

	Cstring				get_info() const;
	Cstring				get_descr() {
		Cstring s;

		s << Key.getetime().to_string()
			<< " " << Cstring(Key.get_event_id())
			<< "|" << Cstring(Key.get_2nd_key())
			<< " <VN " << get_info() << ">";
		return s;
	}

	//
	// for non-consumable resources only. In a pair of nodes that span a resource
	// usage interval, the Other pointer of the start/end node points to the
	// end/start node.
	//
	virtual const value_node*	get_other_const() const {
		return payload->get_other_const();
	}
	virtual value_node*		get_other() {
		return payload->get_other();
	}
	// for states, which can stack multiple 'use' intervals.
	virtual int&			get_usage_depth() {
		return payload->get_usage_depth();
	}
};


//
// payload is a res_val_w_linker
//
class value_node_w_linker: public value_node {
private:

	//
	// NOTE: the factories will have to set the payload pointer
	//
	value_node_w_linker(const CTime_base& t, long modelOrder, res_value_base* arv)
		: value_node(t, modelOrder, arv) {}

	//
	// mandated by Node - do not use!
	//
	value_node_w_linker(const value_node& a)
		: value_node(a) {}
public:

	virtual ~value_node_w_linker() {

		//
		// no longer necessary because we inherit from backptr:
		//
		// well... we no longer do
		//
		// Pointers()->delete_pointers_to_this();
		//

		if(list) {
			list->remove_node(this);
		}

		delete payload;
		payload = NULL;
	}

	value_node_w_linker*			copy() override {
		return new value_node_w_linker(*this);
	}
	value_node_w_linker*			get_this() override {
		return this;
	}
	const value_node_w_linker*		get_this() const override {
		return this;
	}
	
  //
  // Methods that override those of baseC to support linker behavior 
  //

  virtual void			reset_links() override {
	value_node::indicator = TreeDir::BALANCED;
	value_node::Links[TreeDir::RIGHT_DIR] = NULL;
	value_node::Links[TreeDir::LEFT_DIR] = NULL;
	((res_val_w_linker*)payload)->myLinker->init_cons();
  }

				//
				// base method is in baseC; this one is not
				// marked override because the return is
				// different
				//
  virtual value_node_w_linker*		rotate();

				// made virtual so a derived class can disable by throwing
  virtual void			temporarily_remove_from_list() override {
	if(value_node::saved_list != NULL || value_node::list == NULL) {
		Cstring errs(value_node::get_key());
		if(value_node::saved_list) {
			errs << ": saved_list != NULL - cannot remove temporarily";
		} else {
			errs << ": list == NULL - cannot remove temporarily";
		}
		throw(eval_error(errs));
	}

	value_node::saved_list = value_node::list;

	//
	// Save the intrinsic consumption:
	//
	valueHolder*	save_new_cons = get_i_cons().copy();

	//
	// clobbers the intrinsic consumption - that's why we saved it:
	//
	value_node::list->remove_node(get_this());

	//
	// restore the intrinsic consumption:
	//
	((res_val_w_linker*)payload)->myLinker->set_i_cons(save_new_cons);

	delete save_new_cons;
  }

  virtual void		add_cons(	TreeDir::Dir which_dir,
					const valueHolder& countVal,
					const valueHolder& integralVal) override {
	((res_val_w_linker*)payload)->myLinker->add_cons(which_dir, countVal, integralVal);
  }
  virtual void		add_cons_to_ancestors() override {
	((res_val_w_linker*)payload)->myLinker->add_cons_to_ancestors();
  }
  virtual void		attach(		TreeDir::Dir right_or_left,
					value_node* node_to_attach,
					TreeDir::cons_adjust CA) override {
	((res_val_w_linker*)payload)->myLinker->attach(right_or_left, node_to_attach, CA);
  }
  virtual void		attach_left(	value_node* node_to_attach,
					TreeDir::cons_adjust CA) override {
	((res_val_w_linker*)payload)->myLinker->attach_left(node_to_attach, CA);
  }
  virtual void		attach_right(	value_node* node_to_attach,
					TreeDir::cons_adjust CA) override {
	((res_val_w_linker*)payload)->myLinker->attach_right(node_to_attach, CA);
  }
  virtual double	get_consumption() const override {
	return ((res_val_w_linker*)payload)->myLinker->get_consumption();
  }
  virtual valueHolder	get_i_cons() const override {
	return ((res_val_w_linker*)payload)->myLinker->get_i_cons();
  }
  virtual double	get_integral() const override {
	return ((res_val_w_linker*)payload)->myLinker->get_integral();
  }
  virtual double	get_integral_left() const override {
	return ((res_val_w_linker*)payload)->myLinker->get_integral_left();
  }
  virtual double	get_integral_right() const override {
	return ((res_val_w_linker*)payload)->myLinker->get_integral_right();
  }
  virtual true_long	get_int_consumption() const override {
	return ((res_val_w_linker*)payload)->myLinker->get_int_consumption();
  }
  virtual valueHolder	get_l_cons() const override {
	return ((res_val_w_linker*)payload)->myLinker->get_l_cons();
  }
  virtual valueHolder	get_r_cons() const override {
	return ((res_val_w_linker*)payload)->myLinker->get_r_cons();
  }
  virtual value_node*	get_parent() const override {
	return ((res_val_w_linker*)payload)->myLinker->get_parent();
  }
  virtual void		init_cons() override {
	((res_val_w_linker*)payload)->myLinker->init_cons();
  }
  virtual bool		is_float() const override {
	return ((res_val_w_linker*)payload)->myLinker->is_float();
  }
  virtual void		set_i_cons(const valueHolder* vh) override {
	((res_val_w_linker*)payload)->myLinker->set_i_cons(vh);
  }
  virtual void		set_parent(value_node* Par) override {
	((res_val_w_linker*)payload)->myLinker->set_parent(Par);
  }
  virtual void		subtract_cons_from_ancestors(value_node* stop_here = NULL) override {
	((res_val_w_linker*)payload)->myLinker->subtract_cons_from_ancestors(stop_here);
  }
  virtual void		make_root_of(slist<prio_time, value_node, Rsource*>* l) override {
	((res_val_w_linker*)payload)->myLinker->set_parent(NULL);
	l->make_root(get_this());
  }

  friend class value_node;
};


#endif /* _RSOURCE_BASE_H_ */
