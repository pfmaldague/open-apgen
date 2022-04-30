#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "apDEBUG.H"
#include "Rsource.H"
#include "assert.h"

version_changer& RES_history::theVersion() {
	static version_changer the_static_version_changer;
	return the_static_version_changer;
}

void	version_changer::increment() {
	if(res_version <= last_published_version) {
		res_version = last_published_version + 1;
	}
}

Cstring apgen::spell(apgen::RES_CLASS restype) {
	   switch(restype) {
		case apgen::RES_CLASS::ASSOCIATIVE:
			return "ASSOCIATIVE"; break;
		case apgen::RES_CLASS::ASSOCIATIVE_CONSUMABLE:
			return "ASSOCIATIVE CONSUMABLE"; break;
		case apgen::RES_CLASS::CONSUMABLE:
			return "CONSUMABLE"; break;
		case apgen::RES_CLASS::EXTERNAL:
			return "EXTERNAL"; break;
		case apgen::RES_CLASS::INTEGRAL:
			return "INTEGRAL"; break;
		case apgen::RES_CLASS::NONCONSUMABLE:
			return "NONCONSUMABLE"; break;
		case apgen::RES_CLASS::SETTABLE:
			return "SETTABLE"; break;
		case apgen::RES_CLASS::SIGNAL:
			return "SIGNAL"; break;
		case apgen::RES_CLASS::STATE:
			return "STATE"; break;
		default:
			return 	"undefined (ERROR)";
	}
}



Cstring apgen::spell(apgen::USAGE_TYPE c) {
	switch(c) {
		case apgen::USAGE_TYPE::RESET:
			return Cstring("reset");
		case apgen::USAGE_TYPE::SET:
			return Cstring("set");
		case apgen::USAGE_TYPE::USE:
			return Cstring("use");
	}
	return Cstring("UNKNOWN");
}

static int dummy_usage = 0;

value_node1*	value_node::stateFactory(
			const CTime_base&	t,
			const TypedValue&	orig_val,
       			long			modelOrder,
       			int			depth,
       			apgen::RES_VAL_TYPE	T,
       			Rsource&		r) {
	value_node1*	vn = new value_node1(t, modelOrder, NULL);
	vn->payload = new st_value_node(
				vn, orig_val, depth, T, r,
				new simpleLinker<value_node>(vn));
	return vn;
}

st_value_node::st_value_node(   
		value_node*		self,
		const TypedValue&	orig_val,
                int			depth,
                apgen::RES_VAL_TYPE	T,
                Rsource&		r,
		baseLinker<value_node>*	L)
        : res_val_w_linker(T, L),
     		usage_depth(depth),
		val(orig_val),
       	 	an_Other(NULL) {
}

value_node1*	value_node::stateFactory(
			const CTime_base&	t,
			value_node*		VN,
			long			modelOrder,
			int			depth,
			apgen::RES_VAL_TYPE	T,
			Rsource&		r) {
	value_node1*	vn = new value_node1(t, modelOrder, NULL);
	vn->payload = new st_value_node(
				vn, VN, depth, T, r,
				new simpleLinker<value_node>(vn));
	return vn;
}

//
// the value associated with this st_value_node will be figured out later on,
// when adding the node to the resource's history:
//
st_value_node::st_value_node(
			value_node*		self,
			value_node*		VN,
			int			depth,
			apgen::RES_VAL_TYPE	t,
			Rsource&		r,
			baseLinker<value_node>* L)
        : res_val_w_linker(t, L),
		usage_depth(depth),
	        an_Other(NULL) {
	VN->payload->set_other_to(self);
	an_Other = VN;
}

st_value_node::~st_value_node() {
}

value_node*	value_node::settableFloatFactory(
			const CTime_base&	t,
			double			value_set_to,
			long			modelOrder,
                	Rsource&		r) {
	value_node*	vn = new value_node(t, modelOrder, NULL);
	vn->payload = new value_node_f(
				vn,
				value_set_to,
				r);
	return vn;
}

value_node*	value_node::settableIntFactory(
			const CTime_base&	t,
			long			value_set_to,
			long			modelOrder,
                	Rsource&		r) {
	value_node*	vn = new value_node(t, modelOrder, NULL);
	vn->payload = new value_node_i(
				vn,
				value_set_to,
				r);
	return vn;
}

value_node*	value_node::settableTimeFactory(
			const CTime_base&	t,
			CTime_base		value_set_to,
			long			modelOrder,
                	Rsource&		r) {
	value_node*	vn = new value_node(t, modelOrder, NULL);
	vn->payload = new value_node_t(
				vn,
				value_set_to,
				r);
	return vn;
}

value_node*	value_node::settableStringFactory(
			const CTime_base&	t,
			const Cstring&		value_set_to,
			long			modelOrder,
                	Rsource&		r) {
	value_node*	vn = new value_node(t, modelOrder, NULL);
	vn->payload = new value_node_s(
				vn,
				value_set_to,
				r);
	return vn;
}

const Cstring& value_node::get_key() const {
	static Cstring s;

	s.undefine();
	s << Key.getetime().to_string()
		<< " VN (res = " << list->Owner->name << ", val = "
		<< get_value().to_string();
	s << ", m.o. " << Key.get_event_id() << ")";
	return s;
}

value_node1*	value_node::numericFloatFactory(
			const CTime_base&	t,
			double			new_consumption,
			long			modelOrder,
                	Rsource&		r,
			apgen::RES_VAL_TYPE	rvt) {
	value_node1*	vn = new value_node1(t, modelOrder, NULL);
	vn->payload = new num_value_node_f(
				vn, new_consumption, r, rvt,
				new floatLinker<value_node>(vn));
	return vn;
}

value_node1*	value_node::numericFloatFactory(
			const CTime_base&	t,
			double			new_consumption,
			value_node*		ncv,
			long			modelOrder,
                	Rsource&		r,
			apgen::RES_VAL_TYPE	rvt) {
	value_node1*	vn = new value_node1(t, modelOrder, NULL);
	vn->payload = new num_value_node_f(
				vn, new_consumption, ncv, r, rvt,
				new floatLinker<value_node>(vn));
	return vn;
}

value_node1*
value_node1::rotate() {
	TreeDir::Dir	t_direction, u_direction;
	value_node1* new_root;

	if(Dnode0::indicator == TreeDir::BALANCED) {
		return NULL;
	}
	t_direction = Dnode0::indicator;
	u_direction = t_direction ? TreeDir::LEFT_DIR : TreeDir::RIGHT_DIR;

	new_root = (value_node1*) Dnode0::Links[t_direction];
	if(new_root->indicator == u_direction) {
		new_root = new_root->rotate();
		((res_val_w_linker*)payload)->myLinker->attach(t_direction, new_root, TreeDir::UPDATE_CONS);
	}
	if(new_root->indicator == TreeDir::BALANCED)
		Dnode0::indicator = TreeDir::BALANCED;
	else
		Dnode0::indicator = u_direction;
	new_root->indicator = u_direction;
	((res_val_w_linker*)payload)->myLinker->attach(t_direction, new_root->Links[u_direction], TreeDir::UPDATE_CONS);
	((res_val_w_linker*)new_root->payload)->myLinker->attach(u_direction, get_this(), TreeDir::UPDATE_CONS);
	return new_root;
}

value_node_s::value_node_s(
			value_node*		self,
			const Cstring&		value_set_to,
			Rsource&		r)
        : res_value_base(apgen::RES_VAL_TYPE::SET),
		str(value_set_to) {
}

int&	value_node_s::get_usage_depth() {
	return dummy_usage;
}

const TypedValue	value_node_s::get_value(apgen::DATA_TYPE dt) const {
	TypedValue tv(dt);
	tv = str;
	return tv;
}

void	value_node_s::get_delta(TypedValue& v) const {
	throw(eval_error("value_node_s::get_delta() should not be used"));
}

void	value_node_s::set_value(const TypedValue& v) {
	str = v.get_string();
}

value_node_i::value_node_i(
			value_node*	self,
			long int	value_set_to,
			Rsource&	r)
        : res_value_base(apgen::RES_VAL_TYPE::SET),
		I(value_set_to) {
}

int&	value_node_i::get_usage_depth() {
	return dummy_usage;
}

const TypedValue	value_node_i::get_value(apgen::DATA_TYPE dt) const {
	TypedValue tv(dt);
	if(dt == apgen::DATA_TYPE::BOOL_TYPE) {
		tv = (bool)I;
	} else {
		tv = I;
	}
	return tv;
}

void	value_node_i::get_delta(TypedValue& v) const {
	throw(eval_error("value_node_i::get_delta() should not be used"));
}

void	value_node_i::set_value(const TypedValue& v) {
	I = v.get_int();
}

value_node_t::value_node_t(
			value_node*	self,
			CTime_base	value_set_to,
			Rsource&	r)
        : res_value_base(apgen::RES_VAL_TYPE::SET),
		T(value_set_to) {
}

int&	value_node_t::get_usage_depth() {
	return dummy_usage;
}

const TypedValue	value_node_t::get_value(apgen::DATA_TYPE dt) const {
	TypedValue tv(dt);
	tv = T;
	return tv;
}

void	value_node_t::get_delta(TypedValue& v) const {
	throw(eval_error("value_node_t::get_delta() should not be used"));
}

void	value_node_t::set_value(const TypedValue& v) {
	T = v.get_time_or_duration();
}

value_node_f::value_node_f(
			value_node*	self,
			double		value_set_to,
			Rsource&	r)
        : res_value_base(apgen::RES_VAL_TYPE::SET),
		D(value_set_to) {
}

int&	value_node_f::get_usage_depth() {
	return dummy_usage;
}

const TypedValue	value_node_f::get_value(apgen::DATA_TYPE dt) const {
	TypedValue tv(dt);
	tv = D;
	return tv;
}

void	value_node_f::get_delta(TypedValue& v) const {
	throw(eval_error("value_node_f::get_delta() should not be used"));
}

void	value_node_f::set_value(const TypedValue& v) {
	D = v.get_double();
}

num_value_node_f::num_value_node_f(
		value_node*		self,
		double			new_consumption,
		Rsource&		r,
		apgen::RES_VAL_TYPE	T,
		baseLinker<value_node>* L)
	: res_val_w_linker(T, L), 
		an_Other(NULL) {
	((floatLinker<value_node>*)myLinker)->myCount = new_consumption;
}

num_value_node_f::num_value_node_f(
		value_node*	self,
		double		new_consumption,
		value_node*	VN,
		Rsource&	r,
		apgen::RES_VAL_TYPE	T,
		baseLinker<value_node>* L)
	: res_val_w_linker(T, L), 
		an_Other(VN) {
	assert(VN);
	VN->payload->set_other_to(self);
	((floatLinker<value_node>*)myLinker)->myCount = new_consumption;
}

num_value_node_f::~num_value_node_f() {
}

int&	num_value_node_f::get_usage_depth() {
	return dummy_usage;
}

const TypedValue	num_value_node_f::get_value(apgen::DATA_TYPE) const {

	//
	// We need a semaphore because get_consumption() depends
	// on the AVL structure, which is affected by node
	// addition/removal. However, this is too low a level;
	// we can only access the parent resource if the node
	// is inserted in a list.
	//
	// Instead, semaphores are locked in the methods that
	// add nodes to the history (ResourceValue.C), remove
	// history nodes (EventLoop.C), and query history
	// values (ResourceValue.C again)
	//
	TypedValue tv;
	tv = myLinker->get_consumption();
	return tv;
}

void	num_value_node_f::get_delta(TypedValue& v) const {
	v = ((floatLinker<value_node>*)myLinker)->get_i_cons().get_double();
}

void	num_value_node_f::set_value(const TypedValue& v) {
	throw(eval_error("num_value_node_f::set_value(): do not use this method."));
}

Cstring value_node::get_info() const {
	Cstring		s;
	TypedValue	V;

	payload->get_delta(V);
	s << "res = " << list->Owner->name << ", usage = " << V.to_string();
	return s;
}

const TypedValue value_node::get_value() const {
	return payload->get_value(list->Owner->get_datatype());
}

value_node1*	value_node::numericIntFactory(
			const CTime_base&	t,
			true_long		new_consumption,
			long			modelOrder,
                	Rsource&		r,
			apgen::RES_VAL_TYPE	rvt) {
	value_node1*	vn = new value_node1(t, modelOrder, NULL);
	vn->payload = new num_value_node_i(
				vn, new_consumption, r, rvt,
				new intLinker<value_node>(vn));
	return vn;
}

value_node1*	value_node::numericIntFactory(
			const CTime_base&	t,
			true_long		new_consumption,
			value_node*		ncv,
			long			modelOrder,
                	Rsource&		r,
			apgen::RES_VAL_TYPE	rvt) {
	value_node1*	vn = new value_node1(t, modelOrder, NULL);
	vn->payload = new num_value_node_i(
				vn, new_consumption, ncv, r, rvt,
				new intLinker<value_node>(vn));
	return vn;
}

num_value_node_i::num_value_node_i(
		value_node*		self,
		true_long		new_consumption,
		Rsource&		r,
		apgen::RES_VAL_TYPE	T,
		baseLinker<value_node>* L)
	: res_val_w_linker(T, L), 
		an_Other(NULL) {
	((intLinker<value_node>*)myLinker)->myCount = new_consumption;
}

num_value_node_i::num_value_node_i(
		value_node*		self,
		true_long		new_consumption,
		value_node*		VN,
		Rsource&		r,
		apgen::RES_VAL_TYPE	T,
		baseLinker<value_node>* L)
	: res_val_w_linker(T, L), 
		an_Other(VN) {
	assert(VN);
	VN->payload->set_other_to(self);
	((intLinker<value_node>*)myLinker)->myCount = new_consumption;
}

num_value_node_i::~num_value_node_i() {
}

int&	num_value_node_i::get_usage_depth() {
	return dummy_usage;
}

const TypedValue	num_value_node_i::get_value(apgen::DATA_TYPE dt) const {
	TypedValue		tv;

	//
	// We need a semaphore because get_consumption() depends
	// on the AVL structure, which is affected by node
	// addition/removal. However, this is too low a level;
	// we can only access the parent resource if the node
	// is inserted in a list.
	//


	if(   dt == apgen::DATA_TYPE::TIME
	   || dt == apgen::DATA_TYPE::DURATION) {

		//
		// we ALWAYS want a duration, because history stores deltas
		//
		CTime_base T(myLinker->get_int_consumption());
		tv = T;
	} else {
		tv = myLinker->get_int_consumption();
	}
	return tv;
}

void	num_value_node_i::get_delta(TypedValue& v) const {
	v = ((intLinker<value_node>*)myLinker)->myCount.get_content();
}

void	num_value_node_i::set_value(const TypedValue& v) {
	throw(eval_error("num_value_node_i::set_value(): do not use this method."));
}
